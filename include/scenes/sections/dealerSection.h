/*
 * Dealer Section
 * FlexBox section for dealer name, score, and card display
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef DEALER_SECTION_H
#define DEALER_SECTION_H

#include "../../common.h"
#include "../../player.h"
#include "../../enemy.h"
#include "../components/enemyHealthBar.h"
#include "../components/abilityDisplay.h"
#include "../components/abilityTooltipModal.h"
#include "../components/cardTooltipModal.h"

// ============================================================================
// DEALER SECTION
// ============================================================================

typedef struct DealerSection {
    FlexBox_t* layout;                       // Internal vertical FlexBox (optional, for future expansion)
    EnemyHealthBar_t* enemy_hp_bar;          // Enemy health bar component (owned)
    AbilityDisplay_t* ability_display;       // Enemy ability display component (owned)
    AbilityTooltipModal_t* ability_tooltip;  // Ability hover tooltip modal (owned)
    CardTooltipModal_t* card_tooltip;        // Card hover tooltip modal (owned)
    CardHoverState_t hover_state;            // Card hover animation state
    float ability_hover_timer;               // Timer for tutorial ability hover tracking
    int abilities_hovered_count;             // Number of unique abilities hovered (for tutorial)
    bool abilities_hovered[8];               // Track which abilities have been hovered (max 8)
    bool ability_tutorial_completed;         // Track if tutorial event already triggered
} DealerSection_t;

/**
 * CreateDealerSection - Initialize dealer section
 *
 * @return DealerSection_t* - Heap-allocated section (caller must destroy)
 */
DealerSection_t* CreateDealerSection(void);

/**
 * DestroyDealerSection - Cleanup dealer section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyDealerSection(DealerSection_t** section);

/**
 * RenderDealerSection - Render dealer info and cards
 *
 * @param section - Dealer section component
 * @param dealer - Dealer player to render
 * @param enemy - Current enemy (for HP bar, NULL if not in combat)
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Dealer name with "X + ?" score for hidden cards
 * - Enemy health bar next to dealer name (if in combat)
 * - Centered card rendering
 * - Updates card tooltip state (but doesn't render it - see RenderDealerSectionTooltip)
 *
 * NOTE: Card tooltip rendering separated for z-ordering (must render after trinket menu)
 */
void RenderDealerSection(DealerSection_t* section, Player_t* dealer, Enemy_t* enemy, int y);

/**
 * RenderDealerSectionTooltip - Render dealer hand card tooltip
 *
 * @param section - Dealer section component
 *
 * Renders tooltip on top of everything (including trinket menu).
 * Must be called AFTER RenderTrinketUI() in scene rendering order.
 */
void RenderDealerSectionTooltip(DealerSection_t* section);

/**
 * UpdateDealerAbilityHoverTracking - Update ability hover tracking for tutorial
 *
 * Call this each frame to track when user hovers over enemy abilities.
 * Requires hovering over ALL active abilities (1 second each) to trigger event.
 *
 * @param section - Dealer section component
 * @param enemy - Current enemy (NULL if not in combat)
 * @param dt - Delta time in seconds
 * @return bool - true if all abilities have been hovered
 */
bool UpdateDealerAbilityHoverTracking(DealerSection_t* section, Enemy_t* enemy, float dt);

#endif // DEALER_SECTION_H
