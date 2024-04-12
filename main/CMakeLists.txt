set(requires "spi_flash")

idf_component_register(SRCS "hello_world_main.c" "gsampler.c"
                    REQUIRES ${requires}
                    PRIV_REQUIRES "esp_ringbuf"
                    INCLUDE_DIRS "")
