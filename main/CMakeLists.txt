set(requires "spi_flash")

idf_component_register(SRCS "hmi/model.c" "hmi/gdisplay.c" "hmi/graphics.c" "hmi/gfonts.c" "hello_world_main.c" "gsampler.c" "led_matrix.c" "leds/ws2812b_grid.c" "rf/rf_recv.c"
                    REQUIRES ${requires}
                    PRIV_REQUIRES "esp_ringbuf" "espressif__esp-box-lite" "nvs_flash" "esp_wifi"
                    INCLUDE_DIRS "hmi")
