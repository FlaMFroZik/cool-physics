#ifndef PHYSICS_GRID_H
#define PHYSICS_GRID_H

#include <stdbool.h>
#include <stddef.h>

#include "physics/body.h"
#include "physics/vec2.h"

/*
 * Uniform spatial grid for broad-phase collision detection.
 *
 * The simulation field is divided into square cells of side length equal
 * to the circle diameter (`cell_size = 2 * radius`).  Each body is placed
 * into the cell containing its centre, so colliding bodies (distance < 2R)
 * can only reside in the same cell or the 8 immediately adjacent cells.
 */

#define GRID_MAX_DIM 512

typedef struct Grid {
    Vec2      origin;         /* bottom-left corner of the grid          */
    phys_real cell_size;      /* cell width/height (= ball diameter 2*R) */
    phys_real inv_cell_size;  /* 1.0 / cell_size                         */
    int       cols;
    int       rows;

    ptrdiff_t *cells;         /* head body index per cell (-1 if empty)  */
    size_t     cells_cap;
    size_t     cells_init;    /* number of cells already initialized -1  */

    ptrdiff_t *next;          /* next body index in the same cell (-1)   */
    int       *body_cell;     /* cell index (cy * cols + cx) per body    */
    size_t     body_cap;
    size_t     prev_count;    /* number of bodies from previous build    */
} Grid;

void grid_init(Grid *g);
void grid_free(Grid *g);

/* Rebuild the grid for `count` bodies inside `[world_min, world_max]`.
 * Returns true on success, false on allocation failure. */
bool grid_build(Grid *g, const Body *bodies, size_t count,
                Vec2 world_min, Vec2 world_max);

/* Map a world position `pos` to clamped grid cell coordinates `(cx, cy)`. */
void grid_cell_coords(const Grid *g, Vec2 pos, int *out_cx, int *out_cy);

#endif /* PHYSICS_GRID_H */
