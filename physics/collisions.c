/*
 * collisions.c — the simplest possible collision solver: an O(N^2)
 * "every body against every other" sweep.  Fine for a few hundred
 * bodies and a perfect baseline to later compare spatial hashing or
 * sweep-and-prune against.
 */
#include "physics/collisions.h"

#include <math.h>

static phys_real body_inv_mass(const Body *b)
{
    if (b->dragged || b->mass <= (phys_real)0)
        return (phys_real)0;
    return (phys_real)1 / b->mass;
}

void phys_resolve_collisions(World *w)
{
    static const phys_real SLOP    = (phys_real)0.01; /* allowed penetration   */
    static const phys_real PERCENT = (phys_real)0.80; /* correction strength   */

    size_t contacts = 0;

    for (size_t i = 0; i < w->count; i++) {
        Body *a = &w->bodies[i];
        phys_real inv_ma = body_inv_mass(a);

        for (size_t j = i + 1; j < w->count; j++) {
            Body *b = &w->bodies[j];
            phys_real inv_mb  = body_inv_mass(b);
            phys_real inv_sum = inv_ma + inv_mb;
            if (inv_sum <= (phys_real)0)
                continue; /* two static/dragged bodies */

            Vec2 d = vec2_sub(b->pos, a->pos);
            phys_real radius_sum = a->radius + b->radius;
            phys_real dist_sq    = vec2_len_sq(d);
            if (dist_sq >= radius_sum * radius_sum)
                continue; /* not touching */

            /* degenerate overlap: pick any stable direction */
            if (dist_sq <= (phys_real)1e-12) {
                d       = vec2_make((phys_real)1e-6, (phys_real)1e-6);
                dist_sq = vec2_len_sq(d);
            }

            phys_real dist = sqrtl(dist_sq);
            Vec2 n = vec2_scale(d, (phys_real)1 / dist);

            /* --- positional correction: push the pair apart -------- */
            phys_real overlap = radius_sum - dist;
            if (overlap > SLOP) {
                Vec2 corr = vec2_scale(n, PERCENT * (overlap - SLOP) / inv_sum);
                a->pos = vec2_sub(a->pos, vec2_scale(corr, inv_ma));
                b->pos = vec2_add(b->pos, vec2_scale(corr, inv_mb));
            }

            /* --- impulse only if the bodies approach each other ---- */
            Vec2 rel_vel    = vec2_sub(b->vel, a->vel);
            phys_real vel_n = vec2_dot(rel_vel, n);
            if (vel_n < (phys_real)0) {
                phys_real impulse = -((phys_real)1 + w->restitution) * vel_n
                                        / inv_sum;
                a->vel = vec2_sub(a->vel, vec2_scale(n, impulse * inv_ma));
                b->vel = vec2_add(b->vel, vec2_scale(n, impulse * inv_mb));
                contacts++;
            }
        }
    }

    w->contacts_last_step = contacts;
    w->contacts_total    += contacts;
}
