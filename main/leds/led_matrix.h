#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"
#include "../gtypes.h"

#define LED_MATRIX_COLUMNS              (19)
#define LED_MATRIX_ROWS                 (21)
#define LED_MATRIX_PIXELS_COUNT         (LED_MATRIX_COLUMNS * LED_MATRIX_ROWS)
#define LED_MATRIX_BASE_PIXELS_COUNT    (4 * 7)

typedef struct led_matrix_t {
    color_24b_t pixels[LED_MATRIX_PIXELS_COUNT];
    uint16_t columns;
    uint16_t rows;
    color_24b_t base_pixels[LED_MATRIX_BASE_PIXELS_COUNT];
} led_matrix_t;

void led_matrix_init(led_matrix_t* led_matrix);

void led_matrix_clear(led_matrix_t* led_matrix);

void led_matrix_fill_color(led_matrix_t* led_matrix, color_24b_t color);

color_24b_t* led_matrix_access_pixel_at(led_matrix_t* led_matrix, uint16_t x, uint16_t y);

#ifdef __cplusplus
}
#endif