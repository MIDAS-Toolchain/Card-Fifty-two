#ifndef ENEMY_H
#define ENEMY_H

#include "common.h"
#include <SDL2/SDL.h>

// Forward declarations
typedef struct TweenManager TweenManager_t;
typedef struct GameContext GameContext_t;

// Need full game.h for GameEvent_t enum (can't forward-declare enums properly)
// This is OK - enemy.h depends on game.h for event system
#include "game.h"

// Need full ability.h for Ability_t struct (used as pointers in dArray_t)
#include "ability.h"

// ============================================================================
// ENEMY ABILITIES (Data-Driven System)
// ============================================================================

// NOTE: Old EnemyAbility_t enum removed - abilities are now fully data-driven
// See: ability.h for Ability_t, AbilityTrigger_t, AbilityEffect_t
// Abilities are defined in DUF files and loaded dynamically

// ============================================================================
// ENEMY STRUCTURE
// ============================================================================

/**
 * Enemy_t - Combat enemy entity
 *
 * Constitutional pattern:
 * - dString_t for name/description (not char[])
 * - dArray_t for abilities storing Ability_t* (pointers, not values - Ability_t has owned strings)
 * - Data-driven: Loaded from DUF files via LoadEnemyFromDUF()
 */
typedef struct Enemy {
    dString_t* name;                // Enemy name (e.g., "The Didact")
    dString_t* description;         // Enemy lore/flavor text
    int max_hp;                     // Maximum HP
    int current_hp;                 // Current HP (combat ends at 0)
    float display_hp;               // Displayed HP (tweened for smooth HP bar drain)
    int total_damage_taken;         // Cumulative damage dealt (never decreases, even if healed)

    dArray_t* abilities;            // Array of Ability_t* (unified - no passive/active split)

    // Portrait system (hybrid for dynamic effects)
    SDL_Surface* portrait_surface;  // Source pixel data (owned, for manipulation)
    SDL_Texture* portrait_texture;  // Cached GPU texture (owned, for rendering)
    bool portrait_dirty;            // true if surface changed, needs texture rebuild

    // Damage feedback effects (shake + blink)
    float shake_offset_x;           // Horizontal shake offset (tweened)
    float shake_offset_y;           // Vertical shake offset (tweened)
    float red_flash_alpha;          // Red overlay alpha (tweened from 1.0 to 0.0)
    float green_flash_alpha;        // Green overlay alpha for heal effects (tweened)

    // Defeat animation
    float defeat_fade_alpha;        // Alpha for fade-out (1.0 → 0.0)
    float defeat_scale;             // Scale for zoom-out (1.0 → 0.8)

    bool is_defeated;               // true if current_hp <= 0
} Enemy_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateEnemy - Initialize a new enemy
 *
 * @param name - Enemy name
 * @param max_hp - Maximum HP
 * @return Enemy_t* - Heap-allocated enemy, or NULL on failure
 *
 * Constitutional pattern: Heap-allocated entity (like Player_t)
 * Caller must call DestroyEnemy() when done
 */
Enemy_t* CreateEnemy(const char* name, int max_hp);

// NOTE: Enemy factory functions removed - enemies are now loaded from DUF files
// See: data/enemies/tutorial_enemies.duf and LoadEnemyFromDUF() in loaders/enemyLoader.h

/**
 * DestroyEnemy - Free enemy resources
 *
 * @param enemy - Pointer to enemy pointer (double pointer for nulling)
 *
 * Destroys internal dString_t and dArray_t, then frees enemy struct
 */
void DestroyEnemy(Enemy_t** enemy);

// ============================================================================
// ABILITY MANAGEMENT (Data-Driven System)
// ============================================================================

// NOTE: Old ability management functions removed (AddPassiveAbility, AddActiveAbility, etc.)
// Abilities are now loaded from DUF files via LoadEnemyFromDUF()
// See: loaders/enemyLoader.h and ability.h for new system

// ============================================================================
// COMBAT ACTIONS
// ============================================================================

/**
 * TakeDamage - Apply damage to enemy HP
 *
 * @param enemy - Enemy to damage
 * @param damage - Damage amount
 *
 * Clamps HP to 0, sets is_defeated flag if HP reaches 0
 */
void TakeDamage(Enemy_t* enemy, int damage);

/**
 * HealEnemy - Restore enemy HP
 *
 * @param enemy - Enemy to heal
 * @param amount - Healing amount
 * @param tween_manager - Tween manager for green flash animation (can be NULL)
 *
 * Clamps HP to max_hp, triggers green flash effect if tween_manager provided
 */
void HealEnemy(Enemy_t* enemy, int amount, TweenManager_t* tween_manager);

/**
 * GetEnemyHPPercent - Get HP as percentage
 *
 * @param enemy - Enemy to query
 * @return float - HP percentage (0.0-1.0)
 */
float GetEnemyHPPercent(const Enemy_t* enemy);

// ============================================================================
// ABILITY TRIGGERS (Updated for Data-Driven System)
// ============================================================================

/**
 * CheckEnemyAbilityTriggers - Check if any abilities should trigger on event
 *
 * @param enemy - Enemy to check
 * @param event - Game event that occurred
 * @param game - Game context (for effect execution)
 *
 * Iterates through enemy->abilities array, calls CheckAbilityTrigger() on each,
 * executes abilities that should fire via ExecuteAbility()
 * Called by Game_TriggerEvent() when gameplay events occur
 */
void CheckEnemyAbilityTriggers(Enemy_t* enemy, GameEvent_t event, GameContext_t* game);

/**
 * ResetAbilityTriggers - Reset runtime state for new combat
 *
 * @param enemy - Enemy to reset
 *
 * Calls ResetAbilityStates() on enemy->abilities
 * Called when combat starts or enemy is created
 */
void ResetAbilityTriggers(Enemy_t* enemy);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * EnemyToString - Convert enemy to human-readable string
 *
 * @param enemy - Enemy to convert
 * @param out - Output dString_t (caller must create and destroy)
 *
 * Format: "The Broken Dealer | HP: 30/50 | Attack: 5"
 */
void EnemyToString(const Enemy_t* enemy, dString_t* out);

/**
 * GetEnemyName - Get enemy name as const char*
 *
 * @param enemy - Enemy to query
 * @return const char* - Enemy name, or "Unknown Enemy" if NULL
 */
const char* GetEnemyName(const Enemy_t* enemy);

// ============================================================================
// PORTRAIT MANAGEMENT
// ============================================================================

/**
 * LoadEnemyPortrait - Load enemy portrait from file as surface
 *
 * @param enemy - Enemy to load portrait for
 * @param filename - Path to portrait image file
 * @return bool - true on success, false on failure
 *
 * Loads portrait as SDL_Surface for pixel manipulation
 * Sets portrait_dirty = true to trigger texture generation
 */
bool LoadEnemyPortrait(Enemy_t* enemy, const char* filename);

/**
 * RefreshEnemyPortraitTexture - Convert surface to texture
 *
 * @param enemy - Enemy to refresh portrait for
 *
 * Converts portrait_surface → portrait_texture
 * Only call when portrait_dirty == true
 * Automatically sets portrait_dirty = false after conversion
 */
void RefreshEnemyPortraitTexture(Enemy_t* enemy);

/**
 * GetEnemyPortraitTexture - Get current portrait texture for rendering
 *
 * @param enemy - Enemy to query
 * @return SDL_Texture* - Portrait texture, or NULL if not loaded
 *
 * Automatically refreshes texture if portrait_dirty == true
 */
SDL_Texture* GetEnemyPortraitTexture(Enemy_t* enemy);

// ============================================================================
// DAMAGE FEEDBACK EFFECTS
// ============================================================================

/**
 * TriggerEnemyDamageEffect - Trigger shake + red blink effect
 *
 * @param enemy - Enemy that took damage
 * @param tween_manager - Tween manager to use for animation
 *
 * Creates shake animation (horizontal offset) and red flash overlay
 * Shake: 0 → +8px → -8px → 0 over 0.3s
 * Red flash: alpha 0.7 → 0.0 over 0.4s
 */
void TriggerEnemyDamageEffect(Enemy_t* enemy, TweenManager_t* tween_manager);

/**
 * TriggerEnemyHealEffect - Trigger green flash effect for healing
 *
 * @param enemy - Enemy that was healed
 * @param tween_manager - Tween manager to use for animation
 *
 * Creates green flash overlay (alpha 0.6 → 0.0 over 0.5s)
 */
void TriggerEnemyHealEffect(Enemy_t* enemy, TweenManager_t* tween_manager);

/**
 * GetEnemyShakeOffset - Get current shake offset for rendering
 *
 * @param enemy - Enemy to query
 * @param out_x - Output shake X offset
 * @param out_y - Output shake Y offset
 *
 * Use these offsets when rendering enemy portrait/UI
 */
void GetEnemyShakeOffset(const Enemy_t* enemy, float* out_x, float* out_y);

/**
 * GetEnemyRedFlashAlpha - Get current red flash alpha
 *
 * @param enemy - Enemy to query
 * @return float - Red overlay alpha (0.0-1.0)
 *
 * Draw a red filled rect over enemy portrait with this alpha value
 */
float GetEnemyRedFlashAlpha(const Enemy_t* enemy);

/**
 * GetEnemyGreenFlashAlpha - Get current green flash alpha
 *
 * @param enemy - Enemy to query
 * @return float - Green overlay alpha (0.0-1.0)
 *
 * Draw a green filled rect over enemy portrait with this alpha value
 */
float GetEnemyGreenFlashAlpha(const Enemy_t* enemy);

/**
 * TriggerEnemyDefeatAnimation - Trigger fade-out and zoom-out on defeat
 *
 * @param enemy - Enemy that was defeated
 * @param tween_manager - Tween manager to use for animation
 *
 * Creates:
 * - Fade: alpha 1.0 → 0.0 over 1.5s (EASE_OUT_CUBIC)
 * - Zoom out: scale 1.0 → 0.8 over 1.5s (EASE_OUT_CUBIC)
 *
 * Call this when enemy HP reaches 0 in combat
 */
void TriggerEnemyDefeatAnimation(Enemy_t* enemy, TweenManager_t* tween_manager);

/**
 * GetEnemyDefeatAlpha - Get current defeat fade alpha
 *
 * @param enemy - Enemy to query
 * @return float - Defeat alpha (1.0 = visible, 0.0 = invisible)
 */
float GetEnemyDefeatAlpha(const Enemy_t* enemy);

/**
 * GetEnemyDefeatScale - Get current defeat scale
 *
 * @param enemy - Enemy to query
 * @return float - Defeat scale (1.0 = normal, 0.8 = zoomed out)
 */
float GetEnemyDefeatScale(const Enemy_t* enemy);

#endif // ENEMY_H
