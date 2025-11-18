/*
 * High-Quality Random Number Generator
 *
 * Uses PCG (Permuted Congruential Generator) algorithm
 * Seeded from /dev/urandom for true entropy
 *
 * Constitutional pattern: Single global RNG state, initialized once
 */

#ifndef RANDOM_H
#define RANDOM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * InitRNG - Initialize random number generator with high-quality seed
 *
 * Attempts to seed from /dev/urandom (Linux/Unix)
 * Falls back to time + PID if unavailable
 *
 * Called automatically on first use, but can be called explicitly
 */
void InitRNG(void);

/**
 * GetRandomInt - Get random integer in range [min, max] inclusive
 *
 * Uses unbiased algorithm (no modulo bias)
 *
 * @param min - Minimum value (inclusive)
 * @param max - Maximum value (inclusive)
 * @return Random integer in [min, max]
 *
 * Example:
 *   int dice = GetRandomInt(1, 6);  // Fair 6-sided die
 *   int bet = GetRandomInt(10, 100);  // Random bet 10-100
 */
int GetRandomInt(int min, int max);

/**
 * GetRandomFloat - Get random float in range [min, max]
 *
 * @param min - Minimum value
 * @param max - Maximum value
 * @return Random float in [min, max]
 *
 * Example:
 *   float offset = GetRandomFloat(-30.0f, 30.0f);
 */
float GetRandomFloat(float min, float max);

/**
 * GetRandomBool - Get random boolean (50/50 chance)
 *
 * @return true or false with equal probability
 */
bool GetRandomBool(void);

#endif // RANDOM_H
