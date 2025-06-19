#include "sources.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"
#include "../gtypes.h"

#include "esp_dsp.h"

static const char *TAG = "SOURCES";

// 0 - linear, 1 to custom
#define USE_CUSTOM_BINS_MAPPING 1

extern const uint8_t bins_map[FFT_RESULT_SIZE];

static float fft_input_complex[FFT_SIZE * 2] __attribute__((aligned(16)));
static float fft_magnitude[FFT_RESULT_SIZE] __attribute__((aligned(16)));

static float window[FFT_SIZE]  __attribute__((aligned(16)));

// Simulation
extern esp_err_t source_simulation_init();
extern const input_samples_window_t* source_simulation_await_input_samples_window(TickType_t timeout_tick_time);

// Microphones
extern esp_err_t source_microphones_init();
extern const input_samples_window_t* source_microphones_await_input_samples_window(TickType_t timeout_tick_time);

static processed_input_result_t processed_input_result;

float common_gain = 1.0F;
float common_frequency_hz = 1000.0F;

esp_err_t sources_init_all() {
    ESP_LOGI(TAG, "Initializing");

    assert(((uintptr_t)fft_input_complex % 16) == 0);
    ESP_LOGI(TAG, "fft_input_complex at %p", fft_input_complex);
    
    assert(((uintptr_t)fft_magnitude % 16) == 0);
    ESP_LOGI(TAG, "fft_magnitude at %p", fft_magnitude);
    
    assert(((uintptr_t)bins_map % 16) == 0);
    ESP_LOGI(TAG, "bins_map at %p", bins_map);

    esp_err_t ret = ESP_OK;

    ret = dsps_fft2r_init_fc32(NULL, FFT_SIZE);
    ESP_RETURN_ON_ERROR(ret, TAG, "Not possible to initialize FFT.");

    dsps_wind_hann_f32(window, FFT_SIZE);

    ret = source_simulation_init();
    if (ret != ESP_OK) {
        return ret;
    }

    ret = source_microphones_init();
    if (ret != ESP_OK) {
        return ret;
    }

    return ret;
}

void sources_set_gain(float new_gain) {
    common_gain = new_gain;
}

void sources_set_frequency(float new_frequency_hz) {
    common_frequency_hz = new_frequency_hz;
}

static void run_fft(const float* samples, float* out_fft_magnitude) {
    // Fill complex buffer: real = sample, imag = 0
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_input_complex[2 * i + 0] = samples[i] * window[i];;
        fft_input_complex[2 * i + 1] = 0.0f;
    }

    dsps_fft2r_fc32(fft_input_complex, FFT_SIZE);
    dsps_bit_rev_fc32(fft_input_complex, FFT_SIZE);
    dsps_cplx2reC_fc32(fft_input_complex, FFT_SIZE);

    for (int i = 0; i < FFT_RESULT_SIZE; i++) {
        float real = fft_input_complex[2 * i];
        float imag = fft_input_complex[2 * i + 1];
        float mag = sqrtf(real * real + imag * imag);
        out_fft_magnitude[i] = mag > FFT_THRESHOLD ? mag : 0.0f;
    }
}
static void fft_fill_bins(const float* fft_mag, float* out_bins) {
    memset(out_bins, 0, sizeof(float) * BINS_COUNT);

    for (int i = 0; i < FFT_RESULT_SIZE; i++) {
#if USE_CUSTOM_BINS_MAPPING == 0
        uint8_t bin_idx = i / 54;
        bin_idx = MIN(bin_idx, BINS_COUNT - 1);
#else
        uint8_t bin_idx = bins_map[i];
#endif
        if (bin_idx < BINS_COUNT) {
            out_bins[bin_idx] += fft_mag[i];
        }
    }
    
    // scale 
    for (int bin_idx = 0; bin_idx < BINS_COUNT; bin_idx++) {
        out_bins[bin_idx] *= FFT_SCALE;
    }
}

static esp_err_t process_input_samples_window(const input_samples_window_t* input_samples_window, processed_input_result_t* result) {
    ESP_LOGV(TAG, "Samples: %lu us [%f, %f, %f, %f, %f] ", 
        input_samples_window->timestamp_us,
        input_samples_window->samples[0],
        input_samples_window->samples[1],
        input_samples_window->samples[2],
        input_samples_window->samples[3],
        input_samples_window->samples[4]
    );

    run_fft(input_samples_window->samples, fft_magnitude);
    fft_fill_bins(fft_magnitude, result->bins);

    // printf("bins (%lu) =[", input_samples_window->timestamp_us);
    // for (int i = 0; i < BINS_COUNT; i++) {
    //     printf("%.2f, ", result->bins[i]);
    // }
    // printf("]\n");

    return ESP_OK;
}

const processed_input_result_t* sources_await_processed_input_result(effects_source_t source, TickType_t timeout_tick_time) {
    const input_samples_window_t* input_samples_window = NULL;

    switch (source) {
    case EFFECTS_SOURCE_SIMULATION:
        input_samples_window = source_simulation_await_input_samples_window(timeout_tick_time);
        break;

    case EFFECTS_SOURCE_MICROPHONE:
        input_samples_window = source_microphones_await_input_samples_window(timeout_tick_time);
        break;

    case EFFECTS_SOURCE_WIRED:
        ESP_LOGW(TAG, "Not implemented");
        input_samples_window = NULL;
        break; // TODO

    default:
        ESP_LOGW(TAG, "Not implemented");
        input_samples_window = NULL;
        break;
    }

    if (input_samples_window != NULL) {
        process_input_samples_window(input_samples_window, &processed_input_result);
        return &processed_input_result;
    }

    return NULL;
}