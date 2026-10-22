/*
 * main.c — cool-physics entry point: GLFW window, main loop, scenes.
 *
 * The simulation flavour is a compile-time switch driven by the Makefile:
 *   -DSIM_CONSTANT  -> uniform gravity, balls bouncing in a box
 *   -DSIM_NEWTON    -> Newtonian N-body gravity (sun + orbiting planets)
 * Build with `make const` / `make newton` or `make run SIM=const|newton`.
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <GLFW/glfw3.h>

#include "physics/physics.h"
#include "render/render.h"

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
#define DEFAULT_BODIES 90 /* planets; the sun is added on top */
#endif

typedef struct App {
    World       world;
    GLFWwindow *window;

    size_t    body_count;
    uint64_t  rng_state;
    bool      paused;
    phys_real time_scale;
    phys_real accumulator;

    double last_frame;
    double fps_clock;
    int    fps_frames;
} App;

/* ------------------------------- RNG ---------------------------------- */

static uint64_t rng_next(App *app) /* xorshift64* */
{
    uint64_t x = app->rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    app->rng_state = x;
    return x * 2685821657736338717ULL;
}

static void rng_seed(App *app, uint64_t seed)
{
    app->rng_state = seed ? seed : 0x9E3779B97F4A7C15ULL;
    (void)rng_next(app); /* warm-up draws so any seed is usable */
    (void)rng_next(app);
}

static double rng_uniform(App *app)
{
    return (double)(rng_next(app) >> 11) * (1.0 / 9007199254740992.0);
}

static double rng_range(App *app, double lo, double hi)
{
    return lo + (hi - lo) * rng_uniform(app);
}

/* ----------------------------- colours --------------------------------- */

/* HSV -> RGB, golden-ratio hue walk for pleasant distinct colours. */
static void palette_color(float rgb[3], double t)
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

/* ------------------------------ scenes --------------------------------- */

#if defined(SIM_CONSTANT)

/* Balls raining into the box. */
static void spawn_scene(App *app)
{
    World *w = &app->world;
    phys_world_clear(w);

    const double margin = 6.0;
    double x0 = margin;
    double x1 = (double)w->world_max.x - margin;
    double y0 = (double)w->world_max.y * 0.30;
    double y1 = (double)w->world_max.y - margin;
    double aw = x1 - x0, ah = y1 - y0;

    size_t n = app->body_count;
    if (n == 0)
        return;

    size_t cols = (size_t)ceil(sqrt((double)n * aw / ah));
    if (cols < 1)
        cols = 1;
    size_t rows = (n + cols - 1) / cols;
    double dx   = aw / (double)cols;
    double dy   = ah / (double)rows;
    double rmax = 0.45 * (dx < dy ? dx : dy);
    if (rmax < 1.0)
        rmax = 1.0;

    for (size_t i = 0; i < n; i++) {
        Body b;
        memset(&b, 0, sizeof(b));
        size_t col = i % cols, row = i / cols;
        double x   = x0 + dx * ((double)col + 0.5) + rng_range(app, -0.15, 0.15) * dx;
        double y   = y0 + dy * ((double)row + 0.5) + rng_range(app, -0.15, 0.15) * dy;
        double r   = rmax * rng_range(app, 0.55, 1.0);
        b.pos      = vec2_make((phys_real)x, (phys_real)y);
        b.vel      = vec2_make((phys_real)rng_range(app, -8.0, 8.0),
                               (phys_real)rng_range(app, -3.0, 3.0));
        b.radius   = (phys_real)r;
        b.mass     = (phys_real)3.14159265358979323846 * b.radius * b.radius; /* density 1 */
        palette_color(b.color, (double)i * 0.61 + 0.1);
        phys_world_add(w, &b);
    }
}

#else /* GRAV_NEWTON */

/* One heavy sun + planets on near-circular orbits. */
static void spawn_scene(App *app)
{
    World *w = &app->world;
    phys_world_clear(w);

    static const phys_real SUN_MASS   = (phys_real)4000;
    static const phys_real SUN_RADIUS = (phys_real)12;

    Body sun;
    memset(&sun, 0, sizeof(sun));
    sun.mass   = SUN_MASS;
    sun.radius = SUN_RADIUS;
    sun.color[0] = 1.00f; sun.color[1] = 0.62f; sun.color[2] = 0.18f;
    phys_world_add(w, &sun);

    size_t n = app->body_count;
    Vec2 total_momentum = vec2_make(0, 0);
    phys_real total_mass = SUN_MASS;

    for (size_t i = 0; i < n; i++) {
        double t  = (n > 1) ? (double)i / (double)(n - 1) : 0.5;
        double r  = 25.0 + t * (100.0 - 25.0) + rng_range(app, -3.0, 3.0);
        double th = rng_uniform(app) * 6.28318530717958647692;
        double v  = sqrt((double)(w->G * SUN_MASS) / r) *
                    rng_range(app, 0.97, 1.03);

        Body p;
        memset(&p, 0, sizeof(p));
        p.pos    = vec2_make((phys_real)(r * cos(th)), (phys_real)(r * sin(th)));
        p.vel    = vec2_make((phys_real)(-sin(th) * v), (phys_real)(cos(th) * v));
        p.radius = (phys_real)rng_range(app, 0.9, 2.2);
        p.mass   = (phys_real)0.5 * (phys_real)3.14159265358979323846
                 * p.radius * p.radius;
        palette_color(p.color, t * 0.9);
        phys_world_add(w, &p);

        total_momentum = vec2_add(total_momentum, vec2_scale(p.vel, p.mass));
        total_mass += p.mass;
    }

    /* cancel net momentum so the system stays centred on screen */
    for (size_t i = 0; i < w->count; i++)
        w->bodies[i].vel = vec2_sub(w->bodies[i].vel,
            vec2_scale(total_momentum, (phys_real)1 / total_mass));
}

#endif

/* ---------------------------- GLFW plumbing ---------------------------- */

static void on_error(int code, const char *description)
{
    fprintf(stderr, "GLFW error %d: %s\n", code, description);
}

static void on_framebuffer_size(GLFWwindow *window, int w, int h)
{
    (void)window;
    render_resize(w, h);
}

static void on_key(GLFWwindow *window, int key, int scancode, int action,
                   int mods)
{
    (void)scancode;
    (void)mods;
    if (action != GLFW_PRESS && action != GLFW_REPEAT)
        return;
    App *app = glfwGetWindowUserPointer(window);

    switch (key) {
    case GLFW_KEY_ESCAPE:
    case GLFW_KEY_Q:
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;
    case GLFW_KEY_SPACE:
        app->paused = !app->paused;
        break;
    case GLFW_KEY_R:
        rng_seed(app, (uint64_t)time(NULL));
        spawn_scene(app);
        break;
    case GLFW_KEY_UP:
        app->time_scale *= (phys_real)1.25;
        if (app->time_scale > (phys_real)8)
            app->time_scale = (phys_real)8;
        break;
    case GLFW_KEY_DOWN:
        app->time_scale /= (phys_real)1.25;
        if (app->time_scale < (phys_real)0.0625)
            app->time_scale = (phys_real)0.0625;
        break;
    case GLFW_KEY_V:
        app->world.show_velocities = !app->world.show_velocities;
        break;
    case GLFW_KEY_G:
        app->world.walls_enabled = !app->world.walls_enabled;
        break;
    default:
        break;
    }
}

/* ------------------------------- CLI ----------------------------------- */

static void print_usage(const char *argv0)
{
    printf("cool-physics — 2D physics sandbox in long double (%s build)\n\n",
           phys_gravity_mode_name(APP_MODE));
    printf("usage: %s [-n bodies] [--seed S] [--scale X] [-h]\n", argv0);
    printf("  -n N        number of bodies (default %d)\n", DEFAULT_BODIES);
    printf("  --seed S    RNG seed for the scene (default: time)\n");
    printf("  --scale X   initial time scale (default 1.0)\n");
    printf("\nkeys: SPACE pause | R respawn | UP/DOWN time scale | "
           "V velocities | G walls | ESC quit\n");
}

int main(int argc, char **argv)
{
    App app;
    memset(&app, 0, sizeof(app));

    size_t   bodies = DEFAULT_BODIES;
    uint64_t seed   = (uint64_t)time(NULL);
    double   scale  = 1.0;

    for (int i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-n") || !strcmp(argv[i], "--bodies")) &&
            i + 1 < argc) {
            bodies = strtoul(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            seed = strtoull(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--scale") && i + 1 < argc) {
            scale = strtod(argv[++i], NULL);
        } else if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "unknown option: %s\n\n", argv[i]);
            print_usage(argv[0]);
            return 2;
        }
    }
    if (scale < 0.0)   scale = 0.0;
    if (scale > 16.0)  scale = 16.0;

    app.body_count = bodies;
    app.time_scale = (phys_real)scale;
    rng_seed(&app, seed);

    phys_world_init(&app.world, APP_MODE);
    spawn_scene(&app);

    glfwSetErrorCallback(on_error);
    if (!glfwInit()) {
        fprintf(stderr, "failed to initialize GLFW\n");
        return 1;
    }

    char title[128];
    snprintf(title, sizeof(title), "cool-physics — %s [long double]",
             phys_gravity_mode_name(APP_MODE));

    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow *window = glfwCreateWindow(WINDOW_W, WINDOW_H, title, NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create GLFW window\n");
        glfwTerminate();
        return 1;
    }
    app.window = window;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window, &app);
    glfwSetFramebufferSizeCallback(window, on_framebuffer_size);
    glfwSetKeyCallback(window, on_key);

    if (render_init() != 0) {
        fprintf(stderr, "render initialization failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    render_resize(fbw, fbh);

    printf("cool-physics  [%s]\n", phys_gravity_mode_name(APP_MODE));
    printf("physics precision : long double (%d-bit)\n",
           (int)(sizeof(long double) * 8));
    printf("bodies            : %zu\n", app.world.count);
    printf("collisions        : brute-force O(N^2), every pair\n");
    printf("timestep          : %Lg s (fixed, semi-implicit Euler)\n", SIM_DT);
    printf("keys: SPACE pause | R respawn | UP/DOWN time | V velocities | "
           "G walls | ESC quit\n\n");

    app.last_frame = glfwGetTime();
    app.fps_clock  = app.last_frame;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now       = glfwGetTime();
        double frame_dt  = now - app.last_frame;
        app.last_frame   = now;
        if (frame_dt > 0.25)
            frame_dt = 0.25; /* don't spiral after a stall */

        if (!app.paused) {
            app.accumulator += (phys_real)frame_dt * app.time_scale;
            int steps = 0;
            while (app.accumulator >= SIM_DT && steps < MAX_STEPS_PER_FRAME) {
                phys_world_step(&app.world, SIM_DT);
                app.accumulator -= SIM_DT;
                steps++;
            }
            if (steps == MAX_STEPS_PER_FRAME)
                app.accumulator = 0; /* can't keep up: drop sim time */
        }

        render_frame(&app.world);
        glfwSwapBuffers(window);

        app.fps_frames++;
        if (now - app.fps_clock >= 1.0) {
            double fps = (double)app.fps_frames / (now - app.fps_clock);
            app.fps_frames = 0;
            app.fps_clock  = now;
            printf("\r[%s] bodies=%zu fps=%5.1f t=%8.2Lf "
                   "collisions/step=%4zu total=%-9zu scale=%4.2f%s",
                   phys_gravity_mode_name(APP_MODE), app.world.count, fps,
                   app.world.time, app.world.contacts_last_step,
                   app.world.contacts_total, (double)app.time_scale,
                   app.paused ? "  [PAUSED]" : "");
            fflush(stdout);
        }
    }

    printf("\nbye\n");
    render_shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    phys_world_free(&app.world);
    return 0;
}
