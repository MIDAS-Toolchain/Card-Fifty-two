/*
 * Combat Stats Modal Component Implementation
 * Displays combat stats (damage bonus, crit chance, crit bonus)
 */

#include "../../../include/scenes/components/combatStatsModal.h"
#include "../../../include/scenes/sceneBlackjack.h"

// Modern tooltip color scheme (matching other modals)
#define COLOR_BG            ((aColor_t){20, 20, 30, 230})     // Dark background
#define COLOR_BORDER        ((aColor_t){100, 100, 100, 200})  // Light gray border
#define COLOR_TITLE         ((aColor_t){235, 237, 233, 255})  // #ebede9 - off-white title
#define COLOR_TEXT          ((aColor_t){200, 200, 200, 255})  // Light gray text
#define COLOR_VALUE         ((aColor_t){150, 255, 150, 255})  // Green for stat values

// ============================================================================
// LIFECYCLE
// ============================================================================

CombatStatsModal_t* CreateCombatStatsModal(Player_t* player) {
    if (!player) {
        d_LogError("CreateCombatStatsModal: NULL player");
        return NULL;
    }

    CombatStatsModal_t* modal = malloc(sizeof(CombatStatsModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate CombatStatsModal");
        return NULL;
    }

    modal->is_visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->player = player;  // Reference only, not owned

    d_LogInfo("CombatStatsModal created");
    return modal;
}

void DestroyCombatStatsModal(CombatStatsModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("CombatStatsModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowCombatStatsModal(CombatStatsModal_t* modal, int x, int y) {
    if (!modal) return;

    modal->is_visible = true;
    modal->x = x;
    modal->y = y;
}

void HideCombatStatsModal(CombatStatsModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderCombatStatsModal(CombatStatsModal_t* modal) {
    if (!modal || !modal->is_visible || !modal->player) return;

    // Use full game area height (minus top bar) to match sanity modal
    int padding = 16;
    int modal_width = COMBAT_STATS_MODAL_WIDTH;
    int modal_height = SCREEN_HEIGHT - TOP_BAR_HEIGHT;
    int content_width = modal_width - (padding * 2);

    // Draw background (modern dark style)
    a_DrawFilledRect(modal->x, modal->y, modal_width, modal_height,
                     COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);

    // Draw border
    a_DrawRect(modal->x, modal->y, modal_width, modal_height,
               COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);

    // Content positioning
    int content_x = modal->x + padding;
    int current_y = modal->y + padding;

    // ========================================================================
    // Title - "Combat Stats"
    // ========================================================================

    aFontConfig_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .color = COLOR_TITLE,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    a_DrawTextStyled("Combat Stats", content_x, current_y, &title_config);
    int title_height = a_GetWrappedTextHeight("Combat Stats", FONT_ENTER_COMMAND, content_width);
    current_y += title_height + 12;

    // ========================================================================
    // Stat Labels and Values (placeholder values for now)
    // ========================================================================

    aFontConfig_t label_config = {
        .type = FONT_GAME,
        .color = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };

    aFontConfig_t value_config = {
        .type = FONT_GAME,
        .color = COLOR_VALUE,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };

    int line_height = a_GetWrappedTextHeight("Damage Increase:", FONT_GAME, content_width);
    int spacing = 8;

    // Damage Increase (percentage modifier)
    a_DrawTextStyled("Damage Increase:", content_x, current_y, &label_config);
    current_y += line_height + 4;
    a_DrawTextStyled("+0%", content_x + 10, current_y, &value_config);
    current_y += line_height + spacing;

    // Damage Bonus (flat bonus)
    a_DrawTextStyled("Damage Bonus:", content_x, current_y, &label_config);
    current_y += line_height + 4;
    a_DrawTextStyled("+0", content_x + 10, current_y, &value_config);
    current_y += line_height + spacing;

    // Crit Chance
    a_DrawTextStyled("Crit Chance:", content_x, current_y, &label_config);
    current_y += line_height + 4;
    a_DrawTextStyled("0%", content_x + 10, current_y, &value_config);
    current_y += line_height + spacing;

    // Damage Bonus (for crits)
    a_DrawTextStyled("Damage Bonus:", content_x, current_y, &label_config);
    current_y += line_height + 4;
    a_DrawTextStyled("+0%", content_x + 10, current_y, &value_config);
    current_y += line_height + spacing;
}
