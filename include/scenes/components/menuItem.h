#ifndef MENU_ITEM_H
#define MENU_ITEM_H

#include "../../common.h"

/**
 * MenuItem Component
 *
 * Reusable menu option widget with selection highlighting.
 * Used in main menu, pause menus, settings menus, etc.
 */

typedef struct MenuItem {
    int x, y, w, h;
    char label[256];       // Menu option text (e.g., "Play", "Settings")
    char hotkey[64];       // Optional hotkey hint (e.g., "[P]")
    bool is_selected;      // Currently selected by user
    bool is_hovered;       // Mouse is over this item (read-only, set by IsMenuItemHovered)
    bool enabled;          // Can be selected
    bool was_clicked;      // Click state tracking
    void* user_data;       // Custom data (optional)
} MenuItem_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateMenuItem - Create a new menu item
 *
 * @param x X position
 * @param y Y position
 * @param w Width (0 for auto-size based on text)
 * @param h Height
 * @param label Menu option text
 * @return Heap-allocated MenuItem, or NULL on failure
 */
MenuItem_t* CreateMenuItem(int x, int y, int w, int h, const char* label);

/**
 * DestroyMenuItem - Cleanup and free menu item
 *
 * @param item Pointer to MenuItem pointer (set to NULL after free)
 */
void DestroyMenuItem(MenuItem_t** item);

// ============================================================================
// CONFIGURATION
// ============================================================================

/**
 * SetMenuItemLabel - Update menu item text
 */
void SetMenuItemLabel(MenuItem_t* item, const char* label);

/**
 * SetMenuItemHotkey - Set keyboard shortcut hint
 */
void SetMenuItemHotkey(MenuItem_t* item, const char* hotkey);

/**
 * SetMenuItemSelected - Mark item as selected/deselected
 */
void SetMenuItemSelected(MenuItem_t* item, bool selected);

/**
 * SetMenuItemEnabled - Enable/disable item
 */
void SetMenuItemEnabled(MenuItem_t* item, bool enabled);

/**
 * SetMenuItemPosition - Update position
 */
void SetMenuItemPosition(MenuItem_t* item, int x, int y);

// ============================================================================
// INTERACTION
// ============================================================================

/**
 * IsMenuItemHovered - Check if mouse is over item
 */
bool IsMenuItemHovered(const MenuItem_t* item);

/**
 * IsMenuItemClicked - Check if item was clicked this frame
 */
bool IsMenuItemClicked(MenuItem_t* item);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderMenuItem - Draw menu item with selection highlight
 *
 * Visual state:
 * - Selected: Yellow text with ">" indicator
 * - Normal: Gray text
 * - Disabled: Dark gray text
 */
void RenderMenuItem(const MenuItem_t* item);

#endif // MENU_ITEM_H
