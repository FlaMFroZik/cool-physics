#ifndef PHYSICS_VEC2_H
#define PHYSICS_VEC2_H

#include <math.h>

/*
 * Every simulation quantity is stored and computed in `double`
 * (64-bit IEEE-754 floating point, hardware SSE/AVX accelerated).
 */
typedef double phys_real;

#define PHYS_PI ((phys_real)3.14159265358979323846)

/* 2D vector in physics precision. */
typedef struct Vec2 {
    phys_real x, y;
} Vec2;

static inline Vec2 vec2_make(phys_real x, phys_real y)
{
    Vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

static inline Vec2 vec2_add(Vec2 a, Vec2 b)
{
    return vec2_make(a.x + b.x, a.y + b.y);
}

static inline Vec2 vec2_sub(Vec2 a, Vec2 b)
{
    return vec2_make(a.x - b.x, a.y - b.y);
}

static inline Vec2 vec2_scale(Vec2 a, phys_real s)
{
    return vec2_make(a.x * s, a.y * s);
}

static inline phys_real vec2_dot(Vec2 a, Vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

static inline phys_real vec2_len_sq(Vec2 a)
{
    return vec2_dot(a, a);
}

static inline phys_real vec2_len(Vec2 a)
{
    return sqrt(vec2_len_sq(a));
}

static inline phys_real vec2_dist_sq(Vec2 a, Vec2 b)
{
    return vec2_len_sq(vec2_sub(a, b));
}

static inline Vec2 vec2_norm(Vec2 a)
{
    phys_real len = vec2_len(a);
    return (len > (phys_real)0) ? vec2_scale(a, (phys_real)1 / len)
                                : vec2_make(0, 0);
}

#endif /* PHYSICS_VEC2_H */
