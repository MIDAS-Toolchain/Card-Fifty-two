/*
 * Draw Pile Modal Section
 * Modal for viewing draw pile cards in randomized order
 */

#ifndef DRAW_PILE_MODAL_SECTION_H
#define DRAW_PILE_MODAL_SECTION_H

#include "../../common.h"
#include "../components/cardGridModal.h"
#include "../../deck.h"

typedef struct DrawPileModalSection {
    CardGridModal_t* modal;
    Deck_t* deck;  // Pointer to game deck (NOT owned)
} DrawPileModalSection_t;

// Lifecycle
DrawPileModalSection_t* CreateDrawPileModalSection(Deck_t* deck);
void DestroyDrawPileModalSection(DrawPileModalSection_t** section);

// Visibility
void ShowDrawPileModal(DrawPileModalSection_t* section);
void HideDrawPileModal(DrawPileModalSection_t* section);
bool IsDrawPileModalVisible(const DrawPileModalSection_t* section);

// Input and rendering
void HandleDrawPileModalInput(DrawPileModalSection_t* section);
void RenderDrawPileModalSection(DrawPileModalSection_t* section);

#endif // DRAW_PILE_MODAL_SECTION_H
