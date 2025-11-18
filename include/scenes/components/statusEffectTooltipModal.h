/*
 * Status Effect Tooltip Modal Component
 * Shows detailed status effect information on hover
 * Constitutional pattern: Reusable component
 */

#ifndef STATUS_EFFECT_TOOLTIP_MODAL_H
#define STATUS_EFFECT_TOOLTIP_MODAL_H

#include "../../common.h"
#include "../../statusEffects.h"

// Modal dimensions (flexible to fit content)
#define STATUS_TOOLTIP_WIDTH   320
#define STATUS_TOOLTIP_MIN_HEIGHT  180

typedef struct StatusEffectTooltipModal {
    bool visible;
    int x, y;
    const StatusEffectInstance_t* effect;  // Reference to effect (NOT owned)
} StatusEffectTooltipModal_t;

/**
 * CreateStatusEffectTooltipModal - Initialize tooltip modal
 *
 * @return StatusEffectTooltipModal_t* - Heap-allocated modal (caller must destroy)
 */
StatusEffectTooltipModal_t* CreateStatusEffectTooltipModal(void);

/**
 * DestroyStatusEffectTooltipModal - Cleanup modal resources
 *
 * @param modal - Pointer to modal pointer (will be set to NULL)
 */
void DestroyStatusEffectTooltipModal(StatusEffectTooltipModal_t** modal);

/**
 * ShowStatusEffectTooltipModal - Display modal at position
 *
 * @param modal - Modal component
 * @param effect - Status effect to display info for (NOT owned)
 * @param icon_x - X position of status effect icon
 * @param icon_y - Y position of status effect icon
 *
 * Modal will appear to the right of icon (or left if near screen edge)
 */
void ShowStatusEffectTooltipModal(StatusEffectTooltipModal_t* modal,
                                  const StatusEffectInstance_t* effect,
                                  int icon_x, int icon_y);

/**
 * HideStatusEffectTooltipModal - Hide modal
 *
 * @param modal - Modal component
 */
void HideStatusEffectTooltipModal(StatusEffectTooltipModal_t* modal);

/**
 * RenderStatusEffectTooltipModal - Render modal if visible
 *
 * @param modal - Modal component
 *
 * Displays:
 * - Effect full name
 * - Description text
 * - Effect value (e.g., "5 chips per round", "50% reduced winnings")
 * - Duration remaining (e.g., "3 rounds remaining")
 * - Color-coded by effect type
 */
void RenderStatusEffectTooltipModal(const StatusEffectTooltipModal_t* modal);

#endif // STATUS_EFFECT_TOOLTIP_MODAL_H
