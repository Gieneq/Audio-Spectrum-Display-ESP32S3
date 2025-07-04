#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "esp_err.h"

#include "../leds/led_matrix.h"
#include "gdisplay_api.h"
#include "../gtypes.h"

typedef struct model_interface_t {
    void (*set_led_matrix_values)(const led_matrix_t* led_mx);
    void (*set_left_button_clicked)(bool clicked);
    void (*set_middle_button_clicked)(bool clicked);
    void (*set_right_button_clicked)(bool clicked);
    void (*set_option_selected)(option_select_t option);
    void (*set_gain)(float g);
    void (*set_frequency)(float freq);
    void (*set_source)(option_source_t source);
    void (*set_effect)(effect_select_t effect);
} model_interface_t;

bool model_interface_access(model_interface_t** model_if, TickType_t timeout_tick_time);

void model_interface_release();

void model_init();

void model_tick();

void model_draw(gdisplay_api_t* gd_api);

#ifdef __cplusplus
}
#endif