/*
 * Sidebar Button Component
 * Compact button for sidebar navigation/actions
 * Two-line display: Label + dynamic count/info
 */

#ifndef SIDEBAR_BUTTON_H
#define SIDEBAR_BUTTON_H

#include "../../common.h"

// Sidebar button dimensions
#define SIDEBAR_BUTTON_WIDTH  240
#define SIDEBAR_BUTTON_HEIGHT 65

// Internal text positioning
#define SIDEBAR_BUTTON_LABEL_OFFSET_Y   10   // Label Y offset from button top
#define SIDEBAR_BUTTON_COUNT_OFFSET_Y   55   // Count text Y offset from button top
#define SIDEBAR_BUTTON_HOTKEY_PADDING   8    // Padding for hotkey from edges

typedef struct SidebarButton {
    int x, y, w, h;
    char label[128];            // Main label (e.g., "Draw Pile")
    char hotkey_hint[64];       // Hotkey shown in corner (e.g., "[V]")
    bool enabled;
    bool is_selected;           // Keyboard navigation selection
    bool was_pressed;
    void* user_data;            // Optional custom data
} SidebarButton_t;

// Lifecycle
SidebarButton_t* CreateSidebarButton(int x, int y, const char* label, const char* hotkey);
void DestroySidebarButton(SidebarButton_t** button);

// Configuration
void SetSidebarButtonLabel(SidebarButton_t* button, const char* label);
void SetSidebarButtonHotkey(SidebarButton_t* button, const char* hotkey);
void SetSidebarButtonEnabled(SidebarButton_t* button, bool enabled);
void SetSidebarButtonSelected(SidebarButton_t* button, bool selected);
void SetSidebarButtonPosition(SidebarButton_t* button, int x, int y);

// Interaction
bool IsSidebarButtonClicked(SidebarButton_t* button);
bool IsSidebarButtonHovered(const SidebarButton_t* button);

// Rendering (count_text is dynamic, passed from caller each frame)
void RenderSidebarButton(const SidebarButton_t* button, const char* count_text);

#endif // SIDEBAR_BUTTON_H
