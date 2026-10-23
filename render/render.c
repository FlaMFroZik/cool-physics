/*
 * render.c — turns the long-double World into OpenGL draw calls and
 * fits the world's view rectangle into the window (letterboxed).
 */
#include "render/render.h"

#include "render/camera.h"
#include "render/gl.h"

static int viewport_w = 1;
static int viewport_h = 1;

static const float CLEAR_COLOR[3]    = {0.043f, 0.051f, 0.075f};
static const float GRID_COLOR[3]     = {0.11f,  0.14f,  0.20f};
static const float BOUNDS_COLOR[3]   = {0.32f, 0.38f, 0.47f};
static const float VELOCITY_COLOR[3] = {0.80f, 0.87f, 0.95f};
static const float HOVER_COLOR[3]    = {0.90f, 0.95f, 1.00f};
static const float DRAG_COLOR[3]     = {1.00f, 0.92f, 0.45f};

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

static void draw_spatial_grid(const Grid *g)
{
    if (g->cols <= 0 || g->rows <= 0 || g->cell_size <= (phys_real)0)
        return;

    double x0   = (double)g->origin.x;
    double y0   = (double)g->origin.y;
    double cs   = (double)g->cell_size;
    double xmax = x0 + (double)g->cols * cs;
    double ymax = y0 + (double)g->rows * cs;

    for (int c = 0; c <= g->cols; c++) {
        double x = x0 + (double)c * cs;
        rgl_draw_line(x, y0, x, ymax, GRID_COLOR);
    }
    for (int r = 0; r <= g->rows; r++) {
        double y = y0 + (double)r * cs;
        rgl_draw_line(x0, y, xmax, y, GRID_COLOR);
    }
}

void render_frame(const World *w)
{
    CameraBounds cam = camera_compute_bounds(w, viewport_w, viewport_h);

    rgl_begin_frame(CLEAR_COLOR);
    rgl_camera(cam.left, cam.right, cam.bottom, cam.top);

    if (w->show_grid)
        draw_spatial_grid(&w->grid);

    if (w->walls_enabled)
        rgl_draw_rect_outline((double)w->world_min.x, (double)w->world_min.y,
                              (double)w->world_max.x, (double)w->world_max.y,
                              BOUNDS_COLOR, 2.0f);

    for (size_t i = 0; i < w->count; i++) {
        const Body *b = &w->bodies[i];
        double x = (double)b->pos.x;
        double y = (double)b->pos.y;
        double r = (double)b->radius;
        rgl_draw_circle(x, y, r, b->color, 32);

        if ((ptrdiff_t)i == w->dragged_index)
            rgl_draw_circle_outline(x, y, r * 1.15, DRAG_COLOR, 32, 2.2f);
        else if ((ptrdiff_t)i == w->hovered_index)
            rgl_draw_circle_outline(x, y, r * 1.12, HOVER_COLOR, 32, 1.6f);

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
