/*
 * rng.c — small deterministic xorshift64* pseudo-random generator.
 */
#include "util/rng.h"

uint64_t rng_next(Rng *rng)
{
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return x * 2685821657736338717ULL;
}

void rng_seed(Rng *rng, uint64_t seed)
{
    rng->state = seed ? seed : 0x9E3779B97F4A7C15ULL;
    (void)rng_next(rng); /* warm-up draws so any seed is usable */
    (void)rng_next(rng);
}

double rng_uniform(Rng *rng)
{
    return (double)(rng_next(rng) >> 11) * (1.0 / 9007199254740992.0);
}

double rng_range(Rng *rng, double lo, double hi)
{
    return lo + (hi - lo) * rng_uniform(rng);
}
