#ifndef TRINKET_EFFECTS_H
#define TRINKET_EFFECTS_H

#include "common.h"

/**
 * ExecuteTrinketEffect - Execute trinket passive effect
 *
 * @param template - Trinket template with effect definition
 * @param instance - Trinket instance with runtime state
 * @param player - Player who owns the trinket
 * @param game - Game context
 * @param slot_index - Trinket slot index (0-5) for animation targeting
 * @param is_secondary - true if executing passive_trigger_2 effects
 *
 * Dispatches to specific effect executor based on template->passive_effect_type.
 * Handles both primary and secondary passives (is_secondary flag).
 */
void ExecuteTrinketEffect(
    const TrinketTemplate_t* template,
    TrinketInstance_t* instance,
    Player_t* player,
    GameContext_t* game,
    int slot_index,
    bool is_secondary
);

#endif // TRINKET_EFFECTS_H
