/*
 * Character Stats Modal Component Implementation
 * Displays character info and sanity threshold effects
 */

#include "../../../include/scenes/components/characterStatsModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/sanityThreshold.h"

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

// No longer needed - using GetSanityThresholdDescription from sanityThreshold.h

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

    int header_height = a_GetWrappedTextHeight("Sanity Effects:", FONT_ENTER_COMMAND, content_width);

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

    // Correct threshold ranges (matching sanity tier system)
    const char* threshold_ranges[] = {
        "76-100:",  // SANITY_TIER_HIGH
        "51-75:",   // SANITY_TIER_MEDIUM
        "26-50:",   // SANITY_TIER_LOW
        "1-25:",    // SANITY_TIER_VERY_LOW
        "0:"        // SANITY_TIER_ZERO
    };

    SanityTier_t threshold_tiers[] = {
        SANITY_TIER_HIGH,
        SANITY_TIER_MEDIUM,
        SANITY_TIER_LOW,
        SANITY_TIER_VERY_LOW,
        SANITY_TIER_ZERO
    };

    // Tier colors (based on severity)
    aColor_t tier_colors[] = {
        COLOR_SANITY_GOOD,              // 76-100% (no effect)
        COLOR_TEXT,                     // 51-75% (first tier)
        COLOR_SANITY_LOW,               // 26-50% (second tier)
        (aColor_t){222, 158, 65, 255},  // 1-25% (third tier - orange)
        (aColor_t){200, 50, 50, 255}    // 0% (final tier - red)
    };

    // Get current sanity tier for visual highlighting
    SanityTier_t current_sanity_tier = GetSanityTier(modal->player);

    // Render each threshold (5 tiers now, including 100-76%)
    for (int i = 0; i < 5; i++) {
        // Determine visual state of this threshold
        bool is_current = (threshold_tiers[i] == current_sanity_tier);
        bool is_past = (threshold_tiers[i] > current_sanity_tier);  // Higher tier number = lower sanity

        // Calculate heights for background box
        int range_height = a_GetWrappedTextHeight((char*)threshold_ranges[i], FONT_GAME, content_width);
        const char* effect = GetSanityThresholdDescription(modal->player->class, threshold_tiers[i]);
        int effect_height = a_GetWrappedTextHeight((char*)effect, FONT_GAME, content_width - 10);

        // Draw gold background highlight for current tier
        if (is_current) {
            int highlight_height = range_height + effect_height + 10;
            a_DrawFilledRect(content_x - 5, current_y - 2,
                           content_width + 10, highlight_height,
                           232, 193, 112, 64);  // Gold 25% opacity
        }

        // Draw bullet indicator for current tier
        if (is_current) {
            a_DrawText("●", content_x - 15, current_y,
                      232, 193, 112,  // Gold bullet (RGB)
                      FONT_GAME, TEXT_ALIGN_LEFT, 0);
        }

        // Determine text color based on state
        aColor_t range_text_color;
        aColor_t effect_text_color;
        if (is_current) {
            // Current tier: GOLD
            range_text_color = (aColor_t){232, 193, 112, 255};
            effect_text_color = (aColor_t){232, 193, 112, 255};
        } else if (is_past) {
            // Past tier: DIMMED GRAY
            range_text_color = (aColor_t){150, 150, 150, 255};
            effect_text_color = (aColor_t){150, 150, 150, 255};
        } else {
            // Future tier: severity color for range, normal gray for effect
            range_text_color = tier_colors[i];
            effect_text_color = COLOR_TEXT;
        }

        // Threshold range label
        aFontConfig_t range_config = {
            .type = FONT_GAME,
            .color = range_text_color,
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.1f  // ADR-008: Consistent readability
        };

        a_DrawTextStyled((char*)threshold_ranges[i], content_x, current_y, &range_config);
        current_y += range_height + 4;  // Actual height + small spacing

        // Effect description (indented slightly)
        aFontConfig_t effect_config = {
            .type = FONT_GAME,
            .color = effect_text_color,
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width - 10,  // Indent effect text
            .scale = 1.1f  // Match ability/status tooltip size for readability
        };

        a_DrawTextStyled((char*)effect, content_x + 10, current_y, &effect_config);
        current_y += effect_height + 6;  // Actual height + spacing between entries
    }
}
