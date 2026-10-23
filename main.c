/*
 * main.c — cool-physics entry point: CLI options, GLFW window, main loop.
 *
 * The simulation flavour is a compile-time switch driven by the Makefile:
 *   -DSIM_CONSTANT  -> uniform gravity, equal-sized balls bouncing in a box
 *   -DSIM_NEWTON    -> Newtonian N-body gravity, equal-sized balls
 * Build with `make const` / `make newton` or `make run SIM=const|newton`.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "app.h"
#include "input/input.h"
#include "render/render.h"
#include "scene/scene.h"

static void print_usage(const char *argv0)
{
    printf("cool-physics — 2D physics sandbox in double (%s build)\n\n",
           phys_gravity_mode_name(APP_MODE));
    printf("usage: %s [-n bodies] [-r radius] [-s subticks] [--seed S] [--scale X] [-h]\n",
           argv0);
    printf("  -n, --bodies N     number of balls (default %d)\n", DEFAULT_BODIES);
    printf("  -r, --radius R     ball radius (%g..%g, default: auto)\n",
           MIN_BALL_RADIUS, MAX_BALL_RADIUS);
    printf("  -s, --subticks N   physics sub-ticks per 60 Hz tick (default %d)\n",
           DEFAULT_SUBTICKS);
    printf("  --seed S           RNG seed for the scene (default: time)\n");
    printf("  --scale X          initial time scale (default 1.0)\n");
    printf("\nmouse: LMB drag/create | RMB delete | MMB create | WHEEL resize balls\n");
    printf("keys : N/+ add | X/Del/- delete | , / . resize balls | C clear | R respawn\n");
    printf("       [ / ] sub-ticks | SPACE pause | UP/DOWN time scale | "
           "V velocities | B grid | G walls | ESC quit\n");
}

int main(int argc, char **argv)
{
    App app;
    memset(&app, 0, sizeof(app));

    size_t   bodies   = DEFAULT_BODIES;
    size_t   subticks = DEFAULT_SUBTICKS;
    double   radius   = 0.0;
    uint64_t seed     = (uint64_t)time(NULL);
    double   scale    = 1.0;

    for (int i = 1; i < argc; i++) {
        if ((!strcmp(argv[i], "-n") || !strcmp(argv[i], "--bodies")) &&
            i + 1 < argc) {
            bodies = strtoul(argv[++i], NULL, 10);
        } else if ((!strcmp(argv[i], "-r") || !strcmp(argv[i], "--radius") ||
                    !strcmp(argv[i], "--size")) &&
                   i + 1 < argc) {
            radius = strtod(argv[++i], NULL);
            if (radius < (double)MIN_BALL_RADIUS ||
                radius > (double)MAX_BALL_RADIUS) {
                fprintf(stderr, "invalid ball radius (must be %g..%g)\n",
                        MIN_BALL_RADIUS, MAX_BALL_RADIUS);
                return 2;
            }
        } else if ((!strcmp(argv[i], "-s") || !strcmp(argv[i], "--subticks") ||
                    !strcmp(argv[i], "--substeps")) &&
                   i + 1 < argc) {
            unsigned long val = strtoul(argv[++i], NULL, 10);
            if (val < 1 || val > MAX_SUBTICKS) {
                fprintf(stderr, "invalid sub-ticks value (must be 1..%d)\n",
                        MAX_SUBTICKS);
                return 2;
            }
            subticks = (size_t)val;
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
    if (scale < 0.0)  scale = 0.0;
    if (scale > 16.0) scale = 16.0;

    app.body_count = bodies;
    app.time_scale = (phys_real)scale;
    if (radius > 0.0) {
        app.ball_radius   = (phys_real)radius;
        app.custom_radius = true;
    }
    app_set_subticks(&app, subticks);
    rng_seed(&app.rng, seed);

    phys_world_init(&app.world, APP_MODE);
    scene_spawn(&app);

    input_register_error_callback();
    if (!glfwInit()) {
        fprintf(stderr, "failed to initialize GLFW\n");
        phys_world_free(&app.world);
        return 1;
    }

    char title[128];
    snprintf(title, sizeof(title), "cool-physics — %s [double]",
             phys_gravity_mode_name(APP_MODE));

    glfwWindowHint(GLFW_SAMPLES, 4);
    GLFWwindow *window = glfwCreateWindow(WINDOW_W, WINDOW_H, title, NULL, NULL);
    if (!window) {
        fprintf(stderr, "failed to create GLFW window\n");
        glfwTerminate();
        phys_world_free(&app.world);
        return 1;
    }
    app.window = window;

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    input_init(&app);

    if (render_init() != 0) {
        fprintf(stderr, "render initialization failed\n");
        glfwDestroyWindow(window);
        glfwTerminate();
        phys_world_free(&app.world);
        return 1;
    }
    int fbw, fbh;
    glfwGetFramebufferSize(window, &fbw, &fbh);
    render_resize(fbw, fbh);

    printf("cool-physics  [%s]\n", phys_gravity_mode_name(APP_MODE));
    printf("physics precision : double (%d-bit)\n",
           (int)(sizeof(phys_real) * 8));
    printf("bodies            : %zu (radius=%g)\n",
           app.world.count, app.ball_radius);
    printf("collisions        : uniform grid (cell=2*R=%g, 3x3 neighbours)\n",
           (phys_real)2 * app.ball_radius);
    printf("sub-ticks         : %zu per 60 Hz tick (dt = %g s, %.0f Hz)\n",
           app.subticks, app.sub_dt, (phys_real)1 / app.sub_dt);
    printf("mouse: LMB drag/create | RMB delete | MMB create | WHEEL resize\n");
    printf("keys : N/+ add | X/Del/- delete | , / . resize | C clear | [ / ] sub-ticks\n");
    printf("       SPACE pause | R respawn | UP/DOWN time | V vel | B grid | G walls | ESC quit\n\n");

    app.last_frame = glfwGetTime();
    app.fps_clock  = app.last_frame;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        double now      = glfwGetTime();
        double frame_dt = now - app.last_frame;
        app.last_frame  = now;
        if (frame_dt > 0.25)
            frame_dt = 0.25; /* don't spiral after a stall */

        input_update_drag(&app, frame_dt);

        if (!app.paused) {
            app.accumulator += (phys_real)frame_dt * app.time_scale;
            int steps = 0;
            while (app.accumulator >= BASE_TICK_DT &&
                   steps < MAX_STEPS_PER_FRAME) {
                for (size_t s = 0; s < app.subticks; s++)
                    phys_world_step(&app.world, app.sub_dt);
                app.accumulator -= BASE_TICK_DT;
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
            printf("\r[%s] bodies=%zu r=%.2f subticks=%zu fps=%5.1f t=%8.2f "
                   "checks/step=%4zu collisions/step=%4zu total=%-9zu scale=%4.2f%s   ",
                   phys_gravity_mode_name(APP_MODE), app.world.count,
                   app.ball_radius, app.subticks, fps, app.world.time,
                   app.world.pair_checks_last_step,
                   app.world.contacts_last_step, app.world.contacts_total,
                   (double)app.time_scale, app.paused ? "  [PAUSED]" : "");
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
