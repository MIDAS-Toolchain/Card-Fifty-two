#ifndef TRINKET_STATS_H
#define TRINKET_STATS_H

#include "common.h"

/**
 * AggregateTrinketStats - Calculate player combat stats from equipped trinkets
 *
 * @param player - Player to calculate stats for
 *
 * Aggregates all trinket affixes and stack bonuses into player combat stats:
 * - damage_flat
 * - damage_percent
 * - crit_chance
 * - crit_bonus
 *
 * Called when combat_stats_dirty flag is set (ADR-09 dirty-flag pattern).
 * This ensures stats are recalculated when:
 * - Trinket equipped/unequipped
 * - Hand changes (card tags affect stats)
 * - Trinket stacks change
 *
 * Resets stats to 0 before aggregation (fresh calculation each time).
 */
void AggregateTrinketStats(Player_t* player);

#endif // TRINKET_STATS_H
