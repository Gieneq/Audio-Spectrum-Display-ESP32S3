#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#include "../led_matrix.h"

typedef struct ws2812b_grid_interface_t {
    void (*set_led_matrix_values)(const led_matrix_t* led_mx);
} ws2812b_grid_interface_t;

esp_err_t ws2812b_grid_init();

bool ws2812b_grid_access(ws2812b_grid_interface_t** ws2812b_grid_if, TickType_t timeout_tick_time);

void ws2812b_grid_release();

#ifdef __cplusplus
}
#endif