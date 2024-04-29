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

#define LEDS_COUNT                      (3)
#define COLORS_BITS                     (3 * 8)
#define BIT_RESOLUTION                  (8)

#if BIT_RESOLUTION == 8
#define CODE_BIT_LOW                    (0xC0)
#define CODE_BIT_HIGH                   (0xFC)
#else
#error "Not implemented"
#endif

#define RESET_PHASE_SIZE                (50 * BIT_RESOLUTION / 8)
#define LEDSTRIP_REQUIRED_BUFFER_SIZE   ((LEDS_COUNT * COLORS_BITS * BIT_RESOLUTION) + RESET_PHASE_SIZE)
#define TRANSACTIONS_COUNT              (1)
#define TRANSACTION_BUFFER_SIZE         (LEDSTRIP_REQUIRED_BUFFER_SIZE / TRANSACTIONS_COUNT)

#if ((TRANSACTION_BUFFER_SIZE * TRANSACTIONS_COUNT) != LEDSTRIP_REQUIRED_BUFFER_SIZE)
#error "LEDSTRIP_REQUIRED_BUFFER_SIZE not equali divided :("
#endif

#define SPI_CLK_FREQ                    (6 * 1000 * 1000 + 400 * 1000)

#define LEDS_HOST                       SPI3_HOST

static led_matrix_t led_matrix;

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
#error "Not implemented"
#endif
}

static void ws2812b_grid_set_pixel(uint16_t pixel_idx, uint8_t r, uint8_t g, uint8_t b) {
#if BIT_RESOLUTION == 8
    /* Each pixel is stored in 24B */

    /* Green */
    ws2812b_grid_set_byte(pixel_idx * 3 + 0, g);
    
    /* Red */
    ws2812b_grid_set_byte(pixel_idx * 3 + 1, r);
    
    /* Blue */
    ws2812b_grid_set_byte(pixel_idx * 3 + 2, b);

#else
#error "Not implemented"
#endif
}

static void ws2812b_grid_fill_color(uint8_t r, uint8_t g, uint8_t b) {
    for (size_t pixel_idx = 0; pixel_idx < LEDS_COUNT; ++pixel_idx) {
        ws2812b_grid_set_pixel(pixel_idx, r, g, b);
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

esp_err_t ws2812b_grid_init() {
    ESP_LOGI(TAG, "Attempt to allocate DMA MEM %u B... Available=%u B.", 
        LEDSTRIP_REQUIRED_BUFFER_SIZE, heap_caps_get_free_size(MALLOC_CAP_DMA));
    // transfer_buffer = heap_caps_malloc(LEDSTRIP_REQUIRED_BUFFER_SIZE, MALLOC_CAP_DMA);
    // assert(transfer_buffer);
    memset(transfer_buffer, 0, LEDSTRIP_REQUIRED_BUFFER_SIZE);
    
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

    const float estimated_transfer_duration_ms = BIT_RESOLUTION * LEDSTRIP_REQUIRED_BUFFER_SIZE * 1000.0F / ((float)SPI_CLK_FREQ);
    ESP_LOGI(TAG, "Created LED strip driver. LEDs_count=%u, total_bytes=%u, SPI_transfer_size=%u. Transfer should take %f ms.",
        LEDS_COUNT, LEDSTRIP_REQUIRED_BUFFER_SIZE, TRANSACTION_BUFFER_SIZE, estimated_transfer_duration_ms
    );
    int64_t t1, t2, t3, t4, t5=0, t6=0;

    t1 = esp_timer_get_time();

        spi_device_acquire_bus(spi, portMAX_DELAY);

    t2 = esp_timer_get_time();

        ws2812b_grid_fill_color(0, 0, 32+2+1);

    t3 = esp_timer_get_time();

        ws2812b_grid_refresh();

    t4 = esp_timer_get_time();

        spi_device_release_bus(spi);

    t5 = esp_timer_get_time();


    t6 = esp_timer_get_time();
    
    ESP_LOGW(TAG, "A: %lld, B:%lld, C:%lld, D:%lld, E:%lld", t2-t1, t3-t2, t4-t3, t5-t4, t6-t5);

    return ESP_OK;
}













// static led_strip_handle_t led_strip;
// // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
// #define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)
// esp_err_t ws2812b_grid_init() {

//     /* LED strip initialization with the GPIO and pixels number*/
//     led_strip_config_t strip_config = {
//         .strip_gpio_num = LEDSTRIP_GPIO, // The GPIO that connected to the LED strip's data line
//         .max_leds = LEDSTRIP_LEDS_COUNT, // The number of LEDs in the strip,
//         .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
//         .led_model = LED_MODEL_WS2812, // LED strip model
//         .flags.invert_out = false, // whether to invert the output signal (useful when your hardware has a level inverter)
//     };

//     led_strip_rmt_config_t rmt_config = {
//     #if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(5, 0, 0)
//         .rmt_channel = 0,
//     #else
//         .clk_src = RMT_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
//         .resolution_hz = LED_STRIP_RMT_RES_HZ,
//         .flags.with_dma = true, // whether to enable the DMA feature
//     #endif
//     };
    
//     int64_t t1, t2, t3, t4, t5, t6;

//     t1 = esp_timer_get_time();

//     esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
//     ESP_RETURN_ON_ERROR(ret, TAG, "Creating LED Strip driver failed");

//     t2 = esp_timer_get_time();

//     for (size_t pixel_idx = 0; pixel_idx < LEDSTRIP_LEDS_COUNT; ++pixel_idx) {
//         ret = led_strip_set_pixel(led_strip, pixel_idx, 0, pixel_idx % 120, 0);
//         ESP_RETURN_ON_ERROR(ret, TAG, "Setting LED Strip pixel %u failed", pixel_idx);  
//     }

//     t3 = esp_timer_get_time();
    
//     ret = led_strip_refresh(led_strip);
//     ESP_RETURN_ON_ERROR(ret, TAG, "Refreshing LED Strip failed");

//     t4 = esp_timer_get_time();

//         for (size_t pixel_idx = 0; pixel_idx < LEDSTRIP_LEDS_COUNT; ++pixel_idx) {
//         ret = led_strip_set_pixel(led_strip, pixel_idx, 0, 0, pixel_idx % 80);
//         ESP_RETURN_ON_ERROR(ret, TAG, "Setting LED Strip pixel %u failed", pixel_idx);  
//     }

//     t5 = esp_timer_get_time();
    
//     ret = led_strip_refresh(led_strip);
//     ESP_RETURN_ON_ERROR(ret, TAG, "Refreshing LED Strip failed");

//     t6 = esp_timer_get_time();

//     ESP_LOGI(TAG, "Created LED strip object with RMT backend");
//     ESP_LOGW(TAG, "Create: %lld, set:%lld, refr:%lld, set:%lld, refr:%lld", t2-t1, t3-t2, t4-t3, t5-t4, t6-t5);
//     return ESP_OK;
// }