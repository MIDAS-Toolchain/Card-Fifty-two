/*
 * Chips Tooltip Modal Component Implementation
 */

#include "../../../include/scenes/components/chipsTooltipModal.h"
#include "../../../include/stats.h"

// Modern tooltip colors
#define COLOR_BG      ((aColor_t){20, 20, 30, 230})
#define COLOR_BORDER  ((aColor_t){100, 100, 100, 200})
#define COLOR_TITLE   ((aColor_t){235, 237, 233, 255})
#define COLOR_TEXT    ((aColor_t){200, 200, 200, 255})
#define COLOR_CHIPS   ((aColor_t){168, 202, 88, 255})  // Yellow-green
#define COLOR_DIVIDER ((aColor_t){100, 100, 100, 200})  // Divider line
#define COLOR_STAT_GOOD   ((aColor_t){115, 190, 211, 255})  // Cyan for highest
#define COLOR_STAT_BAD    ((aColor_t){222, 158, 65, 255})   // Orange for lowest

// ============================================================================
// LIFECYCLE
// ============================================================================

ChipsTooltipModal_t* CreateChipsTooltipModal(void) {
    ChipsTooltipModal_t* modal = malloc(sizeof(ChipsTooltipModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate ChipsTooltipModal");
        return NULL;
    }

    modal->visible = false;
    modal->x = 0;
    modal->y = 0;

    d_LogInfo("ChipsTooltipModal created");
    return modal;
}

void DestroyChipsTooltipModal(ChipsTooltipModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("ChipsTooltipModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowChipsTooltipModal(ChipsTooltipModal_t* modal, int x, int y) {
    if (!modal) return;

    modal->visible = true;
    modal->x = x;
    modal->y = y;
}

void HideChipsTooltipModal(ChipsTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderChipsTooltipModal(const ChipsTooltipModal_t* modal) {
    if (!modal || !modal->visible) return;

    int padding = 16;
    int content_width = CHIPS_TOOLTIP_WIDTH - (padding * 2);

    // Get stats
    const GlobalStats_t* stats = Stats_GetCurrent();

    // Prepare stat strings
    dString_t* highest_str = d_StringInit();
    dString_t* lowest_str = d_StringInit();

    // Prevent underflow: only show "turns ago" if peak was actually reached
    if (stats->highest_chips_turn <= stats->turns_played) {
        uint64_t turns_ago_highest = stats->turns_played - stats->highest_chips_turn;
        d_StringFormat(highest_str, "Highest: %d (%llu turns ago)", stats->highest_chips, turns_ago_highest);
    } else {
        d_StringFormat(highest_str, "Highest: %d", stats->highest_chips);
    }

    if (stats->lowest_chips_turn <= stats->turns_played) {
        uint64_t turns_ago_lowest = stats->turns_played - stats->lowest_chips_turn;
        d_StringFormat(lowest_str, "Lowest: %d (%llu turns ago)", stats->lowest_chips, turns_ago_lowest);
    } else {
        d_StringFormat(lowest_str, "Lowest: %d", stats->lowest_chips);
    }

    // Measure actual text heights
    int title_height = a_GetWrappedTextHeight("Chips = Life", FONT_ENTER_COMMAND, content_width);
    int desc1_height = a_GetWrappedTextHeight("Run out and you die.", FONT_GAME, content_width);
    int desc2_height = a_GetWrappedTextHeight("Bet wisely.", FONT_GAME, content_width);
    int highest_height = a_GetWrappedTextHeight((char*)d_StringPeek(highest_str), FONT_GAME, content_width);
    int lowest_height = a_GetWrappedTextHeight((char*)d_StringPeek(lowest_str), FONT_GAME, content_width);

    // Calculate dynamic height
    int modal_height = padding;
    modal_height += title_height + 8;    // Title + margin
    modal_height += desc1_height + 4;    // First desc + small spacing
    modal_height += desc2_height + 8;    // Second desc + spacing before divider
    modal_height += 1 + 8;               // Divider + spacing
    modal_height += highest_height + 4;  // Highest stat + spacing
    modal_height += lowest_height + padding;  // Lowest stat + bottom padding

    // Draw background (modern dark style)
    a_DrawFilledRect((aRectf_t){modal->x, modal->y, CHIPS_TOOLTIP_WIDTH, modal_height}, COLOR_BG);

    // Draw border
    a_DrawRect((aRectf_t){modal->x, modal->y, CHIPS_TOOLTIP_WIDTH, modal_height}, COLOR_BORDER);

    int content_x = modal->x + padding;
    int current_y = modal->y + padding;

    // Title
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = COLOR_CHIPS,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    a_DrawText("Chips = Life", content_x, current_y, title_config);
    current_y += title_height + 8;  // Actual height + margin

    // Description
    aTextStyle_t desc_config = {
        .type = FONT_GAME,
        .fg = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };

    a_DrawText("Run out and you die.", content_x, current_y, desc_config);
    current_y += desc1_height + 4;  // Actual height + small spacing
    a_DrawText("Bet wisely.", content_x, current_y, desc_config);
    current_y += desc2_height + 8;  // Spacing before divider

    // Divider line
    a_DrawFilledRect((aRectf_t){content_x, current_y, content_width, 1}, COLOR_DIVIDER);
    current_y += 1 + 8;  // Divider + spacing

    // Stats - Highest chips
    aTextStyle_t highest_config = {
        .type = FONT_GAME,
        .fg = COLOR_STAT_GOOD,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };
    a_DrawText((char*)d_StringPeek(highest_str), content_x, current_y, highest_config);
    current_y += highest_height + 4;  // Spacing between stats

    // Stats - Lowest chips
    aTextStyle_t lowest_config = {
        .type = FONT_GAME,
        .fg = COLOR_STAT_BAD,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f  // ADR-008: Consistent readability
    };
    a_DrawText((char*)d_StringPeek(lowest_str), content_x, current_y, lowest_config);

    // Cleanup
    d_StringDestroy(highest_str);
    d_StringDestroy(lowest_str);
}
