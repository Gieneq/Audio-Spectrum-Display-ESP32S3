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
#include "esp_log.h"
#include "esp_check.h"
#include "../gtypes.h"
#include "esp_adc/adc_continuous.h"
#include "driver/gpio.h"
#include "soc/soc_caps.h"
#include "esp_timer.h"


#ifdef DEBUG_SOURCE_WIRED

#define DEBUG_SOURCE_WIRED_LOG_INTERVAL_MS 250
static int64_t last_debug_log_time_us;
static uint32_t last_continuous_adc_read_count;

static int64_t adc_done_last_time_us;
static int64_t adc_done_recent_time_us;
static int64_t adc_done_interval_us;

static int64_t last_continuous_adc_read_time_us;
static int64_t recent_continuous_adc_read_time_us;
static int64_t interval_continuous_adc_read_time_us;
#endif

static input_samples_window_t input_samples_window;

static EventGroupHandle_t event_group = NULL;

#define EXAMPLE_READ_LEN                    (INPUT_SAMPLES_COUNT * SOC_ADC_DIGI_DATA_BYTES_PER_CONV)

#define EXAMPLE_ADC_UNIT                    ADC_UNIT_1
#define _EXAMPLE_ADC_UNIT_STR(unit)         #unit
#define EXAMPLE_ADC_UNIT_STR(unit)          _EXAMPLE_ADC_UNIT_STR(unit)
#define EXAMPLE_ADC_CONV_MODE               ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_ATTEN                   ADC_ATTEN_DB_11
#define EXAMPLE_ADC_BIT_WIDTH               SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32S3
#define EXAMPLE_ADC_OUTPUT_TYPE             ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data)     ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data)        ((p_data)->type2.data)
#endif

static const char *TAG = "SOURCE_WIRED";

//https://www.espressif.com/sites/default/files/documentation/esp32-s3_technical_reference_manual_en.pdf
// Page 1468
// ADC1 CH0 -> GPIO1 (buttons)
// ADC1 CH8 -> GPIO9
// ADC1 CH9 -> GPIO10


static adc_continuous_handle_t handle = NULL;

static adc_channel_t channel[1] = {ADC_CHANNEL_8};

static uint8_t result[EXAMPLE_READ_LEN];

static TaskHandle_t task_handle = NULL;

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
    BaseType_t mustYield = pdFALSE;
    //Notify that ADC continuous driver has done enough number of conversions
    if (task_handle) {
    vTaskNotifyGiveFromISR(task_handle, &mustYield);
    }
    
#ifdef DEBUG_SOURCE_WIRED
    adc_done_recent_time_us = esp_timer_get_time();
    adc_done_interval_us = adc_done_recent_time_us - adc_done_last_time_us;
    adc_done_last_time_us = adc_done_recent_time_us;
#endif

    return (mustYield == pdTRUE);
}


static void compute_samples() {
    const float ADC_FULL_SCALE = 4095.0f;
    const float VREF = 3.3f;
    const float V_MID = VREF / 2.0f;

    size_t sample_idx = 0;

    for (int i = 0; i < EXAMPLE_READ_LEN; i += SOC_ADC_DIGI_RESULT_BYTES) {
        adc_digi_output_data_t *p = (void*)&result[i];
        const uint32_t data = EXAMPLE_ADC_GET_DATA(p);
        // data is 12b
        // input signal is 0-3.3V
        // assume middle 1.65V
        // Convert to voltage
        float voltage = (data / ADC_FULL_SCALE) * VREF;

        // Shift to be centered around 0
        float sample = voltage - V_MID;

        // Optional: normalize to -1.0 ... +1.0 range
        float normalized = sample / V_MID;

        assert(sample_idx  < sizeof(input_samples_window.samples));
        input_samples_window.samples[sample_idx++] = normalized;
        input_samples_window.timestamp_us = (uint32_t)esp_timer_get_time();
    }
}

static void source_wired_task(void* params) {
    ESP_LOGI(TAG, "Start task");

    esp_err_t ret;
    uint32_t ret_num = 0;
    ESP_ERROR_CHECK(adc_continuous_start(handle));

    while (1) {
        ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, pdMS_TO_TICKS(100));
        if (ret == ESP_OK) {
            if (ret_num != EXAMPLE_READ_LEN) {
                ESP_LOGW(TAG, "Received %lu ADC samples insted of %u.", ret_num, EXAMPLE_READ_LEN);
                vTaskDelay(pdMS_TO_TICKS(50));
                continue;
            }

#ifdef DEBUG_SOURCE_WIRED
            recent_continuous_adc_read_time_us = esp_timer_get_time();
            interval_continuous_adc_read_time_us = recent_continuous_adc_read_time_us - last_continuous_adc_read_time_us;
            last_continuous_adc_read_time_us = recent_continuous_adc_read_time_us;
            last_continuous_adc_read_count = ret_num;
#endif

            // Compute
            compute_samples();

            // Notify
            xEventGroupSetBits(event_group, SAMPLES_READY_BIT);
        }
        
#ifdef DEBUG_SOURCE_WIRED
        if (esp_timer_get_time() > (last_debug_log_time_us + DEBUG_SOURCE_WIRED_LOG_INTERVAL_MS * 1000)) {
            last_debug_log_time_us = esp_timer_get_time();
            ESP_LOGI(TAG, "adc: %lld us, adc_cnt: %lu/%u, callback: %lld us",
                interval_continuous_adc_read_time_us, 
                last_continuous_adc_read_count, EXAMPLE_READ_LEN,
                adc_done_interval_us
            );
        }
#endif

    }
    vTaskDelete(NULL);
}


static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle) {
    adc_continuous_handle_t handle = NULL;

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = 2 * EXAMPLE_READ_LEN, // in bytes
        .conv_frame_size = EXAMPLE_READ_LEN, // 4B * n
    };
    ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

    adc_continuous_config_t dig_cfg = {
        .sample_freq_hz = SAMPLING_RATE_HZ,
        .conv_mode = EXAMPLE_ADC_CONV_MODE,
        .format = EXAMPLE_ADC_OUTPUT_TYPE,
    };

    adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
    dig_cfg.pattern_num = channel_num;
    for (int i = 0; i < channel_num; i++) {
        adc_pattern[i].atten = EXAMPLE_ADC_ATTEN;
        adc_pattern[i].channel = channel[i];// & 0x7;
        adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
        adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

        ESP_LOGI(TAG, "adc_pattern[%d].atten is :%"PRIx8, i, adc_pattern[i].atten);
        ESP_LOGI(TAG, "adc_pattern[%d].channel is :%"PRIx8, i, adc_pattern[i].channel);
        ESP_LOGI(TAG, "adc_pattern[%d].unit is :%"PRIx8, i, adc_pattern[i].unit);
    }
    dig_cfg.adc_pattern = adc_pattern;
    ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

    *out_handle = handle;
}

esp_err_t source_wired_init() {
    event_group = xEventGroupCreate();
    if (event_group == NULL) {
        ESP_LOGE(TAG, "Create event_group fail");
        return ESP_FAIL;
    }
    
    memset(result, 0xcc, EXAMPLE_READ_LEN);

    assert(EXAMPLE_READ_LEN % SOC_ADC_DIGI_DATA_BYTES_PER_CONV == 0);
    gpio_reset_pin(GPIO_NUM_9);
    gpio_set_direction(GPIO_NUM_9, GPIO_MODE_INPUT);

    continuous_adc_init(channel, sizeof(channel) / sizeof(adc_channel_t), &handle);

    adc_continuous_evt_cbs_t cbs = {
        .on_conv_done = s_conv_done_cb,
    };
    ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));

    if (xTaskCreate(source_wired_task, "source_wired_task", 4 * 1024, NULL, 5, &task_handle) != pdTRUE) {
        ESP_LOGE(TAG, "Create source_wired_task fail");
        return ESP_FAIL;
    }

    if (task_handle == NULL) {
        ESP_LOGE(TAG, "Create source_wired_task fail reason handler NULL");
        return ESP_FAIL;
    }

    return ESP_OK;
}


const input_samples_window_t* source_wired_await_input_samples_window(TickType_t timeout_tick_time) {
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
