#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "esp_err.h"
#include "sources_types.h"
#include "freertos/FreeRTOS.h"

// this is layer between sources and effect processor
// sources are all separate tasks
// effects is task
// but this is just think layer, maybe some selector

/* Uncomment to debug microphone source */
// #define DEBUG_SOURCE_MICROPHONE

/* Uncomment to debug wired source */
// #define DEBUG_SOURCE_WIRED

#define BINS_COUNT          19

#define FFT_SIZE            (INPUT_SAMPLES_COUNT)
#define FFT_RESULT_SIZE     (FFT_SIZE / 2)

// Used to remove 50Hz Grid noise
#define FFT_DROPPED_SAMPLES_UP_TO 2

#define FFT_THRESHOLD       0.1F
#define FFT_CUSTOM_SCALE    (3.5)
#define FFT_SCALE           (FFT_CUSTOM_SCALE * 1.0F / (FFT_SIZE / 2))

typedef struct processed_input_result_t {
    float bins[BINS_COUNT];
    float dt_sec;
} processed_input_result_t;

esp_err_t sources_init_all();

void sources_set_gain(float new_gain);

// Simulation specific
void sources_set_frequency(float new_frequency);

const processed_input_result_t* sources_await_processed_input_result(effects_source_t source, TickType_t timeout_tick_time);

#ifdef __cplusplus
}
#endif