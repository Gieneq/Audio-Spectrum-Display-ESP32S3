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

static float heights[LED_MATRIX_COLUMNS];
static float velocities[LED_MATRIX_COLUMNS];

const float gravity_acceleration = -60.0F;

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

static void init_effect() {

    for (size_t bar_idx = 0; bar_idx < LED_MATRIX_COLUMNS; bar_idx++) {
        heights[bar_idx] = 0.0F;
        velocities[bar_idx] = 0.0F;
    }
    
}

static void process_results_draw_effect(
    float delta_time,
    float result[FFT_RESULT_SAMPLES_COUNT],  
    float result_grad[FFT_RESULT_SAMPLES_COUNT], 
    float sum_result,
    float sum_grad,
    float bins[FFT_BINS_COUNT],
    float bins_grad[FFT_BINS_COUNT],
    float beat_positive_grad,
    bool beat
) {


    static int log_counter;
    if(++log_counter >= 10) {
        log_counter = 0;
        // printf("%f, %f, Beat: %f, %d\n\n", sum_result, sum_grad, beat_positive_grad, beat);

    //     printf("result = [");
    //     for (size_t i = 0; i < FFT_RESULT_SAMPLES_COUNT; i++) {
    //         printf("%.2f, ", result[i]);
    //     }
    //     printf("]\n");
    
        // printf("bins_grad = [");
        // for (size_t i = 0; i < FFT_BINS_COUNT; i++) {
        //     printf("%.2f, ", bins_grad[i]);
        // }
        // printf("]\n");
    }


    /* Apply gravity and velocity */
    for (size_t bar_idx = 0; bar_idx < LED_MATRIX_COLUMNS; bar_idx++) {

        // /* Base vel clamped to 0 to 5000, only positive */
        float bin_base_vel = CONSTRAIN(bins_grad[bar_idx], 0.0F, 5000.0F);

        // /* Base vel scaled: 0 to 5000 / 32 = 156.25 */
        // bin_base_vel /= 16;

        float* height = &heights[bar_idx];
        float* velocity = &velocities[bar_idx];

        /* Apply gravity */
        *velocity += gravity_acceleration * delta_time;
        if(*height <= 0.0F) {
            *velocity = 0.0F;
        }

        *height += ((*velocity + bin_base_vel) * delta_time);
        *height = CONSTRAIN(*height, 0.0F, (float)LED_MATRIX_ROWS);

        led_matrix.columns_heights[bar_idx] = (uint8_t)((int32_t)*height);
    }
    

    // for(uint16_t col_idx = 0; col_idx < LED_MATRIX_COLUMNS; ++col_idx) {
    //     const float raw_valuef = bins[col_idx];
    //     const uint16_t raw_value = raw_valuef < 0.0F ? 0 : (uint16_t)(raw_valuef);
    //     const uint16_t col_height = raw_value > LED_MATRIX_ROWS ? LED_MATRIX_ROWS : raw_value;
    //     led_matrix.columns_heights[col_idx] = col_height;
    // }

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
    init_effect();

    esp_err_t ret = ESP_OK;

    ret = gsampler_inti(process_results_draw_effect);
    ESP_ERROR_CHECK(ret);

    ret = gdisplay_lcd_init();
    ESP_ERROR_CHECK(ret);

    while(1) {
        vTaskDelay(1000);
    }
}
