set(requires "spi_flash")

idf_component_register(SRCS "hmi/model.c" "hmi/gdisplay.c" "hmi/graphics.c" "hello_world_main.c" "gsampler.c" "led_matrix.c" 
                    REQUIRES ${requires}
                    PRIV_REQUIRES "esp_ringbuf" "espressif__esp-box-lite"
                    INCLUDE_DIRS "hmi")
