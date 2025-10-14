#ifndef MENU_ITEM_ROW_H
#define MENU_ITEM_ROW_H

#include "../../common.h"

/**
 * MenuItemRow Component
 *
 * Reusable menu option widget optimized for HORIZONTAL row layouts.
 * Uses tighter horizontal padding to prevent overlap when buttons are side-by-side.
 *
 * Key difference from MenuItem:
 * - MENU_ITEM_ROW_PADDING = 20 (vs 80 for vertical column layouts)
 * - Designed for confirmation dialogs, action panels, etc.
 *
 * Used in: pause menu confirmation dialog ("Yes" / "No" buttons)
 */

typedef struct MenuItemRow {
    int x, y, w, h;
    char label[256];       // Menu option text (e.g., "Yes", "No", "Cancel")
    char hotkey[64];       // Optional hotkey hint (e.g., "[Y]")
    bool is_selected;      // Currently selected by user
    bool is_hovered;       // Mouse is over this item (read-only, set by IsMenuItemRowHovered)
    bool enabled;          // Can be selected
    bool was_clicked;      // Click state tracking
    void* user_data;       // Custom data (optional)
} MenuItemRow_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateMenuItemRow - Create a new menu item for horizontal row layout
 *
 * @param x X position (center of button)
 * @param y Y position (baseline of text)
 * @param w Width (0 for auto-size based on text with tight padding)
 * @param h Height
 * @param label Menu option text
 * @return Heap-allocated MenuItemRow, or NULL on failure
 */
MenuItemRow_t* CreateMenuItemRow(int x, int y, int w, int h, const char* label);

/**
 * DestroyMenuItemRow - Cleanup and free menu item row
 *
 * @param item Pointer to MenuItemRow pointer (set to NULL after free)
 */
void DestroyMenuItemRow(MenuItemRow_t** item);

// ============================================================================
// CONFIGURATION
// ============================================================================

/**
 * SetMenuItemRowLabel - Update menu item text
 */
void SetMenuItemRowLabel(MenuItemRow_t* item, const char* label);

/**
 * SetMenuItemRowHotkey - Set keyboard shortcut hint
 */
void SetMenuItemRowHotkey(MenuItemRow_t* item, const char* hotkey);

/**
 * SetMenuItemRowSelected - Mark item as selected/deselected
 */
void SetMenuItemRowSelected(MenuItemRow_t* item, bool selected);

/**
 * SetMenuItemRowEnabled - Enable/disable item
 */
void SetMenuItemRowEnabled(MenuItemRow_t* item, bool enabled);

/**
 * SetMenuItemRowPosition - Update position
 */
void SetMenuItemRowPosition(MenuItemRow_t* item, int x, int y);

// ============================================================================
// INTERACTION
// ============================================================================

/**
 * IsMenuItemRowHovered - Check if mouse is over item
 */
bool IsMenuItemRowHovered(const MenuItemRow_t* item);

/**
 * IsMenuItemRowClicked - Check if item was clicked this frame
 */
bool IsMenuItemRowClicked(MenuItemRow_t* item);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderMenuItemRow - Draw menu item row with selection highlight
 *
 * Visual state:
 * - Selected: Yellow text with ">" indicator
 * - Normal: Gray text
 * - Disabled: Dark gray text
 */
void RenderMenuItemRow(const MenuItemRow_t* item);

#endif // MENU_ITEM_ROW_H
