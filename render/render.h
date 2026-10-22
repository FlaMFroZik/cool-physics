#ifndef RENDER_H
#define RENDER_H

#include "physics/physics.h"

/* One-time OpenGL state setup.  Returns 0 on success. */
int  render_init(void);

/* Call on window framebuffer resize. */
void render_resize(int framebuffer_w, int framebuffer_h);

/* Draw one frame of the world (long double -> double conversion
 * happens right here, at the rendering boundary). */
void render_frame(const World *world);

void render_shutdown(void);

#endif /* RENDER_H */
