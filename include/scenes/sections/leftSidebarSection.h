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
#include "../components/combatStatsModal.h"
#include "../components/statusEffectTooltipModal.h"
#include "../components/chipsTooltipModal.h"
#include "../components/betTooltipModal.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define SIDEBAR_WIDTH           280
#define SIDEBAR_PADDING         20
#define PORTRAIT_SIZE           200
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
    CombatStatsModal_t* combat_stats_modal;  // Combat stats modal (owned)
    StatusEffectTooltipModal_t* effect_tooltip;  // Status effect tooltip modal (owned)
    ChipsTooltipModal_t* chips_tooltip;    // Chips tooltip modal (owned)
    BetTooltipModal_t* bet_tooltip;        // Bet tooltip modal (owned)

    // Bet damage number animation
    bool bet_damage_active;                // Is bet damage animation playing?
    float bet_damage_y_offset;             // Current Y offset (rises up)
    float bet_damage_alpha;                // Current alpha (fades out)
    int bet_damage_amount;                 // Amount to display

    // Hover tracking (for tutorial)
    float chips_hover_timer;               // Time mouse has been hovering over Betting Power
    float bet_hover_timer;                 // Time mouse has been hovering over Active Bet
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

/**
 * RenderLeftSidebarModalOverlay - Render character stats modal if hovering
 *
 * MUST be called AFTER all other UI elements to ensure proper z-ordering.
 * The modal appears when hovering over portrait or sanity bar.
 *
 * @param section - Sidebar section component
 * @param player - Player to render info for
 * @param x - X position of sidebar (usually 0)
 * @param y - Y position of sidebar (usually LAYOUT_TOP_MARGIN)
 */
void RenderLeftSidebarModalOverlay(LeftSidebarSection_t* section, Player_t* player, int x, int y);

/**
 * ShowSidebarBetDamage - Trigger floating damage number for bet placement
 *
 * Shows red "-N tokens" text that floats up and fades out above chip counter
 *
 * @param section - Sidebar section component
 * @param bet_amount - Amount of bet placed (chips spent)
 */
void ShowSidebarBetDamage(LeftSidebarSection_t* section, int bet_amount);

/**
 * UpdateLeftSidebarChipsHover - Update Betting Power (chips) hover timer and trigger tutorial event
 *
 * Call this each frame to track how long the user has been hovering over Betting Power.
 * After 1 second of hovering, triggers TUTORIAL_EVENT_HOVER for tutorial progression.
 *
 * @param section - Sidebar section component
 * @param x - X position of sidebar (usually 0)
 * @param dt - Delta time in seconds
 * @return bool - true if chips hover event was triggered this frame
 */
bool UpdateLeftSidebarChipsHover(LeftSidebarSection_t* section, int x, float dt);

/**
 * UpdateLeftSidebarHoverTracking - Update Active Bet hover timer and trigger tutorial event
 *
 * Call this each frame to track how long the user has been hovering over Active Bet.
 * After 1 second of hovering, triggers TUTORIAL_EVENT_HOVER for tutorial progression.
 *
 * @param section - Sidebar section component
 * @param x - X position of sidebar (usually 0)
 * @param dt - Delta time in seconds
 * @return bool - true if Active Bet hover event was triggered this frame
 */
bool UpdateLeftSidebarHoverTracking(LeftSidebarSection_t* section, int x, float dt);

#endif // LEFT_SIDEBAR_SECTION_H
