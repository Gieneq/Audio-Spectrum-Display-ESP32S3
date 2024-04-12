#include "gsampler.h"

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/ringbuf.h"

#include "esp_log.h"

static const char TAG[] = "Main";

static RingbufHandle_t ringbuff_handle;

// #define MIC_BUFF_SIZE (1024)
// static int16_t mic_buffer[MIC_BUFF_SIZE];

// static void test_generator_task(void* params) {
//     int16_t value = 1<<8;
//     int16_t sample = 0;

//     while(1) {
//         sample = value++;
//         ESP_LOGW(TAG, "Generating test sample: %u, burst:%u", sample, sample % 10);

//         if(sample % 10 == 0) {
//             // burst
//             for (int i = 0; i < 5; i++) {
//                 /* code */
//                 if(xRingbufferSend(ringbuff_handle, (void*)&sample, sizeof(sample), portMAX_DELAY) == pdFALSE) {
//                     ESP_LOGE(TAG, "Generating test stall!");
//                 }
//             }
//         } else {
//             if(xRingbufferSend(ringbuff_handle, (void*)&sample, sizeof(sample), portMAX_DELAY) == pdFALSE) {
//                 ESP_LOGE(TAG, "Generating test stall!");
//             }
//         }

//         vTaskDelay(pdMS_TO_TICKS(1000));
//     }
// }

static void mice_sampler_task(void* params) {
    static int16_t sample = 0;
    while(1) {
        if(xRingbufferSend(ringbuff_handle, (void*)&sample, sizeof(sample), portMAX_DELAY) == pdFALSE) {
            ESP_LOGE(TAG, "Generating test stall!");
        }
        ++sample;
        vTaskDelay(1);
    }
}

static void gsampler_task(void* params) {
    int16_t* data = NULL;
    UBaseType_t ringbuff_waiting_bytes_count = 0;
    size_t item_size = 0;

    while(1) {
        vRingbufferGetInfo(ringbuff_handle, NULL, NULL, NULL, NULL, &ringbuff_waiting_bytes_count);
        // ESP_LOGI(TAG, "Samples in Ringbuffer: %u", ringbuff_waiting_bytes_count);

        if(ringbuff_waiting_bytes_count >= SAMPLES_COUNT * sizeof(int16_t)) {
            data = (int16_t*)xRingbufferReceiveUpTo(ringbuff_handle, &item_size, pdMS_TO_TICKS(200), SAMPLES_COUNT * sizeof(int16_t));

            if(item_size > 0) {
                ESP_LOGI(TAG, "Got enough. data[0]=%u, item_size=%u, target=%u", data[0], item_size, SAMPLES_COUNT * sizeof(int16_t));
                item_size = 0;
                vRingbufferReturnItem(ringbuff_handle, (void*)data);
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
    
    if(ret == ESP_OK) {
        (void)xTaskCreate(gsampler_task, "gsampler_task", 1024 * 8, NULL, 10, NULL);
        
        (void)xTaskCreate(mice_sampler_task, "mice_sampler_task", 1024 * 8, NULL, 5, NULL);
    }

    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "Gsampler initialized successfully!");
    }
    return ret;
}