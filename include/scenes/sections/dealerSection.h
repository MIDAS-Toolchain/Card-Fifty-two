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

// ============================================================================
// DEALER SECTION
// ============================================================================

typedef struct DealerSection {
    FlexBox_t* layout;              // Internal vertical FlexBox (optional, for future expansion)
    EnemyHealthBar_t* enemy_hp_bar; // Enemy health bar component (owned)
    CardHoverState_t hover_state;   // Card hover animation state
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
 */
void RenderDealerSection(DealerSection_t* section, Player_t* dealer, Enemy_t* enemy, int y);

#endif // DEALER_SECTION_H
