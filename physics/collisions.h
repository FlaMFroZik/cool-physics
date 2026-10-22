#ifndef PHYSICS_COLLISIONS_H
#define PHYSICS_COLLISIONS_H

#include "physics.h"

/* Brute-force O(N^2) narrow phase: every body is tested against every
 * other body.  Overlaps are resolved with positional correction plus an
 * impulse along the contact normal (restitution taken from the world). */
void phys_resolve_collisions(World *w);

#endif /* PHYSICS_COLLISIONS_H */
