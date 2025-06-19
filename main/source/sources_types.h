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

#define SAMPLES_READY_BIT BIT0

#define SAMPLING_RATE_HZ    44100U
#define SAMPLING_RATE_FHZ   44100.0F

#define INPUT_SAMPLES_COUNT 2048

#define INPUT_SAMPLING_INTERVAL_FMS (((float)INPUT_SAMPLES_COUNT) * 1000.0F / SAMPLING_RATE_FHZ)

typedef struct input_samples_window_t {
    uint32_t timestamp_us;
    float samples[INPUT_SAMPLES_COUNT];
} input_samples_window_t;

#ifdef __cplusplus
}
#endif