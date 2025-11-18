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
    // NOTE: None of these should affect betting UI/options - only sanity affects betting
    ABILITY_DOUBLE_OR_NOTHING,       // At 50% HP: TODO - needs non-betting implementation
    ABILITY_RESHUFFLE_REALITY,       // Once per fight: Replaces player's hand
    ABILITY_HOUSE_RULES,             // Phase 2: Changes blackjack rules mid-fight (win conditions, not betting)
    ABILITY_ALL_IN,                  // At 25% HP: TODO - needs non-betting implementation
    ABILITY_GLITCH,                  // Random: Dealer's bust becomes 21

    // The Didact Abilities (Tutorial Enemy - Dread-themed names)
    ABILITY_THE_HOUSE_REMEMBERS,     // On player blackjack: Apply STATUS_GREED (2 rounds) - "The casino is alive"
    ABILITY_IRREGULARITY_DETECTED,   // Every 5 cards drawn: Apply STATUS_CHIP_DRAIN (5 chips, 3 rounds) - "You've been noticed"
    ABILITY_SYSTEM_OVERRIDE,         // Below 30% HP (once): Heal 50 HP + Force deck shuffle - "System recalibrates"

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
    TRIGGER_PLAYER_ACTION,      // Triggers on player action
    TRIGGER_ON_EVENT,           // Triggers on specific game event
    TRIGGER_COUNTER             // Triggers after N occurrences of an event
} AbilityTrigger_t;

/**
 * AbilityData_t - Ability configuration
 */
typedef struct AbilityData {
    EnemyAbility_t ability_id;
    AbilityTrigger_t trigger;
    float trigger_value;        // HP threshold (0.0-1.0) or random chance
    bool has_triggered;         // Track one-time abilities

    // Event-based trigger data
    GameEvent_t trigger_event;  // Event to listen for (TRIGGER_ON_EVENT/TRIGGER_COUNTER)
    int counter_max;            // Max count for TRIGGER_COUNTER (e.g., 5 for "every 5 cards")
    int counter_current;        // Current count towards trigger

    // Animation state (for shake/flash on trigger)
    float shake_offset_x;       // X shake offset (tweened)
    float shake_offset_y;       // Y shake offset (tweened)
    float flash_alpha;          // Red flash alpha (tweened)
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
 * @param chip_threat - Chips at risk per round (used for damage calculation)
 * @return Enemy_t* - Heap-allocated enemy, or NULL on failure
 *
 * Constitutional pattern: Heap-allocated entity (like Player_t)
 * Caller must call DestroyEnemy() when done
 */
Enemy_t* CreateEnemy(const char* name, int max_hp, int chip_threat);

/**
 * CreateTheDidact - Create tutorial enemy "The Didact"
 *
 * @return Enemy_t* - Preconfigured tutorial enemy with 3 teaching abilities
 *
 * Theme: Teacher/instructor who introduces mechanics through abilities:
 * - House Rules: On blackjack → GREED (teaches status effects)
 * - Pop Quiz: Every 5 cards → CHIP_DRAIN (teaches counters + pressure)
 * - Final Exam: Below 30% HP → MIN_BET + NO_ADJUST (teaches restrictions)
 */
Enemy_t* CreateTheDidact(void);

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
 * AddEventAbility - Add an ability that triggers on a specific game event
 *
 * @param enemy - Enemy to modify
 * @param ability - Ability ID to add
 * @param event - Game event to trigger on
 *
 * Triggers every time the specified event occurs
 */
void AddEventAbility(Enemy_t* enemy, EnemyAbility_t ability, GameEvent_t event);

/**
 * AddCounterAbility - Add an ability that triggers after N occurrences of an event
 *
 * @param enemy - Enemy to modify
 * @param ability - Ability ID to add
 * @param event - Game event to count
 * @param counter_max - Number of occurrences before trigger (e.g., 5 for "every 5 cards")
 *
 * Counter resets after each trigger
 */
void AddCounterAbility(Enemy_t* enemy, EnemyAbility_t ability, GameEvent_t event, int counter_max);

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
 * CheckEnemyAbilityTriggers - Check if any active abilities should trigger on event
 *
 * @param enemy - Enemy to check
 * @param event - Game event that occurred
 * @param game - Game context (for effect execution)
 *
 * Checks if abilities trigger on this event, executes effects if triggered
 * Called by Game_TriggerEvent() when gameplay events occur
 */
void CheckEnemyAbilityTriggers(Enemy_t* enemy, GameEvent_t event, GameContext_t* game);

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
