/*
 * Chips Tooltip Modal Component
 * Shows chips explanation on hover
 * Constitutional pattern: Reusable component
 */

#ifndef CHIPS_TOOLTIP_MODAL_H
#define CHIPS_TOOLTIP_MODAL_H

#include "../../common.h"

// Modal dimensions
#define CHIPS_TOOLTIP_WIDTH   260
#define CHIPS_TOOLTIP_HEIGHT  100

typedef struct ChipsTooltipModal {
    bool visible;
    int x, y;
} ChipsTooltipModal_t;

/**
 * CreateChipsTooltipModal - Initialize tooltip modal
 *
 * @return ChipsTooltipModal_t* - Heap-allocated modal (caller must destroy)
 */
ChipsTooltipModal_t* CreateChipsTooltipModal(void);

/**
 * DestroyChipsTooltipModal - Cleanup modal resources
 *
 * @param modal - Pointer to modal pointer (will be set to NULL)
 */
void DestroyChipsTooltipModal(ChipsTooltipModal_t** modal);

/**
 * ShowChipsTooltipModal - Display modal at position
 *
 * @param modal - Modal component
 * @param x - X position
 * @param y - Y position
 */
void ShowChipsTooltipModal(ChipsTooltipModal_t* modal, int x, int y);

/**
 * HideChipsTooltipModal - Hide modal
 *
 * @param modal - Modal component
 */
void HideChipsTooltipModal(ChipsTooltipModal_t* modal);

/**
 * RenderChipsTooltipModal - Render modal if visible
 *
 * @param modal - Modal component
 */
void RenderChipsTooltipModal(const ChipsTooltipModal_t* modal);

#endif // CHIPS_TOOLTIP_MODAL_H
