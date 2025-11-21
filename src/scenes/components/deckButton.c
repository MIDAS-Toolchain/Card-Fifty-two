/*
 * Deck Button Component Implementation
 * Renders as card-sized clickable face-down card
 */

#include "../../../include/scenes/components/deckButton.h"

// External globals
extern SDL_Surface* g_card_back_texture;

// Color constants
#define COLOR_HOVER_BORDER      ((aColor_t){232, 193, 112, 255}) // #e8c170 - gold
#define COLOR_COUNT_TEXT        ((aColor_t){200, 200, 200, 255}) // Light gray
#define COLOR_HOTKEY_TEXT       ((aColor_t){150, 150, 150, 255}) // Medium gray

// Layout constants
#define DECK_BUTTON_COUNT_OFFSET    40  // Space above card for count text
#define DECK_BUTTON_HOTKEY_OFFSET   5   // Space below card for hotkey text

// ============================================================================
// LIFECYCLE
// ============================================================================

DeckButton_t* CreateDeckButton(int x, int y, const char* hotkey) {
    DeckButton_t* button = malloc(sizeof(DeckButton_t));
    if (!button) {
        d_LogError("Failed to allocate DeckButton");
        return NULL;
    }

    button->x = x;
    button->y = y;
    button->w = CARD_WIDTH;
    button->h = CARD_HEIGHT;
    button->enabled = true;
    button->was_pressed = false;
    button->user_data = NULL;

    // Set hotkey using strncpy (static storage pattern)
    strncpy(button->hotkey_hint, hotkey ? hotkey : "", sizeof(button->hotkey_hint) - 1);
    button->hotkey_hint[sizeof(button->hotkey_hint) - 1] = '\0';

    return button;
}

void DestroyDeckButton(DeckButton_t** button) {
    if (!button || !*button) return;

    DeckButton_t* btn = *button;

    // No string cleanup needed - using fixed buffer!

    free(btn);
    *button = NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetDeckButtonHotkey(DeckButton_t* button, const char* hotkey) {
    if (!button) return;
    strncpy(button->hotkey_hint, hotkey ? hotkey : "", sizeof(button->hotkey_hint) - 1);
    button->hotkey_hint[sizeof(button->hotkey_hint) - 1] = '\0';
}

void SetDeckButtonEnabled(DeckButton_t* button, bool enabled) {
    if (!button) return;
    button->enabled = enabled;
}

void SetDeckButtonPosition(DeckButton_t* button, int x, int y) {
    if (!button) return;
    button->x = x;
    button->y = y;
}

// ============================================================================
// INTERACTION
// ============================================================================

bool IsDeckButtonClicked(DeckButton_t* button) {
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

bool IsDeckButtonHovered(const DeckButton_t* button) {
    if (!button) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    return (mx >= button->x && mx <= button->x + button->w &&
            my >= button->y && my <= button->y + button->h);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderDeckButton(const DeckButton_t* button, const char* count_text) {
    if (!button) return;

    // Draw card back texture (face-down card)
    if (g_card_back_texture) {
        a_BlitSurfaceRect(g_card_back_texture, (aRectf_t){button->x, button->y, button->w, button->h}, 1);
    } else {
        // Fallback if texture not loaded
        a_DrawFilledRect((aRectf_t){button->x, button->y, button->w, button->h}, (aColor_t){80, 80, 120, 255});
        a_DrawRect((aRectf_t){button->x, button->y, button->w, button->h}, (aColor_t){200, 200, 200, 255});
    }

    // Draw hover border (gold highlight)
    if (button->enabled && IsDeckButtonHovered(button)) {
        // Draw thick gold border on hover
        for (int i = 0; i < 3; i++) {
            a_DrawRect((aRectf_t){button->x - i, button->y - i, button->w + (i * 2), button->h + (i * 2)}, COLOR_HOVER_BORDER);
        }
    }

    // Draw count text above card (centered) - passed as parameter
    if (count_text && count_text[0] != '\0') {
        int count_x = button->x + button->w / 2;
        int count_y = button->y - DECK_BUTTON_COUNT_OFFSET;
        // Cast safe: a_DrawText is read-only, count_text is temporary from caller
        a_DrawText((char*)count_text, count_x, count_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_COUNT_TEXT.r,COLOR_COUNT_TEXT.g,COLOR_COUNT_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    // Draw hotkey below card (centered) - using fixed char buffer
    if (button->hotkey_hint[0] != '\0') {
        int hotkey_x = button->x + button->w / 2;
        int hotkey_y = button->y + button->h + DECK_BUTTON_HOTKEY_OFFSET;
        // Cast safe: a_DrawText is read-only, using fixed char buffer
        a_DrawText((char*)button->hotkey_hint, hotkey_x, hotkey_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_HOTKEY_TEXT.r,COLOR_HOTKEY_TEXT.g,COLOR_HOTKEY_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }
}
