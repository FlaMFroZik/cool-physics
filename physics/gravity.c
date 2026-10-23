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
    for (size_t i = 0; i < w->count; i++) {
        Body *a = &w->bodies[i];
        for (size_t j = i + 1; j < w->count; j++) {
            Body *b = &w->bodies[j];
            Vec2 d  = vec2_sub(b->pos, a->pos);
            phys_real r2    = vec2_len_sq(d) + w->softening_sq;
            phys_real inv_r = (phys_real)1 / sqrtl(r2);
            phys_real s     = w->G * a->mass * b->mass * inv_r * inv_r * inv_r;
            Vec2 f          = vec2_scale(d, s);
            if (!a->dragged)
                a->force = vec2_add(a->force, f);
            if (!b->dragged)
                b->force = vec2_sub(b->force, f);
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
