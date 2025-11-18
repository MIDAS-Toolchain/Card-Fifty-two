/*
 * High-Quality Random Number Generator Implementation
 *
 * Uses PCG32 (Permuted Congruential Generator)
 * Excellent statistical properties, very fast, small state
 */

#include "../include/random.h"
#include "../include/common.h"
#include <stdio.h>
#include <time.h>
#include <unistd.h>

// PCG32 RNG State
static uint64_t g_rng_state = 0x853c49e6748fea9bULL;
static uint64_t g_rng_inc = 0xda3e39cb94b95bdbULL;
static bool g_rng_initialized = false;

// ============================================================================
// PCG32 ALGORITHM (https://www.pcg-random.org/)
// ============================================================================

static uint32_t pcg32_random(void) {
    uint64_t oldstate = g_rng_state;
    // Advance internal state (LCG)
    g_rng_state = oldstate * 6364136223846793005ULL + g_rng_inc;
    // Calculate output function (XSH-RR: xorshift high, random rotation)
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18u) ^ oldstate) >> 27u);
    uint32_t rot = (uint32_t)(oldstate >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

// ============================================================================
// SEEDING
// ============================================================================

void InitRNG(void) {
    if (g_rng_initialized) {
        return;  // Already seeded
    }

    uint64_t seed1 = 0;
    uint64_t seed2 = 0;

    // Try to read from /dev/urandom (high-quality entropy)
    FILE* urandom = fopen("/dev/urandom", "rb");
    if (urandom) {
        size_t read1 = fread(&seed1, sizeof(seed1), 1, urandom);
        size_t read2 = fread(&seed2, sizeof(seed2), 1, urandom);
        fclose(urandom);

        if (read1 == 1 && read2 == 1) {
            d_LogInfo("RNG seeded from /dev/urandom (high-quality entropy)");
            g_rng_state = seed1;
            g_rng_inc = (seed2 << 1u) | 1u;  // Must be odd
            g_rng_initialized = true;

            // Warm up the RNG (discard first few outputs)
            for (int i = 0; i < 10; i++) {
                pcg32_random();
            }
            return;
        }
    }

    // Fallback: Use time + PID (lower quality but still decent)
    d_LogWarning("RNG falling back to time+PID seed (lower quality)");
    seed1 = (uint64_t)time(NULL);
    seed2 = (uint64_t)getpid();

    g_rng_state = seed1 ^ (seed2 << 32);
    g_rng_inc = (seed2 << 1u) | 1u;  // Must be odd
    g_rng_initialized = true;

    // Warm up the RNG
    for (int i = 0; i < 10; i++) {
        pcg32_random();
    }
}

// ============================================================================
// PUBLIC API
// ============================================================================

int GetRandomInt(int min, int max) {
    if (!g_rng_initialized) {
        InitRNG();
    }

    if (min > max) {
        // Swap if backwards
        int temp = min;
        min = max;
        max = temp;
    }

    if (min == max) {
        return min;
    }

    // Unbiased range generation (rejection method to avoid modulo bias)
    uint32_t range = (uint32_t)(max - min + 1);
    uint32_t threshold = (0xFFFFFFFFu - range + 1) % range;
    uint32_t value;

    do {
        value = pcg32_random();
    } while (value < threshold);  // Reject biased values

    return min + (int)(value % range);
}

float GetRandomFloat(float min, float max) {
    if (!g_rng_initialized) {
        InitRNG();
    }

    if (min > max) {
        // Swap if backwards
        float temp = min;
        min = max;
        max = temp;
    }

    // Generate random float in [0, 1]
    uint32_t rand_int = pcg32_random();
    float rand_01 = (float)rand_int / (float)0xFFFFFFFFu;

    // Scale to [min, max]
    return min + rand_01 * (max - min);
}

bool GetRandomBool(void) {
    if (!g_rng_initialized) {
        InitRNG();
    }

    return (pcg32_random() & 1) == 1;
}
