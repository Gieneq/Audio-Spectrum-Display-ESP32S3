#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Buttons */
#define BTN_LEFT_WIDTH  (60)
#define BTN_LEFT_HEIGHT  (54)
#define BTN_LEFT_BYTES_COUNT (BTN_LEFT_WIDTH * BTN_LEFT_HEIGHT * 2)
extern const uint8_t btn_left_graphics_bytes[BTN_LEFT_BYTES_COUNT];

#define BTN_LEFT_PRESSED_WIDTH  (60)
#define BTN_LEFT_PRESSED_HEIGHT  (54)
#define BTN_LEFT_PRESSED_BYTES_COUNT (BTN_LEFT_PRESSED_WIDTH * BTN_LEFT_PRESSED_HEIGHT * 2)
extern const uint8_t btn_left_pressed_graphics_bytes[BTN_LEFT_PRESSED_BYTES_COUNT];


#define BTN_RIGHT_WIDTH  (60)
#define BTN_RIGHT_HEIGHT  (54)
#define BTN_RIGHT_BYTES_COUNT (BTN_RIGHT_WIDTH * BTN_RIGHT_HEIGHT * 2)
extern const uint8_t btn_right_graphics_bytes[BTN_RIGHT_BYTES_COUNT];

#define BTN_RIGHT_PRESSED_WIDTH  (60)
#define BTN_RIGHT_PRESSED_HEIGHT  (54)
#define BTN_RIGHT_PRESSED_BYTES_COUNT (BTN_RIGHT_PRESSED_WIDTH * BTN_RIGHT_PRESSED_HEIGHT * 2)
extern const uint8_t btn_right_pressed_graphics_bytes[BTN_RIGHT_PRESSED_BYTES_COUNT];


#define BTN_MINUS_WIDTH  (60)
#define BTN_MINUS_HEIGHT  (54)
#define BTN_MINUS_BYTES_COUNT (BTN_MINUS_WIDTH * BTN_MINUS_HEIGHT * 2)
extern const uint8_t btn_minus_graphics_bytes[BTN_MINUS_BYTES_COUNT];

#define BTN_PLUS_WIDTH  (60)
#define BTN_PLUS_HEIGHT  (54)
#define BTN_PLUS_BYTES_COUNT (BTN_PLUS_WIDTH * BTN_PLUS_HEIGHT * 2)
extern const uint8_t btn_plus_graphics_bytes[BTN_PLUS_BYTES_COUNT];


/* Selector labels */
#define LABEL_EFFECT_WIDTH  (184)
#define LABEL_EFFECT_HEIGHT  (28)
#define LABEL_EFFECT_BYTES_COUNT (LABEL_EFFECT_WIDTH * LABEL_EFFECT_HEIGHT * 2)
extern const uint8_t label_effect_graphics_bytes[LABEL_EFFECT_BYTES_COUNT];

#define LABEL_SOURCE_WIDTH  (184)
#define LABEL_SOURCE_HEIGHT  (28)
#define LABEL_SOURCE_BYTES_COUNT (LABEL_SOURCE_WIDTH * LABEL_SOURCE_HEIGHT * 2)
extern const uint8_t label_source_graphics_bytes[LABEL_SOURCE_BYTES_COUNT];

#define LABEL_GAIN_WIDTH  (184)
#define LABEL_GAIN_HEIGHT  (28)
#define LABEL_GAIN_BYTES_COUNT (LABEL_GAIN_WIDTH * LABEL_GAIN_HEIGHT * 2)
extern const uint8_t label_gain_graphics_bytes[LABEL_GAIN_BYTES_COUNT];


#ifdef __cplusplus
}
#endif
