#pragma once

#include "lajolla.h"

// Lajolla uses a random number generator called PCG https://www.pcg-random.org/
// which is a very lightweight random number generator based on a simple
// postprocessing of a standard linear congruent generators.
// PCG has generally good statistical properties and is much cheaper to compute compared
// to more advanced RNG e.g., Merserner Twister.
// Highly recommend Melissa O'Neill's talk about PCG https://www.youtube.com/watch?v=45Oet5qjlms

// A crucial feature of PCG is that it allows multiple "streams": 
// given a seed, we can initialize many different streams of RNGs 
// that have different independent random numbers.

struct pcg32_state {
    uint64_t state;
    uint64_t inc;
};

// http://www.pcg-random.org/download.html
inline uint32_t next_pcg32(pcg32_state &rng) {
    uint64_t oldstate = rng.state;
    // Advance internal state
    rng.state = oldstate * 6364136223846793005ULL + (rng.inc|1);
    // Calculate output function (XSH RR), uses old state for max ILP
    uint32_t xorshifted = uint32_t(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = uint32_t(oldstate >> 59u);
    return uint32_t((xorshifted >> rot) | (xorshifted << ((-rot) & 31)));
}

// https://github.com/wjakob/pcg32/blob/master/pcg32.h#L47
inline pcg32_state init_pcg32(uint64_t stream_id = 1, uint64_t seed = 0x31e241f862a1fb5eULL) {
    pcg32_state s;
    s.state = 0U;
    s.inc = (stream_id << 1u) | 1u;
    next_pcg32(s);
    s.state += seed;
    next_pcg32(s);
    return s;
}

template <typename T>
T next_pcg32_real(pcg32_state &rng) {
    return T(0);
}

// https://github.com/wjakob/pcg32/blob/master/pcg32.h
template <>
float next_pcg32_real(pcg32_state &rng) {
    union {
        uint32_t u;
        float f;
    } x;
    x.u = (next_pcg32(rng) >> 9) | 0x3f800000u;
    return x.f - 1.0f;
}

// https://github.com/wjakob/pcg32/blob/master/pcg32.h
template <>
double next_pcg32_real(pcg32_state &rng) {
    union {
        uint64_t u;
        double d;
    } x;
    x.u = ((uint64_t) next_pcg32(rng) << 20) | 0x3ff0000000000000ULL;
    return x.d - 1.0;
}
