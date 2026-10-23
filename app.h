#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stddef.h>

#include <GLFW/glfw3.h>

#include "physics/physics.h"
#include "util/rng.h"

#if defined(SIM_CONSTANT) && defined(SIM_NEWTON)
#error "Build with only one of SIM_CONSTANT / SIM_NEWTON (see Makefile)."
#elif defined(SIM_CONSTANT)
#define APP_MODE GRAV_CONSTANT
#elif defined(SIM_NEWTON)
#define APP_MODE GRAV_NEWTON
#else
#error "No simulation selected: build with -DSIM_CONSTANT=1 or -DSIM_NEWTON=1 (make SIM=const|newton)."
#endif

#define WINDOW_W 1280
#define WINDOW_H 720

#define SIM_DT ((phys_real)1 / (phys_real)240) /* fixed physics timestep, s */
#define MAX_STEPS_PER_FRAME 64

#if defined(SIM_CONSTANT)
#define DEFAULT_BODIES 140
#else
#define DEFAULT_BODIES 90
#endif

typedef struct App {
    World       world;
    GLFWwindow *window;

    size_t    body_count;
    phys_real ball_radius;     /* uniform ball radius for this scene */
    size_t    next_color_idx;  /* palette index for newly spawned balls */

    Rng       rng;
    bool      paused;
    phys_real time_scale;
    phys_real accumulator;

    /* mouse / cursor interaction state */
    Vec2 cursor_world;
    bool cursor_valid;
    Vec2 drag_offset;
    Vec2 drag_last_pos;
    bool rmb_held;

    double last_frame;
    double fps_clock;
    int    fps_frames;
} App;

#endif /* APP_H */
