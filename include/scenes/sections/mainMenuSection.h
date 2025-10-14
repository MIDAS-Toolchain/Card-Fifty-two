#ifndef MAIN_MENU_SECTION_H
#define MAIN_MENU_SECTION_H

#include "../../common.h"
#include "../components/menuItem.h"

/**
 * MainMenuSection Component
 *
 * Renders the main menu: title, subtitle, menu items, and instructions.
 * Uses FlexBox for menu item layout.
 */

typedef struct MainMenuSection {
    FlexBox_t* menu_layout;        // Vertical FlexBox for menu items
    MenuItem_t** menu_items;       // Array of MenuItem pointers (NOT owned!)
    int item_count;                // Number of menu items
    char title[256];               // "CARD FIFTY-TWO"
    char subtitle[256];            // "A Blackjack Demo"
    char instructions[512];        // Controls help text
} MainMenuSection_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateMainMenuSection - Create main menu section
 *
 * @param menu_items Array of MenuItem pointers (scene owns these!)
 * @param item_count Number of menu items
 * @return Heap-allocated MainMenuSection, or NULL on failure
 */
MainMenuSection_t* CreateMainMenuSection(MenuItem_t** menu_items, int item_count);

/**
 * DestroyMainMenuSection - Cleanup section
 *
 * NOTE: Does NOT destroy menu items - scene owns those!
 * Only destroys FlexBox owned by section.
 */
void DestroyMainMenuSection(MainMenuSection_t** section);

// ============================================================================
// CONFIGURATION
// ============================================================================

/**
 * SetMainMenuTitle - Update title text
 */
void SetMainMenuTitle(MainMenuSection_t* section, const char* title);

/**
 * SetMainMenuSubtitle - Update subtitle text
 */
void SetMainMenuSubtitle(MainMenuSection_t* section, const char* subtitle);

/**
 * SetMainMenuInstructions - Update instructions text
 */
void SetMainMenuInstructions(MainMenuSection_t* section, const char* instructions);

/**
 * UpdateMainMenuItemPositions - Sync FlexBox positions to MenuItems
 *
 * Call before rendering to update MenuItem positions from FlexBox.
 */
void UpdateMainMenuItemPositions(MainMenuSection_t* section);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderMainMenuSection - Render entire main menu
 *
 * Renders:
 * - Title (centered, top)
 * - Subtitle (centered, below title)
 * - Menu items (FlexBox positioned, vertical)
 * - Instructions (centered, bottom)
 *
 * @param section MainMenuSection to render
 */
void RenderMainMenuSection(MainMenuSection_t* section);

#endif // MAIN_MENU_SECTION_H
