#ifndef TRINKET_UI_H
#define TRINKET_UI_H

#include "common.h"
#include "structs.h"
#include "player.h"

// ============================================================================
// TRINKET UI COMPONENT
// ============================================================================

/**
 * TrinketUI_t - Interactive trinket display with hover/click detection
 *
 * Constitutional pattern:
 * - Component manages trinket slot rendering and input handling
 * - Supports 1 class trinket (larger, gold border) + 6 regular trinkets (2×3 grid)
 * - Hover detection for tooltips
 * - Click detection for activatable trinkets (targeting mode)
 * - Cooldown overlays, flash animations, empty slot rendering
 *
 * Lifecycle:
 * 1. CreateTrinketUI() in scene initialization
 * 2. HandleTrinketUIInput() each frame for clicks
 * 3. RenderTrinketUI() each frame for slots
 * 4. RenderTrinketTooltips() each frame for hover info
 * 5. DestroyTrinketUI() in scene cleanup
 */
typedef struct TrinketUI {
    // Hover state
    int hovered_trinket_slot;      // -1 = none, 0-5 = regular slot index
    bool hovered_class_trinket;    // true if hovering over class trinket
} TrinketUI_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateTrinketUI - Initialize trinket UI component
 *
 * @return TrinketUI_t* - Heap-allocated component, or NULL on failure
 */
TrinketUI_t* CreateTrinketUI(void);

/**
 * DestroyTrinketUI - Free trinket UI component
 *
 * @param ui - Pointer to UI pointer (double pointer for nulling)
 */
void DestroyTrinketUI(TrinketUI_t** ui);

// ============================================================================
// HOVER STATE UPDATE
// ============================================================================

/**
 * UpdateTrinketUIHover - Update hover state based on mouse position
 *
 * @param ui - UI component
 * @param player - Player (unused, kept for API consistency)
 *
 * MUST be called BEFORE HandleTrinketUIInput() to ensure input handler
 * sees current hover state. Updates:
 * - hovered_class_trinket (true if mouse over class trinket slot)
 * - hovered_trinket_slot (0-5 if mouse over regular slot, -1 otherwise)
 *
 * Called in BlackjackLogic before input handling.
 */
void UpdateTrinketUIHover(TrinketUI_t* ui, Player_t* player);

// ============================================================================
// INPUT HANDLING
// ============================================================================

/**
 * HandleTrinketUIInput - Process mouse clicks on trinket slots
 *
 * @param ui - UI component
 * @param player - Player whose trinkets to check
 * @param game - Game context for targeting mode transition
 *
 * Handles:
 * - Left-click on activatable trinkets → enter STATE_TARGETING
 * - Cooldown checks (prevent activation if on cooldown)
 * - Target type checks (only TRINKET_TARGET_CARD supported)
 *
 * MUST be called AFTER UpdateTrinketUIHover() to see current hover state.
 */
void HandleTrinketUIInput(TrinketUI_t* ui, Player_t* player, GameContext_t* game);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderTrinketUI - Draw all trinket slots
 *
 * @param ui - UI component
 * @param player - Player whose trinkets to render
 *
 * Renders:
 * - Class trinket slot (gold border, larger, left side)
 * - 6 regular trinket slots (2 rows × 3 cols, right side)
 * - Hover glow effects (brighter border + subtle overlay)
 * - Cooldown overlays (red tint + countdown text)
 * - Flash animations on proc (red overlay with tweened alpha)
 * - Empty slots (gray border, "EMPTY" text)
 */
void RenderTrinketUI(TrinketUI_t* ui, Player_t* player);

/**
 * RenderTrinketTooltips - Draw hover tooltips for trinkets
 *
 * @param ui - UI component (for hover state)
 * @param player - Player whose trinkets to show tooltips for
 *
 * Shows tooltip when hovering over any trinket:
 * - Trinket name (tier color: Common/Uncommon/Rare/Legendary)
 * - Ability description
 * - Cooldown/charges info
 * - Positioned to avoid screen edges
 */
void RenderTrinketTooltips(TrinketUI_t* ui, Player_t* player);

// ============================================================================
// STATE QUERIES
// ============================================================================

/**
 * GetHoveredTrinketSlot - Get currently hovered regular trinket slot
 *
 * @param ui - UI component
 * @return int - Slot index (0-5), or -1 if no regular trinket hovered
 */
int GetHoveredTrinketSlot(const TrinketUI_t* ui);

/**
 * IsClassTrinketHovered - Check if class trinket is hovered
 *
 * @param ui - UI component
 * @return bool - true if class trinket slot is hovered
 */
bool IsClassTrinketHovered(const TrinketUI_t* ui);

#endif // TRINKET_UI_H
