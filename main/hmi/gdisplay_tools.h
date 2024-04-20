#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "esp_err.h"

typedef struct gdisplay_api_t {
    void (*draw_pixel)(uint16_t x, uint16_t y, uint16_t color);
} gdisplay_api_t;

typedef struct gdisplay_api_context_t {
    uint16_t* display_buffer;
    uint16_t display_width;
    uint16_t display_height;
} gdisplay_api_context_t;

void draw_pixel_onto_displaybuffer(gdisplay_api_context_t* display_context, uint16_t x, uint16_t y, uint16_t color);

#ifdef __cplusplus
}
#endif