#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "esp_err.h"
#include "gdisplay_tools.h"
#include <stdbool.h>

esp_err_t gdisplay_lcd_init(void);

bool display_api_await_access(gdisplay_api_t** gd_api, TickType_t timeout);

void display_api_release();

#ifdef __cplusplus
}
#endif