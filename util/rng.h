#ifndef UTIL_RNG_H
#define UTIL_RNG_H

#include <stdint.h>

typedef struct Rng {
    uint64_t state;
} Rng;

void     rng_seed(Rng *rng, uint64_t seed);
uint64_t rng_next(Rng *rng);
double   rng_uniform(Rng *rng);
double   rng_range(Rng *rng, double lo, double hi);

#endif /* UTIL_RNG_H */
