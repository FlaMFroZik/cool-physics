/*
 * gl.h — tiny immediate-mode OpenGL helper layer (fixed pipeline,
 * works on OpenGL 2.1+ / compatibility profiles, no GLEW/GLAD needed).
 */
#ifndef RENDER_GL_H
#define RENDER_GL_H

#include <GL/gl.h>

void rgl_viewport(int width, int height);
void rgl_begin_frame(const float clear_rgb[3]);
void rgl_camera(double left, double right, double bottom, double top);

void rgl_draw_circle(double cx, double cy, double radius,
                     const float rgb[3], int segments);
void rgl_draw_circle_outline(double cx, double cy, double radius,
                             const float rgb[3], int segments,
                             float line_width);
void rgl_draw_line(double x1, double y1, double x2, double y2,
                   const float rgb[3]);
void rgl_draw_rect_outline(double x1, double y1, double x2, double y2,
                           const float rgb[3], float line_width);

#endif /* RENDER_GL_H */
