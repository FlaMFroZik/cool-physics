/*
 * render.c — turns the long-double World into OpenGL draw calls and
 * fits the world's view rectangle into the window (letterboxed).
 */
#include "render.h"

#include <stdio.h>

#include "gl.h"

static int viewport_w = 1;
static int viewport_h = 1;

static const float CLEAR_COLOR[3]    = {0.043f, 0.051f, 0.075f};
static const float BOUNDS_COLOR[3]   = {0.32f, 0.38f, 0.47f};
static const float VELOCITY_COLOR[3] = {0.80f, 0.87f, 0.95f};

int render_init(void)
{
#ifdef GL_MULTISAMPLE
    glEnable(GL_MULTISAMPLE);
#endif
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    return 0;
}

void render_resize(int width, int height)
{
    if (width  < 1) width  = 1;
    if (height < 1) height = 1;
    viewport_w = width;
    viewport_h = height;
    rgl_viewport(viewport_w, viewport_h);
}

void render_frame(const World *w)
{
    double cx     = 0.5 * ((double)w->view_min.x + (double)w->view_max.x);
    double cy     = 0.5 * ((double)w->view_min.y + (double)w->view_max.y);
    double half_w = 0.5 * ((double)w->view_max.x - (double)w->view_min.x);
    double half_h = 0.5 * ((double)w->view_max.y - (double)w->view_min.y);
    if (half_w < 1e-9) half_w = 1e-9;
    if (half_h < 1e-9) half_h = 1e-9;

    /* letterbox: match the view aspect to the window aspect */
    double target_aspect = (double)viewport_w / (double)viewport_h;
    double view_aspect   = half_w / half_h;
    if (view_aspect < target_aspect)
        half_w = half_h * target_aspect;
    else
        half_h = half_w / target_aspect;

    rgl_begin_frame(CLEAR_COLOR);
    rgl_camera(cx - half_w, cx + half_w, cy - half_h, cy + half_h);

    if (w->walls_enabled)
        rgl_draw_rect_outline((double)w->world_min.x, (double)w->world_min.y,
                              (double)w->world_max.x, (double)w->world_max.y,
                              BOUNDS_COLOR, 2.0f);

    for (size_t i = 0; i < w->count; i++) {
        const Body *b = &w->bodies[i];
        double x = (double)b->pos.x;
        double y = (double)b->pos.y;
        rgl_draw_circle(x, y, (double)b->radius, b->color, 32);
        if (w->show_velocities)
            rgl_draw_line(x, y,
                          x + (double)b->vel.x * 0.2,
                          y + (double)b->vel.y * 0.2,
                          VELOCITY_COLOR);
    }
}

void render_shutdown(void)
{
    /* nothing to release — immediate mode keeps no GPU objects */
}
