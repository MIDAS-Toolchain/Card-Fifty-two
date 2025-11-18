/*
 * Ability Tooltip Modal Component
 * Shows detailed ability information on hover
 * Constitutional pattern: Reusable component
 */

#ifndef ABILITY_TOOLTIP_MODAL_H
#define ABILITY_TOOLTIP_MODAL_H

#include "../../common.h"
#include "../../enemy.h"

// Modal dimensions (flexible to fit content)
#define ABILITY_TOOLTIP_WIDTH   340
#define ABILITY_TOOLTIP_MIN_HEIGHT  200

typedef struct AbilityTooltipModal {
    bool visible;
    int x, y;
    const AbilityData_t* ability;  // Reference to ability (NOT owned)
} AbilityTooltipModal_t;

/**
 * CreateAbilityTooltipModal - Initialize tooltip modal
 *
 * @return AbilityTooltipModal_t* - Heap-allocated modal (caller must destroy)
 */
AbilityTooltipModal_t* CreateAbilityTooltipModal(void);

/**
 * DestroyAbilityTooltipModal - Cleanup modal resources
 *
 * @param modal - Pointer to modal pointer (will be set to NULL)
 */
void DestroyAbilityTooltipModal(AbilityTooltipModal_t** modal);

/**
 * ShowAbilityTooltipModal - Display modal at position
 *
 * @param modal - Modal component
 * @param ability - Ability to display info for (NOT owned)
 * @param card_x - X position of ability card
 * @param card_y - Y position of ability card
 *
 * Modal will appear to the right of card (or left if near screen edge)
 */
void ShowAbilityTooltipModal(AbilityTooltipModal_t* modal,
                             const AbilityData_t* ability,
                             int card_x, int card_y);

/**
 * HideAbilityTooltipModal - Hide modal
 *
 * @param modal - Modal component
 */
void HideAbilityTooltipModal(AbilityTooltipModal_t* modal);

/**
 * RenderAbilityTooltipModal - Render modal if visible
 *
 * @param modal - Modal component
 *
 * Displays:
 * - Ability full name
 * - Description/flavor text
 * - Trigger condition (e.g., "Every 5 cards drawn")
 * - Effect description (e.g., "Applies CHIP_DRAIN: 5 chips/round for 3 rounds")
 * - Current state (e.g., "Progress: 3/5" or "Already used")
 */
void RenderAbilityTooltipModal(const AbilityTooltipModal_t* modal);

#endif // ABILITY_TOOLTIP_MODAL_H
