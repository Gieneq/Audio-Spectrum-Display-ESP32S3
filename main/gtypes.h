#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum option_select_t {
    OPTION_SELECT_GAIN,
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

color_16b_t color_24b_to16b(color_24b_t color_24b);


#ifdef __cplusplus
}
#endif