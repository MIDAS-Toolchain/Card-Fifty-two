/*
 * Lightweight Tweening System Implementation
 */

#include "../../include/tween/tween.h"
#include "../../include/common.h"  // For dArray_t
#include <string.h>
#include <math.h>
#include <stddef.h>  // For offsetof
#include <stdint.h>  // For uintptr_t

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
// POINTER RESOLUTION (For ARRAY_ELEM targets)
// ============================================================================

/**
 * get_tween_target_pointer - Resolve pointer from tween target specification
 *
 * For DIRECT targets: returns direct_target
 * For ARRAY_ELEM targets: recalculates pointer from array index + offset
 *
 * @param tween - Tween to resolve pointer for
 * @return float* - Resolved pointer, or NULL if array is invalid
 *
 * This function is called every frame in UpdateTweens() to ensure pointer
 * is always valid even if the underlying dArray has been reallocated.
 */
static float* get_tween_target_pointer(Tween_t* tween) {
    if (!tween || !tween->active) {
        return NULL;
    }

    if (tween->target_type == TWEEN_TARGET_DIRECT) {
        // Simple case - return stored pointer
        return tween->direct_target;
    }

    // TWEEN_TARGET_ARRAY_ELEM - recalculate pointer from array
    if (!tween->array_ptr) {
        return NULL;
    }

    // Dereference the dArray_t** to get dArray_t*
    dArray_t** array_ptr_ptr = (dArray_t**)tween->array_ptr;
    dArray_t* array = *array_ptr_ptr;

    if (!array || !array->data) {
        return NULL;
    }

    // Check bounds
    if (tween->element_index >= array->count) {
        return NULL;
    }

    // Calculate pointer: base + (element_index * element_size) + float_offset
    char* element_ptr = (char*)array->data + (tween->element_index * array->element_size);
    float* target_ptr = (float*)(element_ptr + tween->float_offset);

    return target_ptr;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void InitTweenManager(TweenManager_t* manager) {
    if (!manager) return;

    memset(manager, 0, sizeof(TweenManager_t));
    manager->active_count = 0;
    manager->highest_active_slot = -1;  // No active tweens initially

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
    tween->target_type = TWEEN_TARGET_DIRECT;
    tween->direct_target = target;
    tween->array_ptr = NULL;
    tween->element_index = 0;
    tween->float_offset = 0;
    tween->start_value = *target;  // Capture current value
    tween->end_value = end_value;
    tween->duration = duration;
    tween->elapsed = 0.0f;
    tween->easing = easing;
    tween->on_complete = on_complete;
    tween->user_data = user_data;

    manager->active_count++;

    // Update highest active slot tracker
    if (slot > manager->highest_active_slot) {
        manager->highest_active_slot = slot;
    }

    return true;
}

bool TweenFloatInArray(TweenManager_t* manager,
                       void* array_ptr,
                       size_t element_index,
                       size_t float_offset,
                       float end_value,
                       float duration,
                       TweenEasing_t easing) {
    if (!manager || !array_ptr || duration <= 0.0f) return false;

    // Validate array and index
    dArray_t** array_ptr_ptr = (dArray_t**)array_ptr;
    dArray_t* array = *array_ptr_ptr;

    if (!array) {
        d_LogError("TweenFloatInArray: array is NULL");
        return false;
    }

    if (!array->data) {
        d_LogErrorF("TweenFloatInArray: array->data is NULL (count=%zu)", array->count);
        return false;
    }

    if (element_index >= array->count) {
        d_LogErrorF("TweenFloatInArray: element_index %zu >= count %zu",
                   element_index, array->count);
        return false;
    }

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

    // Calculate initial pointer to get start value
    char* element_ptr = (char*)array->data + (element_index * array->element_size);

    // Defensive: Verify offset doesn't exceed element bounds
    if (float_offset + sizeof(float) > array->element_size) {
        d_LogErrorF("TweenFloatInArray: Invalid offset %zu for element_size %zu",
                   float_offset, array->element_size);
        return false;
    }

    float* target_ptr = (float*)(element_ptr + float_offset);

    // Defensive: Check pointer alignment (floats must be 4-byte aligned on most platforms)
    if (((uintptr_t)target_ptr & 0x3) != 0) {
        d_LogErrorF("TweenFloatInArray: Misaligned float pointer %p (offset %zu)",
                   (void*)target_ptr, float_offset);
        return false;
    }

    float start_value = *target_ptr;

    // Initialize tween (ARRAY_ELEM target type)
    Tween_t* tween = &manager->tweens[slot];
    tween->active = true;
    tween->target_type = TWEEN_TARGET_ARRAY_ELEM;
    tween->direct_target = NULL;
    tween->array_ptr = array_ptr;
    tween->element_index = element_index;
    tween->float_offset = float_offset;
    tween->start_value = start_value;
    tween->end_value = end_value;
    tween->duration = duration;
    tween->elapsed = 0.0f;
    tween->easing = easing;
    tween->on_complete = NULL;
    tween->user_data = NULL;

    manager->active_count++;

    // Update highest active slot tracker
    if (slot > manager->highest_active_slot) {
        manager->highest_active_slot = slot;
    }

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

    // Early exit if no active tweens
    if (manager->highest_active_slot < 0) return;

    // Only iterate up to highest active slot (skip empty tail slots)
    for (int i = 0; i <= manager->highest_active_slot; i++) {
        Tween_t* tween = &manager->tweens[i];

        if (!tween->active) continue;

        // Resolve pointer (safe for both DIRECT and ARRAY_ELEM targets)
        float* target = get_tween_target_pointer(tween);
        if (!target) {
            // Array was destroyed or index out of bounds - deactivate tween
            tween->active = false;
            manager->active_count--;
            continue;
        }

        // Update elapsed time
        tween->elapsed += dt;

        // Calculate progress (0.0 to 1.0)
        float t = tween->elapsed / tween->duration;

        if (t >= 1.0f) {
            // Tween complete
            t = 1.0f;
            *target = tween->end_value;  // Snap to exact end value

            // Call completion callback if provided
            if (tween->on_complete) {
                tween->on_complete(tween->user_data);
            }

            // Deactivate tween
            tween->active = false;
            manager->active_count--;

            // Update highest_active_slot if this was the highest
            if (i == manager->highest_active_slot) {
                // Scan backwards to find new highest active slot
                manager->highest_active_slot = -1;
                for (int j = i - 1; j >= 0; j--) {
                    if (manager->tweens[j].active) {
                        manager->highest_active_slot = j;
                        break;
                    }
                }
            }
        } else {
            // Apply easing
            float eased_t = ApplyEasing(t, tween->easing);

            // Interpolate value
            *target = tween->start_value + (tween->end_value - tween->start_value) * eased_t;
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

        if (!tween->active) continue;

        // Check if this tween targets the given pointer
        float* tween_target = get_tween_target_pointer(tween);
        if (tween_target == target) {
            tween->active = false;
            manager->active_count--;
            stopped++;
        }
    }

    // Recalculate highest_active_slot if we stopped any tweens
    if (stopped > 0) {
        manager->highest_active_slot = -1;
        for (int i = TWEEN_MAX_ACTIVE - 1; i >= 0; i--) {
            if (manager->tweens[i].active) {
                manager->highest_active_slot = i;
                break;
            }
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
        Tween_t* tween = (Tween_t*)&manager->tweens[i];
        if (!tween->active) continue;

        float* tween_target = get_tween_target_pointer(tween);
        if (tween_target == target) {
            return true;
        }
    }

    return false;
}

float GetTweenProgress(const TweenManager_t* manager, const float* target) {
    if (!manager || !target) return -1.0f;

    for (int i = 0; i < TWEEN_MAX_ACTIVE; i++) {
        Tween_t* tween = (Tween_t*)&manager->tweens[i];
        if (!tween->active) continue;

        float* tween_target = get_tween_target_pointer(tween);
        if (tween_target == target) {
            // Return normalized progress (0.0 to 1.0)
            float progress = tween->elapsed / tween->duration;
            if (progress > 1.0f) progress = 1.0f;
            return progress;
        }
    }

    return -1.0f;  // No active tween found for this target
}
