/*
 * Bet Tooltip Modal Component Implementation
 * Constitutional pattern: Auto-size tooltip following ADR-008
 */

#include "../../../include/scenes/components/betTooltipModal.h"
#include "../../../include/stats.h"

// Modern tooltip colors (ADR-008 compliance)
#define COLOR_BG      ((aColor_t){20, 20, 30, 230})
#define COLOR_BORDER  ((aColor_t){100, 100, 100, 200})
#define COLOR_TITLE   ((aColor_t){235, 237, 233, 255})
#define COLOR_TEXT    ((aColor_t){200, 200, 200, 255})
#define COLOR_BET_ACTIVE    ((aColor_t){222, 158, 65, 255})   // Orange
#define COLOR_BET_INACTIVE  ((aColor_t){150, 150, 150, 255})  // Gray
#define COLOR_STAT_GOOD     ((aColor_t){115, 190, 211, 255})  // Cyan
#define COLOR_DIVIDER       ((aColor_t){100, 100, 100, 200})  // Divider line

// ============================================================================
// LIFECYCLE
// ============================================================================

BetTooltipModal_t* CreateBetTooltipModal(void) {
    BetTooltipModal_t* modal = malloc(sizeof(BetTooltipModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate BetTooltipModal");
        return NULL;
    }

    modal->visible = false;
    modal->x = 0;
    modal->y = 0;

    d_LogInfo("BetTooltipModal created");
    return modal;
}

void DestroyBetTooltipModal(BetTooltipModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("BetTooltipModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowBetTooltipModal(BetTooltipModal_t* modal, int x, int y) {
    if (!modal) return;

    modal->visible = true;
    modal->x = x;
    modal->y = y;
}

void HideBetTooltipModal(BetTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderBetTooltipModal(const BetTooltipModal_t* modal, const Player_t* player) {
    if (!modal || !modal->visible || !player) return;

    int padding = 16;
    int content_width = BET_TOOLTIP_WIDTH - (padding * 2);

    // Get stats
    const GlobalStats_t* stats = Stats_GetCurrent();
    int average_bet = Stats_GetAverageBet();

    // Prepare stat strings
    dString_t* current_bet_str = d_StringInit();
    dString_t* highest_bet_str = d_StringInit();
    dString_t* average_bet_str = d_StringInit();
    dString_t* total_wagered_str = d_StringInit();

    d_StringFormat(current_bet_str, "Current Bet: %d chips", player->current_bet);

    // Prevent underflow: only show "turns ago" if peak was actually set
    if (stats->highest_bet > 0 && stats->highest_bet_turn <= stats->turns_played) {
        uint64_t turns_ago_highest = stats->turns_played - stats->highest_bet_turn;
        d_StringFormat(highest_bet_str, "Highest Bet: %d (%llu turns ago)",
                       stats->highest_bet, turns_ago_highest);
    } else {
        d_StringFormat(highest_bet_str, "Highest Bet: %d", stats->highest_bet);
    }

    d_StringFormat(average_bet_str, "Average Bet: %d chips", average_bet);
    d_StringFormat(total_wagered_str, "Total Wagered: %llu chips", stats->chips_bet);

    // Measure actual text heights
    int title_height = a_GetWrappedTextHeight("Betting System", FONT_ENTER_COMMAND, content_width);
    int desc1_height = a_GetWrappedTextHeight("Risk chips to damage enemies.", FONT_GAME, content_width);
    int desc2_height = a_GetWrappedTextHeight("Win = bet profit + damage dealt.", FONT_GAME, content_width);
    int desc3_height = a_GetWrappedTextHeight("Push = bet returned + half damage.", FONT_GAME, content_width);
    int desc4_height = a_GetWrappedTextHeight("Lose = chips lost, no damage.", FONT_GAME, content_width);
    int current_bet_height = a_GetWrappedTextHeight((char*)d_StringPeek(current_bet_str), FONT_GAME, content_width);
    int highest_bet_height = a_GetWrappedTextHeight((char*)d_StringPeek(highest_bet_str), FONT_GAME, content_width);
    int average_bet_height = a_GetWrappedTextHeight((char*)d_StringPeek(average_bet_str), FONT_GAME, content_width);
    int total_wagered_height = a_GetWrappedTextHeight((char*)d_StringPeek(total_wagered_str), FONT_GAME, content_width);

    // Calculate dynamic height
    int modal_height = padding;
    modal_height += title_height + 8;       // Title + margin
    modal_height += desc1_height + 4;       // Desc line 1
    modal_height += desc2_height + 4;       // Desc line 2
    modal_height += desc3_height + 4;       // Desc line 3
    modal_height += desc4_height + 8;       // Desc line 4 + spacing
    modal_height += 1 + 8;                  // Divider + spacing
    modal_height += current_bet_height + 4; // Current bet
    modal_height += highest_bet_height + 4; // Highest bet
    modal_height += average_bet_height + 4; // Average bet
    modal_height += total_wagered_height + padding; // Total wagered + bottom padding

    // Draw background (modern dark style)
    a_DrawFilledRect(modal->x, modal->y, BET_TOOLTIP_WIDTH, modal_height,
                     COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);

    // Draw border
    a_DrawRect(modal->x, modal->y, BET_TOOLTIP_WIDTH, modal_height,
               COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);

    int content_x = modal->x + padding;
    int current_y = modal->y + padding;

    // Title
    aFontConfig_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .color = COLOR_TITLE,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    a_DrawTextStyled("Betting System", content_x, current_y, &title_config);
    current_y += title_height + 8;  // Actual height + margin

    // Description
    aFontConfig_t desc_config = {
        .type = FONT_GAME,
        .color = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };

    a_DrawTextStyled("Risk chips to damage enemies.", content_x, current_y, &desc_config);
    current_y += desc1_height + 4;
    a_DrawTextStyled("Win = bet profit + damage dealt.", content_x, current_y, &desc_config);
    current_y += desc2_height + 4;
    a_DrawTextStyled("Push = bet returned + half damage.", content_x, current_y, &desc_config);
    current_y += desc3_height + 4;
    a_DrawTextStyled("Lose = chips lost, no damage.", content_x, current_y, &desc_config);
    current_y += desc4_height + 8;

    // Divider line
    a_DrawFilledRect(content_x, current_y, content_width, 1,
                     COLOR_DIVIDER.r, COLOR_DIVIDER.g, COLOR_DIVIDER.b, COLOR_DIVIDER.a);
    current_y += 1 + 8;  // Divider + spacing

    // Stats - Current bet (color-coded)
    aColor_t current_bet_color = (player->current_bet > 0) ? COLOR_BET_ACTIVE : COLOR_BET_INACTIVE;
    aFontConfig_t current_bet_config = {
        .type = FONT_GAME,
        .color = current_bet_color,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };
    a_DrawTextStyled((char*)d_StringPeek(current_bet_str), content_x, current_y, &current_bet_config);
    current_y += current_bet_height + 4;

    // Stats - Highest bet
    aFontConfig_t highest_bet_config = {
        .type = FONT_GAME,
        .color = COLOR_STAT_GOOD,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };
    a_DrawTextStyled((char*)d_StringPeek(highest_bet_str), content_x, current_y, &highest_bet_config);
    current_y += highest_bet_height + 4;

    // Stats - Average bet
    aFontConfig_t average_bet_config = {
        .type = FONT_GAME,
        .color = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };
    a_DrawTextStyled((char*)d_StringPeek(average_bet_str), content_x, current_y, &average_bet_config);
    current_y += average_bet_height + 4;

    // Stats - Total wagered
    aFontConfig_t total_wagered_config = {
        .type = FONT_GAME,
        .color = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };
    a_DrawTextStyled((char*)d_StringPeek(total_wagered_str), content_x, current_y, &total_wagered_config);

    // Cleanup
    d_StringDestroy(current_bet_str);
    d_StringDestroy(highest_bet_str);
    d_StringDestroy(average_bet_str);
    d_StringDestroy(total_wagered_str);
}
