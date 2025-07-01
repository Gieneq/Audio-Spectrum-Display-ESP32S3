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
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "gdisplay.h"
#include "model.h"

#include "iot_button.h"
#include "bsp/esp-bsp.h"
#include "gtypes.h"

#include "rf/rf_sender.h"
#include "rf/asd_packet.h"
#include "effects/effects.h"

static const char TAG[] = "Main";

#define SIM_FREQUENCIES_COUNT 16
static const float SIM_FREQUENCIES[SIM_FREQUENCIES_COUNT] = {
    25.0F,

    50.0F,
    100.0F,
    250.0F,
    500.0F,
    1000.0F,
    
    2000.0F,
    3000.0F,
    5000.0F,
    8000.0F,
    10000.0F,

    13000.0F,
    16000.0F,
    18000.0F,
    20000.0F,
    21000.0F,
};

static float ampl_gain = 1.0F;
static uint8_t sim_frequency_idx = 5; // 1kHz
static option_select_t recent_option_selected = OPTION_SELECT_GAIN;
static effect_select_t recent_effect_selected = EFFECT_SELECT_RAW;
static option_source_t recent_source_selected = OPTION_SOURCE_MICROPHONE;

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

#define MYBSP_BUTTON_NUM 4

#define BUTTON_SIDE     (0)
#define BUTTON_LEFT     (1)
#define BUTTON_MID      (2)
#define BUTTON_RIGHT    (3)

static button_handle_t buttons[MYBSP_BUTTON_NUM];

static const button_config_t mybsp_button_config[MYBSP_BUTTON_NUM] = {
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = GPIO_NUM_0,
    },
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = GPIO_NUM_41,
    },
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = GPIO_NUM_1,
    },
    {
        .type = BUTTON_TYPE_GPIO,
        .gpio_button_config.active_level = false,
        .gpio_button_config.gpio_num = GPIO_NUM_14,
    },
};

static esp_err_t mybsp_iot_button_create(button_handle_t btn_array[], int *btn_cnt, int btn_array_size) {
    esp_err_t ret = ESP_OK;
    if ((btn_array_size < MYBSP_BUTTON_NUM) ||
            (btn_array == NULL)) {
        return ESP_ERR_INVALID_ARG;
    }

    if (btn_cnt) {
        *btn_cnt = 0;
    }

    for (int i = 0; i < btn_array_size; i++) {
        btn_array[i] = iot_button_create(&mybsp_button_config[i]);
        if (btn_array[i] == NULL) {
            ret = ESP_FAIL;
            break;
        }
        if (btn_cnt) {
            (*btn_cnt)++;
        }
    }

    return ret;
}

static void button_side_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Side button released");
}

static void button_side_longpressed_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Side button longpressed");
}

static void button_left_pressed_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Left button pressed");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_left_button_clicked(true);
        model_interface_release();
    }
}

static void button_left_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Left button released");
    effects_cmd_t cmd;

    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_left_button_clicked(false);

        switch (recent_option_selected) {
        case OPTION_SELECT_GAIN:
            ampl_gain *= 0.9F;

            cmd.type = EFFECTS_CMD_SET_GAIN;
            cmd.data.gain = ampl_gain;
            effects_send_cmd(cmd);
            
            model_if->set_gain(ampl_gain);
            ESP_LOGI(TAG, "Gain decresed to: %f.", ampl_gain);
            break;
            
        case OPTION_SELECT_FREQUENCY:
            if (sim_frequency_idx == 0) {
                sim_frequency_idx = SIM_FREQUENCIES_COUNT - 1;
            } else {
                sim_frequency_idx -= 1;
            }
            
            cmd.type = EFFECTS_CMD_SET_FREQUENCY;
            cmd.data.frequency = SIM_FREQUENCIES[sim_frequency_idx];
            effects_send_cmd(cmd);

            model_if->set_frequency(SIM_FREQUENCIES[sim_frequency_idx]);
            ESP_LOGI(TAG, "Frequency changed to: %f.", SIM_FREQUENCIES[sim_frequency_idx]);

            break;

        case OPTION_SELECT_EFFECT:
            /* Loop left selected effect */
            --recent_effect_selected;
            recent_effect_selected %= EFFECT_SELECT_COUNT;
            //TODO
            ESP_LOGI(TAG, "Effect loop left to: %d.", recent_effect_selected);
            break;

        case OPTION_SELECT_SOURCE:
            if (recent_source_selected == OPTION_SOURCE_SIMULATION) {
                recent_source_selected = OPTION_SOURCE_WIRED;
            } else if (recent_source_selected == OPTION_SOURCE_WIRED) {
                recent_source_selected = OPTION_SOURCE_MICROPHONE;
            } else {
                recent_source_selected = OPTION_SOURCE_SIMULATION;
            }
            
            cmd.data.effects_source = recent_source_selected;
            cmd.type = EFFECTS_CMD_SET_SOURCE;
            effects_send_cmd(cmd);

            model_if->set_source(recent_source_selected);
            break;
            
        default:
            break;
        }

        model_interface_release();
    }
}

static void button_middle_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Middle button released");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_middle_button_clicked(true);

        int next_option_index = recent_option_selected + 1;
        next_option_index %= OPTION_SELECT_COUNT;
        recent_option_selected = next_option_index;
        model_if->set_option_selected((option_select_t)next_option_index);

        model_interface_release();
    }
}

static void button_right_pressed_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Right button pressed");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_right_button_clicked(true);
        model_interface_release();
    }
}

static void button_right_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGV(TAG, "Right button released");
    effects_cmd_t cmd;

    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_right_button_clicked(false);

        switch (recent_option_selected) {
        case OPTION_SELECT_GAIN:
            ampl_gain *= 1.1F;
            
            cmd.type = EFFECTS_CMD_SET_GAIN;
            cmd.data.gain = ampl_gain;
            effects_send_cmd(cmd);

            model_if->set_gain(ampl_gain);
            ESP_LOGI(TAG, "Gain increased to: %f.", ampl_gain);
            break;

        case OPTION_SELECT_FREQUENCY:
            sim_frequency_idx += 1;
            sim_frequency_idx %= SIM_FREQUENCIES_COUNT;
            
            cmd.type = EFFECTS_CMD_SET_FREQUENCY;
            cmd.data.frequency = SIM_FREQUENCIES[sim_frequency_idx];
            effects_send_cmd(cmd);

            model_if->set_frequency(SIM_FREQUENCIES[sim_frequency_idx]);
            ESP_LOGI(TAG, "Frequency changed to: %f.", SIM_FREQUENCIES[sim_frequency_idx]);

            break;

        case OPTION_SELECT_EFFECT:
            /* Loop right selected effect */
            ++recent_effect_selected;
            recent_effect_selected %= EFFECT_SELECT_COUNT;
            //TODO
            ESP_LOGI(TAG, "Effect loop right to: %d.", recent_effect_selected);
            break;

        case OPTION_SELECT_SOURCE:
            if (recent_source_selected == OPTION_SOURCE_SIMULATION) {
                recent_source_selected = OPTION_SOURCE_MICROPHONE;
            } else if (recent_source_selected == OPTION_SOURCE_MICROPHONE) {
                recent_source_selected = OPTION_SOURCE_WIRED;
            } else {
                recent_source_selected = OPTION_SOURCE_SIMULATION;
            }
            
            cmd.data.effects_source = recent_source_selected;
            cmd.type = EFFECTS_CMD_SET_SOURCE;
            effects_send_cmd(cmd);

            model_if->set_source(recent_source_selected);
            break;

        default:
            break;
        }

        model_interface_release();
    }
}

void app_main(void) {
    esp_err_t ret = ESP_OK;

    info_prints();
    ESP_LOGI(TAG, "Starting!");
    
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK( nvs_flash_erase() );
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    led_matrix_t* led_matrix = heap_caps_calloc(1, sizeof(led_matrix_t ), MALLOC_CAP_SPIRAM);
    assert(led_matrix);

    led_matrix_init(led_matrix);
    led_matrix_clear(led_matrix);
    
    ret = effects_init();
    ESP_ERROR_CHECK(ret);
    
    asd_packet_builder_t* asd_packet_builder = heap_caps_calloc(1, sizeof(asd_packet_builder_t ), MALLOC_CAP_SPIRAM);
    asd_packet_builder_init(asd_packet_builder);

    ret = rf_sender_init();
    ESP_ERROR_CHECK(ret);

    ret = gdisplay_lcd_init();
    ESP_ERROR_CHECK(ret);

    /* Tweaked BSP buttons */
    ret = mybsp_iot_button_create(buttons, NULL, 4);
    ESP_ERROR_CHECK(ret);
    
    /* Side button */
    ret = iot_button_register_cb(buttons[BUTTON_SIDE], BUTTON_PRESS_UP, button_side_released_callback, NULL);
    ESP_ERROR_CHECK(ret);
    
    ret = iot_button_register_cb(buttons[BUTTON_SIDE], BUTTON_LONG_PRESS_START, button_side_longpressed_callback, NULL);
    ESP_ERROR_CHECK(ret);

    /* Front buttons */

    /* Left button */
    ret = iot_button_register_cb(buttons[BUTTON_LEFT], BUTTON_PRESS_DOWN, button_left_pressed_callback, NULL);
    ESP_ERROR_CHECK(ret);
    ret = iot_button_register_cb(buttons[BUTTON_LEFT], BUTTON_PRESS_UP, button_left_released_callback, NULL);
    ESP_ERROR_CHECK(ret);
    
    /* MIddle button */
    ret = iot_button_register_cb(buttons[BUTTON_MID], BUTTON_PRESS_UP, button_middle_released_callback, NULL);
    ESP_ERROR_CHECK(ret);

    /* Right button */
    ret = iot_button_register_cb(buttons[BUTTON_RIGHT], BUTTON_PRESS_DOWN, button_right_pressed_callback, NULL);
    ESP_ERROR_CHECK(ret);
    ret = iot_button_register_cb(buttons[BUTTON_RIGHT], BUTTON_PRESS_UP, button_right_released_callback, NULL);
    ESP_ERROR_CHECK(ret);

    ESP_LOGV(TAG, "MEM available: Any=%u B, DMA=%u B, SPI=%u B.", 
        heap_caps_get_free_size(MALLOC_CAP_8BIT),
        heap_caps_get_free_size(MALLOC_CAP_DMA),
        heap_caps_get_free_size(MALLOC_CAP_SPIRAM)
    );
   
    vTaskDelay(pdMS_TO_TICKS(1000));
   
    while(1) {
        effects_draw_to_led_matrix(led_matrix);

        model_interface_t* model_if = NULL;
        if (model_interface_access(&model_if, portMAX_DELAY)) {
            model_if->set_led_matrix_values(led_matrix);
            model_interface_release();
        }

        ret = rf_sender_send_led_matrix(asd_packet_builder, led_matrix);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "rf_sender_send_led_matrix failed");
        }

        // vTaskDelay(pdMS_TO_TICKS(10));
        ESP_LOGV(TAG, "MEM available: Any=%u B, DMA=%u B, SPI=%u B.", 
            heap_caps_get_free_size(MALLOC_CAP_8BIT),
            heap_caps_get_free_size(MALLOC_CAP_DMA),
            heap_caps_get_free_size(MALLOC_CAP_SPIRAM)
        );
    }
}
