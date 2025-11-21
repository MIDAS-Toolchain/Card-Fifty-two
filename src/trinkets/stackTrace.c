/*
 * Stack Trace Trinket
 * Reward from System Maintenance event
 *
 * Theme: Debugging - every crash leaves a trace you can weaponize
 */

#include "../../include/trinket.h"
#include "../../include/player.h"
#include "../../include/game.h"
#include "../../include/enemy.h"
#include "../../include/stats.h"                  // For Stats_RecordDamage
#include "../../include/scenes/sceneBlackjack.h"  // For TweenEnemyHP, GetTweenManager, GetVisualEffects
#include "../../include/scenes/components/visualEffects.h"
#include "../../include/tween/tween.h"            // For TweenFloat, TWEEN_EASE_*

// ============================================================================
// PASSIVE: "Error Handler"
// ============================================================================

/**
 * Trigger: When player BUSTS (goes over 21)
 * Effect: Deal 15 damage to enemy
 *
 * Design: Turns a bad outcome into partial value - "every crash gives you data"
 */
static void StackTracePassive(Player_t* player, GameContext_t* game, Trinket_t* self, size_t slot_index) {
    (void)slot_index;  // Class trinket is embedded, address is stable

    if (!player || !game || !self) {
        return;
    }

    // Only trigger in combat
    if (!game->is_combat_mode || !game->current_enemy) {
        return;
    }

    // Verify player actually busted
    if (!player->hand.is_bust) {
        return;
    }

    // Deal damage
    int damage = 15;

    // Apply damage to enemy
    game->current_enemy->current_hp -= damage;
    if (game->current_enemy->current_hp < 0) {
        game->current_enemy->current_hp = 0;
    }

    // Track total damage for trinket stats
    self->total_damage_dealt += damage;

    // Record for stats system
    Stats_RecordDamage(damage, DAMAGE_SOURCE_TRINKET_PASSIVE);

    // Trigger visual feedback
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
                             false, false);  // Not healing, not crit
    }

    d_LogInfoF("📊 Stack Trace (Passive): Bust triggered! Dealt %d damage to enemy", damage);
}

// ============================================================================
// REGISTRATION
// ============================================================================

Trinket_t* CreateStackTraceTrinket(void) {
    Trinket_t* trinket = CreateTrinketTemplate(
        2,  // trinket_id (Degenerate's Gambit = 0, Elite Membership = 1, Stack Trace = 2)
        "Stack Trace",
        "Every crash leaves a trace. You've learned to read them—and weaponize them.",
        TRINKET_RARITY_UNCOMMON  // Uncommon tier - event reward
    );

    if (!trinket) {
        d_LogError("Failed to create Stack Trace trinket");
        return NULL;
    }

    // Passive setup
    trinket->passive_trigger = GAME_EVENT_PLAYER_BUST;  // Fires when player busts
    trinket->passive_effect = StackTracePassive;
    d_StringSet(trinket->passive_description,
                "When you BUST, deal 15 damage to enemy", 0);

    // No active effect
    trinket->active_target_type = TRINKET_TARGET_NONE;
    trinket->active_effect = NULL;
    trinket->active_cooldown_max = 0;
    d_StringSet(trinket->active_description, "", 0);

    d_LogInfo("Created Stack Trace trinket template");
    return trinket;
}
