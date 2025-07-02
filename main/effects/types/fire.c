#include "../../source/sources_types.h"
#include "../../source/sources.h"
#include "../effects.h"
#include "../../leds/led_matrix.h"

#include "../../gtypes.h"
#include "../data.h"
#include <math.h>

static float gravity = 120.0F;

typedef struct bar_t {
    float body_y;
    float body_vel_y;
    float blaze_elapsed;
    float flame_animation_time_sec;

    float twinkling_value;
    float twinkling_timer;
} bar_t;

static bar_t bars[LED_MATRIX_COLUMNS];

// Mement at which blaze starts 
#define BLAZE_DIFF_THRESHOLD (2.1)

// Blaze duration 1.0 is quite long, 6.0 is superfast
#define BLAZE_EXTINGUISH_SPEED (3.8)

// Brightness of individual flames, 0.2 is really dull
#define BRIGHTNESS_MIN (0.5)

// Top brightness during blaze start
#define BRIGHTNESS_BLAZE_MAX (1.0)

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

            if (diff > BLAZE_DIFF_THRESHOLD) {
                bar->blaze_elapsed = 1.0;
            }
        } else {
            // Otherwise, apply gravity
            bar->body_vel_y -= gravity * delta_time;
            bar->body_y += bar->body_vel_y * delta_time;
        }

        // Blaze
        if (bar->blaze_elapsed > 0.0) {
            bar->blaze_elapsed -= delta_time * BLAZE_EXTINGUISH_SPEED;
            
            if (bar->blaze_elapsed < 0.0) {
                bar->blaze_elapsed = 0;
            }
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

static void update_twinking(const processed_input_result_t* processed_input_result) {
    const float delta_time = processed_input_result->dt_sec;
    
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        // Update log twinkling
        bar->twinkling_timer -= delta_time;
        if (bar->twinkling_timer <= 0.0f) {
            bar->twinkling_timer = 0.05f + (rand() % 40) * 0.01f; // random interval 0.05s - 0.35s

            // pick random brightness between 0.0 and 1.0
            bar->twinkling_value = (float)rand() / (float)RAND_MAX;
        }

        // If blazing then lighten twinking
        if (bar->blaze_elapsed > 0.91) {
            bar->twinkling_timer = 0.2;
            bar->twinkling_value = 1.0;
        }
    }
}

static void draw_bodies(const led_matrix_t* led_matrix) {
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        const float body_y_constrained = CONSTRAIN((bar->body_y), 0, 255);
        const uint8_t body_y_rounded = MIN(roundf(body_y_constrained), (LED_MATRIX_ROWS - 1));
        // const uint8_t body_y_rounded = LED_MATRIX_ROWS - 1;

        for (uint8_t iy = 1; iy <= body_y_rounded; iy++) {
            color_24b_t* pixel = led_matrix_access_pixel_at(led_matrix, ix, iy);

            const uint8_t flame_tongue_idx = LED_MATRIX_ROWS - 1 - body_y_rounded + iy;
            hsv_t tmp_hsv_color = flame_tongue[flame_tongue_idx];

            // 0.0 - 1.0, 1.0 for blaze started
            const float value_modifier = bar->blaze_elapsed * (BRIGHTNESS_BLAZE_MAX - BRIGHTNESS_MIN) + BRIGHTNESS_MIN;

            tmp_hsv_color.value *= value_modifier;

            *pixel = hsv_to_rgb(&tmp_hsv_color);
        }
    }
}

static void draw_twinkling(const led_matrix_t* led_matrix) {
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        color_24b_t* pixel = led_matrix_access_pixel_at(led_matrix, ix, 0);
            
        // Twinkle 0.0 - 1.0:
        // 0.0 - dark
        // 1.0 bright
        const uint8_t twinkle_flame_tongue_idx = (uint8_t)(bar->twinkling_value * LED_MATRIX_ROWS);

        *pixel = hsv_to_rgb(&flame_tongue[twinkle_flame_tongue_idx]);
    }
}

void effect_fire(const led_matrix_t* led_matrix, const processed_input_result_t* processed_input_result) {
    update_bodies(processed_input_result);
    update_twinking(processed_input_result);

    draw_bodies(led_matrix);
    draw_twinkling(led_matrix);
}
