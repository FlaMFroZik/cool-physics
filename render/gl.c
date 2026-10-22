#include "gl.h"

#include <math.h>

static const double RGL_PI = 3.14159265358979323846;

void rgl_viewport(int width, int height)
{
    glViewport(0, 0, width, height);
}

void rgl_begin_frame(const float clear_rgb[3])
{
    glClearColor(clear_rgb[0], clear_rgb[1], clear_rgb[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void rgl_camera(double left, double right, double bottom, double top)
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(left, right, bottom, top, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void rgl_draw_circle(double cx, double cy, double radius,
                     const float rgb[3], int segments)
{
    if (segments < 8)
        segments = 8;
    glColor3f(rgb[0], rgb[1], rgb[2]);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(cx, cy);
    for (int i = 0; i <= segments; i++) {
        double a = 2.0 * RGL_PI * (double)i / (double)segments;
        glVertex2d(cx + radius * cos(a), cy + radius * sin(a));
    }
    glEnd();
}

void rgl_draw_line(double x1, double y1, double x2, double y2,
                   const float rgb[3])
{
    glColor3f(rgb[0], rgb[1], rgb[2]);
    glLineWidth(1.5f);
    glBegin(GL_LINES);
    glVertex2d(x1, y1);
    glVertex2d(x2, y2);
    glEnd();
}

void rgl_draw_rect_outline(double x1, double y1, double x2, double y2,
                           const float rgb[3], float line_width)
{
    glColor3f(rgb[0], rgb[1], rgb[2]);
    glLineWidth(line_width);
    glBegin(GL_LINE_LOOP);
    glVertex2d(x1, y1);
    glVertex2d(x2, y1);
    glVertex2d(x2, y2);
    glVertex2d(x1, y2);
    glEnd();
}
