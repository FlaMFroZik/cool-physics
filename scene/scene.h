#ifndef SCENE_H
#define SCENE_H

#include <stdbool.h>

#include "app.h"

/* Populate (or reset) the initial scene for the active gravity mode. */
void  scene_spawn(App *app);

/* Create a new ball of the scene's uniform radius at `pos` with `vel`. */
Body *scene_spawn_ball(App *app, Vec2 pos, Vec2 vel);

/* Create a new ball at the current cursor position (or view centre). */
Body *scene_spawn_ball_at_cursor(App *app);

/* Remove the ball under `world_pos`, returning true if one was removed. */
bool  scene_remove_ball_at(App *app, Vec2 world_pos);

/* Remove the most recently added ball, returning true if one was removed. */
bool  scene_remove_last_ball(App *app);

#endif /* SCENE_H */
