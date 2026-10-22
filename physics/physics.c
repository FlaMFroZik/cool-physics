#include "physics.h"
#include "collisions.h"

#include <stdlib.h>
#include <string.h>

void phys_world_init(World *w, GravityMode mode)
{
    memset(w, 0, sizeof(*w));
    w->gravity_mode      = mode;
    w->restitution       = (phys_real)0.85;
    w->wall_restitution  = (phys_real)0.80;

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
        /* Open space.  G and the softening length are in sim units;
         * main.c spawns a sun + circular orbits tuned around them. */
        w->G             = (phys_real)1.0;
        w->softening_sq  = (phys_real)25.0;  /* eps = 5 length units */
        w->restitution   = (phys_real)0.70;
        w->walls_enabled = false;
        w->world_min     = vec2_make((phys_real)-1e6, (phys_real)-1e6);
        w->world_max     = vec2_make((phys_real) 1e6, (phys_real) 1e6);
        w->view_min      = vec2_make((phys_real)-200, (phys_real)-112);
        w->view_max      = vec2_make((phys_real) 200, (phys_real) 112);
        break;
    }
}

void phys_world_free(World *w)
{
    free(w->bodies);
    w->bodies   = NULL;
    w->count    = 0;
    w->capacity = 0;
}

void phys_world_clear(World *w)
{
    w->count              = 0;
    w->time               = 0;
    w->contacts_last_step = 0;
    w->contacts_total     = 0;
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

const char *phys_gravity_mode_name(GravityMode mode)
{
    switch (mode) {
    case GRAV_CONSTANT: return "constant gravity";
    case GRAV_NEWTON:   return "Newtonian gravity";
    }
    return "unknown";
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
            a->force = vec2_add(a->force, f);
            b->force = vec2_sub(b->force, f);
        }
    }
}

static void integrate(World *w, phys_real dt)
{
    for (size_t i = 0; i < w->count; i++) {
        Body *b = &w->bodies[i];
        if (b->mass <= (phys_real)0)
            continue; /* static body */
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

    if (w->gravity_mode == GRAV_NEWTON)
        apply_newton_gravity(w);
    else
        for (size_t i = 0; i < w->count; i++)
            w->bodies[i].force = vec2_add(w->bodies[i].force,
                vec2_scale(w->constant_gravity, w->bodies[i].mass));

    integrate(w, dt);

    if (w->walls_enabled)
        apply_walls(w, dt);

    phys_resolve_collisions(w);

    w->time += dt;
}
