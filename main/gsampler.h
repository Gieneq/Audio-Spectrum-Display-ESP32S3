#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "esp_err.h"

/* 
 * Uncomment to use test samples 
 * instead of microphone
 */
//  #define USE_TEST_SAMPLES 1

#define MIC_RECORDING_BUFF_LENGHT   (512U)
#define RINGBUFFER_SIZE (8 * MIC_RECORDING_BUFF_LENGHT)
#define MIC_RECORDING_BUFF_SIZE  (MIC_RECORDING_BUFF_LENGHT * sizeof(int16_t))
#define MIC_RECORDING_SAMPLE_RATE     (44000U)
#define SAMPLER_GAIN (21.0) //42.0

#define RECEIVER_SAMPLES_COUNT (2048U)
#define RECEIVER_SAMPLING_DURATION_MS ((int32_t)(1000.0F*((float)RECEIVER_SAMPLES_COUNT)/((float)MIC_RECORDING_SAMPLE_RATE)))

#define FFT_WORKSPACE_BUFFER_SIZE (RECEIVER_SAMPLES_COUNT * 2) //(((RECEIVER_SAMPLES_COUNT/2) + 1) * 2 )
#define FFT_RESULT_SAMPLES_COUNT (RECEIVER_SAMPLES_COUNT / 2)

#define SAMPLE_IDX_TO_FREQ(_idx)      ( ((float)(MIC_RECORDING_SAMPLE_RATE/2)) * ((float)(_idx)) / ((float)(FFT_RESULT_SAMPLES_COUNT)) / 2.0F )

#define FFT_GAIN    (0.15f)
#define FFT_THRSH   (0.1f)

#define FFT_MAX_HEIGHT          (50)

#define FFT_BINS_COUNT   (21)
#define FFT_SAMPLES_PER_BIN  (FFT_RESULT_SAMPLES_COUNT / FFT_BINS_COUNT)

#define FFT_PRINTF_HEIGHT       (16)

typedef void(*results_processor_t)(float result[FFT_RESULT_SAMPLES_COUNT], float bins[FFT_BINS_COUNT]);

esp_err_t gsampler_inti(results_processor_t result_proc);

#ifdef __cplusplus
}
#endif