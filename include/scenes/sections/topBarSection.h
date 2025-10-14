/*
 * Top Bar Section
 * Persistent top navigation bar for Blackjack scene
 * Constitutional pattern: Section-based architecture with Button component
 */

#ifndef TOP_BAR_SECTION_H
#define TOP_BAR_SECTION_H

#include "../../common.h"
#include "../components/button.h"

// ============================================================================
// TOP BAR SECTION CONSTANTS
// ============================================================================

// Settings button layout
#define SETTINGS_BUTTON_WIDTH    40
#define SETTINGS_BUTTON_HEIGHT   25
#define SETTINGS_BUTTON_MARGIN   20  // Margin from right edge
#define SETTINGS_BUTTON_Y_OFFSET 5   // Vertical centering offset from top bar top

// ============================================================================
// TOP BAR SECTION
// ============================================================================

typedef struct TopBarSection {
    Button_t* settings_button;  // Settings icon button (far right)
} TopBarSection_t;

/**
 * CreateTopBarSection - Initialize top bar section
 *
 * @return TopBarSection_t* - Heap-allocated section (caller must destroy)
 */
TopBarSection_t* CreateTopBarSection(void);

/**
 * DestroyTopBarSection - Cleanup top bar section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyTopBarSection(TopBarSection_t** section);

/**
 * RenderTopBarSection - Render top navigation bar
 *
 * @param section - Top bar section component
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Settings button on far right
 * - Future: back button, breadcrumbs, title on left
 */
void RenderTopBarSection(TopBarSection_t* section, int y);

/**
 * IsTopBarSettingsClicked - Check if settings button was clicked
 *
 * @param section - Top bar section component
 * @return true if settings button clicked this frame
 */
bool IsTopBarSettingsClicked(TopBarSection_t* section);

#endif // TOP_BAR_SECTION_H
