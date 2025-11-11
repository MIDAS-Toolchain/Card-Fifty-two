/*
 * Left Sidebar Section
 * Shows portrait, sanity meter, chips, and deck/discard buttons
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef LEFT_SIDEBAR_SECTION_H
#define LEFT_SIDEBAR_SECTION_H

#include "../../common.h"
#include "../../player.h"
#include "../../deck.h"
#include "../components/sidebarButton.h"
#include "../components/characterStatsModal.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define SIDEBAR_WIDTH           280
#define SIDEBAR_PADDING         20
#define PORTRAIT_SIZE           128
#define SANITY_BAR_WIDTH        200
#define SANITY_BAR_HEIGHT       20
#define ELEMENT_VERTICAL_GAP    15

// ============================================================================
// LEFT SIDEBAR SECTION
// ============================================================================

typedef struct LeftSidebarSection {
    FlexBox_t* layout;                     // Vertical FlexBox for sidebar layout
    SidebarButton_t** sidebar_buttons;     // Array of 2 SidebarButtons (NOT owned)
    Deck_t* deck;                          // Pointer to game deck (for counts)
    CharacterStatsModal_t* stats_modal;    // Character stats modal (owned)
} LeftSidebarSection_t;

/**
 * CreateLeftSidebarSection - Initialize left sidebar section
 *
 * @param sidebar_buttons - Array of 2 SidebarButtons [View Deck, View Discard] (NOT owned)
 * @param deck - Pointer to game deck for counts
 * @param player - Pointer to player for stats modal (NOT owned)
 * @return LeftSidebarSection_t* - Heap-allocated section (caller must destroy)
 */
LeftSidebarSection_t* CreateLeftSidebarSection(SidebarButton_t** sidebar_buttons, Deck_t* deck, Player_t* player);

/**
 * DestroyLeftSidebarSection - Cleanup sidebar section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyLeftSidebarSection(LeftSidebarSection_t** section);

/**
 * RenderLeftSidebarSection - Render sidebar with portrait, sanity, chips, buttons
 *
 * @param section - Sidebar section component
 * @param player - Player to render info for
 * @param x - X position (usually 0)
 * @param y - Y position (usually LAYOUT_TOP_MARGIN)
 * @param height - Total height available for sidebar
 *
 * Renders:
 * - Portrait (128x128)
 * - Sanity meter (bar with gradient)
 * - Chips display
 * - Current bet display
 * - Deck/Discard buttons
 */
void RenderLeftSidebarSection(LeftSidebarSection_t* section, Player_t* player, int x, int y, int height);

#endif // LEFT_SIDEBAR_SECTION_H
