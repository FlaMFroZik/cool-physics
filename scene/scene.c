/*
 * scene.c — initial scene generation and runtime ball creation/deletion
 * for both constant-gravity and Newtonian N-body modes.
 */
#include "scene/scene.h"

#include <math.h>

#include "util/color.h"

#if defined(SIM_CONSTANT)

static phys_real compute_constant_ball_radius(const World *w, size_t ref_count,
                                              double *out_dx, double *out_dy,
                                              size_t *out_cols)
{
    const double margin = 6.0;
    double x0 = margin;
    double x1 = (double)w->world_max.x - margin;
    double y0 = (double)w->world_max.y * 0.30;
    double y1 = (double)w->world_max.y - margin;
    double aw = x1 - x0, ah = y1 - y0;

    size_t n = ref_count ? ref_count : (size_t)DEFAULT_BODIES;
    size_t cols = (size_t)ceil(sqrt((double)n * aw / ah));
    if (cols < 1)
        cols = 1;
    size_t rows = (n + cols - 1) / cols;
    double dx   = aw / (double)cols;
    double dy   = ah / (double)rows;
    double r    = 0.35 * (dx < dy ? dx : dy);
    if (r < 1.0)
        r = 1.0;

    if (out_dx)   *out_dx   = dx;
    if (out_dy)   *out_dy   = dy;
    if (out_cols) *out_cols = cols;
    return (phys_real)r;
}

/* Equal-sized balls raining into the box. */
void scene_spawn(App *app)
{
    World *w = &app->world;
    phys_world_clear(w);
    app->next_color_idx = 0;

    double dx = 0.0, dy = 0.0;
    size_t cols = 1;
    app->ball_radius = compute_constant_ball_radius(w, app->body_count,
                                                    &dx, &dy, &cols);

    size_t n = app->body_count;
    if (n == 0)
        return;

    const double margin = 6.0;
    double x0 = margin;
    double y0 = (double)w->world_max.y * 0.30;

    for (size_t i = 0; i < n; i++) {
        size_t col = i % cols, row = i / cols;
        double x   = x0 + dx * ((double)col + 0.5)
                        + rng_range(&app->rng, -0.15, 0.15) * dx;
        double y   = y0 + dy * ((double)row + 0.5)
                        + rng_range(&app->rng, -0.15, 0.15) * dy;
        Vec2 pos   = vec2_make((phys_real)x, (phys_real)y);
        Vec2 vel   = vec2_make((phys_real)rng_range(&app->rng, -8.0, 8.0),
                               (phys_real)rng_range(&app->rng, -3.0, 3.0));
        scene_spawn_ball(app, pos, vel);
    }
}

#else /* GRAV_NEWTON */

/* Equal-sized balls in mutual Newtonian gravity (no central sun). */
void scene_spawn(App *app)
{
    World *w = &app->world;
    phys_world_clear(w);
    app->next_color_idx = 0;

    static const phys_real BALL_RADIUS = (phys_real)2.5;
    app->ball_radius = BALL_RADIUS;

    size_t n = app->body_count;
    if (n == 0)
        return;

    const phys_real ball_mass = PHYS_PI * BALL_RADIUS * BALL_RADIUS;

    Vec2 center_pos      = vec2_make(0, 0);
    Vec2 total_momentum  = vec2_make(0, 0);
    phys_real total_mass = (phys_real)0;

    for (size_t i = 0; i < n; i++) {
        double frac  = ((double)i + 0.5) / (double)n;
        double r     = 8.0 + sqrt(frac) * 90.0
                           + rng_range(&app->rng, -1.5, 1.5);
        double th    = rng_uniform(&app->rng) * 6.28318530717958647692;
        double m_enc = ((double)i + 0.5) * (double)ball_mass;
        double v     = sqrt((double)w->G * m_enc / (r + 5.0)) *
                       rng_range(&app->rng, 0.75, 1.00);

        Vec2 pos = vec2_make((phys_real)(r * cos(th)),
                             (phys_real)(r * sin(th)));
        Vec2 vel = vec2_make((phys_real)(-sin(th) * v),
                             (phys_real)( cos(th) * v));
        Body *p  = scene_spawn_ball(app, pos, vel);
        if (!p)
            continue;

        center_pos     = vec2_add(center_pos, vec2_scale(p->pos, p->mass));
        total_momentum = vec2_add(total_momentum, vec2_scale(p->vel, p->mass));
        total_mass    += p->mass;
    }

    if (total_mass > (phys_real)0) {
        /* centre the system and cancel net momentum so it stays on screen */
        Vec2 com_pos = vec2_scale(center_pos,     (phys_real)1 / total_mass);
        Vec2 com_vel = vec2_scale(total_momentum, (phys_real)1 / total_mass);
        for (size_t i = 0; i < w->count; i++) {
            w->bodies[i].pos = vec2_sub(w->bodies[i].pos, com_pos);
            w->bodies[i].vel = vec2_sub(w->bodies[i].vel, com_vel);
        }
    }
}

#endif

Body *scene_spawn_ball(App *app, Vec2 pos, Vec2 vel)
{
    World *w = &app->world;
    phys_real r = (app->ball_radius > (phys_real)0)
                      ? app->ball_radius : (phys_real)2.5;
    pos = phys_world_clamp_pos(w, pos, r);

    float color[3];
    palette_color(color, (double)app->next_color_idx * 0.61803398875 + 0.1);
    app->next_color_idx++;

    Body b;
    body_init_ball(&b, pos, vel, r, color);
    return phys_world_add(w, &b);
}

Body *scene_spawn_ball_at_cursor(App *app)
{
    World *w = &app->world;
    Vec2 pos;
    if (app->cursor_valid) {
        pos = app->cursor_world;
    } else {
        phys_real cx = (phys_real)0.5 * (w->view_min.x + w->view_max.x);
        phys_real cy = (phys_real)0.5 * (w->view_min.y + w->view_max.y);
        pos = vec2_make(cx + (phys_real)rng_range(&app->rng, -10.0, 10.0),
                        cy + (phys_real)rng_range(&app->rng, -10.0, 10.0));
    }
    Vec2 vel = vec2_make((phys_real)rng_range(&app->rng, -3.0, 3.0),
                         (phys_real)rng_range(&app->rng, -3.0, 3.0));
    return scene_spawn_ball(app, pos, vel);
}

bool scene_remove_ball_at(App *app, Vec2 world_pos)
{
    World *w = &app->world;
    ptrdiff_t idx = phys_world_pick(w, world_pos, (phys_real)0.5);
    if (idx < 0)
        return false;
    return phys_world_remove_at(w, (size_t)idx);
}

bool scene_remove_last_ball(App *app)
{
    World *w = &app->world;
    if (w->count == 0)
        return false;
    return phys_world_remove_at(w, w->count - 1);
}
