/*
 * Reusable Button Component Implementation
 */

#include "../../../include/scenes/components/button.h"

// Color constants (constitutional pattern)
#define COLOR_BUTTON_BG         ((aColor_t){60, 94, 139, 255})   // #3c5e8b - muted blue
#define COLOR_BUTTON_HOVER      ((aColor_t){76, 117, 171, 255})  // 25% lighter for hover
#define COLOR_BUTTON_DISABLED   ((aColor_t){80, 80, 80, 255})    // Gray
#define COLOR_BUTTON_TEXT       ((aColor_t){235, 237, 233, 255}) // #ebede9 - off-white
#define COLOR_BUTTON_BORDER     ((aColor_t){235, 237, 233, 255}) // #ebede9 - off-white
#define COLOR_BUTTON_INDICATOR  ((aColor_t){232, 193, 112, 255}) // #e8c170 - yellow/gold ">" (matches menu selection)

// ============================================================================
// LIFECYCLE
// ============================================================================

Button_t* CreateButton(int x, int y, int w, int h, const char* label) {
    Button_t* button = malloc(sizeof(Button_t));
    if (!button) {
        d_LogError("Failed to allocate button");
        return NULL;
    }

    button->x = x;
    button->y = y;
    button->w = w;
    button->h = h;
    button->enabled = true;
    button->is_selected = false;
    button->was_pressed = false;
    button->user_data = NULL;

    // Set label using strncpy
    strncpy(button->label, label ? label : "", sizeof(button->label) - 1);
    button->label[sizeof(button->label) - 1] = '\0';

    // Initialize hotkey hint as empty string
    button->hotkey_hint[0] = '\0';

    return button;
}

void DestroyButton(Button_t** button) {
    if (!button || !*button) return;

    Button_t* btn = *button;

    // No string cleanup needed - using fixed buffers!

    free(btn);
    *button = NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetButtonLabel(Button_t* button, const char* label) {
    if (!button) return;
    strncpy(button->label, label ? label : "", sizeof(button->label) - 1);
    button->label[sizeof(button->label) - 1] = '\0';
}

void SetButtonHotkey(Button_t* button, const char* hotkey) {
    if (!button) return;
    strncpy(button->hotkey_hint, hotkey ? hotkey : "", sizeof(button->hotkey_hint) - 1);
    button->hotkey_hint[sizeof(button->hotkey_hint) - 1] = '\0';
}

void SetButtonEnabled(Button_t* button, bool enabled) {
    if (!button) return;
    button->enabled = enabled;
}

void SetButtonSelected(Button_t* button, bool selected) {
    if (!button) return;
    button->is_selected = selected;
}

void SetButtonPosition(Button_t* button, int x, int y) {
    if (!button) return;
    button->x = x;
    button->y = y;
}

// ============================================================================
// INTERACTION
// ============================================================================

bool IsButtonClicked(Button_t* button) {
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

bool IsButtonHovered(const Button_t* button) {
    if (!button) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    return (mx >= button->x && mx <= button->x + button->w &&
            my >= button->y && my <= button->y + button->h);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderButton(const Button_t* button) {
    if (!button) return;

    // Button background color - hover only, selection doesn't change background
    aColor_t bg_color = button->enabled ? COLOR_BUTTON_BG : COLOR_BUTTON_DISABLED;

    // Hover state (lighter blue)
    if (button->enabled && IsButtonHovered(button)) {
        bg_color = COLOR_BUTTON_HOVER;
    }

    // Draw filled rectangle
    a_DrawFilledRect((aRectf_t){button->x, button->y, button->w, button->h},
                     bg_color);

    // Draw yellow selection overlay if arrow-key selected (~25% opacity)
    if (button->is_selected && button->enabled) {
        a_DrawFilledRect((aRectf_t){button->x, button->y, button->w, button->h},
                        (aColor_t){232, 193, 112, 64});  // #e8c170
    }

    // Draw border (off-white to match text)
    a_DrawRect((aRectf_t){button->x, button->y, button->w, button->h},
               COLOR_BUTTON_BORDER);

    // Draw selection indicator ">" if selected (to the LEFT of button, vertically centered)
    if (button->is_selected && button->enabled) {
        int indicator_x = button->x - 25;  // 25px to the left of button
        int indicator_y = button->y + button->h / 2 - 8;  // Vertically centered
        a_DrawText(">", indicator_x, indicator_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_BUTTON_INDICATOR.r,COLOR_BUTTON_INDICATOR.g,COLOR_BUTTON_INDICATOR.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    // Draw label (properly centered, off-white text)
    int text_w, text_h;
    a_CalcTextDimensions((char*)button->label, FONT_ENTER_COMMAND, &text_w, &text_h);

    int text_x = button->x + button->w / 2;
    int text_y = button->y + (button->h - text_h) / 2;
    // Cast safe: a_DrawText is read-only
    a_DrawText((char*)button->label, text_x, text_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_BUTTON_TEXT.r,COLOR_BUTTON_TEXT.g,COLOR_BUTTON_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw hotkey hint if present
    if (button->hotkey_hint[0] != '\0') {
        int hotkey_y = button->y + button->h + 5;
        // Cast safe: a_DrawText is read-only
        a_DrawText((char*)button->hotkey_hint, text_x, hotkey_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={180,180,180,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }
}
