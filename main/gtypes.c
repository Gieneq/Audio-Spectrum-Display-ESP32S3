#include "gtypes.h"
#include <math.h>

color_24b_t hsv_to_rgb(const hsv_t* hsv) {
    float h = hsv->hue;
    float s = hsv->saturation;
    float v = hsv->value;

    float r, g, b;

    if (s <= 0.0f) {
        // Achromatic (grey)
        r = g = b = v;
    } else {
        h = fmodf(h, 360.0f);
        if (h < 0.0f) h += 360.0f;

        float sector = h / 60.0f;
        int i = (int)floorf(sector);
        float f = sector - i;

        float p = v * (1.0f - s);
        float q = v * (1.0f - s * f);
        float t = v * (1.0f - s * (1.0f - f));

        switch (i) {
            case 0:
                r = v; g = t; b = p;
                break;
            case 1:
                r = q; g = v; b = p;
                break;
            case 2:
                r = p; g = v; b = t;
                break;
            case 3:
                r = p; g = q; b = v;
                break;
            case 4:
                r = t; g = p; b = v;
                break;
            case 5:
            default:
                r = v; g = p; b = q;
                break;
        }
    }

    return (color_24b_t) {
        .red   = (uint8_t)(r * 255.0f),
        .green = (uint8_t)(g * 255.0f),
        .blue  = (uint8_t)(b * 255.0f)
    };
}