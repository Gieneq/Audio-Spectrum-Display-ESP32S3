/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_log.h"

#include "gsampler.h"
#include "gdisplay.h"
#include "model.h"

static const char TAG[] = "Main";

static led_matrix_t led_matrix;

static void info_prints() {
    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
}

#define CONSTRAIN(_v, _min, _max) ((_v) < (_min) ? (_min) : ((_v) > (_max) ? (_max) : (_v)))

static void process_results(float result[FFT_RESULT_SAMPLES_COUNT], float bins[FFT_BINS_COUNT]) {
    // static int log_counter;

    // if(++log_counter > 50) {
    //     log_counter = 0;
    //     printf("result = [");
    //     for (size_t i = 0; i < FFT_RESULT_SAMPLES_COUNT; i++)
    //     {
    //         printf("%.2f, ", result[i]);
    //     }
    //     printf("]\n");
    // }

    for(uint16_t col_idx = 0; col_idx < LED_MATRIX_COLUMNS; ++col_idx) {
        const float raw_valuef = bins[col_idx];
        const uint16_t raw_value = raw_valuef < 0.0F ? 0 : (uint16_t)(raw_valuef);
        const uint16_t col_height = raw_value > LED_MATRIX_ROWS ? LED_MATRIX_ROWS : raw_value;
        led_matrix.columns_heights[col_idx] = col_height;
    }

    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_led_matrix_values(&led_matrix);
        model_interface_release();
    }
}

void app_main(void) {
    info_prints();
    ESP_LOGI(TAG, "Starting!");
    
    led_matrix_clear(&led_matrix);

    esp_err_t ret = ESP_OK;

    ret = gsampler_inti(process_results);
    ESP_ERROR_CHECK(ret);

    ret = gdisplay_lcd_init();
    ESP_ERROR_CHECK(ret);

    while(1) {
        vTaskDelay(1000);
    }
}
