/*
 * Combat Stats Modal Component
 * Displays combat stats (damage bonus, crit chance, crit bonus) next to sanity modal
 */

#ifndef COMBAT_STATS_MODAL_H
#define COMBAT_STATS_MODAL_H

#include "../../common.h"
#include "../../structs.h"

#define COMBAT_STATS_MODAL_WIDTH   250

// ============================================================================
// COMBAT STATS MODAL STRUCTURE
// ============================================================================

typedef struct CombatStatsModal {
    bool is_visible;
    int x;
    int y;
    Player_t* player;  // Reference only, not owned
} CombatStatsModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateCombatStatsModal - Create a new combat stats modal
 *
 * @param player - Player to display stats for (reference only)
 * @return Heap-allocated modal, or NULL on failure
 */
CombatStatsModal_t* CreateCombatStatsModal(Player_t* player);

/**
 * DestroyCombatStatsModal - Destroy combat stats modal
 *
 * @param modal - Pointer to modal pointer (will be NULLed)
 */
void DestroyCombatStatsModal(CombatStatsModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowCombatStatsModal - Show modal at specified position
 *
 * @param modal - Modal to show
 * @param x - X position
 * @param y - Y position (usually TOP_BAR_HEIGHT)
 */
void ShowCombatStatsModal(CombatStatsModal_t* modal, int x, int y);

/**
 * HideCombatStatsModal - Hide modal
 *
 * @param modal - Modal to hide
 */
void HideCombatStatsModal(CombatStatsModal_t* modal);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderCombatStatsModal - Render modal if visible
 *
 * @param modal - Modal to render
 */
void RenderCombatStatsModal(CombatStatsModal_t* modal);

#endif // COMBAT_STATS_MODAL_H
