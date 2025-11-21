/*
 * Sanity Threshold System Implementation
 * Class-specific gameplay modifiers based on player sanity level
 */

#include "../include/sanityThreshold.h"
#include "../include/player.h"
#include "../include/scenes/sceneBlackjack.h"  // For BET_AMOUNT constants

// ============================================================================
// SANITY TIER CALCULATION
// ============================================================================

SanityTier_t GetSanityTier(const Player_t* player) {
    if (!player) return SANITY_TIER_HIGH;

    float sanity_percent = GetPlayerSanityPercent(player);

    if (sanity_percent == 0.0f) {
        return SANITY_TIER_ZERO;  // Exactly 0%
    } else if (sanity_percent <= 0.25f) {
        return SANITY_TIER_VERY_LOW;  // 1-25%
    } else if (sanity_percent <= 0.50f) {
        return SANITY_TIER_LOW;  // 26-50%
    } else if (sanity_percent <= 0.75f) {
        return SANITY_TIER_MEDIUM;  // 51-75%
    } else {
        return SANITY_TIER_HIGH;  // 76-100%
    }
}

// ============================================================================
// THRESHOLD DESCRIPTIONS
// ============================================================================

const char* GetSanityThresholdDescription(PlayerClass_t class, SanityTier_t tier) {
    // Tier 0 (100-76%) always returns "No effect" for all classes
    if (tier == SANITY_TIER_HIGH) {
        return "No effect";
    }

    switch (class) {
        case PLAYER_CLASS_DEGENERATE:
            switch (tier) {
                case SANITY_TIER_MEDIUM:    return "Lose access to Minimum Bet";
                case SANITY_TIER_LOW:       return "Maximum bet is doubled";
                case SANITY_TIER_VERY_LOW:  return "Lose access to Medium Bet";
                case SANITY_TIER_ZERO:      return "All or nothing. Maximum bet is doubled again";
                default:                    return "No effect";
            }

        case PLAYER_CLASS_DEALER:
            switch (tier) {
                case SANITY_TIER_MEDIUM:    return "Lose access to Maximum Bet";
                case SANITY_TIER_LOW:       return "Enemy face-down card is always visible";
                case SANITY_TIER_VERY_LOW:  return "Draw 3 cards on turn start, if bust, discard highest card";
                case SANITY_TIER_ZERO:      return "Lose control. Auto-hit on 16 or less. Auto-stand on 17 or above";
                default:                    return "No effect";
            }

        case PLAYER_CLASS_DETECTIVE:
            switch (tier) {
                case SANITY_TIER_MEDIUM:    return "Can only do Medium bets";
                case SANITY_TIER_LOW:       return "Medium bet amount = enemy's score from last round";
                case SANITY_TIER_VERY_LOW:  return "Must hit if your total is lower than enemy's visible card value";
                case SANITY_TIER_ZERO:      return "Pairs in hand count double for damage calculation";
                default:                    return "No effect";
            }

        case PLAYER_CLASS_DREAMER:
            return "[Dreamer class not implemented yet]";

        default:
            return "[Unknown class]";
    }
}

// ============================================================================
// DEGENERATE CLASS SANITY EFFECTS
// ============================================================================

void ApplyDegenerateSanityToBetting(const Player_t* player,
                                    int bet_amounts[3],
                                    bool button_enabled[3]) {
    if (!player || !bet_amounts || !button_enabled) return;

    // Initialize with default values
    bet_amounts[0] = BET_AMOUNT_MIN;  // MIN = 1
    bet_amounts[1] = BET_AMOUNT_MED;  // MED = 5
    bet_amounts[2] = BET_AMOUNT_MAX;  // MAX = 10
    button_enabled[0] = true;
    button_enabled[1] = true;
    button_enabled[2] = true;

    SanityTier_t tier = GetSanityTier(player);

    // Apply cumulative effects based on tier
    // Each tier includes all previous tier effects

    // TIER 1 (75-51%): Disable MIN bet
    if (tier >= SANITY_TIER_MEDIUM) {
        button_enabled[0] = false;  // MIN disabled
        d_LogDebugF("Degenerate sanity tier %d: MIN bet disabled", tier);
    }

    // TIER 2 (50-26%): MAX bet doubled (10 → 20)
    if (tier >= SANITY_TIER_LOW) {
        bet_amounts[2] = BET_AMOUNT_MAX * 2;  // MAX = 20
        d_LogDebugF("Degenerate sanity tier %d: MAX bet doubled to %d", tier, bet_amounts[2]);
    }

    // TIER 3 (25-1%): Disable MED bet
    if (tier >= SANITY_TIER_VERY_LOW) {
        button_enabled[1] = false;  // MED disabled
        d_LogDebugF("Degenerate sanity tier %d: MED bet disabled", tier);
    }

    // TIER 4 (0%): MAX bet doubled again (20 → 40)
    if (tier == SANITY_TIER_ZERO) {
        bet_amounts[2] = BET_AMOUNT_MAX * 4;  // MAX = 40 (doubled twice)
        d_LogDebugF("Degenerate sanity tier %d: MAX bet doubled again to %d", tier, bet_amounts[2]);
    }
}

// ============================================================================
// DEALER CLASS SANITY EFFECTS (placeholder for future)
// ============================================================================

void ApplyDealerSanityToBetting(const Player_t* player,
                                int bet_amounts[3],
                                bool button_enabled[3]) {
    if (!player || !bet_amounts || !button_enabled) return;

    // Initialize with default values
    bet_amounts[0] = BET_AMOUNT_MIN;
    bet_amounts[1] = BET_AMOUNT_MED;
    bet_amounts[2] = BET_AMOUNT_MAX;
    button_enabled[0] = true;
    button_enabled[1] = true;
    button_enabled[2] = true;

    SanityTier_t tier = GetSanityTier(player);

    // TIER 1 (75-51%): Disable MAX bet
    if (tier >= SANITY_TIER_MEDIUM) {
        button_enabled[2] = false;  // MAX disabled
    }

    // TODO: Implement other tiers when Dealer class is ready
    // TIER 2: Enemy face-down card visible (not betting-related)
    // TIER 3: Draw 3 cards logic (not betting-related)
    // TIER 4: Auto-play logic (not betting-related)
}

// ============================================================================
// DETECTIVE CLASS SANITY EFFECTS (placeholder for future)
// ============================================================================

void ApplyDetectiveSanityToBetting(const Player_t* player,
                                   int bet_amounts[3],
                                   bool button_enabled[3]) {
    if (!player || !bet_amounts || !button_enabled) return;

    // Initialize with default values
    bet_amounts[0] = BET_AMOUNT_MIN;
    bet_amounts[1] = BET_AMOUNT_MED;
    bet_amounts[2] = BET_AMOUNT_MAX;
    button_enabled[0] = true;
    button_enabled[1] = true;
    button_enabled[2] = true;

    SanityTier_t tier = GetSanityTier(player);

    // TIER 1 (75-51%): Can only do MED bets
    if (tier >= SANITY_TIER_MEDIUM) {
        button_enabled[0] = false;  // MIN disabled
        button_enabled[2] = false;  // MAX disabled
    }

    // TODO: Implement other tiers when Detective class is ready
    // TIER 2: MED bet = enemy's last score (dynamic value)
    // TIER 3: Must hit logic (not betting-related)
    // TIER 4: Pairs count double (not betting-related)
}

// ============================================================================
// UNIVERSAL SANITY APPLICATION
// ============================================================================

void ApplySanityEffectsToBetting(const Player_t* player,
                                 int bet_amounts[3],
                                 bool button_enabled[3]) {
    if (!player || !bet_amounts || !button_enabled) {
        d_LogError("ApplySanityEffectsToBetting: NULL parameter");
        return;
    }

    // Dispatch to class-specific handler
    switch (player->class) {
        case PLAYER_CLASS_DEGENERATE:
            ApplyDegenerateSanityToBetting(player, bet_amounts, button_enabled);
            break;

        case PLAYER_CLASS_DEALER:
            ApplyDealerSanityToBetting(player, bet_amounts, button_enabled);
            break;

        case PLAYER_CLASS_DETECTIVE:
            ApplyDetectiveSanityToBetting(player, bet_amounts, button_enabled);
            break;

        case PLAYER_CLASS_DREAMER:
            // TODO: Implement when Dreamer class is ready
            // Default: all buttons enabled, no modifications
            bet_amounts[0] = BET_AMOUNT_MIN;
            bet_amounts[1] = BET_AMOUNT_MED;
            bet_amounts[2] = BET_AMOUNT_MAX;
            button_enabled[0] = true;
            button_enabled[1] = true;
            button_enabled[2] = true;
            break;

        default:
            // Unknown class - default behavior
            d_LogWarningF("ApplySanityEffectsToBetting: Unknown class %d", player->class);
            bet_amounts[0] = BET_AMOUNT_MIN;
            bet_amounts[1] = BET_AMOUNT_MED;
            bet_amounts[2] = BET_AMOUNT_MAX;
            button_enabled[0] = true;
            button_enabled[1] = true;
            button_enabled[2] = true;
            break;
    }
}
