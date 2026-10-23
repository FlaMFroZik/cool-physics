#ifndef PHYSICS_BODY_H
#define PHYSICS_BODY_H

#include <stdbool.h>

#include "physics/vec2.h"

typedef struct Body {
    Vec2      pos;
    Vec2      vel;
    Vec2      force;     /* force accumulator for the current step        */
    phys_real mass;      /* <= 0 => static body (never integrated)        */
    phys_real radius;
    float     color[3];  /* render-side, regular float precision          */
    bool      dragged;   /* true while held/moved by the user's cursor    */
} Body;

/* Initialize a uniform-density ball (mass = pi * radius^2). */
void body_init_ball(Body *b, Vec2 pos, Vec2 vel, phys_real radius,
                    const float color[3]);

/* Return true if point `p` lies inside the body's disk (plus `pad`). */
bool body_contains_point(const Body *b, Vec2 p, phys_real pad);

#endif /* PHYSICS_BODY_H */
