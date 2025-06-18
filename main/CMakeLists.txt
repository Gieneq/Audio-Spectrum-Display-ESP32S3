set(requires "spi_flash")

idf_component_register(SRCS 
                        "main.c" 
                        "hmi/model.c" "hmi/gdisplay.c" "hmi/graphics.c" "hmi/gfonts.c" 
                        "gsampler.c" 
                        "rf/rf_sender.c" "rf/asd_packet.c" 
                        "leds/led_matrix.c" 
                        "effects/effects.c" 
                        "effects/source/sources.c" 
                        "effects/source/simulation.c" 
                        "effects/types/simple.c"
                    REQUIRES ${requires}
                    PRIV_REQUIRES "esp_ringbuf" "espressif__esp-box-lite" "nvs_flash" "esp_wifi" "esp_event" "esp_netif" "esp_timer"
                    INCLUDE_DIRS "hmi")
