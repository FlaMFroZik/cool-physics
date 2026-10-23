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

#define BASE_TICK_DT        ((phys_real)1 / (phys_real)60) /* 60 Hz base tick, s */
#define DEFAULT_SUBTICKS    4                              /* default sub-ticks per 60 Hz tick (240 Hz) */
#define MAX_SUBTICKS        4096
#define SIM_DT              (BASE_TICK_DT / (phys_real)DEFAULT_SUBTICKS)
#define MAX_STEPS_PER_FRAME 64

#define MIN_BALL_RADIUS     ((phys_real)0.25)
#define MAX_BALL_RADIUS     ((phys_real)25.0)

#if defined(SIM_CONSTANT)
#define DEFAULT_BODIES 140
#else
#define DEFAULT_BODIES 90
#endif

typedef struct App {
    World       world;
    GLFWwindow *window;

    size_t    body_count;
    size_t    subticks;        /* physics sub-ticks per 60 Hz base tick */
    phys_real sub_dt;          /* BASE_TICK_DT / subticks               */
    phys_real ball_radius;     /* uniform ball radius for this scene    */
    bool      custom_radius;   /* true if user set a custom ball radius */
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

static inline void app_set_subticks(App *app, size_t subticks)
{
    if (subticks < 1)
        subticks = 1;
    if (subticks > MAX_SUBTICKS)
        subticks = MAX_SUBTICKS;
    app->subticks = subticks;
    app->sub_dt   = BASE_TICK_DT / (phys_real)subticks;
}

#endif /* APP_H */
