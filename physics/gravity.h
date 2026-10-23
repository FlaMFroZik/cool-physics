#ifndef PHYSICS_GRAVITY_H
#define PHYSICS_GRAVITY_H

#include "physics/physics.h"

/* Populate `force` on every body in `w` according to `w->gravity_mode`. */
void phys_apply_gravity(World *w);

#endif /* PHYSICS_GRAVITY_H */
