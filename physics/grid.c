/*
 * grid.c — uniform spatial grid broad-phase (cell size = circle diameter).
 */
#include "physics/grid.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

void grid_init(Grid *g)
{
    memset(g, 0, sizeof(*g));
}

void grid_free(Grid *g)
{
    free(g->cells);
    free(g->next);
    free(g->body_cell);
    memset(g, 0, sizeof(*g));
}

static bool ensure_cells_capacity(Grid *g, size_t needed)
{
    if (needed > g->cells_cap) {
        size_t new_cap = g->cells_cap ? g->cells_cap : 1024;
        while (new_cap < needed)
            new_cap *= 2;
        ptrdiff_t *mem = realloc(g->cells, new_cap * sizeof(*mem));
        if (!mem)
            return false;
        g->cells     = mem;
        g->cells_cap = new_cap;
    }
    /* Initialize any newly exposed cells to -1 once. */
    if (needed > g->cells_init) {
        for (size_t c = g->cells_init; c < needed; c++)
            g->cells[c] = -1;
        g->cells_init = needed;
    }
    return true;
}

static bool ensure_body_capacity(Grid *g, size_t needed)
{
    if (needed <= g->body_cap)
        return true;
    size_t new_cap = g->body_cap ? g->body_cap : 256;
    while (new_cap < needed)
        new_cap *= 2;

    ptrdiff_t *new_next = realloc(g->next, new_cap * sizeof(*new_next));
    if (!new_next)
        return false;
    g->next = new_next;

    int *new_cell = realloc(g->body_cell, new_cap * sizeof(*new_cell));
    if (!new_cell)
        return false;
    g->body_cell = new_cell;

    g->body_cap = new_cap;
    return true;
}

void grid_cell_coords(const Grid *g, Vec2 pos, int *out_cx, int *out_cy)
{
    phys_real fx = (pos.x - g->origin.x) * g->inv_cell_size;
    phys_real fy = (pos.y - g->origin.y) * g->inv_cell_size;

    int cx = (int)floorl(fx);
    int cy = (int)floorl(fy);

    if (cx < 0)        cx = 0;
    if (cx >= g->cols) cx = g->cols - 1;
    if (cy < 0)        cy = 0;
    if (cy >= g->rows) cy = g->rows - 1;

    *out_cx = cx;
    *out_cy = cy;
}

bool grid_build(Grid *g, const Body *bodies, size_t count,
                Vec2 world_min, Vec2 world_max)
{
    /* O(N) reset of only the cells occupied during the previous build. */
    for (size_t i = 0; i < g->prev_count; i++) {
        int c = g->body_cell[i];
        if (c >= 0 && (size_t)c < g->cells_init)
            g->cells[c] = -1;
    }
    g->prev_count = 0;

    /* Determine circle size (diameter = 2 * max_radius). */
    phys_real max_r = (phys_real)0;
    for (size_t i = 0; i < count; i++) {
        if (bodies[i].radius > max_r)
            max_r = bodies[i].radius;
    }
    phys_real cell_size = (max_r > (phys_real)0)
                              ? ((phys_real)2 * max_r) : (phys_real)5;

    /* Start from world bounds and expand if bodies lie within a reasonable
     * margin outside (e.g. open space in Newtonian mode). */
    Vec2 min_p = world_min;
    Vec2 max_p = world_max;
    const phys_real max_span = cell_size * (phys_real)GRID_MAX_DIM;

    for (size_t i = 0; i < count; i++) {
        Vec2 p = bodies[i].pos;
        if (p.x < min_p.x && (max_p.x - p.x) <= max_span)
            min_p.x = p.x;
        if (p.x > max_p.x && (p.x - min_p.x) <= max_span)
            max_p.x = p.x;
        if (p.y < min_p.y && (max_p.y - p.y) <= max_span)
            min_p.y = p.y;
        if (p.y > max_p.y && (p.y - min_p.y) <= max_span)
            max_p.y = p.y;
    }

    phys_real width  = max_p.x - min_p.x;
    phys_real height = max_p.y - min_p.y;
    if (width  < cell_size) width  = cell_size;
    if (height < cell_size) height = cell_size;

    int cols = (int)ceill(width  / cell_size);
    int rows = (int)ceill(height / cell_size);
    if (cols < 1)            cols = 1;
    if (rows < 1)            rows = 1;
    if (cols > GRID_MAX_DIM) cols = GRID_MAX_DIM;
    if (rows > GRID_MAX_DIM) rows = GRID_MAX_DIM;

    size_t total_cells = (size_t)cols * (size_t)rows;
    if (!ensure_cells_capacity(g, total_cells))
        return false;
    if (count > 0 && !ensure_body_capacity(g, count))
        return false;

    g->origin        = min_p;
    g->cell_size     = cell_size;
    g->inv_cell_size = (phys_real)1 / cell_size;
    g->cols          = cols;
    g->rows          = rows;

    for (size_t i = 0; i < count; i++) {
        int cx = 0, cy = 0;
        grid_cell_coords(g, bodies[i].pos, &cx, &cy);
        int cell        = cy * cols + cx;
        g->body_cell[i] = cell;
        g->next[i]      = g->cells[cell];
        g->cells[cell]  = (ptrdiff_t)i;
    }
    g->prev_count = count;

    return true;
}
