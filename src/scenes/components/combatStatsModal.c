/*
 * Combat Stats Modal Component Implementation
 * Displays combat stats (damage bonus, crit chance, crit bonus)
 */

#include "../../../include/scenes/components/combatStatsModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/player.h"  // For CalculatePlayerCombatStats

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
    a_DrawFilledRect((aRectf_t){modal->x, modal->y, modal_width, modal_height}, COLOR_BG);

    // Draw border
    a_DrawRect((aRectf_t){modal->x, modal->y, modal_width, modal_height}, COLOR_BORDER);

    // Content positioning
    int content_x = modal->x + padding;
    int current_y = modal->y + padding;

    // ========================================================================
    // Title - "Combat Stats"
    // ========================================================================

    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = COLOR_TITLE,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    current_y += 4;  // Move Combat title down 4px
    a_DrawText("Combat:", content_x, current_y, title_config);
    int title_height = a_GetWrappedTextHeight("Combat:", FONT_ENTER_COMMAND, content_width);
    current_y += title_height + 12;

    // ========================================================================
    // Stat Labels and Values (placeholder values for now)
    // ========================================================================

    aTextStyle_t label_config = {
        .type = FONT_GAME,
        .fg = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };

    aTextStyle_t value_config = {
        .type = FONT_GAME,
        .fg = COLOR_VALUE,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };

    int line_height = a_GetWrappedTextHeight("Damage Increase:", FONT_GAME, content_width);
    int spacing = 8;

    // Recalculate stats if dirty (ADR-09 pattern)
    if (modal->player->combat_stats_dirty) {
        d_LogInfo("CombatStatsModal: Recalculating stats (dirty flag set)");
        CalculatePlayerCombatStats(modal->player);
    }

    // Debug: Log current stat values
    d_LogInfoF("CombatStatsModal: Displaying stats - damage%%=%d, flat=%d, crit%%=%d, crit_bonus=%d",
               modal->player->damage_percent, modal->player->damage_flat,
               modal->player->crit_chance, modal->player->crit_bonus);

    // Damage Increase (percentage modifier from BRUTAL tag)
    a_DrawText("Damage Increase:", content_x, current_y, label_config);
    current_y += line_height + 4;

    dString_t* damage_percent_text = d_StringInit();
    d_StringFormat(damage_percent_text, "+%d%%", modal->player->damage_percent);
    a_DrawText((char*)d_StringPeek(damage_percent_text), content_x + 10, current_y, value_config);
    d_StringDestroy(damage_percent_text);
    current_y += line_height + spacing;

    // Damage Bonus (flat bonus - currently unused)
    a_DrawText("Damage Bonus:", content_x, current_y, label_config);
    current_y += line_height + 4;

    dString_t* damage_flat_text = d_StringInit();
    d_StringFormat(damage_flat_text, "+%d", modal->player->damage_flat);
    a_DrawText((char*)d_StringPeek(damage_flat_text), content_x + 10, current_y, value_config);
    d_StringDestroy(damage_flat_text);
    current_y += line_height + spacing;

    // Crit Chance (from LUCKY tag)
    a_DrawText("Crit Chance:", content_x, current_y, label_config);
    current_y += line_height + 4;

    dString_t* crit_chance_text = d_StringInit();
    d_StringFormat(crit_chance_text, "%d%%", modal->player->crit_chance);
    a_DrawText((char*)d_StringPeek(crit_chance_text), content_x + 10, current_y, value_config);
    d_StringDestroy(crit_chance_text);
    current_y += line_height + spacing;

    // Crit Bonus (default 50%, can be increased by future mechanics)
    a_DrawText("Crit Damage Bonus:", content_x, current_y, label_config);
    current_y += line_height + 4;

    dString_t* crit_bonus_text = d_StringInit();
    // Base crit is +50%, plus any crit_bonus from player
    int total_crit_bonus = 50 + modal->player->crit_bonus;
    d_StringFormat(crit_bonus_text, "+%d%%", total_crit_bonus);
    a_DrawText((char*)d_StringPeek(crit_bonus_text), content_x + 10, current_y, value_config);
    d_StringDestroy(crit_bonus_text);
    current_y += line_height + spacing;

    // ========================================================================
    // Defensive Stats (chip economy percentage modifiers from trinkets)
    // Shows passive bonuses from Elite Membership and defensive affixes
    // ========================================================================

    current_y += spacing * 2;  // Extra spacing before new section

    // Section header
    aTextStyle_t trinket_header_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {168, 202, 88, 255},  // Green
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText("Defensive:", content_x, current_y - 22, trinket_header_config);  // Move up 22px (18 + 4 extra)
    current_y += line_height + spacing;

    // Win Bonus (percentage modifier - always shown even if 0%)
    a_DrawText("Win Bonus:", content_x, current_y, label_config);
    current_y += line_height + 4;

    dString_t* win_bonus_text = d_StringInit();
    d_StringFormat(win_bonus_text, "+%d%%", modal->player->win_bonus_percent);
    a_DrawText((char*)d_StringPeek(win_bonus_text), content_x + 10, current_y, value_config);
    d_StringDestroy(win_bonus_text);
    current_y += line_height + spacing;

    // Loss Refund (percentage modifier - always shown even if 0%)
    a_DrawText("Loss Refund:", content_x, current_y, label_config);
    current_y += line_height + 4;

    dString_t* loss_refund_text = d_StringInit();
    d_StringFormat(loss_refund_text, "+%d%%", modal->player->loss_refund_percent);
    a_DrawText((char*)d_StringPeek(loss_refund_text), content_x + 10, current_y, value_config);
    d_StringDestroy(loss_refund_text);
    current_y += line_height + spacing;
}
