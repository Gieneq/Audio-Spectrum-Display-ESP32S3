#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define SAMPLES_COUNT (1600U)
#define RINGBUFFER_SIZE (8 * SAMPLES_COUNT)
#define MIC_BUFF_SIZE   (1024 / 8)
#define SAMPLE_RATE     (16000) // For recording

esp_err_t gsampler_inti();

#ifdef __cplusplus
}
#endif