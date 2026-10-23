/*
 * body.c — single-body helpers (initialization and hit-testing).
 */
#include "physics/body.h"

#include <string.h>

void body_init_ball(Body *b, Vec2 pos, Vec2 vel, phys_real radius,
                    const float color[3])
{
    memset(b, 0, sizeof(*b));
    b->pos    = pos;
    b->vel    = vel;
    b->radius = radius;
    b->mass   = PHYS_PI * radius * radius; /* uniform density = 1 */
    if (color) {
        b->color[0] = color[0];
        b->color[1] = color[1];
        b->color[2] = color[2];
    }
}

bool body_contains_point(const Body *b, Vec2 p, phys_real pad)
{
    phys_real r = b->radius + pad;
    if (r < (phys_real)0)
        r = (phys_real)0;
    return vec2_dist_sq(b->pos, p) <= r * r;
}
