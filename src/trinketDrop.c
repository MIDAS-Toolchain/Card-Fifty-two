/**
 * Trinket Drop Generation Implementation
 * Constitutional pattern: Value semantics everywhere
 */

#include "../include/trinketDrop.h"
#include "../include/loaders/trinketLoader.h"
#include "../include/loaders/affixLoader.h"
#include "../include/random.h"
#include <stdlib.h>
#include <time.h>

// External access to trinket key cache
extern dArray_t* g_trinket_key_cache;

// ============================================================================
// RARITY ROLL WITH PITY COUNTER
// ============================================================================

TrinketRarity_t RollTrinketRarity(bool is_elite, int pity_counter) {
    // Pity thresholds
    const int PITY_THRESHOLD = 5;  // 5 kills = guaranteed upgrade

    if (is_elite) {
        // Elite enemy drops (no commons, better rates)
        // Pity: 5 kills = guaranteed rare+
        if (pity_counter >= PITY_THRESHOLD) {
            // Guaranteed rare or legendary
            int roll = GetRandomInt(0, 99);  // 0-99
            if (roll < 75) {  // 75% rare
                return TRINKET_RARITY_RARE;
            } else {  // 25% legendary
                return TRINKET_RARITY_LEGENDARY;
            }
        }

        // Normal elite drops (before pity)
        int roll = GetRandomInt(0, 99);  // 0-99
        if (roll < 40) {  // 40% uncommon
            return TRINKET_RARITY_UNCOMMON;
        } else if (roll < 85) {  // 45% rare
            return TRINKET_RARITY_RARE;
        } else {  // 15% legendary
            return TRINKET_RARITY_LEGENDARY;
        }
    } else {
        // Normal enemy drops
        // Pity: 5 kills = guaranteed uncommon+
        if (pity_counter >= PITY_THRESHOLD) {
            // Guaranteed uncommon or better
            int roll = GetRandomInt(0, 99);  // 0-99
            if (roll < 70) {  // 70% uncommon
                return TRINKET_RARITY_UNCOMMON;
            } else if (roll < 95) {  // 25% rare
                return TRINKET_RARITY_RARE;
            } else {  // 5% legendary
                return TRINKET_RARITY_LEGENDARY;
            }
        }

        // Normal drops (before pity)
        int roll = GetRandomInt(0, 99);  // 0-99
        if (roll < 50) {  // 50% common
            return TRINKET_RARITY_COMMON;
        } else if (roll < 85) {  // 35% uncommon
            return TRINKET_RARITY_UNCOMMON;
        } else if (roll < 97) {  // 12% rare
            return TRINKET_RARITY_RARE;
        } else {  // 3% legendary
            return TRINKET_RARITY_LEGENDARY;
        }
    }
}

// ============================================================================
// TEMPLATE SELECTION (weighted random from DUF)
// ============================================================================

const TrinketTemplate_t* SelectTrinketTemplate(TrinketRarity_t rarity, Player_t* player) {
    if (!g_trinket_key_cache) {
        d_LogError("SelectTrinketTemplate: key cache is NULL");
        return NULL;
    }

    // Collect matching trinket keys (not templates yet)
    dArray_t* matching_keys = d_ArrayInit(32, sizeof(const char*));
    if (!matching_keys) {
        d_LogError("SelectTrinketTemplate: Failed to create array");
        return NULL;
    }

    // Iterate cached keys and test each one
    for (size_t i = 0; i < g_trinket_key_cache->count; i++) {
        const char** key_ptr = (const char**)d_ArrayGet(g_trinket_key_cache, i);
        if (!key_ptr || !*key_ptr) continue;

        const char* trinket_key = *key_ptr;

        // Check if player already has this trinket equipped
        if (player) {
            bool already_equipped = false;
            for (int slot = 0; slot < 6; slot++) {
                if (player->trinket_slot_occupied[slot]) {
                    const char* equipped_key = d_StringPeek(player->trinket_slots[slot].base_trinket_key);
                    if (equipped_key && strcmp(equipped_key, trinket_key) == 0) {
                        already_equipped = true;
                        d_LogInfoF("Excluding already-equipped trinket: %s (slot %d)", trinket_key, slot);
                        break;
                    }
                }
            }
            if (already_equipped) continue;  // Skip this trinket
        }

        // Load template from DUF to check rarity
        TrinketTemplate_t* template = LoadTrinketTemplateFromDUF(trinket_key);
        if (template && template->rarity == rarity) {
            // Match! Store key (not template)
            d_ArrayAppend(matching_keys, key_ptr);
        }

        // Cleanup test template
        if (template) {
            CleanupTrinketTemplate(template);
            free(template);
        }
    }

    if (matching_keys->count == 0) {
        d_LogWarningF("SelectTrinketTemplate: No templates found for rarity %d", rarity);
        d_ArrayDestroy(matching_keys);
        return NULL;
    }

    // Pick random key from matching list
    size_t random_index = GetRandomInt(0, matching_keys->count - 1);
    const char** selected_key_ptr = (const char**)d_ArrayGet(matching_keys, random_index);
    const char* selected_key = selected_key_ptr ? *selected_key_ptr : NULL;

    d_ArrayDestroy(matching_keys);

    if (!selected_key) {
        return NULL;
    }

    // Load the selected template (caller must free this!)
    TrinketTemplate_t* result = LoadTrinketTemplateFromDUF(selected_key);
    if (result) {
        d_LogInfoF("Selected trinket template: %s (rarity %d)",
                   d_StringPeek(result->name), result->rarity);
    }

    return result;
}

// ============================================================================
// AFFIX ROLL (shared by drop and reroll)
// ============================================================================

void RollAffixes(int tier, TrinketRarity_t rarity, TrinketInstance_t* out_instance) {
    if (!out_instance) {
        return;
    }

    // Determine affix count based on tier (act number)
    int affix_count = tier;  // Act 1 = 1, Act 2 = 2, Act 3 = 3
    if (affix_count < 1) affix_count = 1;
    if (affix_count > 3) affix_count = 3;

    // Clear existing affixes
    memset(out_instance->affixes, 0, sizeof(out_instance->affixes));
    out_instance->affix_count = 0;

    // Enemy pattern: Get all affix keys from DUF (no preloaded table)
    dArray_t* all_affix_keys = GetAllAffixKeys();
    if (!all_affix_keys || all_affix_keys->count == 0) {
        d_LogWarning("RollAffixes: No affixes in database");
        if (all_affix_keys) d_ArrayDestroy(all_affix_keys);
        return;
    }

    // Rarity weight scaling (legendary trinkets get better affixes)
    float weight_scale = 1.0f;
    if (rarity == TRINKET_RARITY_LEGENDARY) weight_scale = 1.2f;
    else if (rarity == TRINKET_RARITY_RARE) weight_scale = 1.1f;

    // Roll affixes (no duplicates)
    for (int i = 0; i < affix_count; i++) {
        // Calculate total weight of remaining affixes (excluding already rolled)
        int total_weight = 0;
        for (size_t j = 0; j < all_affix_keys->count; j++) {
            const char** key_ptr = (const char**)d_ArrayGet(all_affix_keys, j);
            if (!key_ptr || !*key_ptr) continue;

            const char* stat_key_str = *key_ptr;

            // Skip if already rolled this affix
            bool already_rolled = false;
            for (int k = 0; k < out_instance->affix_count; k++) {
                if (out_instance->affixes[k].stat_key &&
                    strcmp(d_StringPeek(out_instance->affixes[k].stat_key), stat_key_str) == 0) {
                    already_rolled = true;
                    break;
                }
            }
            if (already_rolled) continue;

            // Enemy pattern: Parse affix on-demand (heap-allocated)
            AffixTemplate_t* affix_template = LoadAffixTemplateFromDUF(stat_key_str);
            if (affix_template) {
                total_weight += (int)(affix_template->weight * weight_scale);
                // Cleanup immediately (we only needed the weight)
                CleanupAffixTemplate(affix_template);
                free(affix_template);
            }
        }

        if (total_weight == 0) {
            d_LogWarning("RollAffixes: No affixes left to roll");
            break;
        }

        // Weighted random selection
        int roll = GetRandomInt(0, total_weight - 1);
        int cumulative_weight = 0;

        for (size_t j = 0; j < all_affix_keys->count; j++) {
            const char** key_ptr = (const char**)d_ArrayGet(all_affix_keys, j);
            if (!key_ptr || !*key_ptr) continue;

            const char* stat_key_str = *key_ptr;

            // Skip if already rolled
            bool already_rolled = false;
            for (int k = 0; k < out_instance->affix_count; k++) {
                if (out_instance->affixes[k].stat_key &&
                    strcmp(d_StringPeek(out_instance->affixes[k].stat_key), stat_key_str) == 0) {
                    already_rolled = true;
                    break;
                }
            }
            if (already_rolled) continue;

            // Enemy pattern: Parse affix on-demand (heap-allocated)
            AffixTemplate_t* affix_template = LoadAffixTemplateFromDUF(stat_key_str);
            if (!affix_template) continue;

            cumulative_weight += (int)(affix_template->weight * weight_scale);

            if (roll < cumulative_weight) {
                // Selected this affix! Roll value in [min, max]
                int rolled_value = GetRandomInt(affix_template->min_value, affix_template->max_value);

                // Store in instance
                out_instance->affixes[out_instance->affix_count].stat_key = d_StringInit();
                d_StringSet(out_instance->affixes[out_instance->affix_count].stat_key, stat_key_str, 0);
                out_instance->affixes[out_instance->affix_count].rolled_value = rolled_value;
                out_instance->affix_count++;

                d_LogInfoF("Rolled affix: %s = %d (weight %d)",
                          stat_key_str, rolled_value, affix_template->weight);

                // Cleanup heap-allocated template (enemy pattern)
                CleanupAffixTemplate(affix_template);
                free(affix_template);
                break;
            }

            // Cleanup if not selected (enemy pattern)
            CleanupAffixTemplate(affix_template);
            free(affix_template);
        }
    }

    d_ArrayDestroy(all_affix_keys);

    d_LogInfoF("Rolled %d affixes for tier %d, rarity %d",
               out_instance->affix_count, tier, rarity);
}

// ============================================================================
// DROP GENERATION (full trinket creation)
// ============================================================================

bool GenerateTrinketDrop(int tier, bool is_elite, int* pity_counter, TrinketInstance_t* out_instance, Player_t* player) {
    if (!out_instance || !pity_counter) {
        d_LogError("GenerateTrinketDrop: NULL parameters");
        return false;
    }

    // Zero-initialize output
    memset(out_instance, 0, sizeof(TrinketInstance_t));

    // Step 1: Roll rarity (uses pity counter)
    TrinketRarity_t rolled_rarity = RollTrinketRarity(is_elite, *pity_counter);
    d_LogInfoF("Rolled rarity: %d (pity counter: %d, elite: %d)",
               rolled_rarity, *pity_counter, is_elite);

    // Step 2: Select random template of that rarity (excluding player's equipped trinkets)
    const TrinketTemplate_t* template = SelectTrinketTemplate(rolled_rarity, player);
    if (!template) {
        d_LogErrorF("GenerateTrinketDrop: No template found for rarity %d", rolled_rarity);
        return false;
    }

    // Step 3: Copy template data to instance (deep copy dStrings)
    out_instance->base_trinket_key = d_StringInit();
    if (!out_instance->base_trinket_key) {
        d_LogError("GenerateTrinketDrop: Failed to allocate base_trinket_key");
        return false;
    }
    d_StringSet(out_instance->base_trinket_key, d_StringPeek(template->trinket_key), 0);

    out_instance->rarity = template->rarity;
    out_instance->tier = tier;
    out_instance->sell_value = template->base_value;  // Will be adjusted by affixes

    // Copy trinket stack info (for Broken Watch, Iron Knuckles, etc)
    out_instance->trinket_stacks = 0;  // Starts at 0, increments on trigger
    out_instance->trinket_stack_max = template->passive_stack_max;
    if (template->passive_stack_stat) {
        out_instance->trinket_stack_stat = d_StringInit();
        d_StringSet(out_instance->trinket_stack_stat, d_StringPeek(template->passive_stack_stat), 0);
    }
    out_instance->trinket_stack_value = template->passive_stack_value;

    // Copy tag buff info (for Cursed Skull)
    // Convert tag string to enum (passive_tag is dString_t*, buffed_tag is int)
    if (template->passive_tag && d_StringPeek(template->passive_tag)) {
        const char* tag_str = d_StringPeek(template->passive_tag);
        // Simple string-to-enum conversion (TODO: add CardTagFromString() function)
        if (strcmp(tag_str, "CURSED") == 0) {
            out_instance->buffed_tag = 0;  // CARD_TAG_CURSED
        } else if (strcmp(tag_str, "VAMPIRIC") == 0) {
            out_instance->buffed_tag = 1;  // CARD_TAG_VAMPIRIC
        } else if (strcmp(tag_str, "LUCKY") == 0) {
            out_instance->buffed_tag = 2;  // CARD_TAG_LUCKY
        } else if (strcmp(tag_str, "BRUTAL") == 0) {
            out_instance->buffed_tag = 3;  // CARD_TAG_BRUTAL
        } else {
            out_instance->buffed_tag = -1;  // No tag
        }
    } else {
        out_instance->buffed_tag = -1;  // No tag
    }
    out_instance->tag_buff_value = template->passive_tag_buff_value;

    // Step 4: Roll affixes based on tier + rarity
    RollAffixes(tier, rolled_rarity, out_instance);

    // Step 5: Calculate sell value (base + affix bonuses)
    // Each affix adds ~10% of base value
    for (int i = 0; i < out_instance->affix_count; i++) {
        out_instance->sell_value += template->base_value / 10;
    }

    // Step 6: Initialize runtime fields
    out_instance->total_damage_dealt = 0;
    out_instance->total_bonus_chips = 0;
    out_instance->total_refunded_chips = 0;
    out_instance->shake_offset_x = 0.0f;
    out_instance->shake_offset_y = 0.0f;
    out_instance->flash_alpha = 0.0f;

    // Step 7: Update pity counter
    // Increment counter, reset if rarity upgraded
    (*pity_counter)++;
    if (rolled_rarity > TRINKET_RARITY_COMMON) {
        *pity_counter = 0;  // Reset on uncommon+ drop
        d_LogInfo("Pity counter reset (got upgrade)");
    }

    d_LogInfoF("Generated trinket drop: %s (rarity %d, tier %d, %d affixes, sell value %d)",
               d_StringPeek(template->name), rolled_rarity, tier,
               out_instance->affix_count, out_instance->sell_value);

    // Cleanup heap-allocated template (enemy pattern)
    CleanupTrinketTemplate((TrinketTemplate_t*)template);
    free((void*)template);

    return true;
}

// ============================================================================
// REROLL (for terminal command)
// ============================================================================

void RerollTrinketAffixes(TrinketInstance_t* instance) {
    if (!instance) {
        d_LogError("RerollTrinketAffixes: NULL instance");
        return;
    }

    // Get original template to recalculate base sell value
    const TrinketTemplate_t* template = GetTrinketTemplate(d_StringPeek(instance->base_trinket_key));
    if (!template) {
        d_LogError("RerollTrinketAffixes: Template not found");
        return;
    }

    // Free old affix dStrings
    for (int i = 0; i < instance->affix_count; i++) {
        if (instance->affixes[i].stat_key) {
            d_StringDestroy(instance->affixes[i].stat_key);
            instance->affixes[i].stat_key = NULL;
        }
    }

    // Reroll affixes using stored tier + rarity
    RollAffixes(instance->tier, instance->rarity, instance);

    // Recalculate sell value
    instance->sell_value = template->base_value;
    for (int i = 0; i < instance->affix_count; i++) {
        instance->sell_value += template->base_value / 10;
    }

    d_LogInfoF("Rerolled affixes for %s: %d affixes, new sell value %d",
               d_StringPeek(template->name), instance->affix_count, instance->sell_value);
}
