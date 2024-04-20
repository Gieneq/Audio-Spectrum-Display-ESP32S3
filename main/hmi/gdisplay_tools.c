#include "gdisplay_tools.h"

void draw_pixel_onto_displaybuffer(gdisplay_api_context_t* display_context, uint16_t x, uint16_t y, uint16_t color) {
    display_context->display_buffer[x + display_context->display_width * y] = color;
}