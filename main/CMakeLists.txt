set(requires "spi_flash")

idf_component_register(SRCS 
                        "main.c" 
                        "gtypes.c" 
                        "hmi/model.c" "hmi/gdisplay.c" "hmi/graphics.c" "hmi/gfonts.c" 
                        "rf/rf_sender.c" "rf/asd_packet.c" 
                        "leds/led_matrix.c" 
                        "effects/effects.c" 
                        "effects/data.c" 
                        "effects/types/raw.c"
                        "effects/types/simple.c"
                        "effects/types/fire.c"
                        "effects/types/multicolor.c"
                        "source/sources.c" 
                        "source/simulation.c" 
                        "source/microphones.c" 
                        "source/bins_map.c"
                        "source/wired.c" 
                    REQUIRES ${requires}
                    PRIV_REQUIRES "esp_ringbuf" "espressif__esp-box-lite" "nvs_flash" "esp_wifi" "esp_event" "esp_netif" "esp_timer"
                    INCLUDE_DIRS "hmi")
