/*
 * Deck View Panel Section Implementation
 */

#include "../../../include/scenes/sections/deckViewPanel.h"
#include "../../../include/scenes/sceneBlackjack.h"

// ============================================================================
// DECK VIEW PANEL LIFECYCLE
// ============================================================================

DeckViewPanel_t* CreateDeckViewPanel(DeckButton_t** buttons, int button_count, Deck_t* deck) {
    if (!buttons || button_count != 2 || !deck) {
        d_LogError("CreateDeckViewPanel: Invalid parameters");
        return NULL;
    }

    DeckViewPanel_t* panel = malloc(sizeof(DeckViewPanel_t));
    if (!panel) {
        d_LogFatal("Failed to allocate DeckViewPanel");
        return NULL;
    }

    // Store button references (panel does NOT own them)
    panel->buttons = buttons;
    panel->button_count = button_count;
    panel->deck = deck;

    // Create horizontal FlexBox for button row
    // Use FLEX_JUSTIFY_SPACE_AROUND to place buttons on left/right with spacing
    panel->button_row = a_CreateFlexBox(0, 0, SCREEN_WIDTH, DECK_PANEL_ROW_HEIGHT);
    a_FlexConfigure(panel->button_row, FLEX_DIR_ROW, FLEX_JUSTIFY_SPACE_AROUND, DECK_BUTTON_SPACING);

    // Add buttons to FlexBox
    for (int i = 0; i < button_count; i++) {
        if (buttons[i]) {
            a_FlexAddItem(panel->button_row, buttons[i]->w, buttons[i]->h, buttons[i]);
        }
    }

    d_LogInfo("DeckViewPanel created");
    return panel;
}

void DestroyDeckViewPanel(DeckViewPanel_t** panel) {
    if (!panel || !*panel) return;

    // Destroy FlexBox (but NOT buttons - caller owns them)
    if ((*panel)->button_row) {
        a_DestroyFlexBox(&(*panel)->button_row);
    }

    free(*panel);
    *panel = NULL;
    d_LogInfo("DeckViewPanel destroyed");
}

// ============================================================================
// DECK VIEW PANEL RENDERING
// ============================================================================

void UpdateDeckViewPanelButtons(DeckViewPanel_t* panel) {
    if (!panel || !panel->button_row) return;

    // Recalculate FlexBox layout
    a_FlexLayout(panel->button_row);

    // Sync button positions from FlexBox
    for (int i = 0; i < panel->button_count; i++) {
        if (panel->buttons[i]) {
            int x = a_FlexGetItemX(panel->button_row, i);
            int y = a_FlexGetItemY(panel->button_row, i);
            SetDeckButtonPosition(panel->buttons[i], x, y);
        }
    }
}

void RenderDeckViewPanel(DeckViewPanel_t* panel, int y) {
    if (!panel || !panel->button_row || !panel->deck) return;

    // Position button row at the specified Y
    a_FlexSetBounds(panel->button_row, 0, y, SCREEN_WIDTH, DECK_PANEL_ROW_HEIGHT);

    // Update button positions from FlexBox
    UpdateDeckViewPanelButtons(panel);

    // Update count text for each button and render
    for (int i = 0; i < panel->button_count; i++) {
        if (!panel->buttons[i]) continue;

        DeckButton_t* btn = panel->buttons[i];

        // Update count text based on deck state
        dString_t* count_text = d_InitString();
        if (i == 0) {
            d_FormatString(count_text, "Draw: %zu", GetDeckSize(panel->deck));
        } else {
            d_FormatString(count_text, "Discard: %zu", GetDiscardSize(panel->deck));
        }
        SetDeckButtonCountText(btn, d_PeekString(count_text));
        d_DestroyString(count_text);

        // Render button (includes count above and hotkey below)
        RenderDeckButton(btn);
    }
}
