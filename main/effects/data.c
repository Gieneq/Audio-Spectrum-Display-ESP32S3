#include "data.h"

#include <stdlib.h>
#include "../gtypes.h"
#include <math.h>

const color_24b_t rainbow_colors[LED_MATRIX_COLUMNS] = {
    COLOR_24B(255, 0, 0),       // Red
    COLOR_24B(255, 40, 0),      // Red-Orange
    COLOR_24B(255, 85, 0),      // Orange
    COLOR_24B(255, 170, 0),     // Yellow-Orange
    COLOR_24B(255, 255, 0),     // Yellow
    COLOR_24B(170, 255, 0),     // Yellow-Green
    COLOR_24B(85, 255, 0),      // Greenish
    COLOR_24B(0, 255, 0),       // Green
    COLOR_24B(0, 255, 85),      // Green-Cyan
    COLOR_24B(0, 255, 170),     // Cyanish
    COLOR_24B(0, 255, 255),     // Cyan
    COLOR_24B(0, 170, 255),     // Cyan-Blue
    COLOR_24B(0, 85, 255),      // Blueish
    COLOR_24B(0, 0, 255),       // Blue
    COLOR_24B(40, 0, 255),      // Blue-Indigoish
    COLOR_24B(85, 0, 255),      // Indigoish
    COLOR_24B(170, 0, 255),     // Violetish
    COLOR_24B(255, 0, 255),     // Magenta
    COLOR_24B(255, 0, 170),     // Magenta-Red
};