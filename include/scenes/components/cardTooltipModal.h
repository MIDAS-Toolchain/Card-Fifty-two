/*
 * Card Tooltip Modal Component
 * Shows card name and tag information on hover in draw/discard pile modals
 */

#ifndef CARD_TOOLTIP_MODAL_H
#define CARD_TOOLTIP_MODAL_H

#include "../../common.h"
#include "../../structs.h"

// Modal dimensions (flexible to fit content)
#define CARD_TOOLTIP_WIDTH   340
#define CARD_TOOLTIP_MIN_HEIGHT  120

typedef struct CardTooltipModal {
    bool visible;
    int x, y;
    const Card_t* card;  // Reference to card (NOT owned)
} CardTooltipModal_t;

/**
 * CreateCardTooltipModal - Initialize tooltip modal
 *
 * @return CardTooltipModal_t* - Heap-allocated modal (caller must destroy)
 */
CardTooltipModal_t* CreateCardTooltipModal(void);

/**
 * DestroyCardTooltipModal - Cleanup modal resources
 *
 * @param modal - Pointer to modal pointer (will be set to NULL)
 */
void DestroyCardTooltipModal(CardTooltipModal_t** modal);

/**
 * ShowCardTooltipModal - Display modal at position
 *
 * @param modal - Modal component
 * @param card - Card to display info for (NOT owned)
 * @param card_x - X position of card in grid
 * @param card_y - Y position of card in grid
 *
 * Modal will appear to the right of card (or left if near screen edge)
 */
void ShowCardTooltipModal(CardTooltipModal_t* modal,
                          const Card_t* card,
                          int card_x, int card_y);

/**
 * ShowCardTooltipModalWithSide - Display modal at position with side preference
 *
 * @param modal - Modal component
 * @param card - Card to display info for (NOT owned)
 * @param card_x - X position of card in grid
 * @param card_y - Y position of card in grid
 * @param force_left - If true, always show on left side
 *
 * Used by card grid to flip tooltip for rightmost columns
 */
void ShowCardTooltipModalWithSide(CardTooltipModal_t* modal,
                                   const Card_t* card,
                                   int card_x, int card_y,
                                   bool force_left);

/**
 * HideCardTooltipModal - Hide modal
 *
 * @param modal - Modal component
 */
void HideCardTooltipModal(CardTooltipModal_t* modal);

/**
 * RenderCardTooltipModal - Render modal if visible
 *
 * @param modal - Modal component
 *
 * Displays:
 * - Card name (rank + suit, e.g., "Queen of Hearts")
 * - All tags with colored badges
 * - Tag descriptions (word-wrapped)
 * - "No tags" message if card has no tags
 */
void RenderCardTooltipModal(const CardTooltipModal_t* modal);

#endif // CARD_TOOLTIP_MODAL_H
