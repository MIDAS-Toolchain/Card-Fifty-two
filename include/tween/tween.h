/*
 * Lightweight Tweening System
 * Pure C + SDL2 - No external dependencies
 * Designed for potential inclusion in Archimedes framework
 *
 * Features:
 * - Fixed-size tween pool (no dynamic allocation)
 * - Basic easing functions
 * - Float value interpolation
 * - Optional callbacks on completion
 */

#ifndef TWEEN_H
#define TWEEN_H

#include <stdbool.h>
#include <stddef.h>  // For size_t and offsetof

// ============================================================================
// CONFIGURATION
// ============================================================================

#define TWEEN_MAX_ACTIVE 32  // Maximum simultaneous tweens

// ============================================================================
// EASING TYPES
// ============================================================================

typedef enum TweenEasing {
    TWEEN_LINEAR,
    TWEEN_EASE_IN_QUAD,
    TWEEN_EASE_OUT_QUAD,
    TWEEN_EASE_IN_OUT_QUAD,
    TWEEN_EASE_IN_CUBIC,
    TWEEN_EASE_OUT_CUBIC,
    TWEEN_EASE_IN_OUT_CUBIC,
    TWEEN_EASE_OUT_BOUNCE,
    TWEEN_EASE_IN_ELASTIC,
    TWEEN_EASE_OUT_ELASTIC,
} TweenEasing_t;

// ============================================================================
// TWEEN STRUCTURE
// ============================================================================

typedef void (*TweenCallback_t)(void* user_data);

typedef enum {
    TWEEN_TARGET_DIRECT,      // Direct pointer (for stack/static variables)
    TWEEN_TARGET_ARRAY_ELEM   // Array element (needs recalculation to avoid dangling pointers)
} TweenTargetType_t;

typedef struct Tween {
    bool active;              // Is this slot in use?

    // Target type (determines how to resolve pointer)
    TweenTargetType_t target_type;

    // For DIRECT targets (stack variables, static variables, heap-allocated structs)
    float* direct_target;

    // For ARRAY_ELEM targets (elements inside dArray_t - safe from reallocation)
    void* array_ptr;          // Pointer to dArray_t* (e.g., &manager->active_effects)
    size_t element_index;     // Index in array
    size_t float_offset;      // Byte offset to the float field (use offsetof)

    // Tween parameters
    float start_value;        // Starting value
    float end_value;          // Target value
    float duration;           // Total duration (seconds)
    float elapsed;            // Time elapsed (seconds)
    TweenEasing_t easing;     // Easing function
    TweenCallback_t on_complete; // Optional callback (can be NULL)
    void* user_data;          // User data passed to callback
} Tween_t;

// ============================================================================
// TWEEN MANAGER (Fixed Pool)
// ============================================================================

typedef struct TweenManager {
    Tween_t tweens[TWEEN_MAX_ACTIVE];  // Fixed array of tweens
    int active_count;                   // Number of active tweens
    int highest_active_slot;            // Highest index with active tween (-1 if none active)
} TweenManager_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * InitTweenManager - Initialize tween manager (stack-allocated)
 *
 * @param manager - Pointer to stack-allocated TweenManager_t
 *
 * Example:
 *   TweenManager_t tween_mgr;
 *   InitTweenManager(&tween_mgr);
 */
void InitTweenManager(TweenManager_t* manager);

/**
 * CleanupTweenManager - Clear all active tweens
 *
 * @param manager - Pointer to tween manager
 */
void CleanupTweenManager(TweenManager_t* manager);

// ============================================================================
// TWEEN CREATION
// ============================================================================

/**
 * TweenFloat - Animate a float value from current to target
 *
 * Use this for stack variables, static variables, or heap-allocated structs.
 * For floats inside dArray elements, use TweenFloatInArray instead.
 *
 * @param manager - Tween manager
 * @param target - Pointer to float value to animate
 * @param end_value - Target value
 * @param duration - Duration in seconds
 * @param easing - Easing function
 * @return true if tween was created, false if pool is full
 *
 * Example:
 *   float x = 100.0f;
 *   TweenFloat(&mgr, &x, 500.0f, 1.0f, TWEEN_EASE_OUT_QUAD);
 */
bool TweenFloat(TweenManager_t* manager, float* target, float end_value,
                float duration, TweenEasing_t easing);

/**
 * TweenFloatInArray - Animate a float inside a dArray element (SAFE from reallocation)
 *
 * Use this when tweening floats inside structs stored in dArray_t.
 * Recalculates pointer each frame to avoid dangling pointers if array reallocates.
 *
 * @param manager - Tween manager
 * @param array_ptr - Pointer to the dArray_t* (e.g., &manager->active_effects)
 * @param element_index - Index of element in array
 * @param float_offset - Byte offset to float field (use offsetof(struct, field))
 * @param end_value - Target value
 * @param duration - Duration in seconds
 * @param easing - Easing function
 * @return true if tween was created, false if pool is full
 *
 * Example:
 *   TweenFloatInArray(&mgr, &manager->active_effects, 0,
 *                     offsetof(StatusEffectInstance_t, shake_offset_x),
 *                     5.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);
 */
bool TweenFloatInArray(TweenManager_t* manager,
                       void* array_ptr,
                       size_t element_index,
                       size_t float_offset,
                       float end_value,
                       float duration,
                       TweenEasing_t easing);

/**
 * TweenFloatWithCallback - Animate float with completion callback
 *
 * @param manager - Tween manager
 * @param target - Pointer to float value to animate
 * @param end_value - Target value
 * @param duration - Duration in seconds
 * @param easing - Easing function
 * @param on_complete - Function to call when tween finishes (can be NULL)
 * @param user_data - Data passed to callback
 * @return true if tween was created, false if pool is full
 *
 * Example:
 *   void OnFadeComplete(void* data) { printf("Fade done!\n"); }
 *   float alpha = 1.0f;
 *   TweenFloatWithCallback(&mgr, &alpha, 0.0f, 0.5f, TWEEN_LINEAR, OnFadeComplete, NULL);
 */
bool TweenFloatWithCallback(TweenManager_t* manager, float* target, float end_value,
                             float duration, TweenEasing_t easing,
                             TweenCallback_t on_complete, void* user_data);

// ============================================================================
// UPDATE
// ============================================================================

/**
 * UpdateTweens - Update all active tweens (call every frame)
 *
 * @param manager - Tween manager
 * @param dt - Delta time in seconds
 *
 * Example (in game loop):
 *   UpdateTweens(&tween_mgr, delta_time);
 */
void UpdateTweens(TweenManager_t* manager, float dt);

// ============================================================================
// CONTROL
// ============================================================================

/**
 * StopTweensForTarget - Stop all tweens affecting a specific target
 *
 * @param manager - Tween manager
 * @param target - Pointer to target value
 * @return Number of tweens that were stopped
 *
 * Example:
 *   int stopped = StopTweensForTarget(&mgr, &player_x);
 *   if (stopped > 0) printf("Stopped %d tweens\n", stopped);
 */
int StopTweensForTarget(TweenManager_t* manager, float* target);

/**
 * StopAllTweens - Stop all active tweens
 *
 * @param manager - Tween manager
 * @return Number of tweens that were stopped
 */
int StopAllTweens(TweenManager_t* manager);

/**
 * GetActiveTweenCount - Get number of active tweens
 *
 * @param manager - Tween manager
 * @return Number of active tweens
 */
int GetActiveTweenCount(const TweenManager_t* manager);

// ============================================================================
// UTILITY (Query & Debug)
// ============================================================================

/**
 * IsTweenActive - Check if a specific target has active tweens
 *
 * @param manager - Tween manager
 * @param target - Pointer to target value
 * @return true if target has at least one active tween
 *
 * Example:
 *   if (IsTweenActive(&mgr, &enemy->display_hp)) {
 *       // HP bar is currently animating
 *   }
 */
bool IsTweenActive(const TweenManager_t* manager, const float* target);

/**
 * GetTweenProgress - Get progress of first active tween for target
 *
 * @param manager - Tween manager
 * @param target - Pointer to target value
 * @return Progress value 0.0-1.0, or -1.0f if no active tween found
 *
 * Example:
 *   float progress = GetTweenProgress(&mgr, &card_x);
 *   if (progress >= 0.0f) {
 *       printf("Card animation is %.0f%% complete\n", progress * 100.0f);
 *   }
 */
float GetTweenProgress(const TweenManager_t* manager, const float* target);

#endif // TWEEN_H
