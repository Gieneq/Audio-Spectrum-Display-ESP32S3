#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

#define SAMPLES_COUNT (32U)
#define RINGBUFFER_SIZE (1024U)

esp_err_t gsampler_inti();

#ifdef __cplusplus
}
#endif