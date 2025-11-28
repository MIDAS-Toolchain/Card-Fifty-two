/*
 * Status Effect Tooltip Modal Implementation
 */

#include "../../../include/scenes/components/statusEffectTooltipModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include <stdlib.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

StatusEffectTooltipModal_t* CreateStatusEffectTooltipModal(void) {
    StatusEffectTooltipModal_t* modal = malloc(sizeof(StatusEffectTooltipModal_t));
    if (!modal) {
        d_LogFatal("CreateStatusEffectTooltipModal: Failed to allocate modal");
        return NULL;
    }

    modal->visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->effect = NULL;

    d_LogInfo("StatusEffectTooltipModal created");
    return modal;
}

void DestroyStatusEffectTooltipModal(StatusEffectTooltipModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("StatusEffectTooltipModal destroyed");
}

// ============================================================================
// SHOW/HIDE
// ============================================================================

void ShowStatusEffectTooltipModal(StatusEffectTooltipModal_t* modal,
                                  const StatusEffectInstance_t* effect,
                                  int icon_x, int icon_y) {
    if (!modal || !effect) return;

    modal->visible = true;
    modal->effect = effect;

    // Position to right of icon (with margin)
    const int ICON_SIZE = 48;  // Match RenderStatusEffects icon size
    modal->x = icon_x + ICON_SIZE + 10;
    modal->y = icon_y;

    // If too close to screen edge, show on left instead
    if (modal->x + STATUS_TOOLTIP_WIDTH > SCREEN_WIDTH - 10) {
        modal->x = icon_x - STATUS_TOOLTIP_WIDTH - 10;
    }

    // Clamp Y to screen bounds (use MIN_HEIGHT for estimation)
    if (modal->y + STATUS_TOOLTIP_MIN_HEIGHT > SCREEN_HEIGHT - 10) {
        modal->y = SCREEN_HEIGHT - STATUS_TOOLTIP_MIN_HEIGHT - 10;
    }
    if (modal->y < TOP_BAR_HEIGHT + 10) {
        modal->y = TOP_BAR_HEIGHT + 10;
    }
}

void HideStatusEffectTooltipModal(StatusEffectTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
    modal->effect = NULL;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * GetEffectValueText - Get human-readable effect value description
 */
static void GetEffectValueText(const StatusEffectInstance_t* effect, dString_t* out) {
    if (!effect || !out) return;

    switch (effect->type) {
        case STATUS_CHIP_DRAIN:
            d_StringFormat(out, "Effect: Lose %d chips per round", effect->value);
            break;
        case STATUS_TILT:
            d_StringFormat(out, "Effect: Lose 2x chips on loss");
            break;
        case STATUS_GREED:
            d_StringFormat(out, "Effect: Win only 50%% chips");
            break;
        case STATUS_RAKE:
            d_StringFormat(out, "Effect: Lose %d%% of damage as chips", effect->value);
            break;
        default:
            d_StringFormat(out, "Effect: Unknown");
            break;
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderStatusEffectTooltipModal(const StatusEffectTooltipModal_t* modal) {
    if (!modal || !modal->visible || !modal->effect) return;

    int x = modal->x;
    int y = modal->y;

    // Padding and spacing constants
    int padding = 16;
    int content_width = STATUS_TOOLTIP_WIDTH - (padding * 2);

    // Get text content
    const char* name = GetStatusEffectName(modal->effect->type);
    const char* desc = GetStatusEffectDescription(modal->effect->type);

    // Build effect value text
    dString_t* effect_text = d_StringInit();
    GetEffectValueText(modal->effect, effect_text);

    // Build duration text
    dString_t* duration_text = d_StringInit();
    if (modal->effect->duration == 1) {
        d_StringFormat(duration_text, "Duration: 1 round remaining");
    } else {
        d_StringFormat(duration_text, "Duration: %d rounds remaining", modal->effect->duration);
    }

    // Measure text heights dynamically
    int title_height = a_GetWrappedTextHeight((char*)name, FONT_ENTER_COMMAND, content_width);
    int desc_height = a_GetWrappedTextHeight((char*)desc, FONT_GAME, content_width);
    int effect_height = a_GetWrappedTextHeight((char*)d_StringPeek(effect_text), FONT_GAME, content_width);
    int duration_height = a_GetWrappedTextHeight((char*)d_StringPeek(duration_text), FONT_GAME, content_width);

    // Calculate content height dynamically
    int current_y = padding;  // Start with top padding
    current_y += title_height + 10;  // Title + margin
    current_y += desc_height + 8;  // Description + spacing
    current_y += 1 + 12;  // Divider + spacing
    current_y += effect_height + 6;  // Effect + spacing
    current_y += duration_height + padding;  // Duration + bottom padding

    // Calculate modal height
    int modal_height = current_y;
    if (modal_height < STATUS_TOOLTIP_MIN_HEIGHT) {
        modal_height = STATUS_TOOLTIP_MIN_HEIGHT;
    }

    // Draw background (dark with transparency)
    a_DrawFilledRect((aRectf_t){x, y, STATUS_TOOLTIP_WIDTH, modal_height},
                    (aColor_t){20, 20, 30, 230});

    // Draw border (color-coded by effect type)
    aColor_t border_color = GetStatusEffectColor(modal->effect->type);
    a_DrawRect((aRectf_t){x, y, STATUS_TOOLTIP_WIDTH, modal_height},
              (aColor_t){border_color.r, border_color.g, border_color.b, 255});

    // Now draw content on top
    int content_x = x + padding;
    int content_y = y + padding;
    current_y = content_y;

    // Title (effect name) - color-coded
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = GetStatusEffectColor(modal->effect->type),
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)name, content_x, current_y, title_config);
    current_y += title_height + 10;  // Actual measured height + margin

    // Description (flavor text)
    aTextStyle_t desc_config = {
        .type = FONT_GAME,
        .fg = {180, 180, 180, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)desc, content_x, current_y, desc_config);
    current_y += desc_height + 8;  // Actual measured height + spacing

    // Divider
    a_DrawFilledRect((aRectf_t){content_x, current_y, content_width, 1},
                    (aColor_t){100, 100, 100, 200});
    current_y += 12;  // Spacing around divider

    // Effect description
    aTextStyle_t effect_config = {
        .type = FONT_GAME,
        .fg = {207, 87, 60, 255},  // Red-orange (matches ability tooltip)
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f
    };
    a_DrawText((char*)d_StringPeek(effect_text), content_x, current_y, effect_config);
    current_y += effect_height + 6;  // Actual measured height + spacing

    // Duration remaining
    aTextStyle_t duration_config = {
        .type = FONT_GAME,
        .fg = {255, 255, 0, 255},  // Yellow
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f
    };
    a_DrawText((char*)d_StringPeek(duration_text), content_x, current_y, duration_config);

    // Cleanup dStrings
    d_StringDestroy(effect_text);
    d_StringDestroy(duration_text);
}
