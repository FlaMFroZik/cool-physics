/*
 * physics.c — World lifecycle, body picking/removal, integration, walls.
 */
#include "physics/physics.h"
#include "physics/collisions.h"
#include "physics/gravity.h"

#include <stdlib.h>
#include <string.h>

void phys_world_init(World *w, GravityMode mode)
{
    memset(w, 0, sizeof(*w));
    w->gravity_mode     = mode;
    w->restitution      = (phys_real)0.85;
    w->wall_restitution = (phys_real)0.80;
    w->hovered_index    = -1;
    w->dragged_index    = -1;

    switch (mode) {
    case GRAV_CONSTANT:
        /* A 160 x 90 "metre" box with Earth-like gravity. */
        w->constant_gravity = vec2_make((phys_real)0, (phys_real)-9.81);
        w->walls_enabled    = true;
        w->world_min        = vec2_make((phys_real)0,   (phys_real)0);
        w->world_max        = vec2_make((phys_real)160, (phys_real)90);
        w->view_min         = w->world_min;
        w->view_max         = w->world_max;
        w->G                = 0;
        w->softening_sq     = 0;
        break;

    case GRAV_NEWTON:
        /* Open space (with optional walls around the camera view if toggled). */
        w->G             = (phys_real)1.0;
        w->softening_sq  = (phys_real)25.0;  /* eps = 5 length units */
        w->restitution   = (phys_real)0.70;
        w->walls_enabled = false;
        w->world_min     = vec2_make((phys_real)-200, (phys_real)-112);
        w->world_max     = vec2_make((phys_real) 200, (phys_real) 112);
        w->view_min      = vec2_make((phys_real)-200, (phys_real)-112);
        w->view_max      = vec2_make((phys_real) 200, (phys_real) 112);
        break;
    }
}

void phys_world_free(World *w)
{
    free(w->bodies);
    w->bodies        = NULL;
    w->count         = 0;
    w->capacity      = 0;
    w->hovered_index = -1;
    w->dragged_index = -1;
}

void phys_world_clear(World *w)
{
    w->count              = 0;
    w->time               = 0;
    w->contacts_last_step = 0;
    w->contacts_total     = 0;
    w->hovered_index      = -1;
    w->dragged_index      = -1;
}

Body *phys_world_add(World *w, const Body *b)
{
    if (w->count == w->capacity) {
        size_t new_cap = w->capacity ? w->capacity * 2 : 256;
        Body  *mem     = realloc(w->bodies, new_cap * sizeof(*mem));
        if (!mem)
            return NULL;
        w->bodies   = mem;
        w->capacity = new_cap;
    }
    w->bodies[w->count] = *b;
    return &w->bodies[w->count++];
}

bool phys_world_remove_at(World *w, size_t index)
{
    if (index >= w->count)
        return false;

    if (index + 1 < w->count) {
        memmove(&w->bodies[index], &w->bodies[index + 1],
                (w->count - index - 1) * sizeof(Body));
    }
    w->count--;

    if (w->dragged_index == (ptrdiff_t)index)
        w->dragged_index = -1;
    else if (w->dragged_index > (ptrdiff_t)index)
        w->dragged_index--;

    if (w->hovered_index == (ptrdiff_t)index)
        w->hovered_index = -1;
    else if (w->hovered_index > (ptrdiff_t)index)
        w->hovered_index--;

    return true;
}

ptrdiff_t phys_world_pick(const World *w, Vec2 point, phys_real extra_radius)
{
    ptrdiff_t best_idx = -1;
    phys_real best_d2  = (phys_real)0;

    /* Iterate backwards so top-most (most recently added) bodies win ties. */
    for (size_t i = w->count; i > 0; i--) {
        size_t idx     = i - 1;
        const Body *b  = &w->bodies[idx];
        if (!body_contains_point(b, point, extra_radius))
            continue;
        phys_real d2 = vec2_dist_sq(b->pos, point);
        if (best_idx < 0 || d2 < best_d2) {
            best_idx = (ptrdiff_t)idx;
            best_d2  = d2;
        }
    }
    return best_idx;
}

Vec2 phys_world_clamp_pos(const World *w, Vec2 pos, phys_real radius)
{
    if (!w->walls_enabled)
        return pos;

    if (pos.x - radius < w->world_min.x)
        pos.x = w->world_min.x + radius;
    else if (pos.x + radius > w->world_max.x)
        pos.x = w->world_max.x - radius;

    if (pos.y - radius < w->world_min.y)
        pos.y = w->world_min.y + radius;
    else if (pos.y + radius > w->world_max.y)
        pos.y = w->world_max.y - radius;

    return pos;
}

const char *phys_gravity_mode_name(GravityMode mode)
{
    switch (mode) {
    case GRAV_CONSTANT: return "constant gravity";
    case GRAV_NEWTON:   return "Newtonian gravity";
    }
    return "unknown";
}

static void integrate(World *w, phys_real dt)
{
    for (size_t i = 0; i < w->count; i++) {
        Body *b = &w->bodies[i];
        if (b->mass <= (phys_real)0 || b->dragged)
            continue; /* static or user-held body */
        /* semi-implicit (symplectic) Euler */
        b->vel = vec2_add(b->vel, vec2_scale(b->force, dt / b->mass));
        b->pos = vec2_add(b->pos, vec2_scale(b->vel, dt));
    }
}

static void apply_walls(World *w, phys_real dt)
{
    const phys_real ground_friction = (phys_real)0.05;
    phys_real damp = ground_friction * dt;
    if (damp > (phys_real)1)
        damp = (phys_real)1;

    for (size_t i = 0; i < w->count; i++) {
        Body *b = &w->bodies[i];
        if (b->mass <= (phys_real)0)
            continue;
        if (b->dragged) {
            b->pos = phys_world_clamp_pos(w, b->pos, b->radius);
            continue;
        }

        if (b->pos.x - b->radius < w->world_min.x) {
            b->pos.x = w->world_min.x + b->radius;
            if (b->vel.x < (phys_real)0)
                b->vel.x = -b->vel.x * w->wall_restitution;
        } else if (b->pos.x + b->radius > w->world_max.x) {
            b->pos.x = w->world_max.x - b->radius;
            if (b->vel.x > (phys_real)0)
                b->vel.x = -b->vel.x * w->wall_restitution;
        }

        if (b->pos.y - b->radius < w->world_min.y) {
            b->pos.y = w->world_min.y + b->radius;
            if (b->vel.y < (phys_real)0)
                b->vel.y = -b->vel.y * w->wall_restitution;
            /* simple sliding friction while touching the floor */
            b->vel.x -= b->vel.x * damp;
        } else if (b->pos.y + b->radius > w->world_max.y) {
            b->pos.y = w->world_max.y - b->radius;
            if (b->vel.y > (phys_real)0)
                b->vel.y = -b->vel.y * w->wall_restitution;
        }
    }
}

void phys_world_step(World *w, phys_real dt)
{
    /* reset force accumulators */
    for (size_t i = 0; i < w->count; i++)
        w->bodies[i].force = vec2_make(0, 0);

    phys_apply_gravity(w);
    integrate(w, dt);

    if (w->walls_enabled)
        apply_walls(w, dt);

    phys_resolve_collisions(w);

    w->time += dt;
}
