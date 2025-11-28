/*
 * MenuItem Component Implementation
 */

#include "../../../include/scenes/components/menuItem.h"

// Color constants (from provided color palette)
#define COLOR_SELECTED_HOVER ((aColor_t){222, 158, 65, 255})  // #de9e41 - orange (hover)
#define COLOR_SELECTED       ((aColor_t){232, 193, 112, 255}) // #e8c170 - yellow/gold
#define COLOR_HOVER          ((aColor_t){115, 190, 211, 255}) // #73bed3 - cyan blue
#define COLOR_NORMAL         ((aColor_t){164, 221, 219, 255}) // #a4dddb - light cyan
#define COLOR_DISABLED       ((aColor_t){32, 46, 55, 255})    // #202e37 - dark gray
#define COLOR_INDICATOR      ((aColor_t){232, 193, 112, 255}) // #e8c170 - yellow/gold ">"

// Bounds constants
#define MENU_ITEM_PADDING         80   // Clickable padding around text (generous!)
#define AVG_CHAR_WIDTH            15   // Approximate character width for bounds
#define MENU_ITEM_TOP_PADDING     3    // Minimal padding above text (aligned with baseline)
#define MENU_ITEM_BOTTOM_PADDING  30   // Padding below text
#define MENU_ITEM_TOTAL_HEIGHT    35   // Total hover bounds height (no overlap!)

// ============================================================================
// LIFECYCLE
// ============================================================================

MenuItem_t* CreateMenuItem(int x, int y, int w, int h, const char* label) {
    MenuItem_t* item = malloc(sizeof(MenuItem_t));
    if (!item) {
        d_LogError("Failed to allocate MenuItem");
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

    // Calculate width from label if w=0 (auto-size)
    if (w == 0 && label) {
        int text_width = strlen(label) * AVG_CHAR_WIDTH;
        item->w = text_width + (MENU_ITEM_PADDING * 2);
    } else {
        item->w = w;
    }

    // Initialize hotkey as empty string
    item->hotkey[0] = '\0';

    return item;
}

void DestroyMenuItem(MenuItem_t** item) {
    if (!item || !*item) return;

    MenuItem_t* mi = *item;

    // No string cleanup needed - using fixed buffers!

    free(mi);
    *item = NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetMenuItemLabel(MenuItem_t* item, const char* label) {
    if (!item) return;
    strncpy(item->label, label ? label : "", sizeof(item->label) - 1);
    item->label[sizeof(item->label) - 1] = '\0';
}

void SetMenuItemHotkey(MenuItem_t* item, const char* hotkey) {
    if (!item) return;
    strncpy(item->hotkey, hotkey ? hotkey : "", sizeof(item->hotkey) - 1);
    item->hotkey[sizeof(item->hotkey) - 1] = '\0';
}

void SetMenuItemSelected(MenuItem_t* item, bool selected) {
    if (!item) return;
    item->is_selected = selected;
}

void SetMenuItemEnabled(MenuItem_t* item, bool enabled) {
    if (!item) return;
    item->enabled = enabled;
}

void SetMenuItemPosition(MenuItem_t* item, int x, int y) {
    if (!item) return;
    item->x = x;
    item->y = y;
}

// ============================================================================
// INTERACTION
// ============================================================================

bool IsMenuItemHovered(const MenuItem_t* item) {
    if (!item || !item->enabled) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Calculate text dimensions for accurate bounds
    float text_w, text_h;
    a_CalcTextDimensions((char*)item->label, FONT_ENTER_COMMAND, &text_w, &text_h);

    // Calculate bounds: x centered, y aligned with text rendering
    int left = item->x - (item->w / 2);
    int right = item->x + (item->w / 2);
    int top = item->y - 2;  // Minimal padding above text baseline
    int bottom = item->y + text_h + 2;  // Text height + small padding

    return (mx >= left && mx <= right && my >= top && my <= bottom);
}

bool IsMenuItemClicked(MenuItem_t* item) {
    if (!item || !item->enabled) return false;

    bool hovered = IsMenuItemHovered(item);

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

void RenderMenuItem(const MenuItem_t* item) {
    if (!item) return;

    // Update hover state
    MenuItem_t* mutable_item = (MenuItem_t*)item;  // Needed to update is_hovered
    mutable_item->is_hovered = IsMenuItemHovered(item);

    // Calculate text dimensions for accurate bounds
    float text_w, text_h;
    a_CalcTextDimensions((char*)item->label, FONT_ENTER_COMMAND, &text_w, &text_h);

    // Draw background highlight
    if (item->enabled) {
        int left = item->x - (item->w / 2);
        int top = item->y - 2;  // Minimal padding above text baseline
        int width = item->w;
        int height = text_h + 4;  // Text height + small padding top/bottom

        // Arrow-key selected: yellow background (~25% opacity)
        if (item->is_selected) {
            a_DrawFilledRect((aRectf_t){left, top, width, height}, (aColor_t){232, 193, 112, 64});  // #e8c170
        }
        // Mouse hover (but not selected): white background (~10% opacity)
        else if (item->is_hovered) {
            a_DrawFilledRect((aRectf_t){left, top, width, height}, (aColor_t){255, 255, 255, 25});
        }
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

    // Draw selection indicator ">" if selected (dynamically positioned)
    if (item->is_selected && item->enabled) {
        // Position indicator based on text width to prevent overlap
        int indicator_gap = 15;  // Gap between ">" and text
        int indicator_x = item->x - (text_w / 2) - indicator_gap;

        a_DrawText(">", indicator_x, item->y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_INDICATOR.r,COLOR_INDICATOR.g,COLOR_INDICATOR.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_RIGHT, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    // Draw label (centered at item position)
    a_DrawText((char*)item->label, item->x, item->y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={text_color.r,text_color.g,text_color.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw hotkey hint if present
    if (item->hotkey[0] != '\0' && item->enabled) {
        int hotkey_x = item->x + 150;  // To the right of label
        a_DrawText((char*)item->hotkey, hotkey_x, item->y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={150,150,150,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
    }
}
