/*
 * Ability Tooltip Modal Implementation
 */

#include "../../../include/scenes/components/abilityTooltipModal.h"
#include "../../../include/scenes/components/abilityDisplay.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include <stdlib.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

AbilityTooltipModal_t* CreateAbilityTooltipModal(void) {
    AbilityTooltipModal_t* modal = malloc(sizeof(AbilityTooltipModal_t));
    if (!modal) {
        d_LogFatal("CreateAbilityTooltipModal: Failed to allocate modal");
        return NULL;
    }

    modal->visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->ability = NULL;

    d_LogInfo("AbilityTooltipModal created");
    return modal;
}

void DestroyAbilityTooltipModal(AbilityTooltipModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("AbilityTooltipModal destroyed");
}

// ============================================================================
// SHOW/HIDE
// ============================================================================

void ShowAbilityTooltipModal(AbilityTooltipModal_t* modal,
                             const AbilityData_t* ability,
                             int card_x, int card_y) {
    if (!modal || !ability) return;

    modal->visible = true;
    modal->ability = ability;

    // Position to right of card (with margin)
    modal->x = card_x + ABILITY_CARD_WIDTH + 10;
    modal->y = card_y;

    // If too close to screen edge, show on left instead
    if (modal->x + ABILITY_TOOLTIP_WIDTH > SCREEN_WIDTH - 10) {
        modal->x = card_x - ABILITY_TOOLTIP_WIDTH - 10;
    }

    // Clamp Y to screen bounds (use MIN_HEIGHT for estimation)
    if (modal->y + ABILITY_TOOLTIP_MIN_HEIGHT > SCREEN_HEIGHT - 10) {
        modal->y = SCREEN_HEIGHT - ABILITY_TOOLTIP_MIN_HEIGHT - 10;
    }
    if (modal->y < TOP_BAR_HEIGHT + 10) {
        modal->y = TOP_BAR_HEIGHT + 10;
    }
}

void HideAbilityTooltipModal(AbilityTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
    modal->ability = NULL;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * GetAbilityFullName - Get full ability name
 */
static const char* GetAbilityFullName(EnemyAbility_t ability) {
    switch (ability) {
        case ABILITY_THE_HOUSE_REMEMBERS:    return "The House Remembers";
        case ABILITY_IRREGULARITY_DETECTED:  return "Irregularity Detected";
        case ABILITY_SYSTEM_OVERRIDE:        return "System Override";
        default:                             return "Unknown Ability";
    }
}

/**
 * GetAbilityDescription - Get flavor text description
 */
static const char* GetAbilityDescription(EnemyAbility_t ability) {
    switch (ability) {
        case ABILITY_THE_HOUSE_REMEMBERS:
            return "\"The casino is alive, and it never forgets a winner.\"";
        case ABILITY_IRREGULARITY_DETECTED:
            return "\"The casino is watching. You've been noticed.\"";
        case ABILITY_SYSTEM_OVERRIDE:
            return "\"The casino is taking control. You no longer have a choice.\"";
        default:
            return "";
    }
}

/**
 * GetAbilityTriggerText - Get trigger condition description
 */
static const char* GetAbilityTriggerText(const AbilityData_t* ability) {
    switch (ability->ability_id) {
        case ABILITY_THE_HOUSE_REMEMBERS:
            return "Trigger: When player gets blackjack";
        case ABILITY_IRREGULARITY_DETECTED:
            return "Trigger: Every 5 cards drawn";
        case ABILITY_SYSTEM_OVERRIDE:
            return "Trigger: Once when HP < 30%";
        default:
            return "Trigger: Unknown";
    }
}

/**
 * GetAbilityEffectText - Get effect description
 */
static const char* GetAbilityEffectText(EnemyAbility_t ability) {
    switch (ability) {
        case ABILITY_THE_HOUSE_REMEMBERS:
            return "Effect: Applies GREED - Win only 50% chips for 2 rounds";
        case ABILITY_IRREGULARITY_DETECTED:
            return "Effect: Applies CHIP_DRAIN - Lose 5 chips per round for 3 rounds";
        case ABILITY_SYSTEM_OVERRIDE:
            return "Effect: Heals 50 HP + Forces deck reshuffle (System recalibrates when threatened)";
        default:
            return "Effect: Unknown";
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderAbilityTooltipModal(const AbilityTooltipModal_t* modal) {
    if (!modal || !modal->visible || !modal->ability) return;

    int x = modal->x;
    int y = modal->y;

    // Padding and spacing constants
    int padding = 16;
    int content_width = ABILITY_TOOLTIP_WIDTH - (padding * 2);

    // Measure actual text heights dynamically
    const char* name = GetAbilityFullName(modal->ability->ability_id);
    const char* desc = GetAbilityDescription(modal->ability->ability_id);
    const char* trigger = GetAbilityTriggerText(modal->ability);
    const char* effect = GetAbilityEffectText(modal->ability->ability_id);

    int title_height = a_GetWrappedTextHeight((char*)name, FONT_ENTER_COMMAND, content_width);
    int desc_height = a_GetWrappedTextHeight((char*)desc, FONT_GAME, content_width);
    int trigger_height = a_GetWrappedTextHeight((char*)trigger, FONT_GAME, content_width);
    int effect_height = a_GetWrappedTextHeight((char*)effect, FONT_GAME, content_width);

    // Calculate content height dynamically
    int current_y = padding;  // Start with top padding
    current_y += title_height + 10;  // Title + margin
    current_y += desc_height + 8;  // Description + spacing
    current_y += 1 + 12;  // Divider + spacing
    current_y += trigger_height + 4;  // Trigger + spacing

    // State line (if applicable)
    if (modal->ability->trigger == TRIGGER_COUNTER) {
        current_y += a_GetWrappedTextHeight("Countdown: 0 remaining (0/0)", FONT_GAME, content_width) + 4;
    } else if (modal->ability->has_triggered && modal->ability->trigger == TRIGGER_HP_THRESHOLD) {
        current_y += a_GetWrappedTextHeight("Status: Already used", FONT_GAME, content_width) + 4;
    } else if (modal->ability->trigger == TRIGGER_ON_EVENT) {
        current_y += a_GetWrappedTextHeight("Status: Ready", FONT_GAME, content_width) + 4;
    }

    current_y += 8;  // Spacing before effect
    current_y += effect_height + padding;  // Effect + bottom padding

    // Calculate modal height
    int modal_height = current_y;
    if (modal_height < ABILITY_TOOLTIP_MIN_HEIGHT) {
        modal_height = ABILITY_TOOLTIP_MIN_HEIGHT;
    }

    // Draw background (dark with transparency)
    a_DrawFilledRect((aRectf_t){x, y, ABILITY_TOOLTIP_WIDTH, modal_height},
                    (aColor_t){20, 20, 30, 230});

    // Draw border
    a_DrawRect((aRectf_t){x, y, ABILITY_TOOLTIP_WIDTH, modal_height},
              (aColor_t){255, 255, 255, 255});

    // Now draw content on top
    int content_x = x + padding;
    int content_y = y + padding;
    current_y = content_y;

    // Title (ability name)
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {232, 193, 112, 255},  // Gold
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)name, content_x, current_y, title_config);
    current_y += title_height + 10;  // Actual measured height + margin

    // Description (flavor text, italicized feel via smaller font)
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

    // Trigger condition
    aTextStyle_t trigger_config = {
        .type = FONT_GAME,
        .fg = {168, 202, 88, 255},  // Yellow-green
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f
    };
    a_DrawText((char*)trigger, content_x, current_y, trigger_config);
    current_y += trigger_height + 4;  // Actual measured height + spacing

    // Current state (for counters or used abilities)
    if (modal->ability->trigger == TRIGGER_COUNTER) {
        int remaining = modal->ability->counter_max - modal->ability->counter_current;

        dString_t* state_text = d_StringInit();
        d_StringFormat(state_text, "Countdown: %d remaining (%d/%d)",
                      remaining,
                      modal->ability->counter_current,
                      modal->ability->counter_max);

        int state_height = a_GetWrappedTextHeight((char*)d_StringPeek(state_text), FONT_GAME, content_width);

        aTextStyle_t state_config = {
            .type = FONT_GAME,
            .fg = {222, 158, 65, 255},  // Palette yellow-orange (matches badge)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.1f
        };
        a_DrawText((char*)d_StringPeek(state_text), content_x, current_y, state_config);
        d_StringDestroy(state_text);
        current_y += state_height + 4;
    } else if (modal->ability->has_triggered && modal->ability->trigger == TRIGGER_HP_THRESHOLD) {
        int state_height = a_GetWrappedTextHeight("Status: Already used", FONT_GAME, content_width);

        aTextStyle_t state_config = {
            .type = FONT_GAME,
            .fg = {165, 48, 48, 255},  // Palette red (matches badge)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.1f
        };
        a_DrawText("Status: Already used", content_x, current_y, state_config);
        current_y += state_height + 4;
    } else if (modal->ability->trigger == TRIGGER_ON_EVENT) {
        int state_height = a_GetWrappedTextHeight("Status: Ready", FONT_GAME, content_width);

        aTextStyle_t state_config = {
            .type = FONT_GAME,
            .fg = {222, 158, 65, 255},  // Palette yellow-orange (matches badge)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.1f
        };
        a_DrawText("Status: Ready", content_x, current_y, state_config);
        current_y += state_height + 4;
    }

    current_y += 8;  // Spacing before effect

    // Effect description (multi-line, wraps)
    aTextStyle_t effect_config = {
        .type = FONT_GAME,
        .fg = {207, 87, 60, 255},  // Red-orange
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.1f
    };
    a_DrawText((char*)effect, content_x, current_y, effect_config);
}
