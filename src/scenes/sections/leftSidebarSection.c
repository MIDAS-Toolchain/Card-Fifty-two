/*
 * Left Sidebar Section Implementation
 */

#include "../../../include/scenes/sections/leftSidebarSection.h"
#include "../../../include/scenes/sceneBlackjack.h"

// Layout constants for FlexBox items
#define PORTRAIT_ITEM_HEIGHT        (PORTRAIT_SIZE)
#define SANITY_LABEL_HEIGHT         20
#define SANITY_BAR_ITEM_HEIGHT      (SANITY_BAR_HEIGHT + 5)
#define SANITY_VALUE_HEIGHT         20
#define CHIPS_LABEL_HEIGHT          20
#define CHIPS_VALUE_HEIGHT          25
#define BET_LABEL_HEIGHT            20
#define BET_VALUE_HEIGHT            25
#define SIDEBAR_BUTTON_ITEM_HEIGHT  (SIDEBAR_BUTTON_HEIGHT + 8)  // Button height + small spacing

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * GetSanityColor - Get color for sanity bar based on percentage
 * Dark purple gradient - darker as sanity decreases
 */
static aColor_t GetSanityColor(float sanity_percent) {
    if (sanity_percent > 0.75f) {
        return (aColor_t){64, 39, 81, 255};   // #402751 - highest sanity
    } else if (sanity_percent > 0.50f) {
        return (aColor_t){30, 29, 57, 255};   // #1e1d39 - medium-high
    } else if (sanity_percent > 0.25f) {
        return (aColor_t){36, 21, 39, 255};   // #241527 - medium-low
    } else {
        return (aColor_t){15, 14, 20, 255};   // #0f0e14 - critical low
    }
}

/**
 * RenderSanityBar - Render sanity meter bar
 */
static void RenderSanityBar(int x, int y, int width, int height, float sanity_percent) {
    // Background (black)
    a_DrawFilledRect(x, y, width, height, 0, 0, 0, 255);

    // Border
    a_DrawRect(x, y, width, height, 100, 100, 100, 255);

    // Filled portion based on sanity percentage
    int filled_width = (int)(width * sanity_percent);
    if (filled_width > 0) {
        aColor_t bar_color = GetSanityColor(sanity_percent);
        a_DrawFilledRect(x, y, filled_width, height,
                         bar_color.r, bar_color.g, bar_color.b, bar_color.a);
    }
}

// ============================================================================
// LEFT SIDEBAR SECTION LIFECYCLE
// ============================================================================

LeftSidebarSection_t* CreateLeftSidebarSection(SidebarButton_t** sidebar_buttons, Deck_t* deck, Player_t* player) {
    if (!sidebar_buttons || !deck || !player) {
        d_LogError("CreateLeftSidebarSection: Invalid parameters");
        return NULL;
    }

    LeftSidebarSection_t* section = malloc(sizeof(LeftSidebarSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate LeftSidebarSection");
        return NULL;
    }

    // Store references (section does NOT own these)
    section->sidebar_buttons = sidebar_buttons;
    section->deck = deck;

    // Create character stats modal (section OWNS this)
    section->stats_modal = CreateCharacterStatsModal(player);
    if (!section->stats_modal) {
        d_LogError("Failed to create CharacterStatsModal");
        free(section);
        return NULL;
    }

    // Create vertical FlexBox for sidebar layout (initial bounds - updated in render)
    // Using pattern from actionPanel.c - bounds set dynamically in RenderLeftSidebarSection
    section->layout = a_CreateFlexBox(0, 0, SIDEBAR_WIDTH, 600);
    a_FlexConfigure(section->layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, ELEMENT_VERTICAL_GAP);
    a_FlexSetPadding(section->layout, SIDEBAR_PADDING);

    // Add FlexBox items for each visual element
    // NEW Order: Chips → Bet → Spacer → Buttons → LARGE SPACER → Portrait → Sanity Bar
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, CHIPS_LABEL_HEIGHT, NULL);                // 0: "Betting Power" label
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, CHIPS_VALUE_HEIGHT, NULL);                // 1: Chips value
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, BET_LABEL_HEIGHT, NULL);                  // 2: "Active Bet" label
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, BET_VALUE_HEIGHT, NULL);                  // 3: Bet value
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, 40, NULL);                                 // 4: Spacer before buttons
    a_FlexAddItem(section->layout, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_ITEM_HEIGHT, NULL); // 5: Draw button
    a_FlexAddItem(section->layout, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_ITEM_HEIGHT, NULL); // 6: Discard button
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, 80, NULL);                                // 7: Large spacer to push portrait to bottom
    a_FlexAddItem(section->layout, PORTRAIT_SIZE, PORTRAIT_ITEM_HEIGHT, NULL);              // 8: Portrait
    a_FlexAddItem(section->layout, SANITY_BAR_WIDTH, SANITY_BAR_ITEM_HEIGHT, NULL);         // 9: Sanity bar (no label/text)

    d_LogInfo("LeftSidebarSection created with FlexBox layout");
    return section;
}

void DestroyLeftSidebarSection(LeftSidebarSection_t** section) {
    if (!section || !*section) return;

    // Destroy stats modal if it exists
    if ((*section)->stats_modal) {
        DestroyCharacterStatsModal(&(*section)->stats_modal);
    }

    // Destroy FlexBox if it exists
    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);
    }

    free(*section);
    *section = NULL;
    d_LogInfo("LeftSidebarSection destroyed");
}

// ============================================================================
// LEFT SIDEBAR SECTION RENDERING
// ============================================================================

void RenderLeftSidebarSection(LeftSidebarSection_t* section, Player_t* player, int x, int y, int height) {
    if (!section || !player || !section->layout) return;

    // Background for sidebar (dark semi-transparent)
    a_DrawFilledRect(x, y, SIDEBAR_WIDTH, height, 20, 20, 30, 200);
    a_DrawRect(x, y, SIDEBAR_WIDTH, height, 60, 60, 80, 255);

    // Update FlexBox bounds (pattern from actionPanel.c)
    a_FlexSetBounds(section->layout, x, y, SIDEBAR_WIDTH, height);
    a_FlexLayout(section->layout);

    int center_x = x + SIDEBAR_WIDTH / 2;

    // ========================================================================
    // 1. Chips Display (FlexBox items 0, 1)
    // ========================================================================

    // Betting Power label (item 0)
    int chips_label_y = a_FlexGetItemY(section->layout, 0);
    a_DrawText("Betting Power", center_x, chips_label_y,
               200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Chips value (item 1)
    int chips_value_y = a_FlexGetItemY(section->layout, 1);
    dString_t* chips_text = d_StringInit();
    d_StringFormat(chips_text, "%d chips", player->chips);
    a_DrawText((char*)d_StringPeek(chips_text), center_x, chips_value_y,
               168, 202, 88, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);  // Yellow-green
    d_StringDestroy(chips_text);

    // ========================================================================
    // 2. Current Bet Display (FlexBox items 2, 3) - only if active
    // ========================================================================

    if (player->current_bet > 0) {
        // Active Bet label (item 2)
        int bet_label_y = a_FlexGetItemY(section->layout, 2);
        a_DrawText("Active Bet", center_x, bet_label_y,
                   200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Bet value (item 3)
        int bet_value_y = a_FlexGetItemY(section->layout, 3);
        dString_t* bet_text = d_StringInit();
        d_StringFormat(bet_text, "%d chips", player->current_bet);
        a_DrawText((char*)d_StringPeek(bet_text), center_x, bet_value_y,
                   222, 158, 65, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);  // Orange
        d_StringDestroy(bet_text);
    }

    // ========================================================================
    // 3. Sidebar Buttons (FlexBox items 5, 6)
    // ========================================================================

    for (int i = 0; i < 2; i++) {
        if (!section->sidebar_buttons[i]) continue;

        SidebarButton_t* btn = section->sidebar_buttons[i];

        // Get Y position from FlexBox (items 5 and 6, after spacer at 4)
        int btn_y = a_FlexGetItemY(section->layout, 5 + i);

        // Center buttons horizontally in sidebar
        int btn_x = center_x - SIDEBAR_BUTTON_WIDTH / 2;

        SetSidebarButtonPosition(btn, btn_x, btn_y);

        // Build count text dynamically
        dString_t* count_text = d_StringInit();
        if (i == 0) {
            d_StringFormat(count_text, "%zu cards", GetDeckSize(section->deck));
        } else {
            d_StringFormat(count_text, "%zu cards", GetDiscardSize(section->deck));
        }

        // Render button
        RenderSidebarButton(btn, d_StringPeek(count_text));
        d_StringDestroy(count_text);
    }

    // ========================================================================
    // 4. Portrait (FlexBox item 8)
    // ========================================================================

    int portrait_y = a_FlexGetItemY(section->layout, 8);

    // Load portrait if not already loaded
    if (!player->portrait_surface) {
        // Build path: resources/portraits/player_{id}.png
        dString_t* path = d_StringInit();
        d_StringFormat(path, "resources/portraits/player_%d.png", player->player_id);
        LoadPlayerPortrait(player, d_StringPeek(path));
        d_StringDestroy(path);
    }

    // Get portrait texture (auto-refreshes if dirty)
    SDL_Texture* portrait_texture = GetPlayerPortraitTexture(player);
    if (portrait_texture) {
        // Render centered portrait
        int portrait_x = center_x - PORTRAIT_SIZE / 2;
        SDL_Rect dst = {portrait_x, portrait_y, PORTRAIT_SIZE, PORTRAIT_SIZE};
        SDL_RenderCopy(app.renderer, portrait_texture, NULL, &dst);
    } else {
        // Fallback: Draw placeholder rect
        int portrait_x = center_x - PORTRAIT_SIZE / 2;
        a_DrawFilledRect(portrait_x, portrait_y, PORTRAIT_SIZE, PORTRAIT_SIZE, 100, 100, 100, 255);
        a_DrawRect(portrait_x, portrait_y, PORTRAIT_SIZE, PORTRAIT_SIZE, 150, 150, 150, 255);
        a_DrawText("?", center_x, portrait_y + PORTRAIT_SIZE / 2 - 10,
                   200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    }

    // ========================================================================
    // 5. Sanity Bar (FlexBox item 9) - No label, no text, just bar
    // ========================================================================

    int sanity_bar_y = a_FlexGetItemY(section->layout, 9);
    int bar_x = center_x - SANITY_BAR_WIDTH / 2;
    float sanity_percent = GetPlayerSanityPercent(player);
    RenderSanityBar(bar_x, sanity_bar_y, SANITY_BAR_WIDTH, SANITY_BAR_HEIGHT, sanity_percent);

    // ========================================================================
    // 6. Character Stats Modal (hover over portrait or sanity bar)
    // ========================================================================

    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Calculate portrait bounds
    int portrait_x = center_x - PORTRAIT_SIZE / 2;

    // Check if hovering over portrait OR sanity bar
    bool hovering_portrait = (mx >= portrait_x && mx <= portrait_x + PORTRAIT_SIZE &&
                              my >= portrait_y && my <= portrait_y + PORTRAIT_SIZE);

    bool hovering_bar = (mx >= bar_x && mx <= bar_x + SANITY_BAR_WIDTH &&
                         my >= sanity_bar_y && my <= sanity_bar_y + SANITY_BAR_HEIGHT);

    if (hovering_portrait || hovering_bar) {
        // Position modal to the right of portrait (or left if near screen edge)
        int modal_x = portrait_x + PORTRAIT_SIZE + 10;
        int modal_y = portrait_y;

        // Clamp modal Y to screen bounds
        if (modal_y + STATS_MODAL_HEIGHT > SCREEN_HEIGHT) {
            modal_y = SCREEN_HEIGHT - STATS_MODAL_HEIGHT - 10;
        }
        if (modal_y < LAYOUT_TOP_MARGIN) {
            modal_y = LAYOUT_TOP_MARGIN;
        }

        // If too close to right edge, show on left
        if (modal_x + STATS_MODAL_WIDTH > SCREEN_WIDTH) {
            modal_x = portrait_x - STATS_MODAL_WIDTH - 10;
        }

        ShowCharacterStatsModal(section->stats_modal, modal_x, modal_y);
    } else {
        HideCharacterStatsModal(section->stats_modal);
    }

    // Render modal if visible
    RenderCharacterStatsModal(section->stats_modal);
}
