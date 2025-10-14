/*
 * Reusable Button Component
 * Uses char buffers for labels (static storage, not dynamic building)
 */

#ifndef BUTTON_H
#define BUTTON_H

#include "../../common.h"

typedef struct Button {
    int x, y, w, h;
    char label[256];            // Simple buffer for button text
    char hotkey_hint[64];       // Optional hotkey display (e.g., "[H]")
    bool enabled;
    bool was_pressed;
    void* user_data;            // Optional custom data
} Button_t;

// Lifecycle
Button_t* CreateButton(int x, int y, int w, int h, const char* label);
void DestroyButton(Button_t** button);

// Configuration
void SetButtonLabel(Button_t* button, const char* label);
void SetButtonHotkey(Button_t* button, const char* hotkey);
void SetButtonEnabled(Button_t* button, bool enabled);
void SetButtonPosition(Button_t* button, int x, int y);

// Interaction
bool IsButtonClicked(Button_t* button);
bool IsButtonHovered(const Button_t* button);

// Rendering
void RenderButton(const Button_t* button);

#endif // BUTTON_H
