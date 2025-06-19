#include "sources.h"
#include <stdlib.h>
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
#include "esp_mac.h"
#include "esp_now.h"
#include "esp_crc.h"

static const char *TAG = "SOURCE_SIMULATION";

static input_samples_window_t input_samples_window;

static void source_simulation_task(void *pvParameter) {
    ESP_LOGI(TAG, "Start task");
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t source_simulation_init() {
    
    xTaskCreate(source_simulation_task, "source_simulation_task", 3 * 1024, NULL, 4, NULL);
    
    return ESP_OK;
}

const input_samples_window_t* source_simulation_await_input_samples_window(TickType_t timeout_tick_time) {
    // TODO
    vTaskDelay(pdMS_TO_TICKS(10));
    return &input_samples_window;
}