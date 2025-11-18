/*
 * Character Stats Modal Component Implementation
 * Displays character info and sanity threshold effects
 */

#include "../../../include/scenes/components/characterStatsModal.h"
#include "../../../include/scenes/sceneBlackjack.h"

// Modern tooltip color scheme (matching status/ability tooltips)
#define COLOR_BG            ((aColor_t){20, 20, 30, 230})     // Dark background
#define COLOR_BORDER        ((aColor_t){100, 100, 100, 200})  // Light gray border
#define COLOR_TITLE         ((aColor_t){235, 237, 233, 255})  // #ebede9 - off-white title
#define COLOR_TEXT          ((aColor_t){200, 200, 200, 255})  // Light gray text
#define COLOR_SANITY_GOOD   ((aColor_t){115, 190, 211, 255})  // #73bed3 - cyan (healthy)
#define COLOR_SANITY_LOW    ((aColor_t){222, 158, 65, 255})   // #de9e41 - orange (warning)
#define COLOR_DIVIDER       ((aColor_t){100, 100, 100, 200})  // Divider line

// ============================================================================
// LIFECYCLE
// ============================================================================

CharacterStatsModal_t* CreateCharacterStatsModal(Player_t* player) {
    if (!player) {
        d_LogError("CreateCharacterStatsModal: NULL player");
        return NULL;
    }

    CharacterStatsModal_t* modal = malloc(sizeof(CharacterStatsModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate CharacterStatsModal");
        return NULL;
    }

    modal->is_visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->player = player;  // Reference only, not owned

    d_LogInfo("CharacterStatsModal created");
    return modal;
}

void DestroyCharacterStatsModal(CharacterStatsModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("CharacterStatsModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowCharacterStatsModal(CharacterStatsModal_t* modal, int x, int y) {
    if (!modal) return;

    modal->is_visible = true;
    modal->x = x;
    modal->y = y;
}

void HideCharacterStatsModal(CharacterStatsModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
}

// ============================================================================
// RENDERING
// ============================================================================

// Helper function to get class name as string
static const char* GetClassName(PlayerClass_t class) {
    switch (class) {
        case PLAYER_CLASS_DEGENERATE: return "The Degenerate";
        case PLAYER_CLASS_DEALER:     return "The Dealer";
        case PLAYER_CLASS_DETECTIVE:  return "The Detective";
        case PLAYER_CLASS_DREAMER:    return "The Dreamer";
        default:                      return "Unknown Class";
    }
}

// Helper function to get class-specific sanity threshold effects
static const char* GetSanityThresholdEffect(PlayerClass_t class, int threshold) {
    switch (class) {
        case PLAYER_CLASS_DEGENERATE:
            switch (threshold) {
                case 75: return "Risk Bonus: +10% token gain";
                case 50: return "Shaky Hands: -5% accuracy";
                case 25: return "Desperate: +20% damage taken";
                case 0:  return "Broken: Cannot gain tokens";
                default: return "[Unknown threshold]";
            }
        case PLAYER_CLASS_DEALER:
            return "[Class not implemented]";
        case PLAYER_CLASS_DETECTIVE:
            return "[Class not implemented]";
        case PLAYER_CLASS_DREAMER:
            return "[Class not implemented]";
        default:
            return "[No class selected]";
    }
}

void RenderCharacterStatsModal(CharacterStatsModal_t* modal) {
    if (!modal || !modal->is_visible || !modal->player) return;

    // Use full game area height (minus top bar)
    int padding = 16;
    int modal_width = STATS_MODAL_WIDTH;
    int modal_height = SCREEN_HEIGHT - TOP_BAR_HEIGHT;  // Full height!
    int content_width = modal_width - (padding * 2);

    // Get class name for measurement
    const char* class_name_for_measure = GetClassName(modal->player->class);

    // Measure actual text heights (for spacing only, not for modal height)
    int title_height = a_GetWrappedTextHeight((char*)class_name_for_measure, FONT_ENTER_COMMAND, content_width);

    dString_t* sanity_measure = d_StringInit();
    d_StringFormat(sanity_measure, "Sanity: %d / %d", modal->player->sanity, modal->player->max_sanity);
    int sanity_height = a_GetWrappedTextHeight((char*)d_StringPeek(sanity_measure), FONT_GAME, content_width);
    d_StringDestroy(sanity_measure);

    dString_t* chips_measure = d_StringInit();
    d_StringFormat(chips_measure, "Tokens: %d", modal->player->chips);
    int chips_height = a_GetWrappedTextHeight((char*)d_StringPeek(chips_measure), FONT_GAME, content_width);
    d_StringDestroy(chips_measure);

    int header_height = a_GetWrappedTextHeight("Sanity Effects:", FONT_ENTER_COMMAND, content_width);
    int range_height = a_GetWrappedTextHeight("75-100:", FONT_GAME, content_width);
    int effect_height = a_GetWrappedTextHeight("Risk Bonus: +10% token gain", FONT_GAME, content_width - 10);

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
    // Title - Class Name (e.g., "The Degenerate")
    // ========================================================================

    aFontConfig_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .color = COLOR_TITLE,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    const char* class_name = GetClassName(modal->player->class);
    a_DrawTextStyled((char*)class_name, content_x, current_y, &title_config);
    current_y += title_height + 8;  // Actual height + margin

    // ========================================================================
    // Sanity Display (color-coded based on current sanity)
    // ========================================================================

    dString_t* sanity_text = d_StringInit();
    d_StringFormat(sanity_text, "Sanity: %d / %d", modal->player->sanity, modal->player->max_sanity);

    // Color based on sanity percentage
    float sanity_percent = (float)modal->player->sanity / (float)modal->player->max_sanity;
    aColor_t sanity_color = (sanity_percent >= 0.5f) ? COLOR_SANITY_GOOD : COLOR_SANITY_LOW;

    aFontConfig_t sanity_config = {
        .type = FONT_GAME,
        .color = sanity_color,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    a_DrawTextStyled((char*)d_StringPeek(sanity_text), content_x, current_y, &sanity_config);
    d_StringDestroy(sanity_text);
    current_y += sanity_height + 8;  // Actual height + spacing

    // ========================================================================
    // Divider Line
    // ========================================================================

    a_DrawFilledRect(content_x, current_y, content_width, 1,
                     COLOR_DIVIDER.r, COLOR_DIVIDER.g, COLOR_DIVIDER.b, COLOR_DIVIDER.a);
    current_y += 1 + 8;  // Divider height + spacing

    // ========================================================================
    // Chips Display
    // ========================================================================

    dString_t* chips_text = d_StringInit();
    d_StringFormat(chips_text, "Tokens: %d", modal->player->chips);

    aFontConfig_t chips_config = {
        .type = FONT_GAME,
        .color = COLOR_TEXT,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    a_DrawTextStyled((char*)d_StringPeek(chips_text), content_x, current_y, &chips_config);
    d_StringDestroy(chips_text);
    current_y += chips_height + 12;  // Actual height + section spacing

    // ========================================================================
    // Sanity Thresholds Section
    // ========================================================================

    // Header
    aFontConfig_t header_config = {
        .type = FONT_ENTER_COMMAND,
        .color = COLOR_SANITY_GOOD,
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    a_DrawTextStyled("Sanity Effects:", content_x, current_y, &header_config);
    current_y += header_height + 8;  // Actual height + margin

    // Threshold colors (based on severity)
    aColor_t threshold_colors[] = {
        COLOR_SANITY_GOOD,  // 75+ (good)
        COLOR_TEXT,         // 50-74 (neutral)
        COLOR_SANITY_LOW,   // 25-49 (warning)
        (aColor_t){200, 50, 50, 255}  // 0-24 (danger - red)
    };

    const char* threshold_ranges[] = {
        "75-100:",
        "50-74:",
        "25-49:",
        "0-24:"
    };

    int threshold_values[] = {75, 50, 25, 0};

    // Render each threshold
    for (int i = 0; i < 4; i++) {
        // Threshold range label
        aFontConfig_t range_config = {
            .type = FONT_GAME,
            .color = threshold_colors[i],
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.1f  // ADR-008: Consistent readability
        };

        int range_height = a_GetWrappedTextHeight((char*)threshold_ranges[i], FONT_GAME, content_width);
        a_DrawTextStyled((char*)threshold_ranges[i], content_x, current_y, &range_config);
        current_y += range_height + 4;  // Actual height + small spacing

        // Effect description (indented slightly)
        const char* effect = GetSanityThresholdEffect(modal->player->class, threshold_values[i]);

        aFontConfig_t effect_config = {
            .type = FONT_GAME,
            .color = COLOR_TEXT,
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width - 10,  // Indent effect text
            .scale = 1.1f  // Match ability/status tooltip size for readability
        };

        int effect_height = a_GetWrappedTextHeight((char*)effect, FONT_GAME, content_width - 10);
        a_DrawTextStyled((char*)effect, content_x + 10, current_y, &effect_config);
        current_y += effect_height + 6;  // Actual height + spacing between entries
    }
}
