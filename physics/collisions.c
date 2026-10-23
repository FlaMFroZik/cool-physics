/*
 * collisions.c — uniform-grid collision solver.
 *
 * The field is partitioned into a uniform grid with cell size equal to the
 * circle diameter (2 * R).  For every circle we only inspect circles in its
 * own cell and the 8 neighbouring cells (3x3 neighbourhood).
 */
#include "physics/collisions.h"

#include <math.h>

static phys_real body_inv_mass(const Body *b)
{
    if (b->dragged || b->mass <= (phys_real)0)
        return (phys_real)0;
    return (phys_real)1 / b->mass;
}

/* Narrow-phase test and impulse resolution for a single pair (a, b).
 * Returns true if an approaching contact was resolved. */
static bool resolve_pair(const World *w, Body *a, Body *b,
                         phys_real inv_ma, phys_real inv_mb)
{
    static const phys_real SLOP    = (phys_real)0.01; /* allowed penetration */
    static const phys_real PERCENT = (phys_real)0.80; /* correction strength */

    phys_real inv_sum = inv_ma + inv_mb;
    if (inv_sum <= (phys_real)0)
        return false; /* two static/dragged bodies */

    Vec2 d = vec2_sub(b->pos, a->pos);
    phys_real radius_sum = a->radius + b->radius;
    phys_real dist_sq    = vec2_len_sq(d);
    if (dist_sq >= radius_sum * radius_sum)
        return false; /* not touching */

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
        phys_real impulse = -((phys_real)1 + w->restitution) * vel_n / inv_sum;
        a->vel = vec2_sub(a->vel, vec2_scale(n, impulse * inv_ma));
        b->vel = vec2_add(b->vel, vec2_scale(n, impulse * inv_mb));
        return true;
    }
    return false;
}

void phys_resolve_collisions(World *w)
{
    size_t contacts    = 0;
    size_t pair_checks = 0;

    if (w->count < 2) {
        w->contacts_last_step    = 0;
        w->pair_checks_last_step = 0;
        return;
    }

    if (grid_build(&w->grid, w->bodies, w->count, w->world_min, w->world_max)) {
        const Grid *g = &w->grid;
        const int cols = g->cols;
        const int rows = g->rows;

        for (size_t i = 0; i < w->count; i++) {
            Body *a          = &w->bodies[i];
            phys_real inv_ma = body_inv_mass(a);

            int cell = g->body_cell[i];
            int cx   = cell % cols;
            int cy   = cell / cols;

            int y0 = (cy > 0)        ? (cy - 1) : 0;
            int y1 = (cy + 1 < rows) ? (cy + 1) : (rows - 1);
            int x0 = (cx > 0)        ? (cx - 1) : 0;
            int x1 = (cx + 1 < cols) ? (cx + 1) : (cols - 1);

            /* Check only circles in the 3x3 neighbourhood around (cx, cy). */
            for (int ny = y0; ny <= y1; ny++) {
                int row_offset = ny * cols;
                for (int nx = x0; nx <= x1; nx++) {
                    for (ptrdiff_t j = g->cells[row_offset + nx];
                         j >= 0;
                         j = g->next[j]) {
                        if ((size_t)j <= i)
                            continue;
                        Body *b = &w->bodies[j];
                        pair_checks++;
                        if (resolve_pair(w, a, b, inv_ma, body_inv_mass(b)))
                            contacts++;
                    }
                }
            }
        }
    } else {
        /* Fallback O(N^2) sweep if grid allocation ever fails. */
        for (size_t i = 0; i < w->count; i++) {
            Body *a          = &w->bodies[i];
            phys_real inv_ma = body_inv_mass(a);
            for (size_t j = i + 1; j < w->count; j++) {
                Body *b = &w->bodies[j];
                pair_checks++;
                if (resolve_pair(w, a, b, inv_ma, body_inv_mass(b)))
                    contacts++;
            }
        }
    }

    w->contacts_last_step    = contacts;
    w->contacts_total       += contacts;
    w->pair_checks_last_step = pair_checks;
}
