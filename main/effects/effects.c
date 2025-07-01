#include "effects.h"
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

#include "../source/sources.h"

#define EFFECTS_DATA_PROCESSED_BIT BIT0

extern void effect_raw(led_matrix_t* led_matrix, const processed_input_result_t* processed_input_result);
extern void effect_simple(led_matrix_t* led_matrix, const processed_input_result_t* processed_input_result);

static const char *TAG = "EFFECTS";

static QueueHandle_t effects_cmds_queue = NULL;
static EventGroupHandle_t event_group = NULL;

static led_matrix_t workspace_led_matrix;

static void effects_task(void *params) {
    ESP_LOGI(TAG, "Start task");

    effects_source_t recent_source = EFFECTS_SOURCE_MICROPHONE;
    effects_type_t recent_effect = EFFECTS_TYPE_SIMPLE;
    effects_cmd_t received_cmd;

    while(1) {
        // Poll cmds from control queue
        while (xQueueReceive(effects_cmds_queue, &received_cmd, 0) == pdTRUE) {
            switch (received_cmd.type) {
                case EFFECTS_CMD_SET_GAIN:
                    sources_set_gain(received_cmd.data.gain);
                    break;
                case EFFECTS_CMD_SET_FREQUENCY:
                    sources_set_frequency(received_cmd.data.frequency);
                    break;
                case EFFECTS_CMD_SET_EFFECT:
                    recent_effect = received_cmd.data.effects_type;
                    break;
                case EFFECTS_CMD_SET_SOURCE:
                    recent_source = received_cmd.data.effects_source;
                    break;
            }
        }
        
        // Await samples with some timeout in case of bad source and control signal incomming
        const processed_input_result_t* processed_input_result = sources_await_processed_input_result(recent_source, pdMS_TO_TICKS(100));

        // Apply effect
        switch (recent_effect) {
        case EFFECTS_TYPE_RAW:
            effect_raw(&workspace_led_matrix, processed_input_result);
            break;

        case EFFECTS_TYPE_SIMPLE:
            effect_simple(&workspace_led_matrix, processed_input_result);
            break;
        
        default:
            break;
        }

        // Notify new data were processed
        xEventGroupSetBits(event_group, EFFECTS_DATA_PROCESSED_BIT);
    }
}

esp_err_t effects_init() {
    effects_cmds_queue = xQueueCreate(8, sizeof(effects_cmd_t));
    if (effects_cmds_queue == NULL) {
        ESP_LOGE(TAG, "Create effects_cmds_queue fail");
        return ESP_FAIL;
    }

    event_group = xEventGroupCreate();
    if (event_group == NULL) {
        ESP_LOGE(TAG, "Create event_group fail");
        return ESP_FAIL;
    }

    esp_err_t ret = sources_init_all();
    if (ret != ESP_OK) {
        return ret;
    }

    xTaskCreate(effects_task, "effects_task", 5 * 1024, NULL, 4, NULL);

    return ESP_OK;
}

esp_err_t effects_send_cmd(effects_cmd_t cmd) {
    if (effects_cmds_queue == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xQueueSend(effects_cmds_queue, &cmd, pdMS_TO_TICKS(500)) != pdTRUE) {
        ESP_LOGW(TAG, "Command queue full, dropping command");
        return ESP_ERR_TIMEOUT;
    }

    return ESP_OK;
}

void effects_draw_to_led_matrix(led_matrix_t* led_matrix) {
    // ja bym tak zrobil:
    // - w tasku zbiera sample i przetwarza, pobiera komendy z kolejki
    // - tu tylko kopiuje led_matrix

    if (event_group == NULL) {
        return;
    }

    EventBits_t bits = xEventGroupWaitBits(
        event_group,
        EFFECTS_DATA_PROCESSED_BIT,
        pdTRUE,     // Clear on exit
        pdFALSE,    // Wait for any bit (only one defined)
        portMAX_DELAY
    );

    if (bits & EFFECTS_DATA_PROCESSED_BIT) {
        memcpy(led_matrix, &workspace_led_matrix, sizeof(led_matrix_t));
    }
}