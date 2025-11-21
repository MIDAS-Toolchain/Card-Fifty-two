/*
 * Visual Effects Component - Damage Numbers and Screen Shake
 *
 * Constitutional Pattern: Component-based architecture (ADR-008)
 * - Create/Destroy lifecycle
 * - Update/Render separation
 * - State encapsulated in struct
 *
 * Dependencies:
 * - TweenManager_t* (passed in, not owned)
 */

#ifndef VISUAL_EFFECTS_H
#define VISUAL_EFFECTS_H

#include <stdbool.h>
#include <stdint.h>
#include "../../tween/tween.h"

// Maximum concurrent damage numbers (pool size)
#define MAX_DAMAGE_NUMBERS 16

// ============================================================================
// DAMAGE NUMBER SYSTEM
// ============================================================================

typedef struct DamageNumber {
    bool active;
    uint32_t generation;  // Incremented on reuse to invalidate stale callbacks
    float x, y;           // World position
    float alpha;          // Opacity (1.0 = opaque, 0.0 = invisible)
    int damage;           // Damage amount to display
    bool is_healing;      // true = green (+N), false = red (-N)
    bool is_crit;         // true = gold/large crit number, false = normal
} DamageNumber_t;

// Callback user data for damage numbers (includes generation to prevent stale callbacks)
typedef struct DamageNumberCallbackData {
    DamageNumber_t* dmg;
    uint32_t generation;  // Generation at time of tween creation
} DamageNumberCallbackData_t;

// ============================================================================
// VISUAL EFFECTS COMPONENT
// ============================================================================

typedef struct VisualEffects {
    // Damage numbers pool (fixed-size array, constitutional pattern)
    DamageNumber_t damage_numbers[MAX_DAMAGE_NUMBERS];
    DamageNumberCallbackData_t callback_data[MAX_DAMAGE_NUMBERS];

    // Screen shake state
    float shake_offset_x;
    float shake_offset_y;
    float shake_cooldown;  // Cooldown to prevent shake spam

    // Dependencies (passed in, not owned)
    TweenManager_t* tween_manager;
} VisualEffects_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * Create visual effects component
 * @param tween_mgr Tween manager for animations (not owned by component)
 * @return Allocated VisualEffects_t* or NULL on failure
 */
VisualEffects_t* CreateVisualEffects(TweenManager_t* tween_mgr);

/**
 * Destroy visual effects component and set pointer to NULL
 * @param vfx Pointer to VisualEffects_t* (will be set to NULL)
 */
void DestroyVisualEffects(VisualEffects_t** vfx);

// ============================================================================
// UPDATE / RENDER
// ============================================================================

/**
 * Update visual effects (shake cooldown decay)
 * @param vfx Visual effects component
 * @param dt Delta time in seconds
 */
void UpdateVisualEffects(VisualEffects_t* vfx, float dt);

/**
 * Render all active damage numbers
 * @param vfx Visual effects component
 */
void RenderDamageNumbers(VisualEffects_t* vfx);

/**
 * Apply screen shake offset to SDL viewport
 * Call before rendering scene, then restore viewport after
 * @param vfx Visual effects component
 */
void ApplyScreenShakeViewport(VisualEffects_t* vfx);

/**
 * Restore viewport to default (no shake offset)
 */
void RestoreViewport(void);

// ============================================================================
// PUBLIC EFFECTS API (called by game.c, cardTags.c, etc.)
// ============================================================================

/**
 * Spawn floating damage number at world position
 * @param vfx Visual effects component
 * @param damage Damage amount (positive = damage, negative also works)
 * @param x World X position
 * @param y World Y position
 * @param is_healing true = green (+N), false = red (-N)
 * @param is_crit true = gold/large crit, false = normal
 */
void VFX_SpawnDamageNumber(VisualEffects_t* vfx, int damage, float x, float y, bool is_healing, bool is_crit);

/**
 * Trigger screen shake effect
 * @param vfx Visual effects component
 * @param intensity Shake distance in pixels (e.g., 10.0f)
 * @param duration Duration in seconds (e.g., 0.3f)
 */
void VFX_TriggerScreenShake(VisualEffects_t* vfx, float intensity, float duration);

#endif // VISUAL_EFFECTS_H
