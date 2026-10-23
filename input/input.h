#ifndef INPUT_H
#define INPUT_H

#include "app.h"

/* Register GLFW error callback before glfwInit(). */
void input_register_error_callback(void);

/* Attach keyboard, mouse, and resize callbacks to `app->window`. */
void input_init(App *app);

/* Update the currently dragged ball's position and velocity each frame. */
void input_update_drag(App *app, double frame_dt);

#endif /* INPUT_H */
