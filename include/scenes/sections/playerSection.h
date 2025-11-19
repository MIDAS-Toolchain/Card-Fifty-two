/*
 * Player Section
 * FlexBox section for player name, score, and card display
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef PLAYER_SECTION_H
#define PLAYER_SECTION_H

#include "../../common.h"
#include "../../player.h"
#include "../../deck.h"
#include "../components/cardTooltipModal.h"

// Forward declaration to avoid circular dependency
typedef struct DeckButton DeckButton_t;

// ============================================================================
// PLAYER SECTION
// ============================================================================

typedef struct PlayerSection {
    FlexBox_t* layout;               // Internal vertical FlexBox (optional, for future expansion)
    CardHoverState_t hover_state;    // Card hover animation state
    CardTooltipModal_t* tooltip;     // Owned tooltip for hand cards (shows tag info on hover)
} PlayerSection_t;

/**
 * CreatePlayerSection - Initialize player section
 *
 * @param deck_buttons - DEPRECATED: Pass NULL
 * @param deck - DEPRECATED: Pass NULL
 * @return PlayerSection_t* - Heap-allocated section (caller must destroy)
 *
 * NOTE: Deck buttons moved to LeftSidebarSection
 */
PlayerSection_t* CreatePlayerSection(DeckButton_t** deck_buttons, Deck_t* deck);

/**
 * DestroyPlayerSection - Cleanup player section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyPlayerSection(PlayerSection_t** section);

/**
 * RenderPlayerSection - Render player info and cards
 *
 * @param section - Player section component
 * @param player - Player to render
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Player name with full score
 * - Centered card rendering
 * - Updates tooltip state (but doesn't render it - see RenderPlayerSectionTooltip)
 *
 * NOTE: Chips/bet info moved to LeftSidebarSection
 * NOTE: Tooltip rendering separated for z-ordering (must render after trinket menu)
 */
void RenderPlayerSection(PlayerSection_t* section, Player_t* player, int y);

/**
 * RenderPlayerSectionTooltip - Render player hand card tooltip
 *
 * @param section - Player section component
 *
 * Renders tooltip on top of everything (including trinket menu).
 * Must be called AFTER RenderTrinketUI() in scene rendering order.
 */
void RenderPlayerSectionTooltip(PlayerSection_t* section);

#endif // PLAYER_SECTION_H
