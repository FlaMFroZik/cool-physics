#ifndef PHYSICS_H
#define PHYSICS_H

#include <stdbool.h>
#include <stddef.h>

#include "physics/body.h"
#include "physics/grid.h"
#include "physics/vec2.h"

typedef enum GravityMode {
    GRAV_CONSTANT = 0,  /* uniform gravitational field (balls in a box) */
    GRAV_NEWTON   = 1   /* pairwise Newtonian attraction (N-body)       */
} GravityMode;

typedef struct World {
    Body  *bodies;
    size_t count;
    size_t capacity;

    /* uniform spatial grid (cell size = ball diameter 2*R) */
    Grid grid;

    GravityMode gravity_mode;

    /* gravity parameters */
    phys_real G;             /* Newtonian constant (sim units)            */
    phys_real softening_sq;  /* Plummer softening, tames the r -> 0 blow-up */
    Vec2 constant_gravity;   /* uniform acceleration (GRAV_CONSTANT)      */

    /* collision / material parameters */
    phys_real restitution;       /* body-body bounce (1 = perfectly elastic) */
    phys_real wall_restitution;  /* body-wall bounce                         */

    /* bounds */
    bool walls_enabled;
    Vec2 world_min, world_max;   /* simulation box (walls)   */
    Vec2 view_min,  view_max;    /* suggested camera extents */

    /* stats & toggles */
    phys_real time;              /* simulated time                    */
    size_t contacts_last_step;   /* collisions resolved in last step  */
    size_t contacts_total;
    size_t pair_checks_last_step;/* candidate pairs checked in grid   */
    bool show_velocities;        /* render hint (toggled with V)      */
    bool show_grid;              /* render spatial grid (toggled: B)  */

    /* interactive selection indices (-1 when none) */
    ptrdiff_t hovered_index;
    ptrdiff_t dragged_index;
} World;

void phys_world_init(World *w, GravityMode mode);
void phys_world_free(World *w);
void phys_world_clear(World *w);

Body     *phys_world_add(World *w, const Body *b);
bool      phys_world_remove_at(World *w, size_t index);
ptrdiff_t phys_world_pick(const World *w, Vec2 point, phys_real extra_radius);
Vec2      phys_world_clamp_pos(const World *w, Vec2 pos, phys_real radius);

/* Advance the simulation by a fixed timestep dt (semi-implicit Euler:
 * forces -> velocities -> positions -> walls -> uniform-grid collisions). */
void phys_world_step(World *w, phys_real dt);

const char *phys_gravity_mode_name(GravityMode mode);

#endif /* PHYSICS_H */
