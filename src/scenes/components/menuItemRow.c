/*
 * MenuItemRow Component Implementation
 * Optimized for horizontal row layouts (tighter padding to prevent button overlap)
 */

#include "../../../include/scenes/components/menuItemRow.h"

// Color constants (visual hierarchy)
#define COLOR_SELECTED_HOVER ((aColor_t){255, 255, 255, 255}) // Brightest - selected + hover
#define COLOR_SELECTED       ((aColor_t){255, 255, 0, 255})   // Yellow - selected
#define COLOR_HOVER          ((aColor_t){230, 230, 230, 255}) // Light white - hover only
#define COLOR_NORMAL         ((aColor_t){200, 200, 200, 255}) // Light gray - normal
#define COLOR_DISABLED       ((aColor_t){100, 100, 100, 255}) // Dark gray - disabled
#define COLOR_INDICATOR      ((aColor_t){255, 255, 0, 255})   // Yellow ">"

// Bounds constants - OPTIMIZED FOR HORIZONTAL LAYOUTS
#define MENU_ITEM_ROW_PADDING         20   // Tight horizontal padding (vs 80 for columns)
#define AVG_CHAR_WIDTH                15   // Approximate character width for bounds
#define MENU_ITEM_ROW_TOP_PADDING     3    // Minimal padding above text (aligned with baseline)
#define MENU_ITEM_ROW_BOTTOM_PADDING  30   // Padding below text
#define MENU_ITEM_ROW_TOTAL_HEIGHT    35   // Total hover bounds height

// ============================================================================
// LIFECYCLE
// ============================================================================

MenuItemRow_t* CreateMenuItemRow(int x, int y, int w, int h, const char* label) {
    MenuItemRow_t* item = malloc(sizeof(MenuItemRow_t));
    if (!item) {
        d_LogError("Failed to allocate MenuItemRow");
        return NULL;
    }

    item->x = x;
    item->y = y;
    item->h = h;
    item->is_selected = false;
    item->is_hovered = false;
    item->enabled = true;
    item->was_clicked = false;
    item->user_data = NULL;

    // Set label using strncpy
    strncpy(item->label, label ? label : "", sizeof(item->label) - 1);
    item->label[sizeof(item->label) - 1] = '\0';

    // Calculate width from label if w=0 (auto-size with TIGHT padding for rows)
    if (w == 0 && label) {
        int text_width = strlen(label) * AVG_CHAR_WIDTH;
        item->w = text_width + (MENU_ITEM_ROW_PADDING * 2);  // 40px total (vs 160px for columns)
    } else {
        item->w = w;
    }

    // Initialize hotkey as empty string
    item->hotkey[0] = '\0';

    return item;
}

void DestroyMenuItemRow(MenuItemRow_t** item) {
    if (!item || !*item) return;

    MenuItemRow_t* mi = *item;

    // No string cleanup needed - using fixed buffers!

    free(mi);
    *item = NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetMenuItemRowLabel(MenuItemRow_t* item, const char* label) {
    if (!item) return;
    strncpy(item->label, label ? label : "", sizeof(item->label) - 1);
    item->label[sizeof(item->label) - 1] = '\0';
}

void SetMenuItemRowHotkey(MenuItemRow_t* item, const char* hotkey) {
    if (!item) return;
    strncpy(item->hotkey, hotkey ? hotkey : "", sizeof(item->hotkey) - 1);
    item->hotkey[sizeof(item->hotkey) - 1] = '\0';
}

void SetMenuItemRowSelected(MenuItemRow_t* item, bool selected) {
    if (!item) return;
    item->is_selected = selected;
}

void SetMenuItemRowEnabled(MenuItemRow_t* item, bool enabled) {
    if (!item) return;
    item->enabled = enabled;
}

void SetMenuItemRowPosition(MenuItemRow_t* item, int x, int y) {
    if (!item) return;
    item->x = x;
    item->y = y;
}

// ============================================================================
// INTERACTION
// ============================================================================

bool IsMenuItemRowHovered(const MenuItemRow_t* item) {
    if (!item || !item->enabled) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Calculate text dimensions for accurate bounds
    int text_w, text_h;
    a_CalcTextDimensions((char*)item->label, FONT_ENTER_COMMAND, &text_w, &text_h);

    // Calculate bounds: x centered, y aligned with text rendering
    int left = item->x - (item->w / 2);
    int right = item->x + (item->w / 2);
    int top = item->y - 2;  // Minimal padding above text baseline
    int bottom = item->y + text_h + 2;  // Text height + small padding

    return (mx >= left && mx <= right && my >= top && my <= bottom);
}

bool IsMenuItemRowClicked(MenuItemRow_t* item) {
    if (!item || !item->enabled) return false;

    bool hovered = IsMenuItemRowHovered(item);

    // Press started
    if (app.mouse.pressed && hovered && !item->was_clicked) {
        item->was_clicked = true;
        return false;
    }

    // Press released (click complete)
    if (!app.mouse.pressed && item->was_clicked && hovered) {
        item->was_clicked = false;
        return true;  // Click completed!
    }

    // Reset if released outside
    if (!app.mouse.pressed) {
        item->was_clicked = false;
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderMenuItemRow(const MenuItemRow_t* item) {
    if (!item) return;

    // Update hover state
    MenuItemRow_t* mutable_item = (MenuItemRow_t*)item;  // Needed to update is_hovered
    mutable_item->is_hovered = IsMenuItemRowHovered(item);

    // Calculate text dimensions for accurate bounds
    int text_w, text_h;
    a_CalcTextDimensions((char*)item->label, FONT_ENTER_COMMAND, &text_w, &text_h);

    // Draw hover background (semi-transparent white rectangle)
    if (item->is_hovered && item->enabled) {
        int left = item->x - (item->w / 2);
        int top = item->y - 2;  // Minimal padding above text baseline
        int width = item->w;
        int height = text_h + 4;  // Text height + small padding top/bottom

        // Very subtle semi-transparent white background (~10% opacity)
        a_DrawFilledRect((aRectf_t){left, top, width, height}, (aColor_t){255, 255, 255, 25});
    }

    // Determine color based on state (visual hierarchy)
    aColor_t text_color;
    if (!item->enabled) {
        text_color = COLOR_DISABLED;
    } else if (item->is_selected && item->is_hovered) {
        text_color = COLOR_SELECTED_HOVER;  // Brightest!
    } else if (item->is_selected) {
        text_color = COLOR_SELECTED;
    } else if (item->is_hovered) {
        text_color = COLOR_HOVER;
    } else {
        text_color = COLOR_NORMAL;
    }

    // Draw selection indicator ">" if selected (slightly closer for row layout)
    if (item->is_selected && item->enabled) {
        a_DrawText(">", item->x - 50, item->y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_INDICATOR.r,COLOR_INDICATOR.g,COLOR_INDICATOR.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_RIGHT, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    // Draw label (centered at item position)
    a_DrawText((char*)item->label, item->x, item->y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={text_color.r,text_color.g,text_color.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw hotkey hint if present
    if (item->hotkey[0] != '\0' && item->enabled) {
        int hotkey_x = item->x + 80;  // Closer to label for row layout
        a_DrawText((char*)item->hotkey, hotkey_x, item->y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={150,150,150,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
    }
}
