/*
 * Left Sidebar Section Implementation
 */

#include "../../../include/scenes/sections/leftSidebarSection.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/statusEffects.h"
#include "../../../include/tween/tween.h"

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

    // Create combat stats modal (section OWNS this)
    section->combat_stats_modal = CreateCombatStatsModal(player);
    if (!section->combat_stats_modal) {
        d_LogError("Failed to create CombatStatsModal");
        DestroyCharacterStatsModal(&section->stats_modal);
        free(section);
        return NULL;
    }

    // Create status effect tooltip modal (section OWNS this)
    section->effect_tooltip = CreateStatusEffectTooltipModal();
    if (!section->effect_tooltip) {
        d_LogError("Failed to create StatusEffectTooltipModal");
        DestroyCombatStatsModal(&section->combat_stats_modal);
        DestroyCharacterStatsModal(&section->stats_modal);
        free(section);
        return NULL;
    }

    // Create chips tooltip modal (section OWNS this)
    section->chips_tooltip = CreateChipsTooltipModal();
    if (!section->chips_tooltip) {
        d_LogError("Failed to create ChipsTooltipModal");
        DestroyStatusEffectTooltipModal(&section->effect_tooltip);
        DestroyCombatStatsModal(&section->combat_stats_modal);
        DestroyCharacterStatsModal(&section->stats_modal);
        free(section);
        return NULL;
    }

    // Create bet tooltip modal (section OWNS this)
    section->bet_tooltip = CreateBetTooltipModal();
    if (!section->bet_tooltip) {
        d_LogError("Failed to create BetTooltipModal");
        DestroyChipsTooltipModal(&section->chips_tooltip);
        DestroyStatusEffectTooltipModal(&section->effect_tooltip);
        DestroyCombatStatsModal(&section->combat_stats_modal);
        DestroyCharacterStatsModal(&section->stats_modal);
        free(section);
        return NULL;
    }

    // Initialize bet damage animation
    section->bet_damage_active = false;
    section->bet_damage_y_offset = 0.0f;
    section->bet_damage_alpha = 0.0f;
    section->bet_damage_amount = 0;

    // Initialize hover tracking
    section->chips_hover_timer = 0.0f;
    section->bet_hover_timer = 0.0f;

    // Create vertical FlexBox for sidebar layout (initial bounds - updated in render)
    // Using pattern from actionPanel.c - bounds set dynamically in RenderLeftSidebarSection
    section->layout = a_CreateFlexBox(0, 0, SIDEBAR_WIDTH, 600);
    a_FlexConfigure(section->layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_SPACE_BETWEEN, ELEMENT_VERTICAL_GAP);
    a_FlexSetPadding(section->layout, SIDEBAR_PADDING);

    // Add FlexBox items for each visual element
    // NEW Order: Chips → Bet → Status Effects → Spacer → Buttons → LARGE SPACER → Portrait → Sanity Bar
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, CHIPS_LABEL_HEIGHT, NULL);                // 0: "Betting Power" label
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, CHIPS_VALUE_HEIGHT, NULL);                // 1: Chips value
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, BET_LABEL_HEIGHT, NULL);                  // 2: "Active Bet" label
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, BET_VALUE_HEIGHT, NULL);                  // 3: Bet value
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, 60, NULL);                                 // 4: Status effects area (48px icons + padding)
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, 30, NULL);                                 // 5: Spacer before buttons (reduced from 40)
    a_FlexAddItem(section->layout, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_ITEM_HEIGHT, NULL); // 6: Draw button
    a_FlexAddItem(section->layout, SIDEBAR_BUTTON_WIDTH, SIDEBAR_BUTTON_ITEM_HEIGHT, NULL); // 7: Discard button
    a_FlexAddItem(section->layout, SIDEBAR_WIDTH, 80, NULL);                                // 8: Large spacer to push portrait to bottom
    a_FlexAddItem(section->layout, PORTRAIT_SIZE, PORTRAIT_ITEM_HEIGHT, NULL);              // 9: Portrait
    a_FlexAddItem(section->layout, SANITY_BAR_WIDTH, SANITY_BAR_ITEM_HEIGHT, NULL);         // 10: Sanity bar (no label/text)

    d_LogInfo("LeftSidebarSection created with FlexBox layout");
    return section;
}

void DestroyLeftSidebarSection(LeftSidebarSection_t** section) {
    if (!section || !*section) return;

    // Destroy stats modal if it exists
    if ((*section)->stats_modal) {
        DestroyCharacterStatsModal(&(*section)->stats_modal);
    }

    // Destroy combat stats modal if it exists
    if ((*section)->combat_stats_modal) {
        DestroyCombatStatsModal(&(*section)->combat_stats_modal);
    }

    // Destroy effect tooltip if it exists
    if ((*section)->effect_tooltip) {
        DestroyStatusEffectTooltipModal(&(*section)->effect_tooltip);
    }

    // Destroy chips tooltip if it exists
    if ((*section)->chips_tooltip) {
        DestroyChipsTooltipModal(&(*section)->chips_tooltip);
    }

    // Destroy bet tooltip if it exists
    if ((*section)->bet_tooltip) {
        DestroyBetTooltipModal(&(*section)->bet_tooltip);
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
    // PRE-CALCULATE HOVER STATES (needed for color changes)
    // ========================================================================
    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Check if hovering over chips display (FlexBox items 0-1)
    int chips_label_y = a_FlexGetItemY(section->layout, 0);
    int chips_value_y = a_FlexGetItemY(section->layout, 1);
    int chips_area_height = (chips_value_y - chips_label_y) + 25;
    bool hovering_chips = (mx >= x && mx <= x + SIDEBAR_WIDTH &&
                          my >= chips_label_y && my <= chips_label_y + chips_area_height);

    // Check if hovering over bet display (FlexBox items 2-3)
    int bet_label_y = a_FlexGetItemY(section->layout, 2);
    int bet_value_y = a_FlexGetItemY(section->layout, 3);
    int bet_area_height = (bet_value_y - bet_label_y) + 25;
    bool hovering_bet = (mx >= x && mx <= x + SIDEBAR_WIDTH &&
                        my >= bet_label_y && my <= bet_label_y + bet_area_height);

    // ========================================================================
    // 1. Chips Display (FlexBox items 0, 1)
    // ========================================================================

    // Betting Power label (item 0) - yellow when hovered
    aColor_t chips_label_color = hovering_chips
        ? (aColor_t){255, 220, 100, 255}  // Yellow (hovered)
        : (aColor_t){200, 200, 200, 255}; // Gray (normal)
    a_DrawText("Betting Power", center_x, chips_label_y,
               chips_label_color.r, chips_label_color.g, chips_label_color.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Chips value (item 1) - yellow when hovered
    aColor_t chips_value_color = hovering_chips
        ? (aColor_t){255, 220, 100, 255}  // Yellow (hovered)
        : (aColor_t){168, 202, 88, 255};  // Yellow-green (normal)
    dString_t* chips_text = d_StringInit();
    d_StringFormat(chips_text, "%d chips", player->chips);
    a_DrawText((char*)d_StringPeek(chips_text), center_x, chips_value_y,
               chips_value_color.r, chips_value_color.g, chips_value_color.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_StringDestroy(chips_text);

    // Render bet damage number (floating up and fading out)
    if (section->bet_damage_active && section->bet_damage_alpha > 0.0f) {
        dString_t* damage_text = d_StringInit();
        d_StringFormat(damage_text, "-%d chips", section->bet_damage_amount);

        int damage_x = center_x;
        int damage_y = chips_value_y - 20 + (int)section->bet_damage_y_offset;  // Start 20px higher

        aFontConfig_t damage_config = {
            .type = FONT_GAME,
            .color = {255, 0, 0, (Uint8)section->bet_damage_alpha},  // Red, fading
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.5f
        };
        a_DrawTextStyled((char*)d_StringPeek(damage_text), damage_x, damage_y, &damage_config);
        d_StringDestroy(damage_text);

        // Deactivate when fully faded
        if (section->bet_damage_alpha <= 1.0f) {
            section->bet_damage_active = false;
        }
    }

    // ========================================================================
    // 2. Current Bet Display (FlexBox items 2, 3) - ALWAYS VISIBLE
    // ========================================================================

    // Active Bet label (item 2) - yellow when hovered
    aColor_t bet_label_color = hovering_bet
        ? (aColor_t){255, 220, 100, 255}  // Yellow (hovered)
        : (aColor_t){200, 200, 200, 255}; // Gray (normal)
    a_DrawText("Active Bet", center_x, bet_label_y,
               bet_label_color.r, bet_label_color.g, bet_label_color.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Bet value (item 3) - yellow when hovered, else orange/gray based on bet state
    dString_t* bet_text = d_StringInit();
    d_StringFormat(bet_text, "%d chips", player->current_bet);

    aColor_t bet_color;
    if (hovering_bet) {
        bet_color = (aColor_t){255, 220, 100, 255};  // Yellow (hovered)
    } else if (player->current_bet > 0) {
        bet_color = (aColor_t){222, 158, 65, 255};   // Orange (active)
    } else {
        bet_color = (aColor_t){150, 150, 150, 255};  // Gray (inactive)
    }

    a_DrawText((char*)d_StringPeek(bet_text), center_x, bet_value_y,
               bet_color.r, bet_color.g, bet_color.b, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_StringDestroy(bet_text);

    // ========================================================================
    // 3. Status Effects Display (FlexBox item 4)
    // ========================================================================

    if (player->status_effects) {
        int status_y = a_FlexGetItemY(section->layout, 4) + 12;  // 12px lower to avoid overlap
        // Center status effects horizontally in sidebar
        int status_x = center_x - (48 * 3) / 2;  // Assume max 3 icons visible (48px each)
        RenderStatusEffects(player->status_effects, status_x, status_y);
    }

    // ========================================================================
    // 4. Sidebar Buttons (FlexBox items 6, 7) - UPDATED INDICES
    // ========================================================================

    for (int i = 0; i < 2; i++) {
        if (!section->sidebar_buttons[i]) continue;

        SidebarButton_t* btn = section->sidebar_buttons[i];

        // Get Y position from FlexBox (items 6 and 7, after spacer at 5)
        int btn_y = a_FlexGetItemY(section->layout, 6 + i);

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
    // 5. Portrait (FlexBox item 9) - UPDATED INDEX
    // ========================================================================

    int portrait_y = a_FlexGetItemY(section->layout, 9);

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
    // 6. Sanity Bar (FlexBox item 10) - UPDATED INDEX - No label, no text, just bar
    // ========================================================================

    int sanity_bar_y = a_FlexGetItemY(section->layout, 10);
    int bar_x = center_x - SANITY_BAR_WIDTH / 2;
    float sanity_percent = GetPlayerSanityPercent(player);
    RenderSanityBar(bar_x, sanity_bar_y, SANITY_BAR_WIDTH, SANITY_BAR_HEIGHT, sanity_percent);

}

// ============================================================================
// MODAL OVERLAY RENDERING (Z-ORDER: MUST BE CALLED LAST)
// ============================================================================

void RenderLeftSidebarModalOverlay(LeftSidebarSection_t* section, Player_t* player, int x, int y) {
    (void)y;  // Unused - FlexBox already knows bounds from RenderLeftSidebarSection

    if (!section || !player || !section->layout || !section->stats_modal) return;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    int center_x = x + SIDEBAR_WIDTH / 2;

    // Get portrait and sanity bar positions from FlexBox (same indices as main render)
    int portrait_y = a_FlexGetItemY(section->layout, 9);
    int sanity_bar_y = a_FlexGetItemY(section->layout, 10);

    // Calculate bounds
    int portrait_x = center_x - PORTRAIT_SIZE / 2;
    int bar_x = center_x - SANITY_BAR_WIDTH / 2;

    // Check if hovering over portrait OR sanity bar
    bool hovering_portrait = (mx >= portrait_x && mx <= portrait_x + PORTRAIT_SIZE &&
                              my >= portrait_y && my <= portrait_y + PORTRAIT_SIZE);

    bool hovering_bar = (mx >= bar_x && mx <= bar_x + SANITY_BAR_WIDTH &&
                         my >= sanity_bar_y && my <= sanity_bar_y + SANITY_BAR_HEIGHT);

    // ========================================================================
    // Check if hovering over status effect icons (item 4)
    // ========================================================================
    bool hovering_status_effect = false;
    const StatusEffectInstance_t* hovered_effect = NULL;

    if (player->status_effects && section->effect_tooltip) {
        int status_y = a_FlexGetItemY(section->layout, 4);
        int status_x = center_x - (48 * 3) / 2;  // Match RenderStatusEffects positioning

        const int ICON_SIZE = 48;
        const int ICON_SPACING = 8;
        const int MAX_PER_ROW = 6;

        int current_x = status_x;
        int current_y = status_y;
        int count = 0;

        // Iterate through active effects and check hover
        for (size_t i = 0; i < player->status_effects->active_effects->count; i++) {
            const StatusEffectInstance_t* effect = (const StatusEffectInstance_t*)
                d_IndexDataFromArray(player->status_effects->active_effects, i);

            if (!effect || effect->duration <= 0) continue;

            // Wrap to next row after MAX_PER_ROW
            if (count > 0 && count % MAX_PER_ROW == 0) {
                current_x = status_x;
                current_y += ICON_SIZE + ICON_SPACING;
            }

            // Check if mouse is over this icon
            if (mx >= current_x && mx <= current_x + ICON_SIZE &&
                my >= current_y && my <= current_y + ICON_SIZE) {
                hovering_status_effect = true;
                hovered_effect = effect;
                break;
            }

            current_x += ICON_SIZE + ICON_SPACING;
            count++;
        }
    }

    // ========================================================================
    // Check if hovering over chips display (FlexBox items 0-1)
    // ========================================================================
    int chips_label_y = a_FlexGetItemY(section->layout, 0);
    int chips_value_y = a_FlexGetItemY(section->layout, 1);
    int chips_area_height = (chips_value_y - chips_label_y) + 25;
    bool hovering_chips = (mx >= x && mx <= x + SIDEBAR_WIDTH &&
                          my >= chips_label_y && my <= chips_label_y + chips_area_height);

    // ========================================================================
    // Check if hovering over bet display (FlexBox items 2-3)
    // ========================================================================
    int bet_label_y = a_FlexGetItemY(section->layout, 2);
    int bet_value_y = a_FlexGetItemY(section->layout, 3);
    int bet_area_height = (bet_value_y - bet_label_y) + 25;
    bool hovering_bet = (mx >= x && mx <= x + SIDEBAR_WIDTH &&
                        my >= bet_label_y && my <= bet_label_y + bet_area_height);

    // ========================================================================
    // Show appropriate modal (priority: status effect > chips > bet > character stats)
    // ========================================================================

    if (hovering_status_effect && hovered_effect && section->effect_tooltip) {
        // Show status effect tooltip
        HideCharacterStatsModal(section->stats_modal);
        HideChipsTooltipModal(section->chips_tooltip);
        HideBetTooltipModal(section->bet_tooltip);

        // Calculate icon position for tooltip positioning
        int status_y = a_FlexGetItemY(section->layout, 4);
        int status_x = center_x - (48 * 3) / 2;

        ShowStatusEffectTooltipModal(section->effect_tooltip, hovered_effect, status_x, status_y);
    } else if (hovering_chips && section->chips_tooltip) {
        // Show chips tooltip
        HideCharacterStatsModal(section->stats_modal);
        HideStatusEffectTooltipModal(section->effect_tooltip);
        HideBetTooltipModal(section->bet_tooltip);

        // Position to the right of sidebar with some offset
        int tooltip_x = x + SIDEBAR_WIDTH + 15;  // More spacing from sidebar edge
        int tooltip_y = chips_label_y - 10;  // Move up slightly for better alignment

        // Make sure tooltip doesn't go off top of screen
        if (tooltip_y < LAYOUT_TOP_MARGIN) {
            tooltip_y = LAYOUT_TOP_MARGIN;
        }

        ShowChipsTooltipModal(section->chips_tooltip, tooltip_x, tooltip_y);
    } else if (hovering_bet && section->bet_tooltip) {
        // Show bet tooltip
        HideCharacterStatsModal(section->stats_modal);
        HideStatusEffectTooltipModal(section->effect_tooltip);
        HideChipsTooltipModal(section->chips_tooltip);

        // Position to the right of sidebar with some offset
        int tooltip_x = x + SIDEBAR_WIDTH + 15;  // More spacing from sidebar edge
        int tooltip_y = bet_label_y - 10;  // Move up slightly for better alignment

        // Make sure tooltip doesn't go off top of screen
        if (tooltip_y < LAYOUT_TOP_MARGIN) {
            tooltip_y = LAYOUT_TOP_MARGIN;
        }

        ShowBetTooltipModal(section->bet_tooltip, tooltip_x, tooltip_y);
    } else if (hovering_portrait || hovering_bar) {
        // Show character stats modal
        HideStatusEffectTooltipModal(section->effect_tooltip);
        HideChipsTooltipModal(section->chips_tooltip);
        HideBetTooltipModal(section->bet_tooltip);

        // Show modals side-by-side, full height from top bar to bottom
        int modal_y = TOP_BAR_HEIGHT;  // Start at top bar

        // Position modals after the sidebar ends (SIDEBAR_WIDTH = 280)
        int sanity_modal_x = SIDEBAR_WIDTH + 10;  // Start just after sidebar

        // Position combat stats modal right of sanity modal
        int combat_modal_x = sanity_modal_x + STATS_MODAL_WIDTH + 10;

        ShowCharacterStatsModal(section->stats_modal, sanity_modal_x, modal_y);
        ShowCombatStatsModal(section->combat_stats_modal, combat_modal_x, modal_y);
    } else {
        // Hide all modals
        HideCharacterStatsModal(section->stats_modal);
        HideCombatStatsModal(section->combat_stats_modal);
        HideStatusEffectTooltipModal(section->effect_tooltip);
        HideChipsTooltipModal(section->chips_tooltip);
        HideBetTooltipModal(section->bet_tooltip);
    }

    // Render all modals if visible (will be on top since this is called last)
    RenderCharacterStatsModal(section->stats_modal);
    RenderCombatStatsModal(section->combat_stats_modal);
    RenderStatusEffectTooltipModal(section->effect_tooltip);
    RenderChipsTooltipModal(section->chips_tooltip);
    RenderBetTooltipModal(section->bet_tooltip, player);
}

// ============================================================================
// BET DAMAGE ANIMATION
// ============================================================================

void ShowSidebarBetDamage(LeftSidebarSection_t* section, int bet_amount) {
    if (!section || bet_amount <= 0) return;

    section->bet_damage_active = true;
    section->bet_damage_amount = bet_amount;
    section->bet_damage_y_offset = 0.0f;
    section->bet_damage_alpha = 255.0f;

    // Get tween manager and start animations
    TweenManager_t* tween_mgr = GetTweenManager();
    if (tween_mgr) {
        // Float up 40px over 0.8s
        TweenFloat(tween_mgr, &section->bet_damage_y_offset, -40.0f, 0.8f, TWEEN_EASE_OUT_QUAD);
        // Fade out over 0.8s
        TweenFloat(tween_mgr, &section->bet_damage_alpha, 0.0f, 0.8f, TWEEN_EASE_OUT_QUAD);
    }

    d_LogInfoF("💰 Sidebar bet damage triggered: -%d chips", bet_amount);
}

// ============================================================================
// HOVER TRACKING (for tutorial)
// ============================================================================

bool UpdateLeftSidebarChipsHover(LeftSidebarSection_t* section, int x, float dt) {
    if (!section || !section->layout) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Get Betting Power (chips) display area bounds (FlexBox items 0-1)
    int chips_label_y = a_FlexGetItemY(section->layout, 0);
    int chips_value_y = a_FlexGetItemY(section->layout, 1);
    int chips_area_height = (chips_value_y - chips_label_y) + 25;

    bool hovering_chips = (mx >= x && mx <= x + SIDEBAR_WIDTH &&
                          my >= chips_label_y && my <= chips_label_y + chips_area_height);

    // Update hover timer
    if (hovering_chips) {
        section->chips_hover_timer += dt;

        // Trigger tutorial event after 0.3 seconds of continuous hover
        if (section->chips_hover_timer >= 0.3f) {
            section->chips_hover_timer = 0.0f;  // Reset timer
            return true;  // Event triggered!
        }
    } else {
        // Reset timer if not hovering
        section->chips_hover_timer = 0.0f;
    }

    return false;
}

bool UpdateLeftSidebarHoverTracking(LeftSidebarSection_t* section, int x, float dt) {
    if (!section || !section->layout) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Get Active Bet display area bounds (FlexBox items 2-3)
    int bet_label_y = a_FlexGetItemY(section->layout, 2);
    int bet_value_y = a_FlexGetItemY(section->layout, 3);
    int bet_area_height = (bet_value_y - bet_label_y) + 25;

    bool hovering_bet = (mx >= x && mx <= x + SIDEBAR_WIDTH &&
                        my >= bet_label_y && my <= bet_label_y + bet_area_height);

    // Update hover timer
    if (hovering_bet) {
        section->bet_hover_timer += dt;

        // Trigger tutorial event after 0.3 seconds of continuous hover
        if (section->bet_hover_timer >= 0.3f) {
            section->bet_hover_timer = 0.0f;  // Reset timer
            return true;  // Event triggered!
        }
    } else {
        // Reset timer if not hovering
        section->bet_hover_timer = 0.0f;
    }

    return false;
}
