#include "gdisplay.h"


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"

#include <math.h>
#include "esp_log.h"
#include "esp_check.h"
#include "esp_timer.h"

#include "gstyles.h"
#include "gdisplay_api.h"
#include "model.h"

static const char TAG[] = "GDisplay";

#define LCD_HOST    SPI2_HOST

#define DISPLAY_WIDTH   320
#define DISPLAY_HEIGHT  240
#define CHUNK_LINES     48
#define CHUNK_PIXELS_COUNT (CHUNK_LINES * DISPLAY_WIDTH)
#define CHUNKS_COUNT    (DISPLAY_HEIGHT / CHUNK_LINES)
#if CHUNKS_COUNT * CHUNK_LINES != DISPLAY_HEIGHT
#error "Parallel lines count not equali diving display height"
#endif
#define DISPLAY_PIXELS_COUNT    (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define DISPLAY_PIXELS_PER_CHUNK  (DISPLAY_WIDTH * CHUNK_LINES)
#define DISPLAY_TARGET_DRAW_INTERVAL  (50)

#define PIN_NUM_MISO    -1
#define PIN_NUM_MOSI    6
#define PIN_NUM_CLK     7
#define PIN_NUM_CS      5

#define PIN_NUM_DC      4
#define PIN_NUM_RST     48
#define PIN_NUM_BCKL    45

#define LCD_BK_LIGHT_ON_LEVEL   0

static spi_device_handle_t spi;
static uint16_t* display_buffer;

static gdisplay_api_t gdisplay_api;

typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

static void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len);

static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active);

static void lcd_spi_pre_transfer_callback(spi_transaction_t *t);

static uint32_t lcd_get_id(spi_device_handle_t spi);

static void gdisplay_task(void* params);

/* Tools */

static void gdisplay_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    display_buffer[(DISPLAY_WIDTH - x - 1) + DISPLAY_WIDTH * y] = color;
}

static void gdisplay_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    for (uint16_t iy = 0; iy < h; iy++) {
        for (uint16_t ix = 0; ix < w; ix++) {
            display_buffer[(DISPLAY_WIDTH -(x + ix) - 1) + DISPLAY_WIDTH * (y + iy)] = color;
        }
    }
}


static void gdisplay_fill_black() {
    memset(display_buffer, 0, DISPLAY_PIXELS_COUNT * sizeof(uint16_t));
}

static void gdisplay_fill_color(uint16_t color) {
    for(uint32_t pixel_idx = 0; pixel_idx < DISPLAY_PIXELS_COUNT; ++pixel_idx) {
        display_buffer[pixel_idx] = color;
    }
}

static uint16_t gdisplay_get_display_width() {
    return DISPLAY_WIDTH;
}

static uint16_t gdisplay_get_display_height() {
    return DISPLAY_HEIGHT;
}

//Place data into DRAM. Constant data gets placed into DROM by default, which is not accessible by DMA.
DRAM_ATTR static const lcd_init_cmd_t st_init_cmds[] = {
    /* Memory Data Access Control, MX=MV=1, MY=ML=MH=0, RGB=0 */
    {0x36, {(1 << 5) | (1 << 6)}, 1},
    /* Interface Pixel Format, 16bits/pixel for RGB/MCU interface */
    {0x3A, {0x55}, 1},
    /* Porch Setting */
    {0xB2, {0x0c, 0x0c, 0x00, 0x33, 0x33}, 5},
    /* Gate Control, Vgh=13.65V, Vgl=-10.43V */
    {0xB7, {0x45}, 1},
    /* VCOM Setting, VCOM=1.175V */
    {0xBB, {0x2B}, 1},
    /* LCM Control, XOR: BGR, MX, MH */
    {0xC0, {0x2C}, 1},
    /* VDV and VRH Command Enable, enable=1 */
    {0xC2, {0x01, 0xff}, 2},
    /* VRH Set, Vap=4.4+... */
    {0xC3, {0x11}, 1},
    /* VDV Set, VDV=0 */
    {0xC4, {0x20}, 1},
    /* Frame Rate Control, 60Hz, inversion=0 */
    {0xC6, {0x0f}, 1},
    /* Power Control 1, AVDD=6.8V, AVCL=-4.8V, VDDS=2.3V */
    {0xD0, {0xA4, 0xA1}, 2},
    /* Positive Voltage Gamma Control */
    {0xE0, {0xD0, 0x00, 0x05, 0x0E, 0x15, 0x0D, 0x37, 0x43, 0x47, 0x09, 0x15, 0x12, 0x16, 0x19}, 14},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0, 0x00, 0x05, 0x0D, 0x0C, 0x06, 0x2D, 0x44, 0x40, 0x0E, 0x1C, 0x18, 0x16, 0x19}, 14},
    //??//
    // {0x21, {0}, 0},
    /* Sleep Out */
    {0x11, {0}, 0x80},
    /* Display On */
    {0x29, {0}, 0x80},
    {0, {0}, 0xff}
};

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd, bool keep_cs_active) {
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = 8;                   //Command is 8 bits
    t.tx_buffer = &cmd;             //The data is the cmd itself
    t.user = (void*)0;              //D/C needs to be set to 0
    if (keep_cs_active) {
        t.flags = SPI_TRANS_CS_KEEP_ACTIVE;   //Keep CS active after data transfer
    }
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
static void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len) {
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0) {
        return;    //no need to send anything
    }
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length = len * 8;             //Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;             //Data
    t.user = (void*)1;              //D/C needs to be set to 1
    ret = spi_device_polling_transmit(spi, &t); //Transmit!
    assert(ret == ESP_OK);          //Should have had no issues.
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
static void lcd_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

static uint32_t lcd_get_id(spi_device_handle_t spi) {
    // When using SPI_TRANS_CS_KEEP_ACTIVE, bus must be locked/acquired
    spi_device_acquire_bus(spi, portMAX_DELAY);

    //get_id cmd
    lcd_cmd(spi, 0x04, true);

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8 * 3;
    t.flags = SPI_TRANS_USE_RXDATA;
    t.user = (void*)1;

    esp_err_t ret = spi_device_polling_transmit(spi, &t);
    assert(ret == ESP_OK);

    // Release bus
    spi_device_release_bus(spi);

    return *(uint32_t*)t.rx_data;
}

/* To send a set of lines we have to send a command, 2 data bytes, another command, 2 more data bytes and another command
 * before sending the line data itself; a total of 6 transactions. (We can't put all of this in just one transaction
 * because the D/C line needs to be toggled in the middle.)
 * This routine queues these commands up as interrupt transactions so they get
 * sent faster (compared to calling spi_device_transmit several times), and at
 * the mean while the lines for next transactions can get calculated.
 */
static void send_lines(spi_device_handle_t spi, int ypos, uint16_t *linedata) {
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x = 0; x < 6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x & 1) == 0) {
            //Even transfers are commands
            trans[x].length = 8;
            trans[x].user = (void*)0;
        } else {
            //Odd transfers are data
            trans[x].length = 8 * 4;
            trans[x].user = (void*)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;         //Column Address Set
    trans[1].tx_data[0] = 0;            //Start Col High
    trans[1].tx_data[1] = 0;            //Start Col Low
    trans[1].tx_data[2] = (320 - 1) >> 8;   //End Col High
    trans[1].tx_data[3] = (320 - 1) & 0xff; //End Col Low
    trans[2].tx_data[0] = 0x2B;         //Page address set
    trans[3].tx_data[0] = ypos >> 8;    //Start page high
    trans[3].tx_data[1] = ypos & 0xff;  //start page low
    trans[3].tx_data[2] = (ypos + CHUNK_LINES - 1) >> 8; //end page high
    trans[3].tx_data[3] = (ypos + CHUNK_LINES - 1) & 0xff; //end page low
    trans[4].tx_data[0] = 0x2C;         //memory write
    trans[5].tx_buffer = linedata;      //finally send the line data
    trans[5].length = DISPLAY_WIDTH * sizeof(uint16_t) * 8 * CHUNK_LINES;  //Data length, in bits
    trans[5].flags = 0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x = 0; x < 6; x++) {
        ret = spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret == ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}

static void send_line_finish(spi_device_handle_t spi) {
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x = 0; x < 6; x++) {
        ret = spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret == ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}

//Initialize the display
void lcd_init(spi_device_handle_t spi) {
    int cmd = 0;
    const lcd_init_cmd_t* lcd_init_cmds;

    //Initialize non-SPI GPIOs
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = ((1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BCKL));
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = true;
    gpio_config(&io_conf);

    //Reset the display
    gpio_set_level(PIN_NUM_RST, 0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
    gpio_set_level(PIN_NUM_RST, 1);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    //detect LCD type
    uint32_t lcd_id = lcd_get_id(spi);

    printf("LCD ID: %08"PRIx32"\n", lcd_id);

    lcd_init_cmds = st_init_cmds;

    //Send all the commands
    while (lcd_init_cmds[cmd].databytes != 0xff) {
        lcd_cmd(spi, lcd_init_cmds[cmd].cmd, false);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes & 0x1F);
        if (lcd_init_cmds[cmd].databytes & 0x80) {
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        cmd++;
    }

    ///Enable backlight
    gpio_set_level(PIN_NUM_BCKL, LCD_BK_LIGHT_ON_LEVEL);
}

static void update_draw_buffer() {
    model_tick();

    memset(display_buffer, 0xFF, DISPLAY_PIXELS_COUNT * sizeof(uint16_t));
    model_draw(&gdisplay_api);
}

static void gdisplay_task(void* params) {
    
    model_init();

    uint64_t start_updating_time;
    uint64_t end_updating_time;
    uint64_t updating_time;

    uint64_t start_sending_time;
    uint64_t end_sending_time;

    uint64_t sending_time;
    uint64_t chunk_sending_time;

    uint64_t start_draw_time;
    uint64_t end_draw_time;
    uint64_t draw_time;

    uint64_t total_sending_time = 0;
    uint64_t total_draw_time = 0;
    
    uint64_t start_measurement_time;
    uint64_t end_measurement_time;
    uint64_t total_measurement_time;


    const size_t measure_interval = 20;
    size_t measure_counter = 0;

    float fps;
    float usage;
    uint32_t draw_time_ms;
    
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t target_interval_refresh_ticks = (DISPLAY_TARGET_DRAW_INTERVAL) / 10; 

    start_measurement_time = esp_timer_get_time();

    while(1) {
        start_draw_time = esp_timer_get_time();
        sending_time = 0;

        start_updating_time = esp_timer_get_time();
        update_draw_buffer();
        end_updating_time = esp_timer_get_time();

        updating_time = end_updating_time - start_updating_time;
        sending_time += updating_time; 

        for(size_t chunk_idx = 0; chunk_idx < CHUNKS_COUNT; ++chunk_idx) {
            const size_t offset = chunk_idx * CHUNK_PIXELS_COUNT;

            start_sending_time = esp_timer_get_time();
            send_lines(spi, chunk_idx * CHUNK_LINES, display_buffer + offset);
            end_sending_time = esp_timer_get_time();
            chunk_sending_time = end_sending_time - start_sending_time;
            sending_time += chunk_sending_time;

            send_line_finish(spi);
        }
        
        /* Cap */
        xTaskDelayUntil(&xLastWakeTime, target_interval_refresh_ticks);

        end_draw_time = esp_timer_get_time();
        draw_time = end_draw_time - start_draw_time;

        total_draw_time += draw_time;
        total_sending_time += sending_time;

        if(measure_counter >= measure_interval) {
            end_measurement_time = esp_timer_get_time();
            total_measurement_time = end_measurement_time - start_measurement_time;

            fps = ((float)measure_interval) / (((float)total_measurement_time) / 1000000.0F);
            usage = 100.0F * ((float)total_sending_time) / ((float)total_measurement_time);
            draw_time_ms = (uint32_t)(total_draw_time / measure_interval / 1000);

            total_sending_time = 0;
            total_draw_time = 0;

            ESP_LOGI(TAG, "Draw: time=%lums, fps=%.3f, usage=%.1f", draw_time_ms, fps, usage);
            measure_counter = 0;
            
            start_measurement_time = esp_timer_get_time();
        }
        ++measure_counter;
    }
}

esp_err_t gdisplay_lcd_init(void) {
    ESP_LOGI(TAG, "GDisplay initializing...");

    display_buffer = heap_caps_malloc(DISPLAY_PIXELS_COUNT * sizeof(uint16_t), MALLOC_CAP_DMA);
    assert(display_buffer);

    gdisplay_api.draw_pixel = gdisplay_draw_pixel;
    gdisplay_api.draw_rect = gdisplay_draw_rect;
    gdisplay_api.fill_black = gdisplay_fill_black;
    gdisplay_api.fill_color = gdisplay_fill_color;
    gdisplay_api.get_display_width = gdisplay_get_display_width;
    gdisplay_api.get_display_height = gdisplay_get_display_height;

    gdisplay_api.fill_black();

    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CHUNK_LINES * DISPLAY_WIDTH * sizeof(uint16_t) + 8
    };
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,     //Clock out at 10 MHz
        .mode = 0,                              //SPI mode 0
        .spics_io_num = PIN_NUM_CS,             //CS pin
        .queue_size = 7,                        //We want to be able to queue 7 transactions at a time
        .pre_cb = lcd_spi_pre_transfer_callback, //Specify pre-transfer callback to handle D/C line
    };
    //Initialize the SPI bus
    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);

    //Initialize the LCD
    lcd_init(spi);

    const bool display_task_was_created = xTaskCreate(
        gdisplay_task, 
        "gdisplay_task", 
        1024 * 8, 
        NULL, 
        20, 
        NULL
    ) == pdTRUE;
    ESP_RETURN_ON_FALSE(display_task_was_created, ESP_FAIL, TAG, "Failed creating \'display_task\'!");

    ESP_LOGI(TAG, "GDisplay init success!");
    return ESP_OK;
}