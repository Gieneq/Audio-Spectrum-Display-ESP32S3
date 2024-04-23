#include "gsampler.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "esp_timer.h"

#include "bsp/esp-bsp.h"
#include <math.h>
#include "esp_dsp.h"

#include "esp_log.h"

#include "esp_check.h"

static const char TAG[] = "Main";

static RingbufHandle_t ringbuff_handle;
results_processor_t result_processor;

__attribute__((aligned(16))) float window_time_domain[RECEIVER_SAMPLES_COUNT];
__attribute__((aligned(16))) float testsig[RECEIVER_SAMPLES_COUNT];
__attribute__((aligned(16))) float fft_complex_workspace[FFT_WORKSPACE_BUFFER_SIZE];
__attribute__((aligned(16))) float fft_result[FFT_RESULT_SAMPLES_COUNT];

__attribute__((aligned(16))) float fft_bins[FFT_BINS_COUNT];

static const uint8_t bins_map[1024] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 19, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20};

typedef struct freq_ampl_t {
    size_t idx;
    float freq;
    float ampl;
} freq_ampl_t;

static freq_ampl_t fft_get_max_freq(float fft_res[FFT_RESULT_SAMPLES_COUNT]) {
    assert(fft_result);

    freq_ampl_t result = {
        .idx=0,
        .freq=SAMPLE_IDX_TO_FREQ(0),
        .ampl=fft_res[0]
    };

    for (int sample_idx = 1; sample_idx < FFT_RESULT_SAMPLES_COUNT; sample_idx++) {
        if (fft_res[sample_idx] > result.ampl) {
            result.idx = sample_idx;
            result.ampl = fft_res[sample_idx];
        }
    }

    result.freq = SAMPLE_IDX_TO_FREQ(result.idx);

    return result;
}

static void fft_fill_bins(float fft_res[FFT_RESULT_SAMPLES_COUNT], float fft_dst_bins[FFT_BINS_COUNT]) {
    memset(fft_dst_bins, 0, sizeof(float) * FFT_BINS_COUNT);

    for(int i=0; i<FFT_RESULT_SAMPLES_COUNT; ++i) {
        fft_dst_bins[bins_map[i]] += fft_res[i];
    }
}

// static void fft_printf(const float fft_printf_bins[FFT_BINS_COUNT]) {
//     /* Draw graph */
//     for(int iy=0; iy<FFT_PRINTF_HEIGHT; ++iy) {
//         const int height_threshold = FFT_PRINTF_HEIGHT - iy - 1 + 1;

//         for(int ix=0; ix<FFT_BINS_COUNT; ++ix) {
//             const int16_t height_value = (int16_t)fft_printf_bins[ix];
//             printf("%s", height_value >= height_threshold ? "|" : " ");
//         }
//         printf("\n");
//     }

//     for(int ix=0; ix<FFT_BINS_COUNT; ++ix) {
//         printf("_");
//     }
//     printf("\n");
// }


    // dsps_bit_rev_fc32(workspace, RECEIVER_SAMPLES_COUNT);
        // workspace[i] = workspace[i] / INT16_MAX;

static void fft_process(int16_t samples[RECEIVER_SAMPLES_COUNT], float workspace[FFT_WORKSPACE_BUFFER_SIZE], float fft_result[FFT_RESULT_SAMPLES_COUNT]) {
    // for (size_t sample_idx = 0 ; sample_idx < RECEIVER_SAMPLES_COUNT; sample_idx++) {
    //     // workspace[sample_idx] = ((float)samples[sample_idx]) / 4096.0F;
    //     workspace[sample_idx] = ((float)samples[sample_idx]) / ((float)INT16_MAX);
    // }

    dsps_wind_hann_f32(window_time_domain, RECEIVER_SAMPLES_COUNT);
    // dsps_tone_gen_f32(testsig, RECEIVER_SAMPLES_COUNT, 1.0, 0.16,  0);


    assert(FFT_WORKSPACE_BUFFER_SIZE == (RECEIVER_SAMPLES_COUNT * 2));
    for (int i = 0; i < RECEIVER_SAMPLES_COUNT; i++) {
        testsig[i] = ((float)samples[i]) / 4096.0F;
        workspace[2 * i + 0] = testsig[i] * window_time_domain[i];
        workspace[2 * i + 1] = 0;
    }

    dsps_fft2r_fc32(workspace, RECEIVER_SAMPLES_COUNT);

    dsps_bit_rev_fc32(workspace, RECEIVER_SAMPLES_COUNT);
    // Convert one complex vector to two complex vectors
    dsps_cplx2reC_fc32(workspace, RECEIVER_SAMPLES_COUNT);

    // The result of FFT is now in workspace arranged as interleaved real and imaginary parts.
    // Calculate magnitudes and store them in fft_result
    for (int i = 0; i < FFT_RESULT_SAMPLES_COUNT; i++) {
        float real = workspace[2 * i];  // Real part at even indices
        float imag = workspace[2 * i + 1];  // Imaginary part at odd indices

        fft_result[i] = sqrtf(real * real + imag * imag);
        if(fft_result[i] < FFT_THRSH) {
            fft_result[i] = 0;
        }
        fft_result[i] *= FFT_GAIN;    
    }
}

static void mic_sampler_task(void* params) {
    int ret = ESP_CODEC_DEV_OK;

    esp_codec_dev_handle_t mic_codec_dev = bsp_audio_codec_microphone_init();
    int16_t recording_buffer[MIC_RECORDING_BUFF_LENGHT] = {0};

    if (mic_codec_dev == NULL) {
        ESP_LOGW(TAG, "This board does not support microphone recording!");
        assert(mic_codec_dev != NULL);
    }

    ret = esp_codec_dev_set_in_gain(mic_codec_dev, SAMPLER_GAIN);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed setting volume");
    }
    
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = MIC_RECORDING_SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };

    ret = esp_codec_dev_open(mic_codec_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed opening the device");
        ESP_ERROR_CHECK(ret);
    }

    int64_t last_time = esp_timer_get_time();
    int64_t recent_time = last_time;
    int64_t interval_us = 0;
    int64_t samples_collected = 0;
    float sampling_rate = 0;
    int16_t counter = 0;

#ifdef USE_TEST_SAMPLES
    int16_t test_sample = 0;
#endif

    while(1) {        
        if(counter == 50) {
            last_time = recent_time;
            recent_time = esp_timer_get_time();
            interval_us = recent_time - last_time;

            if(samples_collected == 0) {
                sampling_rate = 0;
            } else {
                sampling_rate = 1000000.0F * ((float)samples_collected) / ((float)interval_us);
            }
            
            ESP_LOGV(TAG, "Batch done! interval_us=%lldus, sampling_rate(in bytes?)=%f, samples_collected=%lld.", 
                interval_us, sampling_rate, samples_collected);

            counter = 0;
            samples_collected = 0;
        }
        ++counter;

#ifdef USE_TEST_SAMPLES
        for(int i=0; i<MIC_RECORDING_BUFF_LENGHT; ++i) {
            recording_buffer[i] = test_sample++;
        }
        vTaskDelay(1);
#else
        ESP_ERROR_CHECK(esp_codec_dev_read(mic_codec_dev, recording_buffer, MIC_RECORDING_BUFF_SIZE));
#endif

        samples_collected += MIC_RECORDING_BUFF_LENGHT;

        if(xRingbufferSend(ringbuff_handle, (void*)recording_buffer, MIC_RECORDING_BUFF_SIZE, 0) == pdFALSE) {
            ESP_LOGW(TAG, "Generating test stall!");
        }
    }
}

static void gsampler_processor_task(void* params) {
    int16_t* data = NULL;

    size_t actual_halfwords_count = 0;
    size_t actual_bytes_count = 0;

    int16_t receiver_buffer[RECEIVER_SAMPLES_COUNT] = {0};
    size_t remaining_halfwords_count = RECEIVER_SAMPLES_COUNT;
    size_t receiver_buffer_idx = 0;

    int32_t real_duration = 0;
    int64_t last_time = esp_timer_get_time();

    int some_couner = 0;

    while(1) {
        const size_t max_bytes_count_to_return = remaining_halfwords_count * sizeof(int16_t);
        data = (int16_t*)xRingbufferReceiveUpTo(ringbuff_handle, &actual_bytes_count, 1, max_bytes_count_to_return);
        actual_halfwords_count = actual_bytes_count / 2;

        if(data && (actual_halfwords_count > 0)) {
            ESP_LOGV(TAG, "data=%d..%d, remaining_halfwords_count=%u, actual_halfwords_count=%u, target=%u", 
                data[0], data[actual_halfwords_count - 1], remaining_halfwords_count, actual_halfwords_count, RECEIVER_SAMPLES_COUNT);
            
            memcpy((void*)(receiver_buffer + receiver_buffer_idx), (void*)data, actual_bytes_count);
            receiver_buffer_idx += actual_halfwords_count;
            remaining_halfwords_count -= actual_halfwords_count;
            vRingbufferReturnItem(ringbuff_handle, (void*)data);

            if (remaining_halfwords_count == 0) {
                /* Got enough */

                const int64_t recent_time = esp_timer_get_time();
                real_duration = (int32_t)((recent_time - last_time) / 1000);
                last_time = recent_time;
                fft_process(receiver_buffer, fft_complex_workspace, fft_result);
                const freq_ampl_t fft_max_freq = fft_get_max_freq(fft_result);
                fft_fill_bins(fft_result, fft_bins);

                ESP_LOGV(TAG, "Got %u samples, recev_buff=%d..%d, dur=%ld/%ldms, fft=[%.2f, %.2f, %.2f, %.2f], max=%.2fHz/%.2f.", 
                    receiver_buffer_idx,
                    receiver_buffer[0], receiver_buffer[receiver_buffer_idx - 1],
                    RECEIVER_SAMPLING_DURATION_MS, real_duration,
                    fft_result[0], fft_result[1], fft_result[2], fft_result[3],
                    fft_max_freq.freq, fft_max_freq.ampl
                );

                result_processor(fft_result, fft_bins);
                if(++some_couner > 50) {
                    some_couner = 0;

                    // fft_printf(fft_bins);

    
                    // dsps_view(fft_complex_workspace, RECEIVER_SAMPLES_COUNT / 2, 64, 10,  -60, 40, '|');

                    // printf("samples = [");
                    // for(int i=0; i<RECEIVER_SAMPLES_COUNT; ++i) {
                    //     printf("%d%s", receiver_buffer[i], i < (RECEIVER_SAMPLES_COUNT - 1) ? ", " : "]\n");
                    // }

                    // printf("fft_res = [");
                    // for(int i=0; i<FFT_RESULT_SAMPLES_COUNT; ++i) {
                    //     printf("%.3f%s", fft_result[i], i < (FFT_RESULT_SAMPLES_COUNT - 1) ? ", " : "]\n");
                    // }
                }

                receiver_buffer_idx = 0;
                remaining_halfwords_count = RECEIVER_SAMPLES_COUNT;
            }
        }

    }
}


esp_err_t gsampler_inti(results_processor_t result_proc) {
    esp_err_t ret = ESP_OK;

    result_processor = result_proc;
    assert(result_processor);

    ringbuff_handle = xRingbufferCreate(RINGBUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    ESP_RETURN_ON_FALSE(ringbuff_handle, ESP_FAIL, TAG, "Failed creating ringbuffer!");
    
    ret = dsps_fft2r_init_fc32(NULL, CONFIG_DSP_MAX_FFT_SIZE);
    ESP_RETURN_ON_ERROR(ret, TAG, "Not possible to initialize FFT.");

    dsps_wind_hann_f32(window_time_domain, RECEIVER_SAMPLES_COUNT);

    const bool processor_task_was_created = xTaskCreate(
        gsampler_processor_task, 
        "gsampler_processor_task", 
        1024 * 8, 
        NULL, 
        10, 
        NULL
    ) == pdTRUE;
    ESP_RETURN_ON_FALSE(processor_task_was_created, ESP_FAIL, TAG, "Failed creating \'gsampler_processor_task\'!");
    
    const bool mic_sampler_task_was_created = xTaskCreate(
        mic_sampler_task, 
        "mic_sampler_task", 
        1024 * 16, 
        NULL, 
        5, 
        NULL
    ) == pdTRUE;
    ESP_RETURN_ON_FALSE(mic_sampler_task_was_created, ESP_FAIL, TAG, "Failed creating \'mic_sampler_task\'!");

    return ret;
}