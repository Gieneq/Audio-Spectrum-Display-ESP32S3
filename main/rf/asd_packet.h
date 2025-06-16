#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "../gtypes.h"
#include "../leds/led_matrix.h"

#define ASD_PACKET_SIZE                 250U
#define ASD_PACKET_DATA_SIZE            238U

#define ASD_LED_MATRIX_DATA_SIZE        (sizeof(led_matrix_t))
#define ASD_LED_MATRIX_PACKETS_COUNT    (ASD_LED_MATRIX_DATA_SIZE / ASD_PACKET_DATA_SIZE + (ASD_LED_MATRIX_DATA_SIZE % ASD_PACKET_DATA_SIZE > 0 ? 1 : 0))

typedef struct asd_packet_t {
    uint32_t transfer_idx;
    uint32_t timestamp_micros;
    uint8_t packet_idx;
    uint8_t total_packets_count;
    uint8_t data_size;
    uint8_t crc;
    uint8_t data[ASD_PACKET_DATA_SIZE];
} __attribute__((packed)) asd_packet_t;

static_assert(sizeof(asd_packet_t) == ASD_PACKET_SIZE, "asd_packet_t must be exactly 250 bytes");

typedef struct asd_packet_builder_t {
    uint32_t next_trasfer_idx;
    asd_packet_t led_matrix_packets[ASD_LED_MATRIX_PACKETS_COUNT];
} asd_packet_builder_t;

void asd_packet_builder_init(asd_packet_builder_t* apb);

esp_err_t asd_packet_build_from_leds_matrix(asd_packet_builder_t* apb, const led_matrix_t* led_matrix);

#ifdef __cplusplus
}
#endif
