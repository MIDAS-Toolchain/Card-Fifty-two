/*
 * Deck View Panel Section
 * Shows deck/discard buttons with counts and hotkeys
 */

#ifndef DECK_VIEW_PANEL_H
#define DECK_VIEW_PANEL_H

#include "../../common.h"
#include "../components/deckButton.h"
#include "../../deck.h"

// Layout constants
#define DECK_PANEL_ROW_HEIGHT      230  // Total height (count text + card + hotkey)
#define DECK_BUTTON_SPACING        60   // Spacing between buttons

typedef struct DeckViewPanel {
    FlexBox_t* button_row;       // Horizontal FlexBox for 2 buttons
    DeckButton_t** buttons;      // [0]=View Deck, [1]=View Discard (NOT owned)
    int button_count;            // Should be 2
    Deck_t* deck;                // Pointer to game deck (for counts)
} DeckViewPanel_t;

// Lifecycle
DeckViewPanel_t* CreateDeckViewPanel(DeckButton_t** buttons, int button_count, Deck_t* deck);
void DestroyDeckViewPanel(DeckViewPanel_t** panel);

// Rendering
void UpdateDeckViewPanelButtons(DeckViewPanel_t* panel);
void RenderDeckViewPanel(DeckViewPanel_t* panel, int y);

#endif // DECK_VIEW_PANEL_H
