#include "../../source/sources_types.h"
#include "../../source/sources.h"
#include "../effects.h"
#include "../../leds/led_matrix.h"

#include "../../gtypes.h"
#include "../data.h"
#include <math.h>

static float gravity = 120.0F;

typedef struct bar_t {
    float block_y;
    float block_vel_y;
    float body_y;
    float body_vel_y;
} bar_t;

static bar_t bars[LED_MATRIX_COLUMNS];

static void update_bodies(const processed_input_result_t* processed_input_result) {
    const float delta_time = processed_input_result->dt_sec;

    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        // Compute new input height for this bin
        const float bin_scaled = processed_input_result->bins[ix] * 2.0;

        bar_t* bar = &bars[ix];

        if (bin_scaled > bar->body_y) {
            // New peak is higher → “jump” bar to new value and set velocity to slightly lag
            const float diff = bin_scaled - bar->body_y;
            bar->body_y = bin_scaled;
            bar->body_vel_y = 0.0;
        } else {
            // Otherwise, apply gravity
            bar->body_vel_y -= gravity * delta_time;
            bar->body_y += bar->body_vel_y * delta_time;
        }

        // Clamp bar peak to floor
        if (bar->body_y < 0.0f) {
            bar->body_y = 0.0f;
            bar->body_vel_y = 0.0f;
        }

        // Clamp bar peak to ceil
        if (bar->body_y >= (LED_MATRIX_ROWS - 1)) {
            bar->body_y = (LED_MATRIX_ROWS - 1);
            bar->body_vel_y = 0.0f;
        }
    }
}

static void update_blocks(const processed_input_result_t* processed_input_result) {
    const float delta_time = processed_input_result->dt_sec;

    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        if (bar->body_y > bar->block_y) {
            // New peak is higher → “jump” bar to new value and set velocity to slightly lag
            const float diff = bar->body_y - bar->block_y;
            bar->block_y = bar->body_y;
            bar->block_vel_y = diff * 5.0F;
        } else {
            // Otherwise, apply gravity
            bar->block_vel_y -= gravity * delta_time;
            bar->block_y += bar->block_vel_y * delta_time;
        }

        // Clamp bar peak to floor
        if (bar->block_y < 0.0f) {
            bar->block_y = 0.0f;
            bar->block_vel_y = 0.0f;
        }

        // Clamp bar peak to ceil
        if (bar->block_y >= (LED_MATRIX_ROWS - 1)) {
            bar->block_y = (LED_MATRIX_ROWS - 1);
        }
    }
}

static void draw(const led_matrix_t* led_matrix) {
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        // Body
        {
            const float body_y_constrained = CONSTRAIN((bar->body_y), 0, 255);
            const uint8_t body_y_rounded = MIN(roundf(body_y_constrained), (LED_MATRIX_ROWS - 1));

            for (uint8_t iy = 0; iy <= body_y_rounded; iy++) {
                color_24b_t* pixel = led_matrix_access_pixel_at(led_matrix, ix, iy);
                *pixel = rainbow_colors[ix];
            }
        }

        // Block
        {
            const float block_y_constrained = CONSTRAIN((bar->block_y), 0, 255);
            const uint8_t block_y_rounded = MIN(roundf(block_y_constrained), (LED_MATRIX_ROWS - 1));
            color_24b_t* pixel = led_matrix_access_pixel_at(led_matrix, ix, block_y_rounded);
            color_24b_set_rgb(pixel, 255, 255, 255);
        }
    }
}

void effect_multicolor(const led_matrix_t* led_matrix, const processed_input_result_t* processed_input_result) {
    update_bodies(processed_input_result);
    
    update_blocks(processed_input_result);

    draw(led_matrix);
}
