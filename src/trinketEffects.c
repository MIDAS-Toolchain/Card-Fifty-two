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
static void ExecuteAddChips(Player_t* player, TrinketInstance_t* instance, int value) {
    if (!player || !instance || value <= 0) return;

    player->chips += value;

    // Track total bonus chips for stats (data-driven)
    TRINKET_ADD_STAT(instance, TRINKET_STAT_BONUS_CHIPS, value);

    d_LogInfoF("Trinket effect: +%d chips (total: %d)", value, player->chips);
}

/**
 * ExecuteAddChipsPercent - Add percentage of bet as bonus chips
 *
 * Example: Elite Membership (+20% of bet on win)
 */
static void ExecuteAddChipsPercent(Player_t* player, TrinketInstance_t* instance, int percent) {
    if (!player || !instance || percent <= 0) return;

    int bet_amount = player->current_bet;
    d_LogInfoF("🎰 ExecuteAddChipsPercent: bet=%d, percent=%d", bet_amount, percent);

    if (bet_amount <= 0) {
        d_LogWarning("ExecuteAddChipsPercent: No active bet (current_bet=0)");
        return;
    }

    int bonus = (bet_amount * percent) / 100;
    if (bonus <= 0) return;

    player->chips += bonus;

    // Track total bonus chips for stats (data-driven)
    TRINKET_ADD_STAT(instance, TRINKET_STAT_BONUS_CHIPS, bonus);

    d_LogInfoF("🎰 Trinket effect: +%d%% of bet = +%d chips (chips: %d, total_bonus_chips: %d)",
               percent, bonus, player->chips, TRINKET_GET_STAT(instance, TRINKET_STAT_BONUS_CHIPS));
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
 * Example: Broken Watch (+1 stack on combat start, +2% damage per stack, max 12, loops to 1)
 *          Streak Counter (+1 stack on win, +4% crit per stack, infinite stacks)
 */
static void ExecuteTrinketStack(TrinketInstance_t* instance, const TrinketTemplate_t* template, Player_t* player) {
    if (!instance || !template) return;

    // Check if already at max stacks (0 = infinite, no cap)
    if (template->passive_stack_max > 0 && instance->trinket_stacks >= template->passive_stack_max) {
        // Check for loop behavior (Broken Watch: reset_to_one)
        const char* on_max = template->passive_stack_on_max ?
                             d_StringPeek(template->passive_stack_on_max) : NULL;

        if (on_max && strcmp(on_max, "reset_to_one") == 0) {
            instance->trinket_stacks = 1;  // Loop back to 1
            d_LogInfoF("⏰ Trinket %s reached max, looping to 1! (time loops!)",
                       d_StringPeek(template->name));
        } else {
            // Stay capped at max (current behavior)
            d_LogDebugF("Trinket %s already at max stacks (%d/%d)",
                        d_StringPeek(template->name),
                        instance->trinket_stacks,
                        template->passive_stack_max);
            return;
        }
    } else {
        // Increment stack (normal case or infinite scaling)
        instance->trinket_stacks++;
    }

    // Track highest streak (ONLY for infinite stack trinkets like Streak Counter - data-driven)
    // Broken Watch has a max, so it doesn't track streaks
    if (template->passive_stack_max == 0 && instance->trinket_stacks > TRINKET_GET_STAT(instance, TRINKET_STAT_HIGHEST_STREAK)) {
        TRINKET_SET_STAT(instance, TRINKET_STAT_HIGHEST_STREAK, instance->trinket_stacks);
    }

    if (template->passive_stack_max == 0) {
        d_LogInfoF("Trinket %s gained stack: %d/∞ (+%d%% %s per stack)",
                   d_StringPeek(template->name),
                   instance->trinket_stacks,
                   instance->trinket_stack_value,
                   d_StringPeek(instance->trinket_stack_stat));
    } else {
        d_LogInfoF("Trinket %s gained stack: %d/%d (+%d%% %s per stack)",
                   d_StringPeek(template->name),
                   instance->trinket_stacks,
                   template->passive_stack_max,
                   instance->trinket_stack_value,
                   d_StringPeek(instance->trinket_stack_stat));
    }

    // Mark combat stats dirty for recalculation (ADR-09: Dirty-flag aggregation)
    if (player) {
        player->combat_stats_dirty = true;
        d_LogDebug("Trinket stack changed - marked combat_stats_dirty for recalculation");
    }
}

/**
 * ExecuteTrinketStackReset - Reset trinket's stacks to 0
 *
 * Example: Streak Counter (reset all stacks on loss)
 */
static void ExecuteTrinketStackReset(TrinketInstance_t* instance, const TrinketTemplate_t* template, Player_t* player) {
    if (!instance || !template) return;

    int old_stacks = instance->trinket_stacks;
    instance->trinket_stacks = 0;

    d_LogInfoF("💔 Trinket %s stack reset: %d → 0 (lost streak!)",
               d_StringPeek(template->name), old_stacks);

    // Mark combat stats dirty for recalculation (ADR-09: Dirty-flag aggregation)
    if (player) {
        player->combat_stats_dirty = true;
        d_LogDebug("Trinket stacks reset - marked combat_stats_dirty for recalculation");
    }
}

/**
 * ExecuteRefundChipsPercent - Refund % of bet to player
 *
 * Example: Tarnished Medal (15% refund on bust), Elite Membership (20% refund on loss)
 */
static void ExecuteRefundChipsPercent(Player_t* player, GameContext_t* game, TrinketInstance_t* instance, int percent) {
    if (!player || !game || !instance || percent <= 0) return;

    int bet_amount = player->current_bet;
    d_LogInfoF("💰 ExecuteRefundChipsPercent: bet=%d, percent=%d, chips_before=%d",
               bet_amount, percent, player->chips);

    if (bet_amount <= 0) {
        d_LogWarning("ExecuteRefundChipsPercent: No active bet to refund (current_bet=0)");
        return;
    }

    int refund = (bet_amount * percent) / 100;
    if (refund <= 0) return;

    player->chips += refund;

    // Track total refunded chips for stats (data-driven)
    TRINKET_ADD_STAT(instance, TRINKET_STAT_REFUNDED_CHIPS, refund);

    d_LogInfoF("💰 Trinket effect: Refund %d%% of bet = +%d chips (chips: %d, total_refunded_chips: %d)",
               percent, refund, player->chips, TRINKET_GET_STAT(instance, TRINKET_STAT_REFUNDED_CHIPS));
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

    // Apply combat stat modifiers (damage%, flat damage, crit) - same as regular combat damage
    bool is_crit = false;
    int modified_damage = ApplyPlayerDamageModifiers(player, damage, &is_crit);

    // Apply damage to enemy (uses TakeDamage for consistency)
    TakeDamage(game->current_enemy, modified_damage);

    // Track modified damage for trinket stats (data-driven)
    TRINKET_ADD_STAT(instance, TRINKET_STAT_DAMAGE_DEALT, modified_damage);

    // Record for stats system
    Stats_RecordDamage(DAMAGE_SOURCE_TRINKET_PASSIVE, modified_damage);

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
        VFX_SpawnDamageNumber(vfx, modified_damage,
                             SCREEN_WIDTH / 2 + ENEMY_HP_BAR_X_OFFSET,
                             ENEMY_HP_BAR_Y - DAMAGE_NUMBER_Y_OFFSET,
                             false, is_crit, false);  // Pass crit flag from modifiers
    }

    d_LogInfoF("📊 Trinket effect: Deal %d damage (base: %d, modified: %d) - Enemy HP: %d",
               modified_damage, damage, modified_damage, game->current_enemy->current_hp);
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

/**
 * ExecuteBlockDebuff - Grant debuff blocks for this combat
 *
 * Example: Warded Charm (block 1 debuff this combat)
 *
 * NOTE: Sets counter on TRINKET INSTANCE, not player (so multiple trinkets stack)
 */
static void ExecuteBlockDebuff(Player_t* player, TrinketInstance_t* instance, int count) {
    if (!player || !instance || count <= 0) return;

    instance->debuff_blocks_remaining += count;
    d_LogInfoF("🛡️ Trinket effect: Block %d debuff(s) this combat (trinket has %d blocks)",
               count, instance->debuff_blocks_remaining);
}

/**
 * ExecutePunishHeal - Grant heal punish charges for this combat
 *
 * Example: Bleeding Heart (punish 1 enemy heal this combat)
 *
 * NOTE: Sets counter on TRINKET INSTANCE, not player (so multiple trinkets stack)
 */
static void ExecutePunishHeal(Player_t* player, TrinketInstance_t* instance, int count) {
    if (!player || !instance || count <= 0) return;

    instance->heal_punishes_remaining += count;
    d_LogInfoF("💔 Trinket effect: Punish %d enemy heal(s) this combat (trinket has %d punishes)",
               count, instance->heal_punishes_remaining);
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
            ExecuteAddChips(player, instance, effect_value);
            break;

        case TRINKET_EFFECT_ADD_CHIPS_PERCENT:
            // Win bonuses now applied centrally in game.c using player->win_bonus_percent
            // Still track stats for trinket tooltip (but don't add chips again - data-driven)
            if (player->current_bet > 0) {
                int bonus_for_this_trinket = (player->current_bet * effect_value) / 100;
                TRINKET_ADD_STAT(instance, TRINKET_STAT_BONUS_CHIPS, bonus_for_this_trinket);
                d_LogDebugF("Trinket %s tracking: +%d bonus chips (tooltip stat only)",
                           d_StringPeek(template->name), bonus_for_this_trinket);
            }
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
            ExecuteTrinketStack(instance, template, player);
            break;

        case TRINKET_EFFECT_TRINKET_STACK_RESET:
            ExecuteTrinketStackReset(instance, template, player);
            break;

        case TRINKET_EFFECT_REFUND_CHIPS_PERCENT:
            // Loss refunds now applied centrally in game.c using player->loss_refund_percent
            // Still track stats for trinket tooltip (but don't add chips again - data-driven)
            if (player->current_bet > 0) {
                int refund_for_this_trinket = (player->current_bet * effect_value) / 100;
                TRINKET_ADD_STAT(instance, TRINKET_STAT_REFUNDED_CHIPS, refund_for_this_trinket);
                d_LogDebugF("Trinket %s tracking: +%d refunded chips (tooltip stat only)",
                           d_StringPeek(template->name), refund_for_this_trinket);
            }
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
            // Handled by stat aggregation system (damage_percent modifier)
            d_LogDebugF("Trinket effect %d deferred to stat aggregation", effect_type);
            break;

        case TRINKET_EFFECT_PUSH_DAMAGE_PERCENT:
            // Push damage tracking happens in game.c AFTER damage modifiers are applied
            // (Similar to how Stack Trace tracks at point of damage application)
            // This ensures tooltip shows actual modified damage, not base contribution
            d_LogDebugF("Trinket effect %d deferred to damage application (tracks modified damage)", effect_type);
            break;

        case TRINKET_EFFECT_BLOCK_DEBUFF:
            ExecuteBlockDebuff(player, instance, effect_value);
            break;

        case TRINKET_EFFECT_PUNISH_HEAL:
            ExecutePunishHeal(player, instance, effect_value);
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
