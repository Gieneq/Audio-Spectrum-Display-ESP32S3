#include "../../source/sources_types.h"
#include "../../source/sources.h"
#include "../effects.h"
#include "../../leds/led_matrix.h"

#include "../../gtypes.h"
#include "../data.h"
#include <math.h>

static float gravity = 120.0F;

#define SMOKE_GENERATE_THRESHOLD_VEL_ABS (0.1F)

#define SMOKE_GENERATE_MINIMAL_HEIGHT (7)

#define SMOKE_GENERATE_SMOKE_PUFFS_MAX_COUNT 4

typedef struct smoke_puff_t {
    float y;                // current height
    float speed;            // vertical speed
    uint8_t color_idx;      // which color from SMOKE_PUFFS_colors
    bool active;            // true if puff is alive

    float age;              // seconds alive
    float lifetime;         // total lifetime in seconds
} smoke_puff_t;

typedef struct smoke_generator_t {
    smoke_puff_t puffs[SMOKE_GENERATE_SMOKE_PUFFS_MAX_COUNT];
    float emission_timer;
} smoke_generator_t;

static void smoke_generator_attempt_generate_smoke_from(smoke_generator_t* smoke_generator, float origin_y) {
    if (smoke_generator->emission_timer > 0.0f) {
        // Not yet time for a new puff
        return;
    }

    // Find an inactive slot
    for (uint8_t i = 0; i < SMOKE_GENERATE_SMOKE_PUFFS_MAX_COUNT; i++) {
        smoke_puff_t* puff = &smoke_generator->puffs[i];
        if (!puff->active) {
            puff->y = origin_y;
            puff->speed = 3.0f + ((float)(rand() % 50)) * 0.1f; // speed 3-8 units/sec
            puff->lifetime = 1.0f + ((float)(rand() % 22)) * 0.1f; // 1.0 - 3.2 sec
            puff->age = 0.0f;
            puff->color_idx = SMOKE_PUFFS_COLORS_COUNT - 1;
            puff->active = true;

            // Reset interval timer (e.g. 0.1-0.3 sec between puffs)
            smoke_generator->emission_timer = 0.1f + ((float)(rand() % 20)) * 0.01f;
            break;
        }
    }
}

static void smoke_generator_tick(smoke_generator_t* smoke_generator, float delta_time) {
    smoke_generator->emission_timer -= delta_time;

    for (uint8_t i = 0; i < SMOKE_GENERATE_SMOKE_PUFFS_MAX_COUNT; i++) {
        smoke_puff_t* puff = &smoke_generator->puffs[i];
        if (puff->active) {
            
            puff->age += delta_time;

            // Calculate fade progress
            float progress = puff->age / puff->lifetime;
            if (progress > 1.0f) {
                progress = 1.0f;
            }

            uint8_t max_idx = SMOKE_PUFFS_COLORS_COUNT - 1;  // e.g. 6 if array has 7 colors
            puff->color_idx = (uint8_t)(progress * max_idx);

            puff->y += puff->speed * delta_time;

            if (puff->y >= (LED_MATRIX_ROWS - 1) || puff->age >= puff->lifetime) {
                puff->active = false;
            }
        }
    }
}

typedef struct bar_t {
    float body_y;
    float body_vel_y;
    float blaze_elapsed;
    float flame_animation_time_sec;

    float twinkling_value;
    float twinkling_timer;

    smoke_generator_t smoke_generator;
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

static void update_smoke(const float delta_time) {
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];
        smoke_generator_t* smoke_generator = &bar->smoke_generator;

        if (bar->body_y > SMOKE_GENERATE_MINIMAL_HEIGHT) {
            if (bar->body_vel_y < -SMOKE_GENERATE_THRESHOLD_VEL_ABS) {
                smoke_generator_attempt_generate_smoke_from(smoke_generator, bar->body_y);
            }
        }
        smoke_generator_tick(smoke_generator, delta_time);
    }
}

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

static void smoke_generator_draw(const smoke_generator_t* smoke_generator, led_matrix_t* led_matrix, uint8_t column) {
    for (uint8_t i = 0; i < SMOKE_GENERATE_SMOKE_PUFFS_MAX_COUNT; i++) {
        const smoke_puff_t* puff = &smoke_generator->puffs[i];
        if (puff->active) {
            int y_int = (int)roundf(puff->y);
            if (y_int >= 0 && y_int < LED_MATRIX_ROWS) {
                color_24b_t* pixel = led_matrix_access_pixel_at(led_matrix, column, y_int);
                *pixel = SMOKE_PUFFS_colors[puff->color_idx];
            }
        }
    }
}

static void draw_smoke(const led_matrix_t* led_matrix) {
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];
        smoke_generator_draw(&bar->smoke_generator, led_matrix, ix);
    }
}

static void draw_bodies(const led_matrix_t* led_matrix) {
    for (uint8_t ix = 0; ix < LED_MATRIX_COLUMNS; ++ix) {
        bar_t* bar = &bars[ix];

        const float body_y_constrained = CONSTRAIN((bar->body_y), 0, 255);
        const uint8_t body_y_rounded = MIN(roundf(body_y_constrained), (LED_MATRIX_ROWS - 1));

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
    update_smoke(processed_input_result->dt_sec);

    draw_smoke(led_matrix);
    draw_bodies(led_matrix);
    draw_twinkling(led_matrix);
}
