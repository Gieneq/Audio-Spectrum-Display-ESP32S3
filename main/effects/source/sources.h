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

#define BINS_COUNT 21

typedef struct sources_sample_data_t {
    float bins[BINS_COUNT];
} sources_sample_data_t;

esp_err_t sources_init_all();

const sources_sample_data_t* sources_await_sample_data(effects_source_t source, TickType_t timeout_tick_time);

#ifdef __cplusplus
}
#endif