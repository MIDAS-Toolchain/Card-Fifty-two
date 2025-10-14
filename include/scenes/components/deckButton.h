/*
 * Deck Button Component
 * Card-sized clickable button for deck/discard viewing
 * Renders as face-down card with count above and hotkey below
 */

#ifndef DECK_BUTTON_H
#define DECK_BUTTON_H

#include "../../common.h"

typedef struct DeckButton {
    int x, y, w, h;
    dString_t* count_text;      // Text shown above (e.g., "Draw: 52")
    dString_t* hotkey_hint;     // Hotkey shown below (e.g., "[V]")
    bool enabled;
    bool was_pressed;
    void* user_data;            // Optional custom data
} DeckButton_t;

// Lifecycle
DeckButton_t* CreateDeckButton(int x, int y, const char* count_text, const char* hotkey);
void DestroyDeckButton(DeckButton_t** button);

// Configuration
void SetDeckButtonCountText(DeckButton_t* button, const char* count_text);
void SetDeckButtonHotkey(DeckButton_t* button, const char* hotkey);
void SetDeckButtonEnabled(DeckButton_t* button, bool enabled);
void SetDeckButtonPosition(DeckButton_t* button, int x, int y);

// Interaction
bool IsDeckButtonClicked(DeckButton_t* button);
bool IsDeckButtonHovered(const DeckButton_t* button);

// Rendering
void RenderDeckButton(const DeckButton_t* button);

#endif // DECK_BUTTON_H
