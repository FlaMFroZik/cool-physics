#ifndef PHYSICS_COLLISIONS_H
#define PHYSICS_COLLISIONS_H

#include "physics/physics.h"

/* Uniform-grid broad phase (cell size = circle diameter 2*R) + narrow-phase
 * impulse solver: for each circle, only circles in the same and 8 adjacent
 * grid cells are tested for collision. */
void phys_resolve_collisions(World *w);

#endif /* PHYSICS_COLLISIONS_H */
