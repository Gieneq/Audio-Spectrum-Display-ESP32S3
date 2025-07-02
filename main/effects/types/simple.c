#include "../../source/sources_types.h"
#include "../../source/sources.h"
#include "../effects.h"
#include "../../leds/led_matrix.h"

#include "../../gtypes.h"
#include <math.h>

static float gravity = 120.0F;

typedef struct bar_t {
    float body_y;
    float body_vel_y;
} bar_t;

static bar_t bars[LED_MATRIX_COLUMNS];

void effect_simple(const led_matrix_t* led_matrix, const processed_input_result_t* processed_input_result) {
    const float delta_time = processed_input_result->dt_sec;

    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        // Compute new input height for this bin
        const float bin_scaled = processed_input_result->bins[ix] * 2.0;
        const float bin_constrained = CONSTRAIN(bin_scaled, 0, 255);
        // const float new_height = MIN(bin_constrained, LED_MATRIX_ROWS);

        bar_t* bar = &bars[ix];

        if (bin_constrained > bar->body_y) {
            // New peak is higher → “jump” bar to new value and reset velocity
            bar->body_y = bin_constrained;
            bar->body_vel_y = 0.0f;
        } else {
            // Otherwise, apply gravity
            bar->body_vel_y -= gravity * delta_time;
            bar->body_y += bar->body_vel_y * delta_time;

            // Clamp bar peak to floor
            if (bar->body_y < 0.0f) {
                bar->body_y = 0.0f;
                bar->body_vel_y = 0.0f;
            }
        }
    }

    // Draw 
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        const float y_constrained = CONSTRAIN((bar->body_y), 0, 255);
        const uint8_t y_peak = roundf(y_constrained);
        const uint8_t y_peak_constrained = MIN(y_peak, (LED_MATRIX_ROWS-1));

        for (uint8_t iy = 0; iy <= y_peak_constrained; iy++) {
            led_matrix_access_pixel_at(led_matrix, ix, iy) -> blue = 255;
        }
        led_matrix_access_pixel_at(led_matrix, ix, y_peak_constrained) -> green = 122;
    }
}
