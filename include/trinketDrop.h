/**
 * Trinket Drop Generation System
 *
 * Handles:
 * - Rarity rolls with pity counters (5 normal kills = uncommon guaranteed)
 * - Template selection (weighted random from DUF database)
 * - Affix rolls (1-3 affixes based on act tier)
 * - Reroll functionality for terminal command
 *
 * Constitutional pattern: Returns TrinketInstance_t by value (caller owns copy)
 */

#ifndef TRINKET_DROP_H
#define TRINKET_DROP_H

#include "common.h"

// ============================================================================
// RARITY ROLL (with pity counter)
// ============================================================================

/**
 * RollTrinketRarity - Determine rarity based on enemy type and pity counter
 *
 * @param is_elite - true for elite enemies (better drop rates)
 * @param pity_counter - Increments on each drop, resets on rarity upgrade
 *                       5 normal enemy kills = guaranteed uncommon+
 *                       5 elite enemy kills = guaranteed rare+
 * @return TrinketRarity_t - Rolled rarity (COMMON/UNCOMMON/RARE/LEGENDARY)
 *
 * Drop rates (normal enemy):
 * - Common: 50% (pity: 0-4 kills)
 * - Uncommon: 35% (pity: guaranteed at 5+ kills)
 * - Rare: 12%
 * - Legendary: 3%
 *
 * Drop rates (elite enemy):
 * - Common: 0%
 * - Uncommon: 40% (pity: 0-4 kills)
 * - Rare: 45% (pity: guaranteed at 5+ kills)
 * - Legendary: 15%
 */
TrinketRarity_t RollTrinketRarity(bool is_elite, int pity_counter);

// ============================================================================
// TEMPLATE SELECTION (weighted random from DUF)
// ============================================================================

/**
 * SelectTrinketTemplate - Pick random trinket template by rarity
 *
 * @param rarity - Target rarity tier
 * @param player - Player to exclude already-equipped trinkets (NULL = no filtering)
 * @return TrinketTemplate_t* - Pointer to template in g_trinket_templates (READ ONLY!)
 *                               Returns NULL if no templates match rarity
 *
 * Uses weighted random selection:
 * - All trinkets of matching rarity have equal chance
 * - Searches g_trinket_templates for matching rarity
 * - Returns pointer to template (DO NOT MODIFY - shared global data)
 * - If player is provided, excludes trinkets already equipped in player->trinket_slots[]
 */
const TrinketTemplate_t* SelectTrinketTemplate(TrinketRarity_t rarity, Player_t* player);

// ============================================================================
// AFFIX ROLL (1-3 affixes based on tier)
// ============================================================================

/**
 * RollAffixes - Generate random affixes for trinket instance
 *
 * @param tier - Act number (1-3), determines affix count:
 *               Act 1 = 1 affix
 *               Act 2 = 2 affixes
 *               Act 3 = 3 affixes
 * @param rarity - Trinket rarity (affects affix weight scaling)
 *                 Higher rarity = better chance of high-weight affixes
 * @param out_instance - Output trinket instance (affixes written here)
 *
 * Affix selection:
 * - Weighted random from g_affix_templates
 * - Each affix: stat_key + random value in [min_value, max_value]
 * - No duplicate affixes (same stat_key can't appear twice)
 * - Rarity scaling: Legendary trinkets get +20% weight on high-tier affixes
 *
 * SHARED FUNCTION: Used by both GenerateTrinketDrop() and RerollTrinketAffixes()
 */
void RollAffixes(int tier, TrinketRarity_t rarity, TrinketInstance_t* out_instance);

// ============================================================================
// DROP GENERATION (full trinket creation)
// ============================================================================

/**
 * GenerateTrinketDrop - Create random trinket drop from combat victory
 *
 * @param tier - Act number (1-3)
 * @param is_elite - true for elite enemy drops
 * @param pity_counter - Pointer to pity counter (incremented/reset by this function)
 * @param out_instance - Output trinket instance (filled with drop data)
 * @param player - Player to exclude already-equipped trinkets (NULL = no filtering)
 * @return bool - true on success, false if no valid templates found
 *
 * Process:
 * 1. Roll rarity (uses pity counter)
 * 2. Select random template of that rarity (excluding player's equipped trinkets)
 * 3. Copy template data to instance
 * 4. Roll affixes based on tier + rarity
 * 5. Calculate sell value (base_value + affix bonuses)
 * 6. Initialize runtime fields (stacks, stats, animation)
 *
 * Pity counter behavior:
 * - Increments every drop
 * - Resets to 0 when rarity upgrades
 * - Separate counters for normal/elite (stored in GameContext_t)
 */
bool GenerateTrinketDrop(int tier, bool is_elite, int* pity_counter, TrinketInstance_t* out_instance, Player_t* player);

// ============================================================================
// REROLL (for terminal command)
// ============================================================================

/**
 * RerollTrinketAffixes - Randomize affixes on existing trinket
 *
 * @param instance - Trinket instance to reroll (affixes overwritten)
 *
 * Used by terminal command: `reroll_trinket <slot_index>`
 * - Preserves: base_trinket_key, rarity, tier, base passive effects
 * - Rerolls: affixes (1-3 based on stored tier)
 * - Recalculates: sell_value
 *
 * Constitutional pattern: Modifies instance in-place (caller owns instance)
 */
void RerollTrinketAffixes(TrinketInstance_t* instance);

#endif // TRINKET_DROP_H
