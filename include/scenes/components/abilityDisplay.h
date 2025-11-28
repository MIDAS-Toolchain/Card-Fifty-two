/*
 * Ability Display Component
 * Shows enemy abilities with counter progress and cooldown state
 * Constitutional pattern: Reusable component
 */

#ifndef ABILITY_DISPLAY_H
#define ABILITY_DISPLAY_H

#include "../../common.h"
#include "../../enemy.h"

// Ability card dimensions (vertical layout)
#define ABILITY_CARD_WIDTH   80
#define ABILITY_CARD_HEIGHT  96
#define ABILITY_CARD_SPACING 12

typedef struct AbilityDisplay {
    Enemy_t* enemy;          // Reference to enemy (NOT owned)
    int x, y;                // Position for rendering
    int hovered_index;       // Index of hovered ability (-1 = none)
} AbilityDisplay_t;

/**
 * CreateAbilityDisplay - Initialize ability display component
 *
 * @param enemy - Reference to enemy (NOT owned)
 * @return AbilityDisplay_t* - Heap-allocated component (caller must destroy)
 */
AbilityDisplay_t* CreateAbilityDisplay(Enemy_t* enemy);

/**
 * DestroyAbilityDisplay - Cleanup ability display resources
 *
 * @param display - Pointer to display pointer (will be set to NULL)
 */
void DestroyAbilityDisplay(AbilityDisplay_t** display);

/**
 * SetAbilityDisplayPosition - Update display position
 *
 * @param display - Ability display component
 * @param x - New X position
 * @param y - New Y position
 */
void SetAbilityDisplayPosition(AbilityDisplay_t* display, int x, int y);

/**
 * SetAbilityDisplayEnemy - Update enemy reference
 *
 * @param display - Ability display component
 * @param enemy - New enemy reference (NOT owned)
 */
void SetAbilityDisplayEnemy(AbilityDisplay_t* display, Enemy_t* enemy);

/**
 * RenderAbilityDisplay - Render enemy abilities with state
 *
 * @param display - Ability display component
 *
 * Renders each active ability as a VERTICAL card showing:
 * - Ability icon (texture if available in g_ability_icons, falls back to 2-letter abbreviation)
 * - Counter progress if TRIGGER_COUNTER (e.g., "3/5")
 * - "USED" badge if already triggered
 * - Dimmed appearance if already triggered (cooldown)
 * - Color-coded by ability type
 * - Hover detection for tooltip modal
 *
 * Icon loading: Add textures to g_ability_icons table with EnemyAbility_t enum as key.
 * If no texture exists, displays text abbreviation as fallback.
 */
void RenderAbilityDisplay(AbilityDisplay_t* display);

/**
 * GetHoveredAbilityData - Get ability for currently hovered ability
 *
 * @param display - Ability display component
 * @return Ability_t* - Hovered ability, or NULL if none hovered
 */
const Ability_t* GetHoveredAbilityData(const AbilityDisplay_t* display);

/**
 * GetHoveredAbilityPosition - Get screen position of hovered ability card
 *
 * @param display - Ability display component
 * @param out_x - Output X position of card
 * @param out_y - Output Y position of card
 * @return bool - true if ability is hovered, false otherwise
 */
bool GetHoveredAbilityPosition(const AbilityDisplay_t* display, int* out_x, int* out_y);

#endif // ABILITY_DISPLAY_H
