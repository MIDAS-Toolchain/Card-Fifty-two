/*
 * Action Panel Section
 */

#include "../../../include/scenes/sections/actionPanel.h"
#include "../../../include/scenes/sceneBlackjack.h"

// ============================================================================
// ACTION PANEL LIFECYCLE
// ============================================================================

ActionPanel_t* CreateActionPanel(const char* instruction, Button_t** buttons, int button_count) {
    // Constitutional pattern: NULL checks
    if (!instruction) {
        d_LogError("CreateActionPanel: Invalid instruction");
        return NULL;
    }

    // Allow NULL buttons for instruction-only panels
    if (button_count < 0) {
        d_LogError("CreateActionPanel: Invalid button_count");
        return NULL;
    }

    ActionPanel_t* panel = malloc(sizeof(ActionPanel_t));
    if (!panel) {
        d_LogFatal("Failed to allocate ActionPanel");
        return NULL;
    }

    // Set instruction text using strncpy (static storage pattern)
    strncpy(panel->instruction_text, instruction, sizeof(panel->instruction_text) - 1);
    panel->instruction_text[sizeof(panel->instruction_text) - 1] = '\0';

    // Store button references (panel does NOT own them)
    panel->buttons = buttons;
    panel->button_count = button_count;

    // Create horizontal FlexBox for button row
    // Initial bounds (0, 0) are placeholder - actual Y position set in RenderActionPanel()
    // This is the "nested FlexBox with dynamic repositioning" pattern:
    // - Parent FlexBox (main_layout) calculates Y position each frame
    // - RenderActionPanel() receives Y and calls a_FlexSetBounds() to reposition
    // - Child FlexBox (button_row) then calculates button positions
    panel->button_row = a_CreateFlexBox(0, 0, SCREEN_WIDTH, BUTTON_ROW_HEIGHT);
    a_FlexConfigure(panel->button_row, FLEX_DIR_ROW, FLEX_JUSTIFY_START, BUTTON_GAP);

    // Add buttons to FlexBox (determine size based on first button)
    if (button_count > 0 && buttons[0]) {
        int button_width = buttons[0]->w;
        int button_height = buttons[0]->h;

        for (int i = 0; i < button_count; i++) {
            a_FlexAddItem(panel->button_row, button_width, button_height, buttons[i]);
        }
    }

    d_LogInfo("ActionPanel created");
    return panel;
}

void DestroyActionPanel(ActionPanel_t** panel) {
    if (!panel || !*panel) return;

    // No string cleanup needed - using fixed buffer!

    // Destroy FlexBox (but NOT buttons - caller owns them)
    if ((*panel)->button_row) {
        a_DestroyFlexBox(&(*panel)->button_row);
    }

    free(*panel);
    *panel = NULL;
    d_LogInfo("ActionPanel destroyed");
}

// ============================================================================
// ACTION PANEL RENDERING
// ============================================================================

void UpdateActionPanelButtons(ActionPanel_t* panel) {
    if (!panel || !panel->button_row) return;

    // Recalculate FlexBox layout
    a_FlexLayout(panel->button_row);

    // Sync button positions from FlexBox
    for (int i = 0; i < panel->button_count; i++) {
        if (panel->buttons[i]) {
            int x = a_FlexGetItemX(panel->button_row, i);
            int y = a_FlexGetItemY(panel->button_row, i);
            SetButtonPosition(panel->buttons[i], x, y);
        }
    }
}

void RenderActionPanel(ActionPanel_t* panel, int y) {
    if (!panel) return;

    // Render instruction text left-aligned above buttons
    // Buttons are at y + ACTION_PANEL_Y_OFFSET (310), so put text ~40px above buttons
    int text_x = GAME_AREA_X + ACTION_PANEL_LEFT_MARGIN;
    int text_y = y + ACTION_PANEL_Y_OFFSET - 40;

    // Draw instruction text (bright white)
    aFontConfig_t instruction_config = {
        .type = FONT_ENTER_COMMAND,
        .color = {255, 255, 255, 255},  // Bright white
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawTextStyled(panel->instruction_text, text_x, text_y, &instruction_config);

    // Only render buttons if panel has them
    if (!panel->button_row || panel->button_count == 0) return;

    // Position button row in game area (left of screen, lower than flex position)
    // FlexBox bounds are updated each frame because Y position comes from parent FlexBox
    a_FlexSetBounds(panel->button_row, GAME_AREA_X + ACTION_PANEL_LEFT_MARGIN, y + ACTION_PANEL_Y_OFFSET, GAME_AREA_WIDTH, BUTTON_ROW_HEIGHT);

    // Update button positions from FlexBox (includes layout calculation)
    UpdateActionPanelButtons(panel);

    // Get actual button Y position after layout
    int button_y = a_FlexGetItemY(panel->button_row, 0);

    // Render all buttons
    for (int i = 0; i < panel->button_count; i++) {
        if (panel->buttons[i]) {
            RenderButton(panel->buttons[i]);
        }
    }
}
