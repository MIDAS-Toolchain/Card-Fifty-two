#ifndef ENEMY_H
#define ENEMY_H

#include "common.h"
#include <SDL2/SDL.h>

// Forward declarations
typedef struct TweenManager TweenManager_t;

// ============================================================================
// ENEMY ABILITIES
// ============================================================================

/**
 * EnemyAbility_t - Types of dealer abilities
 */
typedef enum {
    // Passive Abilities (always active)
    ABILITY_HOUSE_ALWAYS_WINS,      // Dealer wins all ties
    ABILITY_CARD_COUNTER,            // Player's hole card revealed
    ABILITY_LOADED_DECK,             // Dealer's face-down is always 10-value
    ABILITY_CHIP_DRAIN,              // Player loses 5 chips per round
    ABILITY_PRESSURE,                // Sanity drains faster during combat

    // Active Abilities (triggered by conditions)
    ABILITY_DOUBLE_OR_NOTHING,       // At 50% HP: Forces player to double or auto-lose
    ABILITY_RESHUFFLE_REALITY,       // Once per fight: Replaces player's hand
    ABILITY_HOUSE_RULES,             // Phase 2: Changes blackjack rules mid-fight
    ABILITY_ALL_IN,                  // At 25% HP: Both bet all chips
    ABILITY_GLITCH,                  // Random: Dealer's bust becomes 21

    ABILITY_MAX
} EnemyAbility_t;

/**
 * AbilityTrigger_t - When an active ability triggers
 */
typedef enum {
    TRIGGER_NONE,               // Passive ability (no trigger)
    TRIGGER_HP_THRESHOLD,       // Triggers at specific HP percentage
    TRIGGER_ONCE_PER_COMBAT,    // Can only trigger once per fight
    TRIGGER_RANDOM,             // Random chance each round
    TRIGGER_PLAYER_ACTION       // Triggers on player action
} AbilityTrigger_t;

/**
 * AbilityData_t - Ability configuration
 */
typedef struct AbilityData {
    EnemyAbility_t ability_id;
    AbilityTrigger_t trigger;
    float trigger_value;        // HP threshold (0.0-1.0) or random chance
    bool has_triggered;         // Track one-time abilities
} AbilityData_t;

// ============================================================================
// ENEMY STRUCTURE
// ============================================================================

/**
 * Enemy_t - Combat enemy entity
 *
 * Constitutional pattern:
 * - dString_t for name (not char[])
 * - dArray_t for abilities (not raw array)
 * - Integrates with existing chip/bet system
 */
typedef struct Enemy {
    dString_t* name;                // Enemy name (e.g., "The Broken Slot Machine")
    int max_hp;                     // Maximum HP
    int current_hp;                 // Current HP (combat ends at 0)
    float display_hp;               // Displayed HP (tweened for smooth HP bar drain)
    int chip_threat;                // Chips at risk per round (damage source)
    dArray_t* passive_abilities;    // Array of AbilityData_t (passive abilities)
    dArray_t* active_abilities;     // Array of AbilityData_t (active abilities)

    // Portrait system (hybrid for dynamic effects)
    SDL_Surface* portrait_surface;  // Source pixel data (owned, for manipulation)
    SDL_Texture* portrait_texture;  // Cached GPU texture (owned, for rendering)
    bool portrait_dirty;            // true if surface changed, needs texture rebuild

    // Damage feedback effects (shake + blink)
    float shake_offset_x;           // Horizontal shake offset (tweened)
    float shake_offset_y;           // Vertical shake offset (tweened)
    float red_flash_alpha;          // Red overlay alpha (tweened from 1.0 to 0.0)

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
 * @param chip_threat - Chips at risk per round (used for damage calculation)
 * @return Enemy_t* - Heap-allocated enemy, or NULL on failure
 *
 * Constitutional pattern: Heap-allocated entity (like Player_t)
 * Caller must call DestroyEnemy() when done
 */
Enemy_t* CreateEnemy(const char* name, int max_hp, int chip_threat);

/**
 * DestroyEnemy - Free enemy resources
 *
 * @param enemy - Pointer to enemy pointer (double pointer for nulling)
 *
 * Destroys internal dString_t and dArray_t, then frees enemy struct
 */
void DestroyEnemy(Enemy_t** enemy);

// ============================================================================
// ABILITY MANAGEMENT
// ============================================================================

/**
 * AddPassiveAbility - Add a passive ability to enemy
 *
 * @param enemy - Enemy to modify
 * @param ability - Ability ID to add
 *
 * Passive abilities are always active (no trigger condition)
 */
void AddPassiveAbility(Enemy_t* enemy, EnemyAbility_t ability);

/**
 * AddActiveAbility - Add an active ability with trigger
 *
 * @param enemy - Enemy to modify
 * @param ability - Ability ID to add
 * @param trigger - Trigger type
 * @param trigger_value - HP threshold or random chance (0.0-1.0)
 */
void AddActiveAbility(Enemy_t* enemy, EnemyAbility_t ability,
                      AbilityTrigger_t trigger, float trigger_value);

/**
 * HasAbility - Check if enemy has a specific ability
 *
 * @param enemy - Enemy to check
 * @param ability - Ability ID to search for
 * @return true if ability exists (passive or active)
 */
bool HasAbility(const Enemy_t* enemy, EnemyAbility_t ability);

/**
 * GetAbilityName - Convert ability enum to string
 *
 * @param ability - Ability ID
 * @return const char* - Ability name
 */
const char* GetAbilityName(EnemyAbility_t ability);

// ============================================================================
// COMBAT ACTIONS
// ============================================================================

/**
 * EnemyPlaceBet - Enemy places bet for this round
 *
 * @param enemy - Enemy entity
 * @return int - Bet amount (chip_threat + modifiers)
 *
 * Base bet = chip_threat (chips at risk this round)
 * May be modified by abilities or HP thresholds
 */
int EnemyPlaceBet(const Enemy_t* enemy);

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
 *
 * Clamps HP to max_hp
 */
void HealEnemy(Enemy_t* enemy, int amount);

/**
 * GetEnemyHPPercent - Get HP as percentage
 *
 * @param enemy - Enemy to query
 * @return float - HP percentage (0.0-1.0)
 */
float GetEnemyHPPercent(const Enemy_t* enemy);

// ============================================================================
// ABILITY TRIGGERS
// ============================================================================

/**
 * CheckAbilityTriggers - Check if any active abilities should trigger
 *
 * @param enemy - Enemy to check
 *
 * Checks HP thresholds, random chances, and sets has_triggered flags
 * Called at start of each combat round
 */
void CheckAbilityTriggers(Enemy_t* enemy);

/**
 * ResetAbilityTriggers - Reset one-time ability flags
 *
 * @param enemy - Enemy to reset
 *
 * Called when combat ends or enemy is defeated
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

#endif // ENEMY_H
