#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <string.h>

typedef struct gfont_glyph_t {
    char sign;
    struct {
        uint8_t x;
        uint8_t y;
        uint8_t w;
        uint8_t h;
    } bitmap_coords;
} gfont_glyph_t;

typedef struct gfont_t {
    const uint8_t* font_graphics_bytes;
    size_t font_graphics_bytes_count;
    size_t font_graphics_width;
    const gfont_glyph_t* glyphs;
    size_t glyps_count;
    char fallback_char;
} gfont_t;

extern const gfont_t font_rockwell_4pt;

const gfont_glyph_t* gfont_get_glyph_with_char(const gfont_t* font, char ch);

#ifdef __cplusplus
}
#endif
