/*
 * Deck Button Component Implementation
 * Renders as card-sized clickable face-down card
 */

#include "../../../include/scenes/components/deckButton.h"

// External globals
extern SDL_Texture* g_card_back_texture;

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

DeckButton_t* CreateDeckButton(int x, int y, const char* count_text, const char* hotkey) {
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

    // Constitutional: dString_t for count text
    button->count_text = d_InitString();
    if (!button->count_text) {
        free(button);
        d_LogError("Failed to allocate DeckButton count_text");
        return NULL;
    }
    d_SetString(button->count_text, count_text ? count_text : "", 0);

    // Constitutional: dString_t for hotkey hint
    button->hotkey_hint = d_InitString();
    if (!button->hotkey_hint) {
        d_DestroyString(button->count_text);
        free(button);
        d_LogError("Failed to allocate DeckButton hotkey_hint");
        return NULL;
    }
    d_SetString(button->hotkey_hint, hotkey ? hotkey : "", 0);

    return button;
}

void DestroyDeckButton(DeckButton_t** button) {
    if (!button || !*button) return;

    DeckButton_t* btn = *button;

    // Cleanup dString_t members
    if (btn->count_text) {
        d_DestroyString(btn->count_text);
    }
    if (btn->hotkey_hint) {
        d_DestroyString(btn->hotkey_hint);
    }

    free(btn);
    *button = NULL;
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetDeckButtonCountText(DeckButton_t* button, const char* count_text) {
    if (!button || !button->count_text) return;
    d_SetString(button->count_text, count_text ? count_text : "", 0);
}

void SetDeckButtonHotkey(DeckButton_t* button, const char* hotkey) {
    if (!button || !button->hotkey_hint) return;
    d_SetString(button->hotkey_hint, hotkey ? hotkey : "", 0);
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

void RenderDeckButton(const DeckButton_t* button) {
    if (!button) return;

    // Draw card back texture (face-down card)
    if (g_card_back_texture) {
        SDL_Rect dst = {button->x, button->y, button->w, button->h};
        SDL_RenderCopy(app.renderer, g_card_back_texture, NULL, &dst);
    } else {
        // Fallback if texture not loaded
        a_DrawFilledRect(button->x, button->y, button->w, button->h, 80, 80, 120, 255);
        a_DrawRect(button->x, button->y, button->w, button->h, 200, 200, 200, 255);
    }

    // Draw hover border (gold highlight)
    if (button->enabled && IsDeckButtonHovered(button)) {
        // Draw thick gold border on hover
        for (int i = 0; i < 3; i++) {
            a_DrawRect(button->x - i, button->y - i, button->w + (i * 2), button->h + (i * 2),
                       COLOR_HOVER_BORDER.r, COLOR_HOVER_BORDER.g, COLOR_HOVER_BORDER.b, COLOR_HOVER_BORDER.a);
        }
    }

    // Draw count text above card (centered)
    if (button->count_text) {
        const char* count_str = d_PeekString(button->count_text);
        if (count_str && count_str[0] != '\0') {
            int count_x = button->x + button->w / 2;
            int count_y = button->y - DECK_BUTTON_COUNT_OFFSET;
            a_DrawText((char*)count_str, count_x, count_y,
                       COLOR_COUNT_TEXT.r, COLOR_COUNT_TEXT.g, COLOR_COUNT_TEXT.b,
                       FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
        }
    }

    // Draw hotkey below card (centered)
    if (button->hotkey_hint) {
        const char* hotkey_str = d_PeekString(button->hotkey_hint);
        if (hotkey_str && hotkey_str[0] != '\0') {
            int hotkey_x = button->x + button->w / 2;
            int hotkey_y = button->y + button->h + DECK_BUTTON_HOTKEY_OFFSET;
            a_DrawText((char*)hotkey_str, hotkey_x, hotkey_y,
                       COLOR_HOTKEY_TEXT.r, COLOR_HOTKEY_TEXT.g, COLOR_HOTKEY_TEXT.b,
                       FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
        }
    }
}
