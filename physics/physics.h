#ifndef PHYSICS_H
#define PHYSICS_H

#include <math.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * cool-physics — physics core.
 *
 * Every simulation quantity is stored and computed in `long double`
 * (80-bit extended precision on x86-64 Linux) so that the integrator
 * and the collision solver keep as much precision as the C compiler
 * can give us.  Values are converted down to float/double only at the
 * rendering boundary.
 */

typedef long double phys_real;

/* 2D vector in physics precision. */
typedef struct Vec2 {
    phys_real x, y;
} Vec2;

static inline Vec2 vec2_make(phys_real x, phys_real y) { Vec2 v; v.x = x; v.y = y; return v; }
static inline Vec2 vec2_add(Vec2 a, Vec2 b)  { return vec2_make(a.x + b.x, a.y + b.y); }
static inline Vec2 vec2_sub(Vec2 a, Vec2 b)  { return vec2_make(a.x - b.x, a.y - b.y); }
static inline Vec2 vec2_scale(Vec2 a, phys_real s) { return vec2_make(a.x * s, a.y * s); }
static inline phys_real vec2_dot(Vec2 a, Vec2 b)   { return a.x * b.x + a.y * b.y; }
static inline phys_real vec2_len_sq(Vec2 a)        { return vec2_dot(a, a); }
static inline phys_real vec2_len(Vec2 a)           { return sqrtl(vec2_len_sq(a)); }
static inline Vec2 vec2_norm(Vec2 a)
{
    phys_real len = vec2_len(a);
    return (len > (phys_real)0) ? vec2_scale(a, (phys_real)1 / len) : vec2_make(0, 0);
}

typedef enum GravityMode {
    GRAV_CONSTANT = 0,  /* uniform gravitational field (balls in a box) */
    GRAV_NEWTON   = 1   /* pairwise Newtonian attraction (N-body)       */
} GravityMode;

typedef struct Body {
    Vec2 pos;
    Vec2 vel;
    Vec2 force;        /* force accumulator for the current step      */
    phys_real mass;    /* <= 0 => static body (never integrated)      */
    phys_real radius;
    float color[3];    /* render-side, regular float precision        */
} Body;

typedef struct World {
    Body  *bodies;
    size_t count;
    size_t capacity;

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
    bool show_velocities;        /* render hint (toggled with V)      */
} World;

void phys_world_init(World *w, GravityMode mode);
void phys_world_free(World *w);
void phys_world_clear(World *w);

Body *phys_world_add(World *w, const Body *b);

/* Advance the simulation by a fixed timestep dt (semi-implicit Euler:
 * forces -> velocities -> positions -> walls -> O(N^2) collisions). */
void phys_world_step(World *w, phys_real dt);

const char *phys_gravity_mode_name(GravityMode mode);

#endif /* PHYSICS_H */
