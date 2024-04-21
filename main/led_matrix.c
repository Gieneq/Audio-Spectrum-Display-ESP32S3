#include "led_matrix.h"

#include <string.h>

void led_matrix_init(led_matrix_t* led_matrix) {
    memset(led_matrix, 0, sizeof(led_matrix_t));

    led_matrix->columns = LED_MATRIX_COLUMNS;
    led_matrix->rows = LED_MATRIX_ROWS;
    led_matrix->pixels_count = LED_MATRIX_PIXELS_COUNT;

}

void led_matrix_clear(led_matrix_t* led_matrix) {
    for (uint16_t ix = 0; ix < LED_MATRIX_COLUMNS; ix++) {
        *led_matrix_access_column_height_at(led_matrix, ix) = 0;
        for (uint16_t iy = 0; iy < LED_MATRIX_ROWS; iy++) {
            led_matrix_access_pixel_at(led_matrix, ix, iy)->value = 0;
        }
    }
}

color_24b_t* led_matrix_access_pixel_at(led_matrix_t* led_matrix, uint16_t x, uint16_t y) {
    if ((x >= LED_MATRIX_COLUMNS) || (y >= LED_MATRIX_ROWS)) {
        return NULL;
    }

    return led_matrix->pixels + (x + LED_MATRIX_COLUMNS * y);
}

uint8_t* led_matrix_access_column_height_at(led_matrix_t* led_matrix, uint16_t x) {
    if (x >= LED_MATRIX_COLUMNS) {
        return NULL;
    }

    return led_matrix->columns_heights + x;
}