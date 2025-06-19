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
#include "nvs_flash.h"
#include "esp_random.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

static const char *TAG = "SOURCE_SIMULATION";

extern float common_gain;
extern float common_frequency_hz;

static input_samples_window_t input_samples_window;

static EventGroupHandle_t event_group = NULL;

static void source_simulation_task(void* params) {
    ESP_LOGI(TAG, "Start task");

    const float phase_step_base = 2.0f * (float)M_PI / SAMPLING_RATE_FHZ;
    float phase = 0.0f;
    uint32_t recent_time_us = 0;

    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t sampling_interval_ticks = pdMS_TO_TICKS((uint32_t)(roundf(INPUT_SAMPLING_INTERVAL_FMS))); // ~46 ms

    while (1) {
        float freq = common_frequency_hz;
        float gain = common_gain;
        float phase_step = phase_step_base * freq;

        input_samples_window.timestamp_us = recent_time_us;
        
        for (int i = 0; i < INPUT_SAMPLES_COUNT; i++) {
            input_samples_window.samples[i] = gain * sinf(phase);
            phase += phase_step;
            if (phase > 2.0f * (float)M_PI) {
                phase -= 2.0f * (float)M_PI;
            }
        }

        xEventGroupSetBits(event_group, SAMPLES_READY_BIT);

        // Wait until next scheduled period
        vTaskDelayUntil(&last_wake_time, sampling_interval_ticks);

        recent_time_us += INPUT_SAMPLING_INTERVAL_FMS * 1000.0F;
    }
}

esp_err_t source_simulation_init() {
    event_group = xEventGroupCreate();
    if (event_group == NULL) {
        ESP_LOGE(TAG, "Create event_group fail");
        return ESP_FAIL;
    }

    xTaskCreate(source_simulation_task, "source_simulation_task", 3 * 1024, NULL, 4, NULL);
    
    return ESP_OK;
}

const input_samples_window_t* source_simulation_await_input_samples_window(TickType_t timeout_tick_time) {
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
