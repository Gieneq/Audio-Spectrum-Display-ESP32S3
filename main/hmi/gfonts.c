#include "gfonts.h"
#include "graphics.h"


#define FONT_ROCKWELL_4PT_GLYPS_COUNT   (2)
static const gfont_glyph_t font_glyphs_rockwell_4pt[FONT_ROCKWELL_4PT_GLYPS_COUNT] = {
    {
        .sign = '?',
        .bitmap_coords = {0, 0, 6 - 0 + 1, 11 - 0 + 1}
    },
    {
        .sign = '0',
        .bitmap_coords = {8, 0, 15 - 8 + 1, 11 - 0 + 1}
    },
};

const gfont_t font_rockwell_4pt = {
    .glyphs = font_glyphs_rockwell_4pt,
    .glyps_count = FONT_ROCKWELL_4PT_GLYPS_COUNT,
    .font_graphics_width = FONT_ROCKWELL_4PT_WIDTH,
    .font_graphics_bytes = font_rockwell_4pt_graphics_bytes,
    .font_graphics_bytes_count = FONT_ROCKWELL_4PT_BYTES_COUNT,
    .fallback_char = '?',
};


const gfont_glyph_t* gfont_get_glyph_with_char(const gfont_t* font, char ch) {
    for (size_t glyph_idx = 0; glyph_idx < font->glyps_count; glyph_idx++) {
        const gfont_glyph_t* glyph = &font->glyphs[glyph_idx];
        if (glyph->sign == ch) {
            return glyph;
        }
    }
    return NULL; //todo fallback
}