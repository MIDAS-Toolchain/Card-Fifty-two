/*
 * Ability Tooltip Modal Component - STUB Implementation
 * TODO: Rewrite to use new Ability_t system
 */

#include "../../../include/scenes/components/abilityTooltipModal.h"
#include <stdlib.h>

AbilityTooltipModal_t* CreateAbilityTooltipModal(void) {
    AbilityTooltipModal_t* modal = malloc(sizeof(AbilityTooltipModal_t));
    if (!modal) return NULL;
    modal->visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->ability = NULL;
    modal->enemy = NULL;
    return modal;
}

void DestroyAbilityTooltipModal(AbilityTooltipModal_t** modal) {
    if (!modal || !*modal) return;
    free(*modal);
    *modal = NULL;
}

void ShowAbilityTooltipModal(AbilityTooltipModal_t* modal,
                             const Ability_t* ability,
                             const Enemy_t* enemy,
                             int card_x, int card_y) {
    if (!modal) return;
    modal->visible = true;
    modal->ability = ability;
    modal->enemy = enemy;
    modal->x = card_x;
    modal->y = card_y;
}

void HideAbilityTooltipModal(AbilityTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
    modal->ability = NULL;
    modal->enemy = NULL;
}

void RenderAbilityTooltipModal(const AbilityTooltipModal_t* modal) {
    (void)modal;
    // Stub - no rendering
}
