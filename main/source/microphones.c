#include "sources.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <assert.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "freertos/ringbuf.h"
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
#include "bsp/esp-bsp.h"
#include "esp_timer.h"
#include "../gtypes.h"

#define MIC_RECORDING_BUFF_LENGHT   (512U)
#define RINGBUFFER_SIZE (8 * MIC_RECORDING_BUFF_LENGHT)
#define MIC_RECORDING_BUFF_SIZE  (MIC_RECORDING_BUFF_LENGHT * sizeof(int16_t))
#define MIC_RECORDING_SAMPLE_RATE     (SAMPLING_RATE_HZ)
#define SAMPLER_GAIN (35.0) //42.0

#define CONVERTION_AMPLITUDE_MAX (400.0)

static const char *TAG = "SOURCE_MICROPHONES";

extern float common_gain;

static input_samples_window_t input_samples_window;

static EventGroupHandle_t event_group = NULL;

static RingbufHandle_t ringbuff_handle;
static int16_t receiver_buffer[INPUT_SAMPLES_COUNT];
static int16_t recording_buffer[MIC_RECORDING_BUFF_LENGHT] = {0};

#ifdef DEBUG_SOURCE_MICROPHONE
#define DEBUG_SOURCE_MICROPHONE_LOG_INTERVAL_MS 250
static int64_t last_debug_log_time_us;

static int64_t last_codec_read_time_us;
static int64_t recent_codec_read_time_us;
static int64_t interval_codec_read_time_us;

static int64_t last_ringbuffer_receive_time_us;
static int64_t recent_ringbuffer_receive_time_us;
static int64_t interval_ringbuffer_receive_time_us;
static size_t last_ringbuffer_samples_received_count;
#endif

static void mic_sampler_task(void* params) {
    int ret = ESP_CODEC_DEV_OK;

    esp_codec_dev_handle_t mic_codec_dev = bsp_audio_codec_microphone_init();

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

    while(1) {   
        ESP_ERROR_CHECK(esp_codec_dev_read(mic_codec_dev, recording_buffer, MIC_RECORDING_BUFF_SIZE));
        // Capturing 512 samples every: 10885 - 16339 us, mostly lover time.

#ifdef DEBUG_SOURCE_MICROPHONE
        recent_codec_read_time_us = esp_timer_get_time();
        interval_codec_read_time_us = recent_codec_read_time_us - last_codec_read_time_us;
        last_codec_read_time_us = recent_codec_read_time_us;
#endif

        if(xRingbufferSend(ringbuff_handle, (void*)recording_buffer, MIC_RECORDING_BUFF_SIZE, 0) == pdFALSE) {
            ESP_LOGW(TAG, "Microphone samples ringbuffer stall!");
        }
    }
}

static void source_microphones_task(void* params) {
    ESP_LOGI(TAG, "Start task");
#ifdef DEBUG_SOURCE_MICROPHONE
    last_debug_log_time_us = esp_timer_get_time();
#endif

    size_t receiver_buffer_idx = 0;
    size_t remaining_halfwords_count = INPUT_SAMPLES_COUNT;

    while (1) {
        size_t actual_bytes_count = 0;
        const size_t max_bytes_to_receive = remaining_halfwords_count * sizeof(int16_t);

        int16_t* data = (int16_t*)xRingbufferReceiveUpTo(
            ringbuff_handle, 
            &actual_bytes_count, 
            portMAX_DELAY, 
            max_bytes_to_receive
        );

        if (data && actual_bytes_count > 0) {
            // I noticed: actual_bytes_count is: 1024 or 2048

            size_t actual_halfwords_count = actual_bytes_count / sizeof(int16_t);
            memcpy(receiver_buffer + receiver_buffer_idx, data, actual_bytes_count);
            receiver_buffer_idx += actual_halfwords_count;
            remaining_halfwords_count -= actual_halfwords_count;
            vRingbufferReturnItem(ringbuff_handle, (void*)data);

            if (remaining_halfwords_count == 0) {
            // I noticed: interval: 30074 - 51144
            
#ifdef DEBUG_SOURCE_MICROPHONE
            recent_ringbuffer_receive_time_us = esp_timer_get_time();
            interval_ringbuffer_receive_time_us = recent_ringbuffer_receive_time_us - last_ringbuffer_receive_time_us;
            last_ringbuffer_receive_time_us = recent_ringbuffer_receive_time_us;
            last_ringbuffer_samples_received_count = actual_bytes_count;
#endif
                // Fill output buffer
                for (size_t i = 0; i < INPUT_SAMPLES_COUNT; ++i) {
                    const float sample_scaled = ((float)receiver_buffer[i]) * common_gain;
                    const float sample_normalized = sample_scaled / CONVERTION_AMPLITUDE_MAX;
                    const float sample_normalized_constrained = CONSTRAIN(sample_normalized, -1.0F, 1.0F);
                    input_samples_window.samples[i] = sample_normalized_constrained;
                }

                input_samples_window.timestamp_us = esp_timer_get_time();

                // Notify
                xEventGroupSetBits(event_group, SAMPLES_READY_BIT);

                // Reset for next round
                receiver_buffer_idx = 0;
                remaining_halfwords_count = INPUT_SAMPLES_COUNT;
            }
        }

#ifdef DEBUG_SOURCE_MICROPHONE
        if (esp_timer_get_time() > (last_debug_log_time_us + DEBUG_SOURCE_MICROPHONE_LOG_INTERVAL_MS * 1000)) {
            last_debug_log_time_us = esp_timer_get_time();
            ESP_LOGI(TAG, "micr: %lld us, micr_cnt: %u, rb_rcv: %lld us, rb_cnt: %u/%u",
                interval_codec_read_time_us, MIC_RECORDING_BUFF_LENGHT,
                interval_ringbuffer_receive_time_us, last_ringbuffer_samples_received_count, INPUT_SAMPLES_COUNT
            );
        }
#endif

    }

}

esp_err_t source_microphones_init() {
    event_group = xEventGroupCreate();
    if (event_group == NULL) {
        ESP_LOGE(TAG, "Create event_group fail");
        return ESP_FAIL;
    }

    ringbuff_handle = xRingbufferCreate(RINGBUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    ESP_RETURN_ON_FALSE(ringbuff_handle, ESP_FAIL, TAG, "Failed creating ringbuffer!");

    xTaskCreate(source_microphones_task, "source_microphones_task", 6 * 1024, NULL, 4, NULL);
    xTaskCreate(mic_sampler_task, "mic_sampler_task", 6 * 1024, NULL, 10, NULL);
    
    return ESP_OK;
}

const input_samples_window_t* source_microphones_await_input_samples_window(TickType_t timeout_tick_time) {
    if (event_group == NULL) {
        return NULL;
    }

    EventBits_t bits = xEventGroupWaitBits(
        event_group,
        SAMPLES_READY_BIT,
        pdTRUE,     // Clear on exit
        pdFALSE,    // Wait for any bit (only one defined)
        portMAX_DELAY
    );

    if (bits & SAMPLES_READY_BIT) {
        return &input_samples_window;
    }

    return NULL;
}
