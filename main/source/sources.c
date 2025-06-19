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

static const char *TAG = "SOURCES";

extern esp_err_t source_simulation_init();

extern const input_samples_window_t* source_simulation_await_input_samples_window(TickType_t timeout_tick_time);

static processed_input_result_t processed_input_result;

esp_err_t sources_init_all() {
    ESP_LOGI(TAG, "Initializing");
    esp_err_t ret = ESP_OK;

    ret = source_simulation_init();
    if (ret != ESP_OK) {
        return ret;
    }

    return ret;
}

void sources_set_gain(float new_gain) {
    // TODO
}

const processed_input_result_t* sources_await_processed_input_result(effects_source_t source, TickType_t timeout_tick_time) {
    // TODO here probably I capture only samples and calculate fft

    const input_samples_window_t* input_samples_window = NULL;

    switch (source) {
    case EFFECTS_SOURCE_SIMULATION:
        input_samples_window = source_simulation_await_input_samples_window(timeout_tick_time);
        break;

    case EFFECTS_SOURCE_MICROPHONE:
        ESP_LOGW(TAG, "Not implemented");
        input_samples_window = NULL;
        break; // TODO

    case EFFECTS_SOURCE_WIRED:
        ESP_LOGW(TAG, "Not implemented");
        input_samples_window = NULL;
        break; // TODO

    default:
        ESP_LOGW(TAG, "Not implemented");
        input_samples_window = NULL;
        break; // TODO
    }

    // Process samples window into bins and similar
    // TODO
    return &processed_input_result;
}