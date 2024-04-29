#include "ws2812b_grid.h"

// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "led_strip.h"
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"

#include "../gtypes.h"
#include "../led_matrix.h"

#define LEDSTRIP_GPIO                   39

#define LEDS_COUNT                      (21 * 19)
#define COLORS_BITS                     (3 * 8)
#define BIT_RESOLUTION                  (4)

#if BIT_RESOLUTION == 8
#define CODE_BIT_LOW                    (0xC0)
#define CODE_BIT_HIGH                   (0xFC)
#else
#define CODE_BIT_LOW                    (0x08)
#define CODE_BIT_HIGH                   (0x0E)
#endif

#define RESET_PHASE_SIZE                (52 * BIT_RESOLUTION / 8)
#define LEDSTRIP_REQUIRED_BUFFER_SIZE   ((LEDS_COUNT * COLORS_BITS * BIT_RESOLUTION) + RESET_PHASE_SIZE)
#define TRANSACTIONS_COUNT              (2)
#define TRANSACTION_BUFFER_SIZE         (LEDSTRIP_REQUIRED_BUFFER_SIZE / TRANSACTIONS_COUNT)

#if ((TRANSACTION_BUFFER_SIZE * TRANSACTIONS_COUNT) != LEDSTRIP_REQUIRED_BUFFER_SIZE)
#error "LEDSTRIP_REQUIRED_BUFFER_SIZE not equali divided :("
#endif

#define SPI_CLK_FREQ                    ((6 * 1000 * 1000 + 400 * 1000) * BIT_RESOLUTION / 8)

#define LEDS_HOST                       SPI3_HOST

#define LEDSTRIP_TARGET_DRAW_INTERVAL  (40)

static led_matrix_t led_matrix;

ws2812b_grid_interface_t ws2812b_grid_interface;
static SemaphoreHandle_t ws2812b_grid_interface_mutex;

// static uint8_t* transfer_buffer;
DMA_ATTR static uint8_t transfer_buffer[LEDSTRIP_REQUIRED_BUFFER_SIZE];

static const char TAG[] = "WS2812BGrid";

static spi_device_handle_t spi;
static spi_transaction_t transactions[TRANSACTIONS_COUNT];

static void ws2812b_grid_set_byte(uint16_t byte_index, uint8_t value) {
#if BIT_RESOLUTION == 8
    /* 1B takes 8B of transfer */
    for (uint8_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
        transfer_buffer[byte_index * 8 + (8 - bit_idx - 1)] = 
            (value & (1<<bit_idx)) > 0 ? CODE_BIT_HIGH : CODE_BIT_LOW;
    }
#else
    /* 1B takes 4B of transfer */
    for (uint8_t bit_idx = 0; bit_idx < 8; ++bit_idx) {
        const uint8_t inv_bit_idx = 8 - bit_idx - 1;
        const uint8_t code_value = (value & (1<<inv_bit_idx)) > 0 ? CODE_BIT_HIGH : CODE_BIT_LOW;
        
        const uint16_t selected_byte_idx = bit_idx / 2;

        if(bit_idx % 2 == 0) {
            transfer_buffer[byte_index * 4 + selected_byte_idx] = (code_value<<4) & 0xF0;
        }
        else {
            transfer_buffer[byte_index * 4 + selected_byte_idx] |= code_value & 0x0F;
        }
    }
#endif
}

static void ws2812b_grid_set_pixel(uint16_t pixel_idx, uint8_t r, uint8_t g, uint8_t b) {
    /* Green */
    ws2812b_grid_set_byte(pixel_idx * 3 + 0, g);//0x80
    
    /* Red */
    ws2812b_grid_set_byte(pixel_idx * 3 + 1, r);
    
    /* Blue */
    ws2812b_grid_set_byte(pixel_idx * 3 + 2, b);//0x01
}

static void ws2812b_grid_fill_color(uint8_t r, uint8_t g, uint8_t b) {
    for (size_t pixel_idx = 0; pixel_idx < LEDS_COUNT; ++pixel_idx) {
        ws2812b_grid_set_pixel(pixel_idx, r, g, b);
    }
}

static void ws2812b_draw_matrix_in_zigzak(const led_matrix_t* led_matrix) {
    const uint8_t column_0_height = led_matrix->columns_heights[0] / 2;
    for (size_t pixel_idx = 0; pixel_idx < 7; ++pixel_idx) {
        if(pixel_idx < column_0_height) {
            ws2812b_grid_set_pixel(pixel_idx, 64, 0, 0);
        } else {
            ws2812b_grid_set_pixel(pixel_idx, 0, 0, 0);
        }
    }
}

static void ws2812b_grid_refresh() {
    esp_err_t ret;

    for (size_t transaction_idx = 0; transaction_idx < TRANSACTIONS_COUNT; ++transaction_idx) {
        spi_transaction_t* transaction = &transactions[transaction_idx];
        memset(transaction, 0, sizeof(spi_transaction_t));

        transaction->tx_buffer  = transfer_buffer + (transaction_idx * TRANSACTION_BUFFER_SIZE);
        transaction->length     = LEDSTRIP_REQUIRED_BUFFER_SIZE;

        ret = spi_device_queue_trans(spi, transaction, portMAX_DELAY);
        ESP_ERROR_CHECK(ret);
    }
}

static void ws2812b_grid_set_led_matrix_values(const led_matrix_t* led_mx) {
    memcpy(&led_matrix, led_mx, sizeof(led_matrix_t));
}

static void ws2812b_grid_task(void* params) {
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t target_interval_refresh_ticks = (LEDSTRIP_TARGET_DRAW_INTERVAL) / 10;

    while(1) {
        ws2812b_grid_interface_t* grid_if = NULL;
        if (ws2812b_grid_access(&grid_if, portMAX_DELAY)) {
            (void)grid_if; //not needed, only lock recsources

            ws2812b_draw_matrix_in_zigzak(&led_matrix);
            ws2812b_grid_refresh();

            ws2812b_grid_release();
        }

        /* Cap */
        xTaskDelayUntil(&xLastWakeTime, target_interval_refresh_ticks);
    }
}

esp_err_t ws2812b_grid_init() {
    ESP_LOGI(TAG, "Attempt to allocate DMA MEM %u B... Available=%u B.", 
        LEDSTRIP_REQUIRED_BUFFER_SIZE, heap_caps_get_free_size(MALLOC_CAP_DMA));
    // transfer_buffer = heap_caps_malloc(LEDSTRIP_REQUIRED_BUFFER_SIZE, MALLOC_CAP_DMA);
    // assert(transfer_buffer);
    memset(transfer_buffer, 0, LEDSTRIP_REQUIRED_BUFFER_SIZE);

    ws2812b_grid_interface_mutex = xSemaphoreCreateMutex();
    assert(ws2812b_grid_interface_mutex);

    ws2812b_grid_interface.set_led_matrix_values = ws2812b_grid_set_led_matrix_values;
    
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = LEDSTRIP_GPIO,
        .sclk_io_num = -1,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = TRANSACTION_BUFFER_SIZE + 8 // add 8 to be sure
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLK_FREQ,
        .mode = 0,         
        .spics_io_num = -1,   
        .queue_size = 1,  
        .pre_cb = NULL, //Specify pre-transfer callback to handle
    };

    //Initialize the SPI bus
    ret = spi_bus_initialize(LEDS_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);

    //Attach the LED to the SPI bus
    ret = spi_bus_add_device(LEDS_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    const float estimated_transfer_duration_ms = LEDSTRIP_REQUIRED_BUFFER_SIZE * 1000.0F / ((float)SPI_CLK_FREQ);
    ESP_LOGI(TAG, "Created LED strip driver. LEDs_count=%u, total_bytes=%u, SPI_transfer_size=%u. Transfer should take %f ms.",
        LEDS_COUNT, LEDSTRIP_REQUIRED_BUFFER_SIZE, TRANSACTION_BUFFER_SIZE, estimated_transfer_duration_ms
    );
    int64_t t1, t2, t3, t4, t5=0, t6=0;

    t1 = esp_timer_get_time();

    spi_device_acquire_bus(spi, portMAX_DELAY);

    t2 = esp_timer_get_time();

    ws2812b_grid_fill_color(0, 0, 0);

    t3 = esp_timer_get_time();

    ws2812b_grid_refresh();

    t4 = esp_timer_get_time();
        
    // spi_transaction_t* transaction = NULL;
    // //Wait for all transactions to be done and get back the results.
    // for (size_t transaction_idx = 0; transaction_idx < 6; ++transaction_idx) {
    //     ret = spi_device_get_trans_result(spi, &transaction, portMAX_DELAY);
    //     assert(ret == ESP_OK);
    // }

    t5 = esp_timer_get_time();

    spi_device_release_bus(spi);

    t6 = esp_timer_get_time();
    
    ESP_LOGI(TAG, "Initial draw done! Timings[us]: aquire=%lld, draw:%lld, init_transfer:%lld, duration:%lld, release:%lld", t2-t1, t3-t2, t4-t3, t5-t4, t6-t5);

    const bool ws2812b_grid_was_created = xTaskCreate(
        ws2812b_grid_task, 
        "ws2812b_grid_task", 
        1024 * 4, 
        NULL, 
        20, 
        NULL
    ) == pdTRUE;
    ESP_RETURN_ON_FALSE(ws2812b_grid_was_created, ESP_FAIL, TAG, "Failed creating \'ws2812b_grid_task\'!");

    ESP_LOGI(TAG, "Task created successfully!");
    return ESP_OK;
}

bool ws2812b_grid_access(ws2812b_grid_interface_t** ws2812b_grid_if, TickType_t timeout_tick_time) {
    if(!ws2812b_grid_interface_mutex) {
        ESP_LOGW(TAG, "Failed aquiring interface. Reason NULL");
        return false;
    }

    const bool accessed = xSemaphoreTake(ws2812b_grid_interface_mutex, timeout_tick_time) == pdTRUE ? true : false;
    *ws2812b_grid_if = &ws2812b_grid_interface;
    if(!accessed) {
        ESP_LOGW(TAG, "Failed aquiring interface. Reason Semaphore");
    }
    return accessed;
}

void ws2812b_grid_release() {
    xSemaphoreGive(ws2812b_grid_interface_mutex);
}



