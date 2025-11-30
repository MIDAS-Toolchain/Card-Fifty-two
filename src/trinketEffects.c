/*
 * Trinket Effect Execution System
 *
 * Executes passive effects from DUF-loaded trinkets based on TrinketEffectType_t.
 * Called by CheckTrinketPassiveTriggers() when game events match trinket triggers.
 */

#include "../include/trinketEffects.h"
#include "../include/player.h"
#include "../include/enemy.h"                  // For Enemy_t, TriggerEnemyDamageEffect
#include "../include/statusEffects.h"
#include "../include/loaders/trinketLoader.h"
#include "../include/stats.h"                  // For Stats_RecordDamage
#include "../include/scenes/sceneBlackjack.h"  // For TweenEnemyHP, GetTweenManager, GetVisualEffects
#include "../include/scenes/components/visualEffects.h"
#include "../include/tween/tween.h"            // For TweenFloat, TWEEN_EASE_*

// ============================================================================
// EFFECT EXECUTORS (Internal)
// ============================================================================

/**
 * ExecuteAddChips - Add flat chips to player
 *
 * Example: Loaded Dice (+5 chips on win)
 */
static void ExecuteAddChips(Player_t* player, int value) {
    if (!player || value <= 0) return;

    player->chips += value;
    d_LogInfoF("Trinket effect: +%d chips (total: %d)", value, player->chips);
}

/**
 * ExecuteLoseChips - Deduct flat chips from player
 *
 * Example: Cursed trinkets (future feature)
 */
static void ExecuteLoseChips(Player_t* player, int value) {
    if (!player || value <= 0) return;

    player->chips -= value;
    if (player->chips < 0) player->chips = 0;  // Clamp to 0
    d_LogInfoF("Trinket effect: -%d chips (total: %d)", value, player->chips);
}

/**
 * ExecuteApplyStatus - Apply status effect to player
 *
 * Example: Lucky Chip (apply LUCKY_STREAK on win)
 */
static void ExecuteApplyStatus(Player_t* player, const char* status_key, int stacks) {
    if (!player || !status_key || stacks <= 0) return;

    d_LogInfoF("Trinket effect: Apply %s (%d stacks)", status_key, stacks);

    // TODO: Call status effect system when it's implemented
    // For now, just log the effect
    // ApplyStatusEffect(player, status_key, stacks);
}

/**
 * ExecuteClearStatus - Remove status effect from player
 *
 * Example: Lucky Chip (clear LUCKY_STREAK on loss)
 */
static void ExecuteClearStatus(Player_t* player, const char* status_key) {
    if (!player || !status_key) return;

    d_LogInfoF("Trinket effect: Clear %s", status_key);

    // TODO: Call status effect system when it's implemented
    // ClearStatusEffect(player, status_key);
}

/**
 * ExecuteTrinketStack - Increment trinket's stack counter
 *
 * Example: Broken Watch (+1 stack on combat start, +2% damage per stack, max 12)
 */
static void ExecuteTrinketStack(TrinketInstance_t* instance, const TrinketTemplate_t* template) {
    if (!instance || !template) return;

    // Check if already at max stacks
    if (instance->trinket_stacks >= instance->trinket_stack_max) {
        d_LogDebugF("Trinket %s already at max stacks (%d/%d)",
                    d_StringPeek(template->name),
                    instance->trinket_stacks,
                    instance->trinket_stack_max);
        return;
    }

    // Increment stack
    instance->trinket_stacks++;

    d_LogInfoF("Trinket %s gained stack: %d/%d (+%d%% %s per stack)",
               d_StringPeek(template->name),
               instance->trinket_stacks,
               instance->trinket_stack_max,
               instance->trinket_stack_value,
               d_StringPeek(instance->trinket_stack_stat));

    // Mark combat stats dirty for recalculation
    // TODO: Set player->combat_stats_dirty when stat aggregation is implemented
}

/**
 * ExecuteRefundChipsPercent - Refund % of bet to player
 *
 * Example: Tarnished Medal (15% refund on bust)
 */
static void ExecuteRefundChipsPercent(Player_t* player, GameContext_t* game, int percent) {
    if (!player || !game || percent <= 0) return;

    int bet_amount = player->current_bet;
    if (bet_amount <= 0) {
        d_LogDebug("ExecuteRefundChipsPercent: No active bet to refund");
        return;
    }

    int refund = (bet_amount * percent) / 100;
    if (refund <= 0) return;

    player->chips += refund;
    d_LogInfoF("Trinket effect: Refund %d%% of bet = +%d chips (total: %d)",
               percent, refund, player->chips);
}

/**
 * ExecuteAddDamageFlat - Deal flat damage to enemy immediately
 *
 * Example: Stack Trace (deal 15 damage on bust)
 */
static void ExecuteAddDamageFlat(Player_t* player, GameContext_t* game, TrinketInstance_t* instance, int damage) {
    if (!player || !game || !instance || damage <= 0) return;

    // Only trigger in combat
    if (!game->is_combat_mode || !game->current_enemy) {
        d_LogDebug("ExecuteAddDamageFlat: Not in combat mode, skipping");
        return;
    }

    int old_hp = game->current_enemy->current_hp;

    // Apply damage to enemy
    game->current_enemy->current_hp -= damage;
    if (game->current_enemy->current_hp < 0) {
        game->current_enemy->current_hp = 0;
    }

    // Track total damage for trinket stats
    instance->total_damage_dealt += damage;

    // Record for stats system
    Stats_RecordDamage(damage, DAMAGE_SOURCE_TRINKET_PASSIVE);

    // Trigger visual feedback
    TweenEnemyHP(game->current_enemy);  // Smooth HP bar drain animation

    TweenManager_t* tween_mgr = GetTweenManager();
    if (tween_mgr) {
        TriggerEnemyDamageEffect(game->current_enemy, tween_mgr);  // Shake + red flash on enemy

        // TODO: Trigger trinket icon shake + flash animation
        // Requires TweenFloatInArray support for TrinketInstance_t in Player_t.trinket_slots[]
        // For now, just set flash alpha directly
        instance->flash_alpha = 255.0f;
    }

    // Spawn damage number via visual effects component
    VisualEffects_t* vfx = GetVisualEffects();
    if (vfx) {
        VFX_SpawnDamageNumber(vfx, damage,
                             SCREEN_WIDTH / 2 + ENEMY_HP_BAR_X_OFFSET,
                             ENEMY_HP_BAR_Y - DAMAGE_NUMBER_Y_OFFSET,
                             false, false, false);  // Not healing, not crit, not rake
    }

    d_LogInfoF("📊 Trinket effect: Deal %d flat damage to enemy (HP: %d → %d)",
               damage, old_hp, game->current_enemy->current_hp);
}

/**
 * ExecuteBuffTagDamage - Buff damage from specific card tag
 *
 * Example: Cursed Skull (add CURSED to 4 cards, +5 damage each)
 */
static void ExecuteBuffTagDamage(TrinketInstance_t* instance, const TrinketTemplate_t* template) {
    if (!instance || !template) return;

    // Store tag buff info in instance for stat aggregation
    const char* tag_str = d_StringPeek(template->passive_tag);
    if (!tag_str) return;

    // TODO: Convert tag string to CardTag_t enum
    // For now, just log
    d_LogInfoF("Trinket %s buffing tag %s by +%d damage",
               d_StringPeek(template->name),
               tag_str,
               template->passive_tag_buff_value);

    // instance->buffed_tag = CardTagFromString(tag_str);
    // instance->tag_buff_value = template->passive_tag_buff_value;
}

/**
 * ExecuteAddTagToCards - Add card tag to N random cards in deck
 *
 * Example: Cursed Skull (add CURSED to 4 random cards on equip)
 */
static void ExecuteAddTagToCards(Player_t* player, const TrinketTemplate_t* template) {
    if (!player || !template) return;

    const char* tag_str = d_StringPeek(template->passive_tag);
    int count = template->passive_tag_count;

    if (!tag_str || count <= 0) return;

    d_LogInfoF("Trinket %s adding tag %s to %d random cards",
               d_StringPeek(template->name),
               tag_str,
               count);

    // TODO: Implement when card tag system is ready
    // AddTagToRandomCards(player, tag_str, count);
}

// ============================================================================
// PUBLIC API
// ============================================================================

void ExecuteTrinketEffect(
    const TrinketTemplate_t* template,
    TrinketInstance_t* instance,
    Player_t* player,
    GameContext_t* game,
    int slot_index,
    bool is_secondary
) {
    if (!template || !instance || !player || !game) {
        d_LogError("ExecuteTrinketEffect: NULL parameter");
        return;
    }

    // Select which passive to execute (primary or secondary)
    TrinketEffectType_t effect_type = is_secondary ?
        template->passive_effect_type_2 :
        template->passive_effect_type;

    int effect_value = is_secondary ?
        template->passive_effect_value_2 :
        template->passive_effect_value;

    const char* status_key = NULL;
    int status_stacks = 0;

    if (is_secondary) {
        status_key = template->passive_status_key_2 ?
            d_StringPeek(template->passive_status_key_2) : NULL;
        status_stacks = template->passive_status_stacks_2;
    } else {
        status_key = template->passive_status_key ?
            d_StringPeek(template->passive_status_key) : NULL;
        status_stacks = template->passive_status_stacks;
    }

    // Log trigger
    d_LogInfoF("Trinket %s triggered (%s passive): %d",
               d_StringPeek(template->name),
               is_secondary ? "secondary" : "primary",
               effect_type);

    // Dispatch to effect executor
    switch (effect_type) {
        case TRINKET_EFFECT_NONE:
            // No effect
            break;

        case TRINKET_EFFECT_ADD_CHIPS:
            ExecuteAddChips(player, effect_value);
            break;

        case TRINKET_EFFECT_LOSE_CHIPS:
            ExecuteLoseChips(player, effect_value);
            break;

        case TRINKET_EFFECT_APPLY_STATUS:
            if (status_key) {
                ExecuteApplyStatus(player, status_key, status_stacks);
            }
            break;

        case TRINKET_EFFECT_CLEAR_STATUS:
            if (status_key) {
                ExecuteClearStatus(player, status_key);
            }
            break;

        case TRINKET_EFFECT_TRINKET_STACK:
            ExecuteTrinketStack(instance, template);
            break;

        case TRINKET_EFFECT_REFUND_CHIPS_PERCENT:
            ExecuteRefundChipsPercent(player, game, effect_value);
            break;

        case TRINKET_EFFECT_BUFF_TAG_DAMAGE:
            ExecuteBuffTagDamage(instance, template);
            break;

        case TRINKET_EFFECT_ADD_TAG_TO_CARDS:
            ExecuteAddTagToCards(player, template);
            break;

        case TRINKET_EFFECT_ADD_DAMAGE_FLAT:
            ExecuteAddDamageFlat(player, game, instance, effect_value);
            break;

        case TRINKET_EFFECT_DAMAGE_MULTIPLIER:
        case TRINKET_EFFECT_PUSH_DAMAGE_PERCENT:
            // These will be handled by stat aggregation system (Phase 6)
            d_LogDebugF("Trinket effect %d deferred to stat aggregation", effect_type);
            break;

        default:
            d_LogWarningF("Unknown trinket effect type: %d", effect_type);
            break;
    }

    // TODO: Trigger shake/flash animation on trinket UI
    // instance->shake_offset_x/y and flash_alpha tweening
    // (Requires tween system integration - defer to polish phase)
    (void)slot_index;  // Unused for now
}
