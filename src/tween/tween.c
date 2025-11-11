/*
 * Lightweight Tweening System Implementation
 */

#include "../../include/tween/tween.h"
#include <string.h>
#include <math.h>

// ============================================================================
// CONSTANTS
// ============================================================================

#define TWEEN_PI            3.14159265358979323846f
#define TWEEN_MAX_DT        0.1f  // Clamp dt to 100ms (10 FPS) to prevent lag spike issues

// Bounce easing constants (derived from Robert Penner's easing equations)
// These create 4 bounces with decreasing amplitude
#define BOUNCE_DIVISOR      2.75f       // Total duration divisor
#define BOUNCE_COEFFICIENT  7.5625f     // Parabola steepness (1 / (BOUNCE_DIVISOR^2 / 4))
#define BOUNCE_T1           (1.0f / BOUNCE_DIVISOR)        // First bounce: 0.0 - 0.36
#define BOUNCE_T2           (2.0f / BOUNCE_DIVISOR)        // Second bounce: 0.36 - 0.73
#define BOUNCE_T3           (2.5f / BOUNCE_DIVISOR)        // Third bounce: 0.73 - 0.91
#define BOUNCE_OFFSET1      (1.5f / BOUNCE_DIVISOR)
#define BOUNCE_OFFSET2      (2.25f / BOUNCE_DIVISOR)
#define BOUNCE_OFFSET3      (2.625f / BOUNCE_DIVISOR)
#define BOUNCE_HEIGHT1      0.75f
#define BOUNCE_HEIGHT2      0.9375f
#define BOUNCE_HEIGHT3      0.984375f

// Elastic easing constants
#define ELASTIC_PERIOD      0.3f        // Oscillation period
#define ELASTIC_AMPLITUDE   1.0f        // Overshoot amount

// ============================================================================
// EASING FUNCTIONS (Pure Math)
// Reference: Robert Penner's Easing Functions
// All functions take normalized time t ∈ [0, 1] and return eased value ∈ [0, 1]
// ============================================================================

/**
 * EaseLinear - No easing, constant velocity
 * Formula: f(t) = t
 */
static float EaseLinear(float t) {
    return t;
}

/**
 * EaseInQuad - Quadratic acceleration from zero velocity
 * Formula: f(t) = t²
 */
static float EaseInQuad(float t) {
    return t * t;
}

/**
 * EaseOutQuad - Quadratic deceleration to zero velocity
 * Formula: f(t) = -t² + 2t
 */
static float EaseOutQuad(float t) {
    return t * (2.0f - t);
}

/**
 * EaseInOutQuad - Quadratic acceleration until halfway, then deceleration
 * Formula: f(t) = 2t² if t < 0.5, else -2t² + 4t - 1
 */
static float EaseInOutQuad(float t) {
    if (t < 0.5f) {
        return 2.0f * t * t;
    } else {
        return -1.0f + (4.0f - 2.0f * t) * t;
    }
}

/**
 * EaseInCubic - Cubic acceleration from zero velocity
 * Formula: f(t) = t³
 */
static float EaseInCubic(float t) {
    return t * t * t;
}

/**
 * EaseOutCubic - Cubic deceleration to zero velocity
 * Formula: f(t) = (t-1)³ + 1
 */
static float EaseOutCubic(float t) {
    float f = t - 1.0f;
    return f * f * f + 1.0f;
}

/**
 * EaseInOutCubic - Cubic acceleration until halfway, then deceleration
 * Formula: f(t) = 4t³ if t < 0.5, else (2t-2)³/2 + 1
 */
static float EaseInOutCubic(float t) {
    if (t < 0.5f) {
        return 4.0f * t * t * t;
    } else {
        float f = (2.0f * t) - 2.0f;
        return 0.5f * f * f * f + 1.0f;
    }
}

/**
 * EaseOutBounce - Exponentially decaying bounce effect
 * Simulates 4 bounces with decreasing amplitude
 */
static float EaseOutBounce(float t) {
    if (t < BOUNCE_T1) {
        return BOUNCE_COEFFICIENT * t * t;
    } else if (t < BOUNCE_T2) {
        float f = t - BOUNCE_OFFSET1;
        return BOUNCE_COEFFICIENT * f * f + BOUNCE_HEIGHT1;
    } else if (t < BOUNCE_T3) {
        float f = t - BOUNCE_OFFSET2;
        return BOUNCE_COEFFICIENT * f * f + BOUNCE_HEIGHT2;
    } else {
        float f = t - BOUNCE_OFFSET3;
        return BOUNCE_COEFFICIENT * f * f + BOUNCE_HEIGHT3;
    }
}

/**
 * EaseInElastic - Exponentially increasing sinusoidal wave (rubber band pull-back)
 * Creates overshoot effect at the start
 */
static float EaseInElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;

    float p = ELASTIC_PERIOD;
    float s = p / (2.0f * TWEEN_PI) * asinf(1.0f / ELASTIC_AMPLITUDE);
    float f = t - 1.0f;

    return -(ELASTIC_AMPLITUDE * powf(2.0f, 10.0f * f) *
             sinf((f - s) * (2.0f * TWEEN_PI) / p));
}

/**
 * EaseOutElastic - Exponentially decaying sinusoidal wave (rubber band release)
 * Creates overshoot effect at the end
 */
static float EaseOutElastic(float t) {
    if (t == 0.0f || t == 1.0f) return t;

    float p = ELASTIC_PERIOD;
    float s = p / (2.0f * TWEEN_PI) * asinf(1.0f / ELASTIC_AMPLITUDE);

    return ELASTIC_AMPLITUDE * powf(2.0f, -10.0f * t) *
           sinf((t - s) * (2.0f * TWEEN_PI) / p) + 1.0f;
}

// ============================================================================
// EASING DISPATCHER
// ============================================================================

static float ApplyEasing(float t, TweenEasing_t easing) {
    switch (easing) {
        case TWEEN_LINEAR:          return EaseLinear(t);
        case TWEEN_EASE_IN_QUAD:    return EaseInQuad(t);
        case TWEEN_EASE_OUT_QUAD:   return EaseOutQuad(t);
        case TWEEN_EASE_IN_OUT_QUAD: return EaseInOutQuad(t);
        case TWEEN_EASE_IN_CUBIC:   return EaseInCubic(t);
        case TWEEN_EASE_OUT_CUBIC:  return EaseOutCubic(t);
        case TWEEN_EASE_IN_OUT_CUBIC: return EaseInOutCubic(t);
        case TWEEN_EASE_OUT_BOUNCE: return EaseOutBounce(t);
        case TWEEN_EASE_IN_ELASTIC: return EaseInElastic(t);
        case TWEEN_EASE_OUT_ELASTIC: return EaseOutElastic(t);
        default:                    return t;
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void InitTweenManager(TweenManager_t* manager) {
    if (!manager) return;

    memset(manager, 0, sizeof(TweenManager_t));
    manager->active_count = 0;

    // Mark all slots as inactive
    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        manager->tweens[i].active = false;
    }
}

void CleanupTweenManager(TweenManager_t* manager) {
    if (!manager) return;

    // Stop all tweens (but don't call callbacks)
    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        manager->tweens[i].active = false;
    }
    manager->active_count = 0;
}

// ============================================================================
// TWEEN CREATION
// ============================================================================

bool TweenFloat(TweenManager_t* manager, float* target, float end_value,
                float duration, TweenEasing_t easing) {
    return TweenFloatWithCallback(manager, target, end_value, duration, easing, NULL, NULL);
}

bool TweenFloatWithCallback(TweenManager_t* manager, float* target, float end_value,
                             float duration, TweenEasing_t easing,
                             TweenCallback_t on_complete, void* user_data) {
    if (!manager || !target || duration <= 0.0f) return false;

    // Find free slot
    int slot = -1;
    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        if (!manager->tweens[i].active) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        return false;  // Pool full
    }

    // Initialize tween
    Tween_t* tween = &manager->tweens[slot];
    tween->active = true;
    tween->target = target;
    tween->start_value = *target;  // Capture current value
    tween->end_value = end_value;
    tween->duration = duration;
    tween->elapsed = 0.0f;
    tween->easing = easing;
    tween->on_complete = on_complete;
    tween->user_data = user_data;

    manager->active_count++;
    return true;
}

// ============================================================================
// UPDATE
// ============================================================================

void UpdateTweens(TweenManager_t* manager, float dt) {
    if (!manager || dt <= 0.0f) return;

    // Clamp dt to prevent lag spikes from causing tweens to skip to completion
    if (dt > TWEEN_MAX_DT) {
        dt = TWEEN_MAX_DT;
    }

    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        Tween_t* tween = &manager->tweens[i];

        if (!tween->active) continue;

        // Update elapsed time
        tween->elapsed += dt;

        // Calculate progress (0.0 to 1.0)
        float t = tween->elapsed / tween->duration;

        if (t >= 1.0f) {
            // Tween complete
            t = 1.0f;
            *tween->target = tween->end_value;  // Snap to exact end value

            // Call completion callback if provided
            if (tween->on_complete) {
                tween->on_complete(tween->user_data);
            }

            // Deactivate tween
            tween->active = false;
            manager->active_count--;
        } else {
            // Apply easing
            float eased_t = ApplyEasing(t, tween->easing);

            // Interpolate value
            *tween->target = tween->start_value + (tween->end_value - tween->start_value) * eased_t;
        }
    }
}

// ============================================================================
// CONTROL
// ============================================================================

int StopTweensForTarget(TweenManager_t* manager, float* target) {
    if (!manager || !target) return 0;

    int stopped = 0;
    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        Tween_t* tween = &manager->tweens[i];

        if (tween->active && tween->target == target) {
            tween->active = false;
            manager->active_count--;
            stopped++;
        }
    }

    return stopped;
}

int StopAllTweens(TweenManager_t* manager) {
    if (!manager) return 0;

    int stopped = manager->active_count;

    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        manager->tweens[i].active = false;
    }
    manager->active_count = 0;

    return stopped;
}

int GetActiveTweenCount(const TweenManager_t* manager) {
    if (!manager) return 0;
    return manager->active_count;
}

// ============================================================================
// UTILITY (Query & Debug)
// ============================================================================

bool IsTweenActive(const TweenManager_t* manager, const float* target) {
    if (!manager || !target) return false;

    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        const Tween_t* tween = &manager->tweens[i];
        if (tween->active && tween->target == target) {
            return true;
        }
    }

    return false;
}

float GetTweenProgress(const TweenManager_t* manager, const float* target) {
    if (!manager || !target) return -1.0f;

    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        const Tween_t* tween = &manager->tweens[i];
        if (tween->active && tween->target == target) {
            // Return normalized progress (0.0 to 1.0)
            float progress = tween->elapsed / tween->duration;
            if (progress > 1.0f) progress = 1.0f;
            return progress;
        }
    }

    return -1.0f;  // No active tween found for this target
}
