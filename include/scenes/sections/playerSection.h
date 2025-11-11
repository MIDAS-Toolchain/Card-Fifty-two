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

// Forward declaration to avoid circular dependency
typedef struct DeckButton DeckButton_t;

// ============================================================================
// PLAYER SECTION
// ============================================================================

typedef struct PlayerSection {
    FlexBox_t* layout;               // Internal vertical FlexBox (optional, for future expansion)
    CardHoverState_t hover_state;    // Card hover animation state
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
 *
 * NOTE: Chips/bet info moved to LeftSidebarSection
 */
void RenderPlayerSection(PlayerSection_t* section, Player_t* player, int y);

#endif // PLAYER_SECTION_H
