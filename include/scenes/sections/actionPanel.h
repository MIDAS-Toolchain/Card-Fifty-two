/*
 * Action Panel Section Component
 * FlexBox section for instruction text + button row
 * Reusable for both betting and player action panels
 * Constitutional pattern: Stack-allocated struct, proper lifecycle
 */

#ifndef ACTION_PANEL_H
#define ACTION_PANEL_H

#include "../../common.h"
#include "../components/button.h"

// ============================================================================
// ACTION PANEL COMPONENT
// ============================================================================

typedef struct ActionPanel {
    FlexBox_t* button_row;         // Horizontal FlexBox for buttons
    char instruction_text[256];    // Static instruction text (e.g., "Place Your Bet:")
    Button_t** buttons;            // Array of button pointers (NOT owned by panel)
    int button_count;
} ActionPanel_t;

/**
 * CreateActionPanel - Initialize action panel component
 *
 * @param instruction - Instruction text to display above buttons
 * @param buttons - Array of Button_t* (panel does NOT take ownership)
 * @param button_count - Number of buttons in array
 * @return ActionPanel_t* - Heap-allocated panel (caller must destroy)
 *
 * NOTE: Panel stores button pointers but does NOT destroy them.
 * Caller retains ownership of buttons.
 */
ActionPanel_t* CreateActionPanel(const char* instruction, Button_t** buttons, int button_count);

/**
 * DestroyActionPanel - Cleanup action panel resources
 *
 * @param panel - Pointer to panel pointer (will be set to NULL)
 *
 * NOTE: Does NOT destroy buttons (caller owns them)
 */
void DestroyActionPanel(ActionPanel_t** panel);

/**
 * RenderActionPanel - Render instruction text and buttons
 *
 * @param panel - Action panel component
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Instruction text centered above buttons
 * - Button row FlexBox layout
 * - Button rendering
 */
void RenderActionPanel(ActionPanel_t* panel, int y);

/**
 * UpdateActionPanelButtons - Sync button positions from FlexBox
 *
 * @param panel - Action panel component
 *
 * Call this before checking button clicks to ensure positions are current.
 */
void UpdateActionPanelButtons(ActionPanel_t* panel);

#endif // ACTION_PANEL_H
