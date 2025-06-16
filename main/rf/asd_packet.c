#include "asd_packet.h"

#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "esp_timer.h"

#include "esp_log.h"

static const char *TAG = "ASD_PACKET";

static uint8_t calculate_crc(const uint8_t* data_bytes, uint32_t data_size) {
    uint8_t result = 0;
    for (uint32_t data_idx = 0; data_idx < data_size; ++data_idx) {
        result += data_bytes[data_idx];
    }
    return result;
}

void asd_packet_builder_init(asd_packet_builder_t* apb) {
    assert(apb);
    memset(apb, 0, sizeof(asd_packet_builder_t));
    apb->next_trasfer_idx = 0;
}

esp_err_t asd_packet_build_from_leds_matrix(asd_packet_builder_t* apb, const led_matrix_t* led_matrix) {
    assert(apb);
    assert(led_matrix);
    // const uint32_t packets_count = data_size / ASD_PACKET_DATA_SIZE + data_size % ASD_PACKET_DATA_SIZE > 0 ? 1 : 0;
    // ESP_LOGD(TAG, "Building ASD packets: data_size=%u, packets_count=%u", data_size, packets_count);
    // if (packets_count > max_packets_count) {
    //     ESP_LOGW(TAG, "Packet count exceeded %u > %u", packets_count, max_packets_count);
    //     return ESP_ERR_INVALID_ARG;
    // }

    const uint8_t* data_bytes = (const uint8_t*)led_matrix;
    const uint32_t data_size = sizeof(led_matrix_t);

    ESP_LOGI(TAG, "Building ASD packets: data_size=%lu, packets_count=%u", data_size, ASD_LED_MATRIX_PACKETS_COUNT);

    uint32_t data_offset = 0;
    uint32_t remaining_data = data_size;

    for (uint32_t packet_idx = 0; packet_idx < ASD_LED_MATRIX_PACKETS_COUNT; ++packet_idx) {
        asd_packet_t* recent_packet = &apb->led_matrix_packets[packet_idx];
        memset(recent_packet, 0, sizeof(asd_packet_t));

        const uint32_t packet_data_size = remaining_data > ASD_PACKET_DATA_SIZE ? ASD_PACKET_DATA_SIZE : remaining_data;

        recent_packet->packet_idx = packet_idx;
        recent_packet->total_packets_count = ASD_LED_MATRIX_PACKETS_COUNT;
        memcpy(recent_packet->data, data_bytes + data_offset, packet_data_size);
        recent_packet->data_size = packet_data_size;
        recent_packet->transfer_idx = apb->next_trasfer_idx;
        recent_packet->timestamp_micros = (uint32_t)esp_timer_get_time(); // I wonder what it will bring
        recent_packet->crc = calculate_crc(data_bytes + data_offset, packet_data_size);
        ESP_LOGI(TAG, "Building ASD packet: packet_idx=%lu, data_offset=%lu, packet_data_size=%lu", 
            packet_idx, data_offset, packet_data_size
        );

        data_offset += packet_data_size;
        remaining_data -= packet_data_size;
    }

    apb->next_trasfer_idx++;
    ESP_LOGI(TAG, "Building done!");
    return ESP_OK;
}
