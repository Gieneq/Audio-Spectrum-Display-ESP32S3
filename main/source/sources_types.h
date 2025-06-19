#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum effects_source_t {
    EFFECTS_SOURCE_SIMULATION,
    EFFECTS_SOURCE_MICROPHONE,
    EFFECTS_SOURCE_WIRED,
} effects_source_t;

#define INPUT_SAMPLES_COUNT 2048

typedef struct input_samples_window_t {
    float samples[INPUT_SAMPLES_COUNT];
} input_samples_window_t;

#ifdef __cplusplus
}
#endif