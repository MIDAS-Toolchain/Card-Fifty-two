#ifndef SANITY_THRESHOLD_H
#define SANITY_THRESHOLD_H

#include "common.h"
#include "player.h"

// ============================================================================
// SANITY THRESHOLD SYSTEM
// ============================================================================

/**
 * Sanity Threshold System - Class-specific gameplay modifiers based on sanity
 *
 * Each class has unique sanity threshold effects that activate at different
 * sanity levels (75%, 50%, 25%, 0%). Effects modify betting, actions, and
 * gameplay mechanics to create psychological horror/deterioration feedback.
 *
 * Constitutional pattern: Separate from status effects (those are temporary,
 * these are permanent class traits based on current sanity)
 */

// ============================================================================
// SANITY TIER ENUM
// ============================================================================

/**
 * SanityTier_t - Sanity percentage tier (determines which threshold is active)
 */
typedef enum {
    SANITY_TIER_HIGH   = 0,  // 100-76% sanity (no penalties)
    SANITY_TIER_MEDIUM = 1,  // 75-51% sanity  (first threshold)
    SANITY_TIER_LOW    = 2,  // 50-26% sanity  (second threshold)
    SANITY_TIER_VERY_LOW = 3, // 25-1% sanity   (third threshold)
    SANITY_TIER_ZERO   = 4   // 0% sanity      (final threshold)
} SanityTier_t;

// ============================================================================
// SANITY TIER QUERIES
// ============================================================================

/**
 * GetSanityTier - Get current sanity tier for player
 *
 * @param player - Player to query
 * @return SanityTier_t - Current tier (0-4)
 *
 * Thresholds:
 * - SANITY_TIER_HIGH:      76-100%
 * - SANITY_TIER_MEDIUM:    51-75%
 * - SANITY_TIER_LOW:       26-50%
 * - SANITY_TIER_VERY_LOW:  1-25%
 * - SANITY_TIER_ZERO:      0%
 */
SanityTier_t GetSanityTier(const Player_t* player);

/**
 * GetSanityThresholdDescription - Get description text for threshold tier
 *
 * @param class - Player class
 * @param tier - Sanity tier (0-4)
 * @return const char* - Description of threshold effect
 *
 * Returns class-specific threshold text for UI display.
 * SANITY_TIER_HIGH (tier 0) returns "No effect" for all classes.
 */
const char* GetSanityThresholdDescription(PlayerClass_t class, SanityTier_t tier);

// ============================================================================
// DEGENERATE CLASS EFFECTS
// ============================================================================

/**
 * ApplyDegenerateSanityToBetting - Modify bet amounts based on Degenerate sanity
 *
 * @param player - Degenerate player
 * @param bet_amounts - Array of 3 bet amounts [MIN, MED, MAX] to modify
 * @param button_enabled - Array of 3 bools [MIN, MED, MAX] to set enabled state
 *
 * The Degenerate's sanity effects modify betting:
 * - TIER 0 (100-76%): No effect
 * - TIER 1 (75-51%):  Disable MIN bet
 * - TIER 2 (50-26%):  MAX bet doubled (20)
 * - TIER 3 (25-1%):   Disable MED bet
 * - TIER 4 (0%):      MAX bet doubled again (40)
 *
 * Effects are CUMULATIVE - tier 4 has all previous tier effects applied.
 */
void ApplyDegenerateSanityToBetting(const Player_t* player,
                                    int bet_amounts[3],
                                    bool button_enabled[3]);

// ============================================================================
// DEALER CLASS EFFECTS (future implementation)
// ============================================================================

/**
 * ApplyDealerSanityToBetting - Modify bet amounts based on Dealer sanity
 *
 * @param player - Dealer player
 * @param bet_amounts - Array of 3 bet amounts [MIN, MED, MAX] to modify
 * @param button_enabled - Array of 3 bools [MIN, MED, MAX] to set enabled state
 *
 * The Dealer's sanity effects:
 * - TIER 0 (100-76%): No effect
 * - TIER 1 (75-51%):  Disable MAX bet
 * - TIER 2 (50-26%):  Enemy face-down card visible (not betting)
 * - TIER 3 (25-1%):   Draw 3 cards, discard highest if bust (not betting)
 * - TIER 4 (0%):      Auto-play (not betting)
 *
 * TODO: Implement when Dealer class is ready
 */
void ApplyDealerSanityToBetting(const Player_t* player,
                                int bet_amounts[3],
                                bool button_enabled[3]);

// ============================================================================
// DETECTIVE CLASS EFFECTS (future implementation)
// ============================================================================

/**
 * ApplyDetectiveSanityToBetting - Modify bet amounts based on Detective sanity
 *
 * @param player - Detective player
 * @param bet_amounts - Array of 3 bet amounts [MIN, MED, MAX] to modify
 * @param button_enabled - Array of 3 bools [MIN, MED, MAX] to set enabled state
 *
 * The Detective's sanity effects:
 * - TIER 0 (100-76%): No effect
 * - TIER 1 (75-51%):  Can only do MED bets
 * - TIER 2 (50-26%):  MED bet = enemy's last score
 * - TIER 3 (25-1%):   Must hit if lower than enemy visible
 * - TIER 4 (0%):      Pairs count double for damage
 *
 * TODO: Implement when Detective class is ready
 */
void ApplyDetectiveSanityToBetting(const Player_t* player,
                                   int bet_amounts[3],
                                   bool button_enabled[3]);

// ============================================================================
// UNIVERSAL SANITY EFFECT APPLICATION
// ============================================================================

/**
 * ApplySanityEffectsToBetting - Apply class-specific sanity effects to betting
 *
 * @param player - Player to apply effects for
 * @param bet_amounts - Array of 3 bet amounts [MIN, MED, MAX] to modify
 * @param button_enabled - Array of 3 bools [MIN, MED, MAX] to set enabled state
 *
 * Dispatches to class-specific sanity application functions.
 * Defaults all buttons to enabled if class not implemented yet.
 */
void ApplySanityEffectsToBetting(const Player_t* player,
                                 int bet_amounts[3],
                                 bool button_enabled[3]);

#endif // SANITY_THRESHOLD_H
