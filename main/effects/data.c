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

const hsv_t flame_tongue[LED_MATRIX_ROWS] = {
    /* Whitish inner: 6 */
    HSV(0,    0.00f, 1.00f), 
    HSV(10,   0.05f, 1.00f),
    HSV(20,   0.10f, 0.95f),
    HSV(30,   0.15f, 0.90f),
    HSV(40,   0.20f, 0.95f),
    HSV(60,   0.30f, 1.00f),

    /* Yellow inner: 4 */
    HSV(55,   0.50f, 0.92f),
    HSV(50,   0.55f, 0.88f),
    HSV(47,   0.64f, 0.86f),
    HSV(44,   0.74f, 0.91f),

    /* Orange inner: 5 */
    HSV(42,   0.76f, 0.84f),
    HSV(38,   0.81f, 0.86f),
    HSV(34,   0.83f, 0.83f),
    HSV(29,   0.86f, 0.87f),
    HSV(22,   0.91f, 0.81f),

    /* Dark red top: 6 */
    HSV(18,   1.00f, 0.75f),
    HSV(12,   1.00f, 0.58f),
    HSV(8,    1.00f, 0.37f),
    HSV(4,    1.00f, 0.17f),
    HSV(3,    1.00f, 0.11f),
    HSV(2,    1.00f, 0.05f),
};