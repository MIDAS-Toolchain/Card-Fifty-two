/*
 * Title Section
 * FlexBox section for game title and state display
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef TITLE_SECTION_H
#define TITLE_SECTION_H

#include "../../common.h"
#include "../../game.h"

// ============================================================================
// TITLE SECTION CONSTANTS
// ============================================================================

// Layout constants
#define TITLE_TEXT_Y_OFFSET     0   // Title text Y offset from section top
#define STATE_TEXT_Y_OFFSET     35  // State info Y offset from section top

// ============================================================================
// TITLE SECTION
// ============================================================================

typedef struct TitleSection {
    FlexBox_t* layout;  // Internal vertical FlexBox (optional, for future expansion)
} TitleSection_t;

/**
 * CreateTitleSection - Initialize title section
 *
 * @return TitleSection_t* - Heap-allocated section (caller must destroy)
 */
TitleSection_t* CreateTitleSection(void);

/**
 * DestroyTitleSection - Cleanup title section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyTitleSection(TitleSection_t** section);

/**
 * RenderTitleSection - Render game title and state info
 *
 * @param section - Title section component
 * @param game - Game context for state display
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - "Blackjack" title centered
 * - Game state and round number
 */
void RenderTitleSection(TitleSection_t* section, const GameContext_t* game, int y);

#endif // TITLE_SECTION_H
