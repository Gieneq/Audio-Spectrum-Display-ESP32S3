#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#include "gdisplay_api.h"


typedef struct model_interface_t {
    void (*set_bar_heights)(int16_t* bars, size_t bars_count);
} model_interface_t;

bool model_interface_access(model_interface_t** model_if, TickType_t timeout_tick_time);

void model_interface_release();

void model_init();

void model_tick();

void model_draw(gdisplay_api_t* gd_api);

#ifdef __cplusplus
}
#endif