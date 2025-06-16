#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

#include "rf_sender.h"
#include "asd_packet.h"

static const char *TAG = "RF_SENDER";

static uint8_t s_example_broadcast_mac[ESP_NOW_ETH_ALEN] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

static QueueHandle_t s_example_espnow_queue = NULL;
static QueueHandle_t s_espnow_send_status_queue;

static void example_wifi_init(void) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start());
    ESP_ERROR_CHECK( esp_wifi_set_channel(CONFIG_ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE));   
}

static void example_espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
    ESP_LOGI(TAG, "Send callback: status = %d", status);
    // Send the status to the queue
    if (xQueueOverwrite(s_espnow_send_status_queue, &status) != pdTRUE) {
        ESP_LOGW(TAG, "Send feedback event was not consumed by sender task!");
    }
}

static void example_espnow_recv_cb(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len) {
    ESP_LOGW(TAG, "example_espnow_recv_cb dummy");
}

static void example_espnow_task(void *pvParameter) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    ESP_LOGI(TAG, "Start sending data");

    asd_packet_t packet_to_be_sent = {0};
    esp_now_send_status_t send_status;
    esp_err_t ret = ESP_OK;

    while(1) {
        while (xQueueReceive(s_example_espnow_queue, &packet_to_be_sent, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI(TAG, "s_example_espnow_queue transfer=%lu, packet=%u/%u", 
                packet_to_be_sent.transfer_idx, 
                packet_to_be_sent.packet_idx, 
                packet_to_be_sent.total_packets_count
            );

            int retry_count = 0;
            do {
                ESP_LOGI(TAG, "esp_now_send: packet_idx=%lu, data_offset=%u, packet_data_size=%u",
                    packet_to_be_sent.transfer_idx,
                    packet_to_be_sent.packet_idx,
                    packet_to_be_sent.total_packets_count
                );

                ret = esp_now_send(s_example_broadcast_mac, (const uint8_t *)&packet_to_be_sent, sizeof(asd_packet_t));
                if (ret != ESP_OK) {
                    // Callback cont be triggered after error
                    ESP_LOGW(TAG, "esp_now_send() failed immediately, ret=%d", ret);
                    vTaskDelay(pdMS_TO_TICKS(10));
                    retry_count++;
                    continue;
                }

                // Wait for send callback to return result
                if (xQueueReceive(s_espnow_send_status_queue, &send_status, pdMS_TO_TICKS(1000)) == pdTRUE) {
                    if (send_status == ESP_NOW_SEND_SUCCESS) {
                        ESP_LOGI(TAG, "Send successful");
                        break; // exit retry loop
                    } else {
                        ESP_LOGW(TAG, "Send failed, retrying...");
                    }
                } else {
                    ESP_LOGW(TAG, "Timeout waiting for send callback");
                }
                retry_count++;

            } while( retry_count < 3);

            if (retry_count >= 3) {
                ESP_LOGE(TAG, "Failed to send packet after 3 retries, giving up");
            }
        }
    }
}

static esp_err_t example_espnow_init(void) {
    /* Initialize ESPNOW and register sending and receiving callback function. */
    s_example_espnow_queue = xQueueCreate(ESPNOW_QUEUE_SIZE, sizeof(asd_packet_t));
    if (s_example_espnow_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return ESP_FAIL;
    }

    s_espnow_send_status_queue = xQueueCreate(1, sizeof(esp_now_send_status_t));
    if (s_espnow_send_status_queue == NULL) {
        ESP_LOGE(TAG, "Create queue fail");
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(example_espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(example_espnow_recv_cb) );

    /* Set primary master key. */
    ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );

    /* Add broadcast peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL) {
        ESP_LOGE(TAG, "Malloc peer information fail");
        vQueueDelete(s_example_espnow_queue);
        vQueueDelete(s_espnow_send_status_queue);
        esp_now_deinit();
        return ESP_FAIL;
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, s_example_broadcast_mac, ESP_NOW_ETH_ALEN);
    esp_err_t ret = esp_now_add_peer(peer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add peer: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Peer added successfully");
    }
    ESP_ERROR_CHECK( ret );
    free(peer);

    xTaskCreate(example_espnow_task, "example_espnow_task", 4 * 1024, NULL, 4, NULL);
    return ESP_OK;
}

esp_err_t rf_sender_init() {
    ESP_LOGI(TAG, "Initializing RF Sender!");
    example_wifi_init();
    example_espnow_init();
    return ESP_OK;
}

esp_err_t rf_sender_send_led_matrix(asd_packet_builder_t* apb, const led_matrix_t* led_matrix) {
    esp_err_t ret = asd_packet_build_from_leds_matrix(apb, led_matrix);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "asd_packet_build_from_leds_matrix failed!");
        return ret;
    }

    // Queue created packets
    for (uint32_t packet_idx = 0; packet_idx < ASD_LED_MATRIX_PACKETS_COUNT; packet_idx++) {
        if (xQueueSend(s_example_espnow_queue, &apb->led_matrix_packets[packet_idx], pdMS_TO_TICKS(100)) != pdTRUE) {
            ESP_LOGW(TAG, "RF sender queue full, dropping packet");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}