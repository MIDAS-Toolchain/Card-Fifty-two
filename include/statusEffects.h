#ifndef STATUS_EFFECTS_H
#define STATUS_EFFECTS_H

#include "common.h"
#include "player.h"

// ============================================================================
// STATUS EFFECT TYPES
// ============================================================================

/**
 * DurationType_t - How status effect duration is decremented
 *
 * Round-based: Decremented by TickStatusEffectDurations() at round end
 * Stack-based: Decremented by effect-specific logic (e.g., ApplyRakeEffect)
 *
 * This distinction allows flexible status effect behavior:
 * - Round-based: Traditional debuffs that last N rounds (CHIP_DRAIN, TILT)
 * - Stack-based: Consumable effects that trigger N times (RAKE, future buffs)
 */
typedef enum {
    DURATION_ROUNDS = 0,  // Decremented at round end (default)
    DURATION_STACKS       // Decremented when effect triggers
} DurationType_t;

/**
 * StatusEffect_t - Types of status effects that can be applied to players
 *
 * IMPORTANT: Status effects are OUTCOME MODIFIERS ONLY (ADR-002)
 * - Modify chip gains/losses after rounds resolve
 * - DO NOT modify betting behavior or restrictions
 * - Betting mechanics controlled by sanity system, not status effects
 *
 * Status effects create persistent pressure through chip manipulation
 * and outcome modifiers during combat.
 */
typedef enum {
    STATUS_NONE = 0,

    // Outcome modifiers (lose chips per round or on wins/losses)
    STATUS_CHIP_DRAIN,      // Lose X chips per round (at round start) [ROUNDS]
    STATUS_TILT,            // Lose 2x chips on loss (outcome modifier) [ROUNDS]
    STATUS_GREED,           // Win 0.5x chips on win (outcome modifier) [ROUNDS]
    STATUS_RAKE,            // Lose X% of damage dealt as chips (per win) [STACKS]

    STATUS_MAX
} StatusEffect_t;

// ============================================================================
// STATUS EFFECT INSTANCE
// ============================================================================

/**
 * StatusEffectInstance_t - Active status effect instance
 *
 * Stores effect type, value (magnitude), duration, and intensity for rendering
 */
typedef struct {
    StatusEffect_t type;
    int value;              // Chips per round, multiplier %, min bet, etc.
    int duration;           // Rounds/stacks remaining (0 = expired)
    DurationType_t duration_type;  // How duration is decremented
    float intensity;        // Visual feedback intensity (0.0-1.0)
    float shake_offset_x;   // Shake animation X offset (tweened)
    float shake_offset_y;   // Shake animation Y offset (tweened)
    float flash_alpha;      // Red flash overlay alpha (tweened, 0-255)
} StatusEffectInstance_t;

// ============================================================================
// STATUS EFFECT MANAGER
// ============================================================================

/**
 * StatusEffectManager_t - Manages active status effects on a player
 *
 * Constitutional pattern: dArray_t for effect storage
 */
typedef struct StatusEffectManager {
    dArray_t* active_effects;  // Array of StatusEffectInstance_t
} StatusEffectManager_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateStatusEffectManager - Initialize status effect manager
 *
 * @return StatusEffectManager_t* - Heap-allocated manager (caller must destroy)
 */
StatusEffectManager_t* CreateStatusEffectManager(void);

/**
 * DestroyStatusEffectManager - Cleanup manager resources
 *
 * @param manager - Pointer to manager pointer (will be set to NULL)
 */
void DestroyStatusEffectManager(StatusEffectManager_t** manager);

// ============================================================================
// EFFECT MANAGEMENT
// ============================================================================

/**
 * ApplyStatusEffect - Apply status effect to player
 *
 * @param manager - Status effect manager
 * @param type - Effect type
 * @param value - Effect magnitude (chips, multiplier, etc.)
 * @param duration - Rounds remaining
 *
 * If effect already exists, refreshes duration and updates value
 */
void ApplyStatusEffect(StatusEffectManager_t* manager,
                      StatusEffect_t type,
                      int value,
                      int duration);

/**
 * RemoveStatusEffect - Remove specific status effect
 *
 * @param manager - Status effect manager
 * @param type - Effect type to remove
 */
void RemoveStatusEffect(StatusEffectManager_t* manager, StatusEffect_t type);

/**
 * HasStatusEffect - Check if player has specific effect active
 *
 * @param manager - Status effect manager
 * @param type - Effect type to check
 * @return bool - true if effect is active
 */
bool HasStatusEffect(const StatusEffectManager_t* manager, StatusEffect_t type);

/**
 * GetStatusEffect - Get active status effect instance
 *
 * WARNING: Returned pointer may become invalid if array reallocates!
 * For tweening, use GetStatusEffectIndex() + TweenFloatInArray() instead.
 *
 * @param manager - Status effect manager
 * @param type - Effect type to get
 * @return StatusEffectInstance_t* - Effect instance, or NULL if not active
 */
StatusEffectInstance_t* GetStatusEffect(StatusEffectManager_t* manager, StatusEffect_t type);

/**
 * GetStatusEffectIndex - Get index of active status effect
 *
 * SAFE for tweening - returns index that can be used with TweenFloatInArray()
 *
 * @param manager - Status effect manager
 * @param type - Effect type to find
 * @return ssize_t - Index in active_effects array, or -1 if not found
 */
ssize_t GetStatusEffectIndex(StatusEffectManager_t* manager, StatusEffect_t type);

// ============================================================================
// ROUND PROCESSING
// ============================================================================

/**
 * ProcessStatusEffectsRoundStart - Apply round-start effects (chip drain)
 *
 * @param manager - Status effect manager
 * @param player - Player to apply effects to
 *
 * Called at start of round (after betting, before dealing)
 * Processes: CHIP_DRAIN (lose X chips)
 */
void ProcessStatusEffectsRoundStart(StatusEffectManager_t* manager, Player_t* player);

/**
 * ProcessStatusEffectsRoundEnd - Apply round-end effects
 *
 * @param manager - Status effect manager
 * @param player - Player to apply effects to
 *
 * Called at end of round (after payouts)
 * Currently unused but reserved for future effects
 */
void ProcessStatusEffectsRoundEnd(StatusEffectManager_t* manager, Player_t* player);

/**
 * TickStatusEffectDurations - Decrement effect durations
 *
 * @param manager - Status effect manager
 *
 * Called at round end, decrements all durations and removes expired effects
 */
void TickStatusEffectDurations(StatusEffectManager_t* manager);

/**
 * ClearAllStatusEffects - Remove all active status effects
 *
 * @param manager - Status effect manager
 * @return size_t - Number of effects that were cleared
 *
 * Called on combat victory to ensure victory screen shows clean rewards
 * without chip drain from status effects
 */
size_t ClearAllStatusEffects(StatusEffectManager_t* manager);

// ============================================================================
// OUTCOME MODIFIERS (ADR-002: Status effects modify outcomes only)
// ============================================================================
//
// NOTE: Betting modifiers REMOVED - status effects do NOT control betting!
// Betting behavior is controlled by the sanity system, not status effects.
// Status effects ONLY modify chip outcomes after rounds resolve.
//

/**
 * ModifyWinnings - Apply effect-based winning modifiers
 *
 * @param manager - Status effect manager
 * @param base_winnings - Base winning amount
 * @param bet_amount - Original bet amount
 * @return int - Modified winnings
 *
 * Checks: STATUS_GREED (caps winnings at 50% of bet, not 50% of winnings)
 */
int ModifyWinnings(const StatusEffectManager_t* manager, int base_winnings, int bet_amount);

/**
 * ModifyLosses - Apply effect-based loss modifiers
 *
 * @param manager - Status effect manager
 * @param base_loss - Base loss amount
 * @return int - Modified loss (additional penalty)
 *
 * Checks: STATUS_TILT (doubles loss penalty)
 * Returns ADDITIONAL loss on top of base (not total)
 */
int ModifyLosses(const StatusEffectManager_t* manager, int base_loss);

/**
 * ApplyRakeEffect - Process RAKE status on round win (ADR-002 compliant)
 *
 * Called during round resolution when player deals damage to enemy.
 * Calculates chip penalty based on damage dealt, consumes 1 stack.
 *
 * @param manager - Player's status effect manager
 * @param player - Player to deduct chips from
 * @param damage_dealt - Damage dealt this round (for percentage calc)
 * @return int - Chip penalty amount (for UI feedback)
 *
 * RAKE behavior:
 * - Penalty = (damage_dealt × value) / 100 (minimum 1 chip)
 * - Consumes 1 stack (duration decrements)
 * - Removed when duration reaches 0
 * - Only triggers on winning rounds (damage > 0)
 */
int ApplyRakeEffect(StatusEffectManager_t* manager, Player_t* player, int damage_dealt);

// ============================================================================
// UI/QUERY FUNCTIONS
// ============================================================================

/**
 * GetStatusEffectName - Get human-readable effect name
 *
 * @param type - Effect type
 * @return const char* - Effect name
 */
const char* GetStatusEffectName(StatusEffect_t type);

/**
 * GetStatusEffectAbbreviation - Get 2-letter abbreviation for icon
 *
 * @param type - Effect type
 * @return const char* - 2-letter abbreviation (e.g., "Cd" for Chip Drain)
 */
const char* GetStatusEffectAbbreviation(StatusEffect_t type);

/**
 * GetStatusEffectDescription - Get effect description
 *
 * @param type - Effect type
 * @return const char* - Effect description
 */
const char* GetStatusEffectDescription(StatusEffect_t type);

/**
 * GetStatusEffectColor - Get UI color for effect type
 *
 * @param type - Effect type
 * @return aColor_t - RGB color for effect icon/text
 */
aColor_t GetStatusEffectColor(StatusEffect_t type);

/**
 * RenderStatusEffects - Render active status effect icons
 *
 * @param manager - Status effect manager
 * @param x - X position (top-left)
 * @param y - Y position (top-left)
 *
 * Draws icons with duration counters and value indicators
 */
void RenderStatusEffects(const StatusEffectManager_t* manager, int x, int y);

#endif // STATUS_EFFECTS_H
