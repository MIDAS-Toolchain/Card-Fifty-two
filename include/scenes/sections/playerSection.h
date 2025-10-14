/*
 * Player Section
 * FlexBox section for player name, score, chips, bet, and card display
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef PLAYER_SECTION_H
#define PLAYER_SECTION_H

#include "../../common.h"
#include "../../player.h"
#include "deckViewPanel.h"

// ============================================================================
// PLAYER SECTION
// ============================================================================

typedef struct PlayerSection {
    FlexBox_t* layout;          // Internal vertical FlexBox (optional, for future expansion)
    DeckViewPanel_t* deck_panel; // Deck/Discard button panel
} PlayerSection_t;

/**
 * CreatePlayerSection - Initialize player section
 *
 * @param deck_buttons - Array of 2 DeckButtons [View Deck, View Discard] (NOT owned)
 * @param deck - Pointer to game deck for counts
 * @return PlayerSection_t* - Heap-allocated section (caller must destroy)
 */
PlayerSection_t* CreatePlayerSection(DeckButton_t** deck_buttons, Deck_t* deck);

/**
 * DestroyPlayerSection - Cleanup player section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyPlayerSection(PlayerSection_t** section);

/**
 * RenderPlayerSection - Render player info, chips, bet, and cards
 *
 * @param section - Player section component
 * @param player - Player to render
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Player name with full score
 * - Chips and bet info
 * - Centered card rendering
 */
void RenderPlayerSection(PlayerSection_t* section, Player_t* player, int y);

#endif // PLAYER_SECTION_H
