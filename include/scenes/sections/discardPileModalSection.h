/*
 * Discard Pile Modal Section
 * Modal for viewing discard pile cards in order
 */

#ifndef DISCARD_PILE_MODAL_SECTION_H
#define DISCARD_PILE_MODAL_SECTION_H

#include "../../common.h"
#include "../components/cardGridModal.h"
#include "../../deck.h"

typedef struct DiscardPileModalSection {
    CardGridModal_t* modal;
    Deck_t* deck;  // Pointer to game deck (NOT owned)
} DiscardPileModalSection_t;

// Lifecycle
DiscardPileModalSection_t* CreateDiscardPileModalSection(Deck_t* deck);
void DestroyDiscardPileModalSection(DiscardPileModalSection_t** section);

// Visibility
void ShowDiscardPileModal(DiscardPileModalSection_t* section);
void HideDiscardPileModal(DiscardPileModalSection_t* section);
bool IsDiscardPileModalVisible(const DiscardPileModalSection_t* section);

// Input and rendering
void HandleDiscardPileModalInput(DiscardPileModalSection_t* section);
void RenderDiscardPileModalSection(DiscardPileModalSection_t* section);

#endif // DISCARD_PILE_MODAL_SECTION_H
