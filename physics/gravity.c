/*
 * gravity.c — constant uniform gravity and O(N^2) pairwise Newtonian
 * attraction with Plummer softening.
 */
#include "physics/gravity.h"

static void apply_constant_gravity(World *w)
{
    for (size_t i = 0; i < w->count; i++) {
        Body *b = &w->bodies[i];
        if (b->mass <= (phys_real)0 || b->dragged)
            continue;
        b->force = vec2_add(b->force, vec2_scale(w->constant_gravity, b->mass));
    }
}

/* O(N^2) pairwise Newtonian attraction with Plummer softening:
 * F = G * ma * mb * d / (|d|^2 + eps^2)^{3/2}. */
static void apply_newton_gravity(World *w)
{
    const size_t    n    = w->count;
    Body *const     bods = w->bodies;
    const phys_real G    = w->G;
    const phys_real eps2 = w->softening_sq;

    for (size_t i = 0; i < n; i++) {
        Body *a              = &bods[i];
        const phys_real ax   = a->pos.x;
        const phys_real ay   = a->pos.y;
        const phys_real g_ma = G * a->mass;
        const bool a_dragged = a->dragged;
        phys_real fax        = (phys_real)0;
        phys_real fay        = (phys_real)0;

        for (size_t j = i + 1; j < n; j++) {
            Body *b         = &bods[j];
            phys_real dx    = b->pos.x - ax;
            phys_real dy    = b->pos.y - ay;
            phys_real r2    = dx * dx + dy * dy + eps2;
            phys_real inv_r = (phys_real)1 / sqrtl(r2);
            phys_real s     = g_ma * b->mass * inv_r * inv_r * inv_r;
            phys_real fx    = dx * s;
            phys_real fy    = dy * s;

            fax += fx;
            fay += fy;
            if (!b->dragged) {
                b->force.x -= fx;
                b->force.y -= fy;
            }
        }

        if (!a_dragged) {
            a->force.x += fax;
            a->force.y += fay;
        }
    }
}

void phys_apply_gravity(World *w)
{
    if (w->gravity_mode == GRAV_NEWTON)
        apply_newton_gravity(w);
    else
        apply_constant_gravity(w);
}
