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

extern esp_err_t sources_simulation_init();

extern const sources_sample_data_t* sources_simulation_await_sample_data(TickType_t timeout_tick_time);

esp_err_t sources_init_all() {
    ESP_LOGI(TAG, "Initializing");
    esp_err_t ret = ESP_OK;

    ret = sources_simulation_init();
    if (ret != ESP_OK) {
        return ret;
    }

    return ret;
}

const sources_sample_data_t* sources_await_sample_data(effects_source_t source, TickType_t timeout_tick_time) {
    switch (source) {
    case EFFECTS_SOURCE_SIMULATION:
        return sources_simulation_await_sample_data(timeout_tick_time);

    case EFFECTS_SOURCE_MICROPHONE:
        ESP_LOGW(TAG, "Not implemented");
        return NULL; // TODO

    case EFFECTS_SOURCE_WIRED:
        ESP_LOGW(TAG, "Not implemented");
        return NULL; // TODO

    default:
        ESP_LOGW(TAG, "Not implemented");
        return NULL; // TODO
    }
}