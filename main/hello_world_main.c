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
#include "esp_heap_caps.h"

#include "gsampler.h"
#include "gdisplay.h"
#include "model.h"

#include "iot_button.h"
#include "bsp/esp-bsp.h"
#include "gtypes.h"
#include "leds/ws2812b_grid.h"

static const char TAG[] = "Main";

static led_matrix_t led_matrix;

static float heights[LED_MATRIX_COLUMNS];
static float velocities[LED_MATRIX_COLUMNS];

const float gravity_acceleration = -60.0F;

static float ampl_gain = 1.0F;
static option_select_t recent_option_selected = OPTION_SELECT_GAIN;

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

static adc_oneshot_unit_handle_t bsp_adc_handle = NULL;

static button_handle_t side_button;

#define BUTTONS_COUNT       (3)
static button_handle_t front_buttons[BUTTONS_COUNT];

#define ADC_BUTTON_LEFT     (0)
#define ADC_BUTTON_MID      (1)
#define ADC_BUTTON_RIGHT    (2)

static void button_side_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGI(TAG, "Side button released");
}

static void button_side_longpressed_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGI(TAG, "Side button longpressed");
}

static void button_left_pressed_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGI(TAG, "Left button pressed");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_left_button_clicked(true);
        model_interface_release();
    }
}

static void button_left_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGI(TAG, "Left button released");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_left_button_clicked(false);

        if (recent_option_selected == OPTION_SELECT_GAIN) {
            ampl_gain *= 0.9F;
            gsampler_set_gain(ampl_gain);
            model_if->set_gain(ampl_gain);
        }

        model_interface_release();
    }
}

static void button_middle_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGI(TAG, "Middle button released");
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
    ESP_LOGI(TAG, "Right button pressed");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_right_button_clicked(true);
        model_interface_release();
    }
}

static void button_right_released_callback(void *arg, void *data) {
    (void)arg;
    (void)data;
    ESP_LOGI(TAG, "Right button released");
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        model_if->set_right_button_clicked(false);

        if (recent_option_selected == OPTION_SELECT_GAIN) {
            ampl_gain *= 1.1F;
            gsampler_set_gain(ampl_gain);
            model_if->set_gain(ampl_gain);
        }

        model_interface_release();
    }
}


static const button_config_t adc_button_config[BUTTONS_COUNT] = {
    {
        .type = BUTTON_TYPE_ADC,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .adc_button_config.adc_handle = &bsp_adc_handle,
#endif
        .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
        .adc_button_config.button_index = ADC_BUTTON_LEFT,
        .adc_button_config.min = 2310, // middle is 2410mV
        .adc_button_config.max = 2510
    },
    {
        .type = BUTTON_TYPE_ADC,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .adc_button_config.adc_handle = &bsp_adc_handle,
#endif
        .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
        .adc_button_config.button_index = ADC_BUTTON_MID,
        .adc_button_config.min = 1880, // middle is 1980mV
        .adc_button_config.max = 2080
    },
    {
        .type = BUTTON_TYPE_ADC,
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
        .adc_button_config.adc_handle = &bsp_adc_handle,
#endif
        .adc_button_config.adc_channel = ADC_CHANNEL_0, // ADC1 channel 0 is GPIO1
        .adc_button_config.button_index = ADC_BUTTON_RIGHT,
        .adc_button_config.min = 720, // middle is 820mV
        .adc_button_config.max = 920
    },
};

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

    ret = ws2812b_grid_init();
    ESP_ERROR_CHECK(ret);

    /* Side button */
    ret = bsp_iot_button_create(&side_button, NULL, 1);
    ESP_ERROR_CHECK(ret);
    
    ret = iot_button_register_cb(side_button, BUTTON_PRESS_UP, button_side_released_callback, NULL);
    ESP_ERROR_CHECK(ret);
    
    ret = iot_button_register_cb(side_button, BUTTON_LONG_PRESS_START, button_side_longpressed_callback, NULL);
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(bsp_adc_initialize());
    bsp_adc_handle = bsp_adc_get_handle();
    assert(bsp_adc_handle);

    /* Front buttons */

    /* Left button */
    front_buttons[ADC_BUTTON_LEFT] = iot_button_create(&adc_button_config[ADC_BUTTON_LEFT]);
    assert(front_buttons[ADC_BUTTON_LEFT]);
    ret = iot_button_register_cb(front_buttons[ADC_BUTTON_LEFT], BUTTON_PRESS_DOWN, button_left_pressed_callback, NULL);
    ESP_ERROR_CHECK(ret);
    ret = iot_button_register_cb(front_buttons[ADC_BUTTON_LEFT], BUTTON_PRESS_UP, button_left_released_callback, NULL);
    ESP_ERROR_CHECK(ret);
    
    /* MIddle button */
    front_buttons[ADC_BUTTON_MID] = iot_button_create(&adc_button_config[ADC_BUTTON_MID]);
    assert(front_buttons[ADC_BUTTON_MID]);
    ret = iot_button_register_cb(front_buttons[ADC_BUTTON_MID], BUTTON_PRESS_UP, button_middle_released_callback, NULL);
    ESP_ERROR_CHECK(ret);

    /* Right button */
    front_buttons[ADC_BUTTON_RIGHT] = iot_button_create(&adc_button_config[ADC_BUTTON_RIGHT]);
    assert(front_buttons[ADC_BUTTON_RIGHT]);
    ret = iot_button_register_cb(front_buttons[ADC_BUTTON_RIGHT], BUTTON_PRESS_DOWN, button_right_pressed_callback, NULL);
    ESP_ERROR_CHECK(ret);
    ret = iot_button_register_cb(front_buttons[ADC_BUTTON_RIGHT], BUTTON_PRESS_UP, button_right_released_callback, NULL);
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "Free DMA space %u B", heap_caps_get_free_size(MALLOC_CAP_DMA));
    ESP_LOGI(TAG, "Free SPIRAM space %u B", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    ESP_LOGI(TAG, "Free Internal space %u B", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
   
    while(1) {
        vTaskDelay(500);
    }
}
