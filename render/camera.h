#ifndef RENDER_CAMERA_H
#define RENDER_CAMERA_H

#include "physics/physics.h"

typedef struct CameraBounds {
    double left;
    double right;
    double bottom;
    double top;
} CameraBounds;

/* Compute letterboxed orthographic bounds for `w` given aspect w/h. */
CameraBounds camera_compute_bounds(const World *w, int width, int height);

/* Convert window cursor coordinates (top-left origin) to world Vec2. */
Vec2 camera_screen_to_world(const World *w, double screen_x, double screen_y,
                            int win_w, int win_h);

#endif /* RENDER_CAMERA_H */
