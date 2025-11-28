/*
 * Ability Display Component - STUB Implementation
 * TODO: Rewrite to use new Ability_t system
 */

#include "../../../include/scenes/components/abilityDisplay.h"
#include <stdlib.h>

AbilityDisplay_t* CreateAbilityDisplay(Enemy_t* enemy) {
    AbilityDisplay_t* display = malloc(sizeof(AbilityDisplay_t));
    if (!display) return NULL;
    display->enemy = enemy;
    display->x = 0;
    display->y = 0;
    display->hovered_index = -1;
    return display;
}

void DestroyAbilityDisplay(AbilityDisplay_t** display) {
    if (!display || !*display) return;
    free(*display);
    *display = NULL;
}

void SetAbilityDisplayPosition(AbilityDisplay_t* display, int x, int y) {
    if (!display) return;
    display->x = x;
    display->y = y;
}

void SetAbilityDisplayEnemy(AbilityDisplay_t* display, Enemy_t* enemy) {
    if (!display) return;
    display->enemy = enemy;
}

void RenderAbilityDisplay(AbilityDisplay_t* display) {
    (void)display;
    // Stub - no rendering
}

const Ability_t* GetHoveredAbilityData(const AbilityDisplay_t* display) {
    (void)display;
    return NULL;  // Stub - no hover detection
}

bool GetHoveredAbilityPosition(const AbilityDisplay_t* display, int* out_x, int* out_y) {
    (void)display;
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    return false;  // Stub - no hover detection
}
