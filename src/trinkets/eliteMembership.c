/*
 * Elite Membership Trinket
 * Casino loyalty program - win bonuses + loss protection
 *
 * Theme: VIP rewards for high rollers
 *
 * Implementation: Uses modifier system (ModifyWinningsWithTrinkets/ModifyLossesWithTrinkets)
 * instead of passive_trigger events. Follows same pattern as status effects (ADR-002).
 */

#include "../../include/trinket.h"
#include "../../include/player.h"
#include "../../include/game.h"

// ============================================================================
// REGISTRATION
// ============================================================================

/**
 * Elite Membership Trinket
 *
 * Passive Effects (via modifier system):
 * - Win Bonus: +50% chips on wins (applied in ModifyWinningsWithTrinkets)
 * - Loss Refund: +50% chips back on losses (applied in ModifyLossesWithTrinkets)
 *
 * Stats Tracking:
 * - total_bonus_chips: Cumulative bonus chips won
 * - total_refunded_chips: Cumulative chips refunded on losses
 *
 * No active effect - pure passive trinket
 */
Trinket_t* CreateEliteMembershipTrinket(void) {
    Trinket_t* trinket = CreateTrinketTemplate(
        1,  // trinket_id (Degenerate's Gambit is 0, this is 1)
        "Elite Membership",
        "The casino's loyalty program. Win more, lose less. The house always wins—unless you're VIP.",
        TRINKET_RARITY_RARE  // Rare tier - powerful passive income
    );

    if (!trinket) {
        d_LogError("Failed to create Elite Membership trinket");
        return NULL;
    }

    // Passive setup: Uses modifier system (no passive_trigger needed)
    // Set to COMBAT_START as no-op trigger (won't fire, but required by structure)
    trinket->passive_trigger = GAME_EVENT_COMBAT_START;
    trinket->passive_effect = NULL;  // No callback - uses ModifyWinnings/ModifyLosses instead
    d_StringSet(trinket->passive_description,
                "Win 30% more chips from victories, refund 30% chips on losses", 0);

    // No active effect
    trinket->active_target_type = TRINKET_TARGET_NONE;
    trinket->active_effect = NULL;
    trinket->active_cooldown_max = 0;
    trinket->active_cooldown_current = 0;
    d_StringSet(trinket->active_description, "", 0);

    // Stats initialized to 0 in CreateTrinket():
    // - total_bonus_chips = 0
    // - total_refunded_chips = 0

    d_LogInfo("Created Elite Membership trinket");

    return trinket;
}
