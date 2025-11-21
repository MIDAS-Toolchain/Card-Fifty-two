/*
 * Degenerate's Gambit Trinket
 * Starter trinket for Degenerate class
 *
 * Theme: Risk/reward gambler - high-risk plays grant escalating damage
 */

#include "../../include/trinket.h"
#include "../../include/player.h"
#include "../../include/game.h"
#include "../../include/enemy.h"
#include "../../include/cardTags.h"
#include "../../include/card.h"
#include "../../include/hand.h"                   // For CalculateHandValue
#include "../../include/stats.h"                  // For Stats_RecordDamage
#include "../../include/scenes/sceneBlackjack.h"  // For TweenEnemyHP, GetTweenManager, GetVisualEffects
#include "../../include/scenes/components/visualEffects.h"
#include "../../include/tween/tween.h"            // For TweenFloat, TWEEN_EASE_*

// ============================================================================
// PASSIVE: "Reckless Payoff"
// ============================================================================

/**
 * Trigger: When player draws a card (HIT)
 * Effect: If hand was ALREADY 15+ before the hit, deal damage to enemy
 *
 * Design: Rewards risky high-value hits. Player must be at 15+ BEFORE hitting.
 */
void DegenerateGambitPassive(Player_t* player, GameContext_t* game, Trinket_t* self, size_t slot_index) {
    (void)slot_index;  // Trinkets are heap-allocated, pointers are stable - no need for index-based tweening

    if (!player || !game || !self) {
        return;
    }

    // Only trigger in combat
    if (!game->is_combat_mode || !game->current_enemy) {
        return;
    }

    // Check: Did player HIT while ALREADY at 15+?
    if (player->hand.cards->count < 1) return;  // Need at least 1 card

    // Get the last drawn card
    Card_t* last_card = (Card_t*)d_IndexDataFromArray(player->hand.cards, player->hand.cards->count - 1);
    if (!last_card) return;

    // Calculate value BEFORE the hit (current value - last card value)
    // Cards are worth their rank (1-10), face cards = 10
    int last_card_value = (last_card->rank >= 10) ? 10 : last_card->rank;
    int value_before_hit = player->hand.total_value - last_card_value;

    // Trigger only if we were ALREADY at 15+ before hitting
    if (value_before_hit >= 15) {
        int base_damage = 10 + self->passive_damage_bonus;

        // Apply ALL damage modifiers (ADR-010: Universal damage modifier)
        bool is_crit = false;
        int damage = ApplyPlayerDamageModifiers(player, base_damage, &is_crit);

        TakeDamage(game->current_enemy, damage);

        // Track total damage dealt (trinket-specific counter)
        self->total_damage_dealt += damage;

        // Track trinket passive damage in global stats
        Stats_RecordDamage(DAMAGE_SOURCE_TRINKET_PASSIVE, damage);

        // Trigger visual feedback (matches pattern in game.c:568-576)
        TweenEnemyHP(game->current_enemy);  // Smooth HP bar drain animation
        TweenManager_t* tween_mgr = GetTweenManager();
        if (tween_mgr) {
            TriggerEnemyDamageEffect(game->current_enemy, tween_mgr);  // Shake + red flash on enemy

            // Trigger trinket icon shake + flash
            // SAFE: Class trinket is embedded in Player_t (stable address)
            // NOTE: For regular trinkets in dArray, use TweenFloatInArray() with slot_index
            self->shake_offset_x = 0.0f;
            self->shake_offset_y = 0.0f;
            TweenFloat(tween_mgr, &self->shake_offset_x, 5.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);
            TweenFloat(tween_mgr, &self->shake_offset_y, -3.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);
            self->flash_alpha = 255.0f;
            TweenFloat(tween_mgr, &self->flash_alpha, 0.0f, 0.5f, TWEEN_EASE_OUT_QUAD);
        }

        // Spawn damage number via visual effects component
        VisualEffects_t* vfx = GetVisualEffects();
        if (vfx) {
            VFX_SpawnDamageNumber(vfx, damage,
                                 SCREEN_WIDTH / 2 + ENEMY_HP_BAR_X_OFFSET,
                                 ENEMY_HP_BAR_Y - DAMAGE_NUMBER_Y_OFFSET,
                                 false, is_crit);  // Pass crit flag
        }

        d_LogInfoF("⚡ Degenerate's Gambit (Passive): Reckless hit at %d! Dealt %d damage%s",
                   player->hand.total_value, damage, is_crit ? " (CRIT!)" : "");
    }
}

// ============================================================================
// ACTIVE: "Double Down" (not the blackjack rule)
// ============================================================================

/**
 * Target: Card with rank ≤ 5
 * Effect: That card's value is doubled this hand (CARD_TAG_DOUBLED)
 * Cooldown: 3 turns
 * Side Effect: Increases passive damage by +5 permanently
 *
 * Design: Risky active (need low cards to avoid bust) that permanently scales passive
 */
void DegenerateGambitActive(Player_t* player, GameContext_t* game, void* target, Trinket_t* self, size_t slot_index) {
    (void)slot_index;  // Not needed for active (no animations)
    if (!player || !game || !target || !self) {
        d_LogError("DegenerateGambitActive: NULL parameter");
        return;
    }

    Card_t* card = (Card_t*)target;

    // Validate: Card rank <= 5
    if (card->rank > 5) {
        d_LogWarning("Cannot double card above rank 5");
        return;
    }

    // Add DOUBLED tag to card
    AddCardTag(card->card_id, CARD_TAG_DOUBLED);

    d_LogInfoF("⚡ Degenerate's Gambit (Active): Doubled %s of %s (rank %d)",
               GetRankString(card->rank), GetSuitString(card->suit), card->rank);

    // Recalculate hand values for both player and dealer (since we don't know which hand has the card)
    // This is safe - CalculateHandValue will recalculate based on current cards + tags
    extern Player_t* g_human_player;
    extern Player_t* g_dealer;

    if (g_human_player) {
        int new_value = CalculateHandValue(&g_human_player->hand);
        d_LogInfoF("Player hand recalculated: %d", new_value);
    }

    if (g_dealer) {
        int new_value = CalculateHandValue(&g_dealer->hand);
        d_LogInfoF("Dealer hand recalculated: %d", new_value);
    }

    // Increase passive damage permanently
    self->passive_damage_bonus += 5;
    d_LogInfoF("Passive damage increased to %d", 10 + self->passive_damage_bonus);
}

// ============================================================================
// REGISTRATION
// ============================================================================

Trinket_t* CreateDegenerateGambitTrinket(void) {
    Trinket_t* trinket = CreateTrinketTemplate(
        0,  // trinket_id
        "Degenerate's Gambit",
        "The desperate gambler's last resort. Risk everything for fleeting power.",
        TRINKET_RARITY_CLASS  // Class trinket (purple) - unique to Degenerate class
    );

    if (!trinket) {
        d_LogError("Failed to create Degenerate's Gambit trinket");
        return NULL;
    }

    // Passive setup
    trinket->passive_trigger = GAME_EVENT_CARD_DRAWN;  // Fires when player HITs
    trinket->passive_effect = DegenerateGambitPassive;
    d_StringSet(trinket->passive_description,
                "When you HIT on 15+: Deal 10 damage (+5 per active use)", 0);

    // Active setup
    trinket->active_target_type = TRINKET_TARGET_CARD;
    trinket->active_effect = DegenerateGambitActive;
    trinket->active_cooldown_max = 3;
    trinket->active_cooldown_current = 0;  // Ready on start
    d_StringSet(trinket->active_description,
                "Target a card value 5 or less, double it for this turns hand value. (3 turn cooldown)", 0);

    // State initialization
    trinket->passive_damage_bonus = 0;  // Starts at 10, scales to 15, 20, etc.

    d_LogInfo("Created Degenerate's Gambit trinket");

    return trinket;
}
