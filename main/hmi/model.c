#include "model.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_check.h"

#include "gstyles.h"
#include "graphics.h"
#include "gfonts.h"

static const char *TAG = "Model";

static SemaphoreHandle_t model_mutex;

typedef struct model_t {
    model_interface_t interface;
    led_matrix_t led_matrix;

    struct {
        bool left_clicked;
        bool right_clicked;
        // bool middle_clicked;
    } front_buttons;

    struct {
        float gain;
        float frequency;
        option_source_t source;
        effect_select_t effect;
    } options_values;

    option_select_t option_selected;
} model_t;

static model_t* get_model();

static void model_set_led_matrix_values(const led_matrix_t* led_mx) {
    if(led_mx) {
        memcpy(&(get_model()->led_matrix), led_mx, sizeof(led_matrix_t));
    }
}

static void model_set_left_button_clicked(bool clicked) {
    get_model()->front_buttons.left_clicked = clicked;
}

static void model_set_right_button_clicked(bool clicked) {
    get_model()->front_buttons.right_clicked = clicked;
}

static void model_set_option_selected(option_select_t option) {
    get_model()->option_selected = option;
}

static void model_set_middle_button_clicked(bool clicked) {
}

static void model_set_gain(float g) {
    get_model()->options_values.gain = g;
}

static void model_set_frequency(float freq) {
    get_model()->options_values.frequency = freq;
}

static void model_set_source(option_source_t source) {
    get_model()->options_values.source = source;
}

static void model_set_effect(effect_select_t effect) {
    get_model()->options_values.effect = effect;
}

static model_t* get_model() {
    static model_t* _model_ptr;
    if(!_model_ptr) {
        ESP_LOGI(TAG, "Model just created!");
        _model_ptr = heap_caps_calloc(1, sizeof(model_t), MALLOC_CAP_SPIRAM);
        assert(_model_ptr);
        
        _model_ptr->interface.set_led_matrix_values = model_set_led_matrix_values;
        _model_ptr->interface.set_left_button_clicked = model_set_left_button_clicked;
        _model_ptr->interface.set_right_button_clicked = model_set_right_button_clicked;
        _model_ptr->interface.set_middle_button_clicked = model_set_middle_button_clicked;
        _model_ptr->interface.set_option_selected = model_set_option_selected;
        _model_ptr->interface.set_gain = model_set_gain;
        _model_ptr->interface.set_frequency= model_set_frequency;
        _model_ptr->interface.set_source = model_set_source;
        _model_ptr->interface.set_effect = model_set_effect;

        _model_ptr->front_buttons.left_clicked = false;
        _model_ptr->front_buttons.right_clicked = false;

        _model_ptr->options_values.gain = 1.0F;
        _model_ptr->options_values.frequency = 1000.0F;
        _model_ptr->options_values.source = OPTION_SOURCE_MICROPHONE;
        _model_ptr->options_values.effect = EFFECT_SELECT_FIRE;

        _model_ptr->option_selected = OPTION_SELECT_GAIN;

        led_matrix_clear(&_model_ptr->led_matrix);
    }
    return _model_ptr;
}

bool model_interface_access(model_interface_t** model_if, TickType_t timeout_tick_time) {
    if(!model_mutex) {
        ESP_LOGW(TAG, "Failed aquiring model interface. Reason NULL");
        return false;
    }

    const bool accessed = xSemaphoreTake(model_mutex, timeout_tick_time) == pdTRUE ? true : false;
    *model_if = &get_model()->interface;
    if(!accessed) {
        ESP_LOGW(TAG, "Failed aquiring model interface. Reason Semaphore");
    }
    return accessed;
}

void model_interface_release() {
    xSemaphoreGive(model_mutex);
}

void model_init() {
    model_mutex = xSemaphoreCreateMutex();
    assert(model_mutex);
}

void model_tick() {
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        (void)model_if; //not needed, only lock recsources

        /* Some internal updates, called before drawing */

        model_interface_release();
    }
}

void model_draw(gdisplay_api_t* gd_api) {
    model_interface_t* model_if = NULL;
    if (model_interface_access(&model_if, portMAX_DELAY)) {
        (void)model_if; //not needed, only lock recsources
        char text_buff[32] = {0};

        /* Bottom panel */
        gd_api->draw_rect(0, 0, DISPL_TOTAL_WIDTH, PANE_BOTTOM_HEIGHT, VIS_PANE_BOTTOM_BG_COLOR);
        gd_api->draw_rect(68, 39, 184, 16, VIS_PANE_BOTTOM_TEXT_BG_COLOR);
        
        if (get_model()->front_buttons.left_clicked) {
            gd_api->draw_bytes_bitmap(4, 4, BTN_LEFT_PRESSED_WIDTH, btn_left_pressed_graphics_bytes, BTN_LEFT_PRESSED_BYTES_COUNT);
        } else {
            gd_api->draw_bytes_bitmap(4, 4, BTN_LEFT_WIDTH, btn_left_graphics_bytes,BTN_LEFT_BYTES_COUNT);
        }
        
        if (get_model()->front_buttons.right_clicked) {
            gd_api->draw_bytes_bitmap(256, 4, BTN_RIGHT_PRESSED_WIDTH, btn_right_pressed_graphics_bytes, BTN_RIGHT_PRESSED_BYTES_COUNT);
        } else {
            gd_api->draw_bytes_bitmap(256, 4, BTN_RIGHT_WIDTH, btn_right_graphics_bytes, BTN_RIGHT_BYTES_COUNT);
        }

        /* Central value & label */
        if (get_model()->option_selected == OPTION_SELECT_GAIN) {
            snprintf(text_buff, sizeof(text_buff), "%.3f", get_model()->options_values.gain);
            gd_api->draw_text(140, 41, &font_rockwell_4pt, text_buff);

            gd_api->draw_bytes_bitmap(68, 6, LABEL_GAIN_WIDTH, label_gain_graphics_bytes, LABEL_GAIN_BYTES_COUNT);
        } 
        else if (get_model()->option_selected == OPTION_SELECT_FREQUENCY) {
            snprintf(text_buff, sizeof(text_buff), "%.3f", get_model()->options_values.frequency);
            gd_api->draw_text(140, 41, &font_rockwell_4pt, text_buff);

            gd_api->draw_text(68, 6, &font_rockwell_4pt, "FREQUENCY");
        } 
        else if (get_model()->option_selected == OPTION_SELECT_SOURCE) {
            const char* source_text = NULL;
            switch (get_model()->options_values.source) {
            case OPTION_SOURCE_SIMULATION:
                source_text = "SIMULATION";
                break;

            case OPTION_SOURCE_MICROPHONE:
                source_text = "MICROPHONE";
                break;

            default:
                source_text = "UNKNOWN";
            }

            gd_api->draw_text(140, 41, &font_rockwell_4pt, source_text);

            gd_api->draw_bytes_bitmap(68, 6, LABEL_SOURCE_WIDTH, label_source_graphics_bytes, LABEL_SOURCE_BYTES_COUNT);
        } 
        else if (get_model()->option_selected == OPTION_SELECT_EFFECT) {
            const char* effect_text = NULL;
            switch (get_model()->options_values.effect) {
            case EFFECT_SELECT_RAW:
                effect_text = "RAW";
                break;

            case EFFECT_SELECT_SIMPLE:
                effect_text = "SIMPLE";
                break;

            case EFFECT_SELECT_FIRE:
                effect_text = "FIRE";
                break;

            case EFFECT_SELECT_MULTICOLOR:
                effect_text = "MULTICOLOR";
                break;

            default:
                effect_text = "UNKNOWN";
            }

            gd_api->draw_text(140, 41, &font_rockwell_4pt, effect_text);

            gd_api->draw_bytes_bitmap(68, 6, LABEL_EFFECT_WIDTH, label_effect_graphics_bytes, LABEL_EFFECT_BYTES_COUNT);
        }
        
        /* Display background: base + grid box */
        gd_api->draw_rect(VIS_BASE_X, VIS_BASE_Y, VIS_DISPLAY_BASE_WIDTH, VIS_DISPLAY_BASE_HEIGHT, VIS_DISPLAY_BASE_COLOR);
        gd_api->draw_rect(VIS_X, VIS_Y - VIS_V_OFFSET, VIS_DISPLAY_WIDTH, VIS_V_OFFSET, VIS_DISPLAY_BG_COLOR);
        gd_api->draw_rect(VIS_X, VIS_Y, VIS_DISPLAY_WIDTH, VIS_DISPLAY_HEIGHT, VIS_DISPLAY_BG_COLOR);

        /* Display blocks */
        assert(LED_MATRIX_COLUMNS == VIS_BARS_COUNT);
        assert(LED_MATRIX_ROWS == VIS_ROWS_COUNT);

        for(size_t row_idx = 0; row_idx < VIS_ROWS_COUNT; ++row_idx) {
            for(size_t bar_idx = 0; bar_idx < VIS_BARS_COUNT; ++bar_idx) {
                const color_24b_t* matrix_color = led_matrix_access_pixel_at(&(get_model()->led_matrix), bar_idx, row_idx);
                const uint16_t color_raw = ((matrix_color->red >> 3) << (6+5)) | ((matrix_color->green >> 2) << 5) | ((matrix_color->blue >> 3));
                const uint16_t display_color = 0xFFFF - GREV(color_raw);

                gd_api->draw_rect(
                    VIS_X + VIS_BAR_HGAP + bar_idx * (VIS_BLOCK_WIDTH  + VIS_BAR_HGAP),
                    VIS_Y + VIS_BAR_VGAP + row_idx * (VIS_BLOCK_HEIGHT + VIS_BAR_VGAP), 
                    VIS_BLOCK_WIDTH, 
                    VIS_BLOCK_HEIGHT, 
                    display_color
                );
            }
        }
    
        model_interface_release();
    }
}

