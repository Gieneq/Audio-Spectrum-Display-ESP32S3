#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

#define MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define CONSTRAIN(_v, _min, _max) ((_v) < (_min) ? (_min) : ((_v) > (_max) ? (_max) : (_v)))

#define MAP(_x, _in_min, _in_max, _out_min, _out_max) \
    (((_x) - (_in_min)) * ((_out_max) - (_out_min)) / ((_in_max) - (_in_min)) + (_out_min))

#define NORMALIZE(_x, _in_min, _in_max) \
    CONSTRAIN(((_x) - (_in_min)) / ((_in_max) - (_in_min)), 0.0f, 1.0f)

#define LERP(_a, _b, _t) ((_a) + ((_b) - (_a)) * (_t))

typedef enum option_select_t {
    OPTION_SELECT_GAIN,
    OPTION_SELECT_FREQUENCY,
    OPTION_SELECT_EFFECT,
    OPTION_SELECT_SOURCE,

    OPTION_SELECT_COUNT,
} option_select_t;

typedef enum effect_select_t {
    EFFECT_SELECT_RAW,
    EFFECT_SELECT_TRIBARS,
    EFFECT_SELECT_BLUEVIOLET,
    EFFECT_SELECT_WHITEBLOCKS,
    EFFECT_SELECT_ARCADE,

    EFFECT_SELECT_COUNT,
} effect_select_t;

typedef enum option_source_t {
    OPTION_SOURCE_SIMULATION,
    OPTION_SOURCE_MICROPHONE,
    OPTION_SOURCE_WIRED,

    OPTION_SOURCE_COUNT,
} option_source_t;

typedef struct color_16b_t {    union {
        uint16_t value;  // Access the whole 16-bit value
        struct {
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            uint16_t blue  : 5;  // 5 bits for blue
            uint16_t green : 6;  // 6 bits for green
            uint16_t red   : 5;  // 5 bits for red
            #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            uint16_t red   : 5;  // 5 bits for red
            uint16_t green : 6;  // 6 bits for green
            uint16_t blue  : 5;  // 5 bits for blue
            #endif
        };
    };
} __attribute__((packed)) color_16b_t;

typedef struct color_24b_t {
    /* data */
    union {
        uint32_t value;
        struct {
            #if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            uint8_t _reserved;
            #elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
            uint8_t _reserved;
            uint8_t blue;
            uint8_t green;
            uint8_t red;
            #endif
        };
    };
    
} __attribute__((packed)) color_24b_t;

#define WHITE ((color_24b_t){ .red = 255, .green = 255, .blue = 255 })

#define COLOR_24B(_red, _green, _blue) ((color_24b_t){ .red = _red, .green = _green, .blue = _blue })

static inline void color_24b_set_rgb(color_24b_t* color, uint8_t red, uint8_t green, uint8_t blue) {
    color->red = red;
    color->green = green;
    color->blue = blue;
}

typedef struct hsv_t {
    union {
        struct {
            float hue;        // 0–360 degrees
            float saturation; // 0–1
            float value;      // 0–1
        };
        float data[3];
    };
} __attribute__((packed)) hsv_t;

#define HSV(h, s, v) ((hsv_t){ .hue = (h), .saturation = (s), .value = (v) })

color_24b_t hsv_to_rgb(const hsv_t* hsv);

#ifdef __cplusplus
}
#endif