/*
 * Trinket Stat Aggregation System
 *
 * Calculates player combat stats from equipped trinket affixes + stack bonuses.
 * Called when combat_stats_dirty flag is set (ADR-09 dirty-flag pattern).
 */

#include "../include/trinketStats.h"
#include "../include/player.h"
#include "../include/loaders/trinketLoader.h"
#include <string.h>

/**
 * ApplyAffixToStats - Apply single affix to player stats
 *
 * Maps affix stat_key to Player_t field and adds bonus.
 */
static void ApplyAffixToStats(Player_t* player, const char* stat_key, int value) {
    if (!player || !stat_key || value == 0) return;

    // Map stat_key to player field
    if (strcmp(stat_key, "damage_bonus_percent") == 0) {
        player->damage_percent += value;
        d_LogDebugF("Trinket affix: +%d%% damage", value);
    }
    else if (strcmp(stat_key, "damage_flat") == 0) {
        player->damage_flat += value;
        d_LogDebugF("Trinket: +%d flat damage", value);
    }
    else if (strcmp(stat_key, "crit_chance") == 0) {
        player->crit_chance += value;
        d_LogDebugF("Trinket affix: +%d%% crit chance", value);
    }
    else if (strcmp(stat_key, "crit_bonus") == 0) {
        player->crit_bonus += value;
        d_LogDebugF("Trinket affix: +%d%% crit bonus", value);
    }
    else if (strcmp(stat_key, "won_chips_bonus_percent") == 0) {
        player->win_bonus_percent += value;
        d_LogDebugF("Trinket affix: +%d%% win bonus", value);
    }
    else if (strcmp(stat_key, "lost_chips_refund_percent") == 0) {
        player->loss_refund_percent += value;
        d_LogDebugF("Trinket affix: +%d%% loss refund", value);
    }
    else if (strcmp(stat_key, "push_damage_percent") == 0) {
        player->push_damage_percent += value;
        d_LogDebugF("Trinket: +%d%% push damage", value);
    }
    else if (strcmp(stat_key, "flat_chips_on_win") == 0) {
        player->flat_chips_on_win += value;
        d_LogDebugF("Trinket affix: +%d flat chips on win", value);
    }
    // Note: Other affixes like bust_save_chance, etc.
    // are NOT combat stats - they're passive effects handled by trinket system
    else {
        d_LogDebugF("Trinket affix %s (+%d) is not a combat stat (passive effect)", stat_key, value);
    }
}

/**
 * ApplyTrinketStackBonus - Apply trinket stack bonus to stats
 *
 * Trinkets like Broken Watch (+2% damage per stack, max 12) and
 * Iron Knuckles (+5 flat damage per stack, max 10) have persistent stacks.
 */
static void ApplyTrinketStackBonus(Player_t* player, const TrinketInstance_t* instance) {
    if (!player || !instance || instance->trinket_stacks == 0) return;

    const char* stat_key = d_StringPeek(instance->trinket_stack_stat);
    if (!stat_key) return;

    int bonus = instance->trinket_stacks * instance->trinket_stack_value;

    d_LogDebugF("Trinket stack bonus: %d stacks × %d = +%d %s",
                instance->trinket_stacks,
                instance->trinket_stack_value,
                bonus,
                stat_key);

    // Apply stack bonus (same mapping as affixes)
    ApplyAffixToStats(player, stat_key, bonus);
}

void AggregateTrinketStats(Player_t* player) {
    if (!player) return;

    // Reset combat stats to 0 (fresh calculation)
    // NOTE: We only reset trinket-related stats, not card tag stats
    // Card tags are calculated separately in cardTags.c
    // This is ADDITIVE aggregation - card tags + trinket affixes
    // So we DON'T reset here, we ADD to existing values

    d_LogDebug("Aggregating trinket stats...");

    int trinket_count = 0;
    int total_affixes = 0;

    // Iterate all 6 trinket slots
    for (int slot = 0; slot < 6; slot++) {
        if (!player->trinket_slot_occupied[slot]) {
            continue;  // Empty slot
        }

        TrinketInstance_t* instance = &player->trinket_slots[slot];
        const TrinketTemplate_t* template = GetTrinketTemplate(d_StringPeek(instance->base_trinket_key));

        if (!template) {
            d_LogWarningF("Trinket in slot %d has invalid template key: %s",
                          slot, d_StringPeek(instance->base_trinket_key));
            continue;
        }

        trinket_count++;
        d_LogDebugF("Processing trinket slot %d: %s (%d affixes)",
                    slot, d_StringPeek(template->name), instance->affix_count);

        // Apply all affixes
        for (int i = 0; i < instance->affix_count; i++) {
            const char* stat_key = d_StringPeek(instance->affixes[i].stat_key);
            int value = instance->affixes[i].rolled_value;

            ApplyAffixToStats(player, stat_key, value);
            total_affixes++;
        }

        // Apply trinket stack bonus (if any)
        ApplyTrinketStackBonus(player, instance);

        // Apply trinket passive effects to defensive stats
        // Elite Membership: add_chips_percent (20%) → win_bonus_percent
        // Elite Membership: refund_chips_percent (20%) → loss_refund_percent
        // Pusher's Pebble: push_damage_percent (50%) → push_damage_percent
        if (template->passive_effect_type == TRINKET_EFFECT_ADD_CHIPS_PERCENT) {
            player->win_bonus_percent += template->passive_effect_value;
            d_LogDebugF("Trinket passive: %s adds +%d%% win bonus",
                        d_StringPeek(template->name), template->passive_effect_value);
        }
        if (template->passive_effect_type_2 == TRINKET_EFFECT_REFUND_CHIPS_PERCENT) {
            player->loss_refund_percent += template->passive_effect_value_2;
            d_LogDebugF("Trinket passive: %s adds +%d%% loss refund",
                        d_StringPeek(template->name), template->passive_effect_value_2);
        }
        if (template->passive_effect_type == TRINKET_EFFECT_PUSH_DAMAGE_PERCENT) {
            player->push_damage_percent += template->passive_effect_value;
            d_LogDebugF("Trinket passive: %s adds +%d%% push damage",
                        d_StringPeek(template->name), template->passive_effect_value);
        }

        // NOTE: template is borrowed pointer from cache - no cleanup needed
    }

    d_LogInfoF("Trinket stat aggregation complete: %d trinkets, %d affixes applied",
               trinket_count, total_affixes);
    d_LogInfoF("Final combat stats: %d flat dmg, %d%% dmg, %d%% crit chance, %d%% crit bonus",
               player->damage_flat, player->damage_percent, player->crit_chance, player->crit_bonus);
    d_LogInfoF("Final defensive stats: %d%% win bonus, %d flat win chips, %d%% loss refund, %d%% push damage",
               player->win_bonus_percent, player->flat_chips_on_win, player->loss_refund_percent, player->push_damage_percent);
}
