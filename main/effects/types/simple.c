#include "../source/sources_types.h"
#include "../source/sources.h"
#include "../effects.h"
#include "../../leds/led_matrix.h"

//TODO consider some structs to hold state of individual effects
void effect_simple(led_matrix_t* led_matrix, const sources_sample_data_t* data) {
    led_matrix_clear(led_matrix);
    led_matrix_access_pixel_at(led_matrix, 1, 1)->red = 255;
}
