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

#include "hmi/gdisplay.h"

static const char TAG[] = "Main";

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

void app_main(void) {
    info_prints();
    ESP_LOGI(TAG, "Starting!");

    esp_err_t ret = ESP_OK;

    ret = gsampler_inti();
    ESP_ERROR_CHECK(ret);

    ret = gdisplay_lcd_init();
    ESP_ERROR_CHECK(ret);

    // int16_t cnt = 0;
    while(1) {
        // model_interface_t* model_if = NULL;
        // if(display_access_model(&model_if, portMAX_DELAY)) {
        //     model_if->set_bar_heights(NULL, 0);
        //     display_release_model();
        // }
        vTaskDelay(1);
    }
}

        // if(!was_drawn) {
        //     const esp_err_t draw_ret = display_draw_custom(NULL);
        //     if(draw_ret != ESP_OK) {
        //         ESP_LOGW(TAG, "Draw failed! ret=%d.", draw_ret);
        //     }
        // }
        //         was_drawn = true;
        //         const esp_err_t draw_ret = display_draw_custom(fft_bins);
        //         if(draw_ret != ESP_OK) {
        //             ESP_LOGW(TAG, "Draw failed! ret=%d.", draw_ret);
        //         }