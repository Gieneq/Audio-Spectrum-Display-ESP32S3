#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "../gtypes.h"
#include "../leds/led_matrix.h"
#include "asd_packet.h"

#define ESPNOW_WIFI_MODE WIFI_MODE_STA
#define ESPNOW_WIFI_IF   ESP_IF_WIFI_STA

#define ESPNOW_QUEUE_SIZE           32

// ESPNOW primary master for the example to use. The length of ESPNOW primary master must be 16 bytes.
#define CONFIG_ESPNOW_PMK "pmk1234567890123"

// ESPNOW local master for the example to use. The length of ESPNOW local master must be 16 bytes.
#define CONFIG_ESPNOW_LMK "lmk1234567890123"

#define CONFIG_ESPNOW_CHANNEL 1

esp_err_t rf_sender_init();

esp_err_t rf_sender_send_led_matrix(asd_packet_builder_t* apb, const led_matrix_t* led_matrix);

#ifdef __cplusplus
}
#endif