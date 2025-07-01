#include "../../source/sources_types.h"
#include "../../source/sources.h"
#include "../effects.h"
#include "../../leds/led_matrix.h"

#include "../../gtypes.h"
#include <math.h>

void effect_raw(led_matrix_t* led_matrix, const processed_input_result_t* processed_input_result) {
    led_matrix_clear(led_matrix);

    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        const float bin_scaled = processed_input_result->bins[ix] * 2.0;
        const float bin_constrained = CONSTRAIN(bin_scaled, 0, 255);
        const uint8_t bar_height = roundf(bin_constrained);
        const uint8_t bar_height_constrained = MIN(bar_height, LED_MATRIX_ROWS);

        for (uint8_t iy = 0; iy < bar_height_constrained; iy++) {
            led_matrix_access_pixel_at(led_matrix, ix, iy) -> red = 255;
        }
    }
}
