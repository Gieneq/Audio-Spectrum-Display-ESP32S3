#include "gsampler.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "esp_timer.h"

#include "bsp/esp-bsp.h"

#include "esp_log.h"

static const char TAG[] = "Main";

static RingbufHandle_t ringbuff_handle;

// static int16_t mic_buffer[MIC_BUFF_SIZE];

static void mic_sampler_task(void* params) {
    int ret = ESP_CODEC_DEV_OK;

    esp_codec_dev_handle_t mic_codec_dev = bsp_audio_codec_microphone_init();
    int16_t recording_buffer[MIC_BUFF_SIZE] = {0};

    // int16_t *recording_buffer = heap_caps_malloc(MIC_BUFF_SIZE, MALLOC_CAP_DEFAULT);
    // assert(recording_buffer != NULL);

    if (mic_codec_dev == NULL) {
        ESP_LOGW(TAG, "This board does not support microphone recording!");
        assert(mic_codec_dev != NULL);
    }

    ret = esp_codec_dev_set_in_gain(mic_codec_dev, 42.0);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed setting volume");
    }

    esp_codec_dev_sample_info_t fs = {
        .sample_rate = SAMPLE_RATE,
        .channel = 1,
        .bits_per_sample = 16,
    };

    ret = esp_codec_dev_open(mic_codec_dev, &fs);
    if (ret != ESP_CODEC_DEV_OK) {
        ESP_LOGE(TAG, "Failed opening the device");
    }

    int64_t last_time = esp_timer_get_time();
    int64_t recent_time = last_time;
    int64_t interval_us = 0;
    int64_t samples_collected = 0;
    float sampling_rate = 0;
    int16_t counter = 0;
    int16_t tmp_sample = 0;
    while(1) {
        //todo not sure bytes or halfwords
        
        if(counter == 50) {
            last_time = recent_time;
            recent_time = esp_timer_get_time();
            interval_us = recent_time - last_time;

            if(samples_collected == 0) {
                sampling_rate = 0;
            } else {
                sampling_rate = 1000000.0F * ((float)samples_collected) / ((float)interval_us);
            }
            
            ESP_LOGE(TAG, "Batch done! interval_us=%lldus, sampling_rate(in bytes?)=%f, samples_collected=%lld.", 
                interval_us, sampling_rate, samples_collected);

            counter = 0;
            samples_collected = 0;
        }
        ++counter;

        // ESP_ERROR_CHECK(esp_codec_dev_read(mic_codec_dev, recording_buffer, MIC_BUFF_SIZE));
        for(int i=0; i<MIC_BUFF_SIZE; ++i) {
            recording_buffer[i] = tmp_sample++;
        }
        vTaskDelay(50);

        samples_collected += MIC_BUFF_SIZE;

        if(xRingbufferSend(ringbuff_handle, (void*)recording_buffer, MIC_BUFF_SIZE * sizeof(int16_t), portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Generating test stall!");
        }
    }
}

static void gsampler_task(void* params) {
    int16_t* data = NULL;
    // UBaseType_t ringbuff_waiting_bytes_count = 0;
    size_t actual_halfwords_count = 0;
    size_t actual_bytes_count = 0;

    int16_t receiver_buffer[SAMPLES_COUNT] = {0};
    size_t remaining_halfwords_count = SAMPLES_COUNT; // * sizeof(int16_t)
    size_t receiver_buffer_idx = 0;


    while(1) {
        const size_t max_bytes_count_to_return = remaining_halfwords_count * sizeof(int16_t);
        data = (int16_t*)xRingbufferReceiveUpTo(ringbuff_handle, &actual_bytes_count, portMAX_DELAY, max_bytes_count_to_return);
        actual_halfwords_count = actual_bytes_count / 2;

        if(actual_halfwords_count > 0) {
            ESP_LOGW(TAG, "data=%d..%d, remaining_halfwords_count=%u, actual_halfwords_count=%u, target=%u", 
                data[0], data[actual_halfwords_count - 1], remaining_halfwords_count, actual_halfwords_count, SAMPLES_COUNT);
            memcpy((void*)(receiver_buffer + receiver_buffer_idx), (void*)data, actual_bytes_count);
            receiver_buffer_idx += actual_halfwords_count;
            remaining_halfwords_count -= actual_halfwords_count;
            vRingbufferReturnItem(ringbuff_handle, (void*)data);

            if (remaining_halfwords_count == 0) {
                ESP_LOGI(TAG, "Got enough. receiver_buffer=%d..%d, target=%u", receiver_buffer[0], receiver_buffer[receiver_buffer_idx - 1], SAMPLES_COUNT);
            
                receiver_buffer_idx = 0;
                remaining_halfwords_count = SAMPLES_COUNT;
            }
        }


        vTaskDelay(1);
    }
}


esp_err_t gsampler_inti() {
    esp_err_t ret = ESP_OK;

    ringbuff_handle = xRingbufferCreate(RINGBUFFER_SIZE, RINGBUF_TYPE_BYTEBUF);
    if(ringbuff_handle == NULL) {
        ESP_LOGE(TAG, "Failed creating ringbuffer!");
        ret = ESP_FAIL;
    }

    ret = esp_timer_init();
    if(ret == ESP_ERR_NO_MEM) {
        ESP_LOGE(TAG, "Failed initializing timer!");
    }

    ret = ESP_OK;
    
    if(ret == ESP_OK) {
        (void)xTaskCreate(gsampler_task, "gsampler_task", 1024 * 8, NULL, 10, NULL);
        
        (void)xTaskCreate(mic_sampler_task, "mic_sampler_task", 1024 * 8, NULL, 5, NULL);
    }

    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "Gsampler initialized successfully!");
    }
    return ret;
}