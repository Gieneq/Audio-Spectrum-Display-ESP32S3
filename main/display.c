#include <math.h>
#include "bsp/esp-bsp.h"
#include "bsp/display.h"
#include "display.h"
#include "driver/ledc.h"
#include "hal/spi_ll.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"

#include "styles.h"

static const char *TAG = "display";
/****************** LCD Configuration ************************************************/
#define LCD_WIDTH              BSP_LCD_H_RES
#define LCD_HEIGHT             BSP_LCD_V_RES
#define LCD_BUFFER_SIZE        320*240*2

#ifndef SPI_LL_DATA_MAX_BIT_LEN
#define SPI_LL_DATA_MAX_BIT_LEN ((uint32_t)(1LU << 18LU))
#define LCD_SPI_MAX_DATA_SIZE (SPI_LL_DATA_MAX_BIT_LEN / 8)
#else
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 1, 0)
#define LCD_SPI_MAX_DATA_SIZE (SPI_LL_DATA_MAX_BIT_LEN / 8)
#else
/**
 * @brief release5.0 align = 4092(4096 -4)
 */
#define LCD_SPI_MAX_DATA_SIZE ((SPI_LL_DATA_MAX_BIT_LEN / 8)/4092*4092)
#endif
#endif


typedef struct model_t {
    model_interface_t interface;
    int16_t bar_heights[VIS_BARS_COUNT];
} model_t;

static model_t* get_model();

SemaphoreHandle_t model_mutex;

bool display_access_model(model_interface_t** model_if, TickType_t timeout_tick_time) {
    const bool accessed = xSemaphoreTake(model_mutex, timeout_tick_time) == pdTRUE ? true : false;
    *model_if = &get_model()->interface;
    return accessed;
}

void display_release_model() {
    xSemaphoreGive(model_mutex);
}

static void model_set_bar_heights(int16_t* bars, size_t bars_count) {
    bars_count = bars_count > VIS_BARS_COUNT ? VIS_BARS_COUNT : bars_count;
    memcpy(get_model()->bar_heights, bars, sizeof(bars[0]) * bars_count);
}

static model_t* get_model() {
    static model_t _model;
    static model_t* _model_ptr;
    if(!_model_ptr) {
        ESP_LOGI(TAG, "Model just created!");
        _model_ptr = &_model;
        _model.interface.set_bar_heights = model_set_bar_heights;
        memset(_model.bar_heights, 0, sizeof(_model.bar_heights));
    }
    return _model_ptr;
}

static void display_draw_point(int16_t x, int16_t y, uint16_t color);

static void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

static int64_t last_draw_done_time;
static int64_t draw_done_time;

static esp_lcd_panel_handle_t panel_handle = NULL;
__NOINIT_ATTR static uint16_t display_buffer[LCD_BUFFER_SIZE/2];

#define DISPLAY_EVENT_DRAW_DONE     (1<<0)
#define DISPLAY_EVENT_CAN_DRAW      (1<<1)
static EventGroupHandle_t display_events;

static volatile uint32_t callback_ctr;

static void display_draw_point(int16_t x, int16_t y, uint16_t color) {
    assert(x >= 0);
    assert(y >= 0);
    assert(x < LCD_WIDTH);
    assert(y < LCD_HEIGHT);
    display_buffer[(LCD_HEIGHT - y - 1) * LCD_WIDTH + x] = color;
    // display_buffer[(LCD_HEIGHT - y - 1) * LCD_WIDTH + x] = color;
}

static void display_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    for (int16_t iy = 0; iy < h; iy++) {
        for (int16_t ix = 0; ix < w; ix++) {
            display_draw_point(x + ix, y + iy, color);
        }
    }
}

static void model_tick(model_t* model) {
}

static void model_draw(model_t* model) {
    /* BG */
    const int16_t vis_x = 12;
    const int16_t vis_y = 12;
    display_draw_rect(
        vis_x,
        vis_y,
        VIS_DISPLAY_WIDTH,
        VIS_DISPLAY_HEIGHT,
        VIS_DISPLAY_BG_COLOR
    );

    for(size_t row_idx = 0; row_idx < VIS_ROWS_COUNT; ++row_idx) {
        for(size_t bar_idx = 0; bar_idx < VIS_BARS_COUNT; ++bar_idx) {
            display_draw_rect(
                vis_x + VIS_BAR_HGAP + bar_idx * (VIS_BLOCK_WIDTH  + VIS_BAR_HGAP),
                vis_y + VIS_BAR_VGAP + row_idx * (VIS_BLOCK_HEIGHT + VIS_BAR_VGAP),
                VIS_BLOCK_WIDTH,
                VIS_BLOCK_HEIGHT,
                VIS_BLOCK_OFF_COLOR
            );
        }
    }
}

static void on_blanking_period() {
    model_tick(get_model());
    model_draw(get_model());
}

bool display_colors_transfer_done_callback(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    last_draw_done_time = draw_done_time;
    draw_done_time = esp_timer_get_time();

    BaseType_t xHigherPriorityTaskWoken;
    if(xEventGroupSetBitsFromISR(display_events, DISPLAY_EVENT_DRAW_DONE, &xHigherPriorityTaskWoken) == pdPASS) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    return true;
}

static void display_task(void* params) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t interval_ticks = 6;

    while(1) {
        (void)xEventGroupWaitBits(display_events, DISPLAY_EVENT_DRAW_DONE, pdFALSE, pdTRUE, portMAX_DELAY);


        const uint64_t right_after_draw = esp_timer_get_time() - draw_done_time;
        const uint64_t draw_interval = draw_done_time - last_draw_done_time;

        // if(++callback_ctr > 10) {
        //     ESP_LOGI(TAG, "draw_interval = %lld, right_after=%lld", draw_interval, right_after_draw);
        //     callback_ctr=0;
        // }

        (void)xEventGroupSetBits(display_events, DISPLAY_EVENT_CAN_DRAW);
        /* Some time to prepare graphics */
        memset(display_buffer, 0, LCD_BUFFER_SIZE);
        on_blanking_period();
        xTaskDelayUntil( &xLastWakeTime, interval_ticks);
        (void)xEventGroupClearBits(display_events, DISPLAY_EVENT_CAN_DRAW);

        esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, (void *)display_buffer);
    }
}

esp_err_t display_lcd_init(void) {
    ESP_LOGI(TAG, "Display SPI size: %lu", LCD_SPI_MAX_DATA_SIZE);
    esp_lcd_panel_io_handle_t io_handle = NULL;
    const bsp_display_config_t bsp_disp_cfg = {
        .max_transfer_sz = (LCD_SPI_MAX_DATA_SIZE),
    };
    bsp_display_new(&bsp_disp_cfg, &panel_handle, &io_handle);

    const esp_lcd_panel_io_callbacks_t callbacks = {
        .on_color_trans_done = display_colors_transfer_done_callback
    };
    esp_lcd_panel_io_register_event_callbacks(io_handle, &callbacks, NULL);

    esp_lcd_panel_disp_on_off(panel_handle, true);
    bsp_display_backlight_on();

    // display_buffer = (uint16_t *)heap_caps_calloc(1, LCD_BUFFER_SIZE, MALLOC_CAP_INTERNAL);
    // assert(display_buffer != NULL);

    display_events = xEventGroupCreate();
    ESP_RETURN_ON_FALSE(display_events, ESP_FAIL, TAG, "Event group not created");
    
    model_mutex = xSemaphoreCreateMutex();
    ESP_RETURN_ON_FALSE(model_mutex, ESP_FAIL, TAG, "Model mutex not created");

    (void)get_model();

    const bool display_task_was_created = xTaskCreate(
        display_task, 
        "display_task", 
        1024 * 8, 
        NULL, 
        10, 
        NULL
    ) == pdTRUE;
    ESP_RETURN_ON_FALSE(display_task_was_created, ESP_FAIL, TAG, "Failed creating \'display_task\'!");

    (void)xEventGroupSetBits(display_events, DISPLAY_EVENT_DRAW_DONE);

    return ESP_OK;
}














// esp_err_t display_draw_custom(float *bins_values) {
//     if(!bins_values) {
//         return ESP_OK;
//     }
//     memset(display_buffer, 0, LCD_BUFFER_SIZE);

//     for(int ix=0; ix<FFT_BINS_COUNT; ++ix) {
//         const int bar_height = (int)(bins_values[ix] < 0.0f ? 0.0f : (bins_values[ix] < LCD_HEIGHT ? bins_values[ix] : LCD_HEIGHT));
//         for(int iy=0; iy<bar_height; ++iy) {
//             const int draw_y = LCD_HEIGHT - iy - 1;
//             assert(draw_y >= 0);
//             display_buffer[draw_y * LCD_WIDTH + 4*ix + 0] = 0xFFFF;
//             display_buffer[draw_y * LCD_WIDTH + 4*ix + 1] = 0xFFFF;
//         }
//     }
//     esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, (void *)display_buffer);
//     return ESP_OK;
// }


// static int16_t fre_point[STRIP_NUM] = {0};

// /****************** configure the example working mode *******************************/
// #define BASIC_HIGH             40                          /* Subtract the height of the column height */
// #define GROUP_WIDTH            10                          /* Width of each set of columns */
// #define STRIP_WIDTH            8                           /* Width of the strip */
// #define STRIP_DROP_SPEED       4                           /* The speed of the cube's fall */
// #define COLOR_MODE             4                           /* 4: Customer mode 3: Tri-color gradient 2: Two-color gradient 1: One_color gradient */
// #define SQUARE_MODE            2                           /* 1: Even fall mode 2: Gravity simulation mode */

// #if SQUARE_MODE == 2
// #define ACCELERATION           1.5
// #define CRASH_SPEED_RATIO      0.05
// #endif

// #if COLOR_MODE == 1
// #define COLOR_RANGE            320
// #elif COLOR_MODE == 2
// #define COLOR_RANGE            214
// #elif COLOR_MODE == 3
// #define COLOR_RANGE            107
// #elif COLOR_MODE == 4
// #define COLOR_RANGE            400
// #endif

// #define STRIP_NUM              320 / GROUP_WIDTH
// #define INTERVAL_WIDTH         GROUP_WIDTH - STRIP_WIDTH


// typedef struct {
//     float speed[STRIP_NUM];
//     int16_t square_high[STRIP_NUM];
// } display_square_t;

// static display_square_t display_square = {0};

// static void frequency_multiplier_calculation(void)
// {
//     double freq_interval = 24 * 1000 / FFT_RESULT_SAMPLES_COUNT;
//     double x = STRIP_NUM / (log(24000) / log(2));
//     double freq = 0;
//     for (int i = 1; i < STRIP_NUM + 1; i++) {
//         freq = pow(2, i / x) / freq_interval;
//         if (freq > i) {
//             if (freq > 1023) {
//                 fre_point[i - 1] = 1023;
//             } else {
//                 fre_point[i - 1] = (int)freq;
//             }
//         } else {
//             fre_point[i - 1] = i;
//         }
//         ESP_LOGD(TAG, "calculation fre %d", fre_point[i - 1]);
//     }
// }

// static int adjust_height(int y, float coefficients)
// {
//     if (y <= 0) {
//         return 0;
//     }

//     return pow(y, coefficients);
// }

// static uint16_t fade_color(float num, float range)
// {
//     int value = num / range;
//     uint16_t r, g, b;
//     switch (value) {
//     case 0:
//         r = (uint16_t)255 * num / range;
//         g = 0;
//         b = (uint16_t)255 * (1 - num / range);
//         break;
//     case 1:
//         r = (uint16_t)255 * (1 - num / range);
//         g = (uint16_t)255 * num / range;
//         b = 0;
//         break;
//     default:
//         r = 0;
//         g = (uint16_t)255 * (1 - num / range);
//         b = (uint16_t)255 * num / range;
//         break;
//     }
//     return ((uint16_t)((r & 0xF8) | ((b & 0xF8) << 5) | ((g & 0x1c) << 11) | ((g & 0xE0) >> 5)));
// }

// static int draw_square(int y, int point_i)
// {
// #if SQUARE_MODE ==1
//     if (y >= display_square.square_high[point_i]) {
//         display_square.square_high[point_i] = y - 1;
//     } else {
//         if (display_square.square_high[point_i] <= 0) {
//             display_square.square_high[point_i] = 1;
//         } else {
//             display_square.square_high[point_i] -= STRIP_DROP_SPEED;
//         }
//     }
// #elif SQUARE_MODE == 2
//     if (y >= display_square.square_high[point_i]) {
//         display_square.square_high[point_i] = y - 1;
//         display_square.speed[point_i] = CRASH_SPEED_RATIO * y;
//     } else {
//         if (display_square.square_high[point_i] <= 0) {
//             display_square.square_high[point_i] = 1;
//         } else {
//             display_square.square_high[point_i] += display_square.speed[point_i];
//             display_square.speed[point_i] -= ACCELERATION;
//         }
//     }
// #endif
//     return display_square.square_high[point_i];
// }

// esp_err_t display_draw(float *data)
// {
//     int fre_point_i = 0;
//     uint16_t line_color = 0;
//     uint16_t color = 0;
//     int correct_y = 0;
//     for (int x = 1; x < LCD_WIDTH; x += GROUP_WIDTH) {
//         if (data != NULL) {
//             correct_y = data[fre_point[fre_point_i]] - BASIC_HIGH;
//             if ( x <= 40 ) {
//                 correct_y = adjust_height(correct_y, 1.15);
//             } else if (x > 40 || x < 100) {
//                 correct_y = adjust_height(correct_y, 1.3);
//             } else {
//                 correct_y = adjust_height(correct_y, 1);
//             }
//         }
//         int square_high = LCD_HEIGHT - draw_square(correct_y, fre_point_i);
//         correct_y = LCD_HEIGHT - correct_y;
//         line_color = fade_color(x, COLOR_RANGE);
//         for (int y = 0; y < LCD_HEIGHT; y++) {
//             if (correct_y <= y) {
//                 color = line_color;
//             } else if (square_high - STRIP_WIDTH <= y && square_high >= y) {
//                 color = 0xFFFF;
//             } else {
//                 color = 0x0000;
//             }

//             for (int z = 0; z < STRIP_WIDTH; z++) {
//                 display_buffer[y * LCD_WIDTH + x + z] = color;
//             }
//         }
//         fre_point_i++;
//         correct_y = 0;
//     }
//     esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, (void *)display_buffer);
//     return ESP_OK;
// }
