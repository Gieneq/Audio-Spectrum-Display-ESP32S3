#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "effects.h"

extern const color_24b_t rainbow_colors[LED_MATRIX_COLUMNS];

extern const hsv_t flame_tongue[LED_MATRIX_ROWS];

#define SMOKE_PUFFS_COLORS_COUNT 10
extern const color_24b_t SMOKE_PUFFS_colors[SMOKE_PUFFS_COLORS_COUNT];

#ifdef __cplusplus
}
#endif