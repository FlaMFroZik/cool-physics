/*
 * color.c — HSV -> RGB conversion for pleasant distinct ball colours.
 */
#include "util/color.h"

#include <math.h>

void palette_color(float rgb[3], double t)
{
    const double sat = 0.72, val = 0.95;
    double h = fmod(t, 1.0);
    if (h < 0.0)
        h += 1.0;
    h *= 6.0;
    int    sector = (int)h % 6;
    double f      = h - floor(h);
    double q      = val * (1.0 - sat * f);
    double p      = val * (1.0 - sat * (1.0 - f));
    double r = 0, g = 0, b = 0;
    switch (sector) {
    case 0:  r = val; g = p;   b = q;   break;
    case 1:  r = q;   g = val; b = p;   break;
    case 2:  r = p;   g = val; b = q;   break;
    case 3:  r = p;   g = q;   b = val; break;
    case 4:  r = q;   g = p;   b = val; break;
    default: r = val; g = q;   b = p;   break;
    }
    rgb[0] = (float)r;
    rgb[1] = (float)g;
    rgb[2] = (float)b;
}
