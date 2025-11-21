/*
 * Sidebar Button Component Implementation
 * Compact two-line button for sidebar
 */

#include "../../../include/scenes/components/sidebarButton.h"

// Color constants from palette
#define COLOR_BG            ((aColor_t){60, 94, 139, 255})   // #3c5e8b - muted blue
#define COLOR_BG_DARK       ((aColor_t){37, 58, 94, 255})    // #253a5e - darker variant
#define COLOR_HOVER         ((aColor_t){79, 143, 186, 255})  // #4f8fba - lighter blue
#define COLOR_BORDER        ((aColor_t){57, 74, 80, 255})    // #394a50 - dark blue-gray
#define COLOR_TEXT          ((aColor_t){235, 237, 233, 255}) // #ebede9 - off-white
#define COLOR_COUNT_TEXT    ((aColor_t){164, 221, 219, 255}) // #a4dddb - light cyan
#define COLOR_HOTKEY_TEXT   ((aColor_t){115, 190, 211, 255}) // #73bed3 - cyan blue
#define COLOR_DISABLED      ((aColor_t){80, 80, 80, 255})    // Gray
#define COLOR_INDICATOR     ((aColor_t){232, 193, 112, 255}) // #e8c170 - gold ">"

// ============================================================================
// LIFECYCLE
// ============================================================================

SidebarButton_t* CreateSidebarButton(int x, int y, const char* label, const char* hotkey) {
    SidebarButton_t* button = malloc(sizeof(SidebarButton_t));
    if (!button) {
        d_LogError("Failed to allocate SidebarButton");
        return NULL;
    }

    button->x = x;
    button->y = y;
    button->w = SIDEBAR_BUTTON_WIDTH;
    button->h = SIDEBAR_BUTTON_HEIGHT;
    button->enabled = true;
    button->is_selected = false;
    button->was_pressed = false;
    button->user_data = NULL;

    // Set label using strncpy
    strncpy(button->label, label ? label : "", sizeof(button->label) - 1);
    button->label[sizeof(button->label) - 1] = '\0';

    // Set hotkey using strncpy
    strncpy(button->hotkey_hint, hotkey ? hotkey : "", sizeof(button->hotkey_hint) - 1);
    button->hotkey_hint[sizeof(button->hotkey_hint) - 1] = '\0';

    return button;
}

void DestroySidebarButton(SidebarButton_t** button) {
    if (!button || !*button) return;

    SidebarButton_t* btn = *button;

    // No string cleanup needed - using fixed buffers!

    free(btn);
    *button = NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetSidebarButtonLabel(SidebarButton_t* button, const char* label) {
    if (!button) return;
    strncpy(button->label, label ? label : "", sizeof(button->label) - 1);
    button->label[sizeof(button->label) - 1] = '\0';
}

void SetSidebarButtonHotkey(SidebarButton_t* button, const char* hotkey) {
    if (!button) return;
    strncpy(button->hotkey_hint, hotkey ? hotkey : "", sizeof(button->hotkey_hint) - 1);
    button->hotkey_hint[sizeof(button->hotkey_hint) - 1] = '\0';
}

void SetSidebarButtonEnabled(SidebarButton_t* button, bool enabled) {
    if (!button) return;
    button->enabled = enabled;
}

void SetSidebarButtonSelected(SidebarButton_t* button, bool selected) {
    if (!button) return;
    button->is_selected = selected;
}

void SetSidebarButtonPosition(SidebarButton_t* button, int x, int y) {
    if (!button) return;
    button->x = x;
    button->y = y;
}

// ============================================================================
// INTERACTION
// ============================================================================

bool IsSidebarButtonClicked(SidebarButton_t* button) {
    if (!button || !button->enabled) {
        return false;
    }

    int mx = app.mouse.x;
    int my = app.mouse.y;

    bool mouse_over = (mx >= button->x && mx <= button->x + button->w &&
                       my >= button->y && my <= button->y + button->h);

    // Press started
    if (app.mouse.pressed && mouse_over && !button->was_pressed) {
        button->was_pressed = true;
        return false;
    }

    // Press released (click complete)
    if (!app.mouse.pressed && button->was_pressed && mouse_over) {
        button->was_pressed = false;
        return true;
    }

    // Reset if released outside
    if (!app.mouse.pressed) {
        button->was_pressed = false;
    }

    return false;
}

bool IsSidebarButtonHovered(const SidebarButton_t* button) {
    if (!button) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    return (mx >= button->x && mx <= button->x + button->w &&
            my >= button->y && my <= button->y + button->h);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderSidebarButton(const SidebarButton_t* button, const char* count_text) {
    if (!button) return;

    // Background color
    aColor_t bg_color = button->enabled ? COLOR_BG_DARK : COLOR_DISABLED;

    // Hover state (lighter blue)
    if (button->enabled && IsSidebarButtonHovered(button)) {
        bg_color = COLOR_HOVER;
    }

    // Draw filled rectangle
    a_DrawFilledRect((aRectf_t){button->x, button->y, button->w, button->h}, bg_color);

    // Draw selection overlay if arrow-key selected (~25% opacity)
    if (button->is_selected && button->enabled) {
        a_DrawFilledRect((aRectf_t){button->x, button->y, button->w, button->h},
                         (aColor_t){COLOR_INDICATOR.r, COLOR_INDICATOR.g, COLOR_INDICATOR.b, 64});
    }

    // Draw border
    a_DrawRect((aRectf_t){button->x, button->y, button->w, button->h}, COLOR_BORDER);

    // Calculate center positions
    int center_x = button->x + button->w / 2;
    int label_y = button->y + SIDEBAR_BUTTON_LABEL_OFFSET_Y;
    int count_y = button->y + SIDEBAR_BUTTON_COUNT_OFFSET_Y;

    // Draw main label (centered)
    a_DrawText((char*)button->label, center_x, label_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_TEXT.r,COLOR_TEXT.g,COLOR_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw count text if provided (centered, smaller/lighter color)
    if (count_text && count_text[0] != '\0') {
        a_DrawText((char*)count_text, center_x, count_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_COUNT_TEXT.r,COLOR_COUNT_TEXT.g,COLOR_COUNT_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    // Draw hotkey in bottom-right corner if present (moved up 12px for visibility)
    if (button->hotkey_hint[0] != '\0') {
        int hotkey_x = button->x + button->w - SIDEBAR_BUTTON_HOTKEY_PADDING;
        int hotkey_y = button->y + button->h - SIDEBAR_BUTTON_HOTKEY_PADDING - 12;
        a_DrawText((char*)button->hotkey_hint, hotkey_x, hotkey_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_HOTKEY_TEXT.r,COLOR_HOTKEY_TEXT.g,COLOR_HOTKEY_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_RIGHT, .wrap_width=0, .scale=1.0f, .padding=0});
    }
}
