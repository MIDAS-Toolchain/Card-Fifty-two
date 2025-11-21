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
    panel->button_row = a_FlexBoxCreate(0, 0, SCREEN_WIDTH, BUTTON_ROW_HEIGHT);
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
        a_FlexBoxDestroy(&(*panel)->button_row);
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
    // IMPORTANT: Loop through FlexBox items, not panel->button_count!
    // After RebuildActionPanelLayout(), FlexBox may have fewer items than button_count
    int item_count = a_FlexGetItemCount(panel->button_row);
    for (int i = 0; i < item_count; i++) {
        const FlexItem_t* item = a_FlexGetItem(panel->button_row, i);
        if (item && item->user_data) {
            Button_t* button = (Button_t*)item->user_data;
            int x = a_FlexGetItemX(panel->button_row, i);
            int y = a_FlexGetItemY(panel->button_row, i);
            SetButtonPosition(button, x, y);
        }
    }
}

void RebuildActionPanelLayout(ActionPanel_t* panel, bool button_visible[3], int available_width) {
    if (!panel || !panel->button_row || !button_visible) return;

    // Count visible buttons
    int visible_count = 0;
    for (int i = 0; i < panel->button_count && i < 3; i++) {
        if (button_visible[i]) {
            visible_count++;
        }
    }

    if (visible_count == 0) return;  // No buttons to show

    // Calculate dynamic button width
    // Formula: (available_width - (gaps * (visible_count - 1))) / visible_count
    int total_gap_width = BUTTON_GAP * (visible_count - 1);
    int button_width = (available_width - total_gap_width) / visible_count;

    // Clear existing FlexBox items
    a_FlexClearItems(panel->button_row);

    // Add only visible buttons with new width
    for (int i = 0; i < panel->button_count && i < 3; i++) {
        if (button_visible[i] && panel->buttons[i]) {
            // Update button size directly (no setter function exists)
            panel->buttons[i]->w = button_width;

            // Add to FlexBox
            a_FlexAddItem(panel->button_row, button_width, panel->buttons[i]->h, panel->buttons[i]);
        }
    }

    d_LogDebugF("ActionPanel rebuilt: %d visible buttons, %dpx each", visible_count, button_width);
}

void RenderActionPanel(ActionPanel_t* panel, int y) {
    if (!panel) return;

    // Render instruction text left-aligned above buttons
    // Buttons are at y + ACTION_PANEL_Y_OFFSET (310), so put text ~40px above buttons
    int text_x = GAME_AREA_X + ACTION_PANEL_LEFT_MARGIN;
    int text_y = y + ACTION_PANEL_Y_OFFSET - 40;

    // Draw instruction text (bright white)
    aTextStyle_t instruction_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, 255},  // Bright white
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText(panel->instruction_text, text_x, text_y, instruction_config);

    // Only render buttons if panel has them
    if (!panel->button_row || panel->button_count == 0) return;

    // Position button row in game area (left of screen, lower than flex position)
    // FlexBox bounds are updated each frame because Y position comes from parent FlexBox
    a_FlexSetBounds(panel->button_row, GAME_AREA_X + ACTION_PANEL_LEFT_MARGIN, y + ACTION_PANEL_Y_OFFSET, GAME_AREA_WIDTH, BUTTON_ROW_HEIGHT);

    // Update button positions from FlexBox (includes layout calculation)
    UpdateActionPanelButtons(panel);

    // Render only buttons that are in the FlexBox
    // FlexBox item count may be less than button_count after RebuildActionPanelLayout()
    int item_count = a_FlexGetItemCount(panel->button_row);
    for (int i = 0; i < item_count; i++) {
        // Get the button pointer from FlexBox user_data
        const FlexItem_t* item = a_FlexGetItem(panel->button_row, i);
        if (item && item->user_data) {
            Button_t* button = (Button_t*)item->user_data;
            RenderButton(button);
        }
    }
}
