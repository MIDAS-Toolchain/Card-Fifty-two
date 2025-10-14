/*
 * Dealer Section
 * FlexBox section for dealer name, score, and card display
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef DEALER_SECTION_H
#define DEALER_SECTION_H

#include "../../common.h"
#include "../../player.h"

// ============================================================================
// DEALER SECTION
// ============================================================================

typedef struct DealerSection {
    FlexBox_t* layout;  // Internal vertical FlexBox (optional, for future expansion)
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
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Dealer name with "X + ?" score for hidden cards
 * - Centered card rendering
 */
void RenderDealerSection(DealerSection_t* section, Player_t* dealer, int y);

#endif // DEALER_SECTION_H
