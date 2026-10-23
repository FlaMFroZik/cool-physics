/*
 * camera.c — letterboxed 2D camera projection and screen-to-world mapping.
 */
#include "render/camera.h"

CameraBounds camera_compute_bounds(const World *w, int width, int height)
{
    if (width  < 1) width  = 1;
    if (height < 1) height = 1;

    double cx     = 0.5 * ((double)w->view_min.x + (double)w->view_max.x);
    double cy     = 0.5 * ((double)w->view_min.y + (double)w->view_max.y);
    double half_w = 0.5 * ((double)w->view_max.x - (double)w->view_min.x);
    double half_h = 0.5 * ((double)w->view_max.y - (double)w->view_min.y);
    if (half_w < 1e-9) half_w = 1e-9;
    if (half_h < 1e-9) half_h = 1e-9;

    double target_aspect = (double)width / (double)height;
    double view_aspect   = half_w / half_h;
    if (view_aspect < target_aspect)
        half_w = half_h * target_aspect;
    else
        half_h = half_w / target_aspect;

    CameraBounds b;
    b.left   = cx - half_w;
    b.right  = cx + half_w;
    b.bottom = cy - half_h;
    b.top    = cy + half_h;
    return b;
}

Vec2 camera_screen_to_world(const World *w, double screen_x, double screen_y,
                            int win_w, int win_h)
{
    if (win_w < 1) win_w = 1;
    if (win_h < 1) win_h = 1;

    CameraBounds b = camera_compute_bounds(w, win_w, win_h);
    double u  = screen_x / (double)win_w;
    double v  = 1.0 - (screen_y / (double)win_h);
    double wx = b.left   + u * (b.right - b.left);
    double wy = b.bottom + v * (b.top   - b.bottom);
    return vec2_make((phys_real)wx, (phys_real)wy);
}
