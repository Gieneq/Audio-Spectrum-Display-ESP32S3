#include "gfonts.h"
#include "graphics.h"


// #define FONT_ROCKWELL_4PT_GLYPS_COUNT   (3)
static const gfont_glyph_t font_glyphs_rockwell_4pt[] = {
    //
    {
        .sign = '?',
        .bitmap_coords = {0, 0, (6 - 0) + 1, (11 - 0) + 1}
    },
    {
        .sign = '0',
        .bitmap_coords = {8, 0, (15 - 8) + 1, (11 - 0) + 1}
    },
    {
        .sign = '1',
        .bitmap_coords = {18, 0, (23 - 18) + 1, (11 - 0) + 1}
    },
    {
        .sign = '2',
        .bitmap_coords = {26, 0, (33 - 26) + 1, (11 - 0) + 1}
    },
    {
        .sign = '3',
        .bitmap_coords = {35, 0, (43 - 35) + 1, (11 - 0) + 1}
    },
    {
        .sign = '4',
        .bitmap_coords = {44, 0, (52 - 44) + 1, (11 - 0) + 1}
    },
    {
        .sign = '5',
        .bitmap_coords = {53, 0, (61 - 53) + 1, (11 - 0) + 1}
    },
    {
        .sign = '6',
        .bitmap_coords = {63, 0, (70 - 63) + 1, (11 - 0) + 1}
    },
    {
        .sign = '7',
        .bitmap_coords = {72, 0, (79 - 72) + 1, (11 - 0) + 1}
    },
    {
        .sign = '8',
        .bitmap_coords = {81, 0, (89 - 81) + 1, (11 - 0) + 1}
    },

//
    {
        .sign = '9',
        .bitmap_coords = {0, 17, (7 - 0) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'A',
        .bitmap_coords = {9, 17, (19 - 9) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'B',
        .bitmap_coords = {21, 17, (29 - 21) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'C',
        .bitmap_coords = {31, 17, (41 - 31) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'D',
        .bitmap_coords = {43, 17, (54 - 43) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'E',
        .bitmap_coords = {56, 17, (65 - 56) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'F',
        .bitmap_coords = {66, 17, (75 - 66) + 1, (28 - 17) + 1}
    },
    {
        .sign = 'G',
        .bitmap_coords = {76, 17, (87 - 76) + 1, (28 - 17) + 1}
    },

//
    {
        .sign = '.',
        .bitmap_coords = {14, 66, (18 - 14) + 1, (77 - 66) + 1}
    },
    {
        .sign = '+',
        .bitmap_coords = {25, 66, (37 - 25) + 1, (77 - 66) + 1}
    },
    {
        .sign = '-',
        .bitmap_coords = {18, 67, (25 - 18) + 1, (78 - 67) + 1}
    },

    
    // {
    //     .sign = '[',
    //     .bitmap_coords = {18, 66, (25 - 18) + 1, (81 - 66) + 1}
    // },
};

const gfont_t font_rockwell_4pt = {
    .glyphs = font_glyphs_rockwell_4pt,
    .glyps_count = sizeof(font_glyphs_rockwell_4pt) / sizeof(font_glyphs_rockwell_4pt[0]),
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