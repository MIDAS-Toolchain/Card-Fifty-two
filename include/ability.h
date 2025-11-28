#ifndef ABILITY_H
#define ABILITY_H

#include "Daedalus.h"
#include "statusEffects.h"
#include "game.h"  // For GameEvent_t enum
#include "defs.h"  // For PlayerAction_t enum

// Forward declarations
typedef struct Enemy Enemy_t;
typedef struct GameContext GameContext_t;

// ============================================================================
// EFFECT SYSTEM
// ============================================================================

/**
 * EffectType_t - Types of effects abilities can execute
 */
typedef enum {
    EFFECT_NONE,
    EFFECT_APPLY_STATUS,    // Apply status effect to player
    EFFECT_REMOVE_STATUS,   // Remove status effect from player
    EFFECT_HEAL,            // Restore HP (enemy) or chips (player)
    EFFECT_DAMAGE,          // Deal damage to HP (enemy) or chips (player)
    EFFECT_SHUFFLE_DECK,    // Force deck reshuffle
    EFFECT_DISCARD_HAND,    // Force player to discard current hand
    EFFECT_FORCE_HIT,       // Force player to draw a card
    EFFECT_REVEAL_HOLE,     // Reveal dealer's hole card
    EFFECT_MESSAGE,         // Display flavor text
    EFFECT_MAX
} EffectType_t;

/**
 * EffectTarget_t - Who the effect applies to
 */
typedef enum {
    TARGET_PLAYER,
    TARGET_SELF     // The enemy itself
} EffectTarget_t;

/**
 * AbilityEffect_t - Single effect primitive (value type, stored in dArray_t)
 *
 * Constitutional pattern: Value type with owned dString_t* for message
 */
typedef struct {
    EffectType_t type;
    EffectTarget_t target;

    // Status effect params
    StatusEffect_t status;
    int value;          // Chips to drain/heal, HP to restore, etc.
    int duration;       // Rounds (for status effects)

    // Message text (owned - must be freed)
    dString_t* message;
} AbilityEffect_t;

// ============================================================================
// TRIGGER SYSTEM
// ============================================================================

/**
 * TriggerType_t - When an ability triggers
 */
typedef enum {
    TRIGGER_PASSIVE,            // Always active (stat modifiers, auras)
    TRIGGER_ON_EVENT,           // Fires every time specific event occurs
    TRIGGER_COUNTER,            // Fires after N occurrences of event
    TRIGGER_HP_THRESHOLD,       // Fires when enemy HP% ≤ threshold (one-time)
    TRIGGER_RANDOM,             // % chance on event
    TRIGGER_ON_ACTION,          // Fires on player HIT/STAND/DOUBLE
    TRIGGER_HP_SEGMENT,         // Fires at regular HP intervals (e.g., every 25% lost)
    TRIGGER_DAMAGE_ACCUMULATOR  // Fires based on total damage dealt (ignores healing)
} TriggerType_t;

/**
 * AbilityTrigger_t - Trigger configuration (value type)
 */
typedef struct {
    TriggerType_t type;

    // Event-based triggers
    GameEvent_t event;       // Event to listen for

    // Counter trigger
    int counter_max;         // Max count before trigger (e.g., 5 for "every 5 cards")

    // HP threshold trigger
    float threshold;         // HP% threshold (0.0-1.0)
    bool once;               // true = only trigger once per combat

    // Random trigger
    float chance;            // Probability (0.0-1.0)

    // Action trigger
    PlayerAction_t action;   // Player action to trigger on

    // HP segment trigger (fires at regular intervals)
    int segment_percent;      // Interval size (25 = every 25% HP lost)
    uint8_t segments_triggered;  // Bitmask tracking which thresholds crossed

    // Damage accumulator trigger (fires every X total damage dealt)
    int damage_threshold;     // Damage required per trigger (e.g., 1250)
    int damage_accumulated;   // Runtime: Total damage dealt when last checked
} AbilityTrigger_t;

// ============================================================================
// ABILITY
// ============================================================================

/**
 * Ability_t - Complete ability definition (loaded from DUF)
 *
 * Constitutional pattern: Heap-allocated with owned dString_t* fields
 */
typedef struct {
    dString_t* name;            // Ability name (e.g., "The House Remembers")
    dString_t* description;     // Flavor text

    AbilityTrigger_t trigger;   // When it fires
    dArray_t* effects;          // Array of AbilityEffect_t (stored by value)

    int cooldown_max;           // Cooldown rounds (0 = no cooldown)

    // Runtime state
    int cooldown_current;       // Rounds until can trigger again
    bool has_triggered;         // For one-time triggers
    int counter_current;        // Current count towards counter trigger

    // Animation (for UI feedback when ability triggers)
    float shake_offset_x;
    float shake_offset_y;
    float flash_alpha;
    float fade_alpha;          // Fade-out alpha when consumed (1.0 = visible, 0.0 = hidden)
} Ability_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateAbility - Create new ability with name and description
 *
 * @param name - Ability name
 * @param description - Flavor text
 * @return Ability_t* - Heap-allocated ability (caller must destroy)
 */
Ability_t* CreateAbility(const char* name, const char* description);

/**
 * DestroyAbility - Free ability resources
 *
 * @param ability - Pointer to ability pointer (will be set to NULL)
 *
 * Frees name, description, effects array (including effect messages), and struct
 */
void DestroyAbility(Ability_t** ability);

/**
 * AddEffect - Add effect to ability
 *
 * @param ability - Ability to modify
 * @param effect - Effect to add (copied by value)
 *
 * OWNERSHIP: This function TAKES OWNERSHIP of effect->message (if non-NULL).
 * Do NOT free effect.message after calling - DestroyAbility() will free it.
 */
void AddEffect(Ability_t* ability, const AbilityEffect_t* effect);

// ============================================================================
// EXECUTION
// ============================================================================

/**
 * CheckAbilityTrigger - Test if ability should fire on event
 *
 * @param ability - Ability to check
 * @param event - Game event that occurred
 * @param enemy_hp_percent - Enemy's current HP% (0.0-1.0)
 * @param enemy_total_damage - Enemy's total cumulative damage taken (for TRIGGER_DAMAGE_ACCUMULATOR)
 * @return bool - true if ability should execute
 *
 * Checks trigger type and conditions, updates counters/flags
 */
bool CheckAbilityTrigger(Ability_t* ability, GameEvent_t event, float enemy_hp_percent, int enemy_total_damage);

/**
 * ExecuteAbility - Run all effects in ability
 *
 * @param ability - Ability to execute
 * @param enemy - Enemy that owns the ability
 * @param game - Game context (for accessing deck, player, etc.)
 *
 * Executes effects in sequence, logs ability activation
 */
void ExecuteAbility(Ability_t* ability, Enemy_t* enemy, GameContext_t* game);

/**
 * ExecuteEffect - Execute single effect primitive
 *
 * @param effect - Effect to execute
 * @param enemy - Enemy that owns the ability
 * @param game - Game context
 *
 * Applies effect based on type and target
 */
void ExecuteEffect(const AbilityEffect_t* effect, Enemy_t* enemy, GameContext_t* game);

/**
 * TickAbilityCooldowns - Decrement cooldowns on all abilities
 *
 * @param abilities - Array of Ability_t* (enemy's abilities)
 *
 * Called at round end, decrements all cooldown_current values
 */
void TickAbilityCooldowns(dArray_t* abilities);

/**
 * ResetAbilityStates - Reset runtime state for combat start
 *
 * @param abilities - Array of Ability_t*
 *
 * Clears has_triggered and counter_current for new combat
 */
void ResetAbilityStates(dArray_t* abilities);

// ============================================================================
// STRING CONVERSION (DUF parsing helpers)
// ============================================================================

/**
 * EffectTypeFromString - Convert "apply_status" → EFFECT_APPLY_STATUS
 */
EffectType_t EffectTypeFromString(const char* str);

/**
 * TriggerTypeFromString - Convert "on_event" → TRIGGER_ON_EVENT
 */
TriggerType_t TriggerTypeFromString(const char* str);

/**
 * TargetFromString - Convert "player" → TARGET_PLAYER
 */
EffectTarget_t TargetFromString(const char* str);

/**
 * GameEventFromString - Convert "PLAYER_BLACKJACK" → GAME_EVENT_PLAYER_BLACKJACK
 */
GameEvent_t GameEventFromString(const char* str);

/**
 * StatusEffectFromString - Convert "GREED" → STATUS_GREED
 */
StatusEffect_t StatusEffectFromString(const char* str);

/**
 * PlayerActionFromString - Convert "HIT" → ACTION_HIT
 */
PlayerAction_t PlayerActionFromString(const char* str);

/**
 * PlayerActionToString - Convert ACTION_HIT → "HIT"
 */
const char* PlayerActionToString(PlayerAction_t action);

#endif // ABILITY_H
