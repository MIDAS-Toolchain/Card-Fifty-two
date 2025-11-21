/*
 * Ability Display Component Implementation
 */

#include "../../../include/scenes/components/abilityDisplay.h"
#include <stdlib.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

AbilityDisplay_t* CreateAbilityDisplay(Enemy_t* enemy) {
    AbilityDisplay_t* display = malloc(sizeof(AbilityDisplay_t));
    if (!display) {
        d_LogFatal("CreateAbilityDisplay: Failed to allocate display");
        return NULL;
    }

    display->enemy = enemy;
    display->x = 0;
    display->y = 0;
    display->hovered_index = -1;  // No hover initially

    d_LogInfo("AbilityDisplay created");
    return display;
}

void DestroyAbilityDisplay(AbilityDisplay_t** display) {
    if (!display || !*display) return;

    free(*display);
    *display = NULL;
    d_LogInfo("AbilityDisplay destroyed");
}

// ============================================================================
// SETTERS
// ============================================================================

void SetAbilityDisplayPosition(AbilityDisplay_t* display, int x, int y) {
    if (!display) return;
    display->x = x;
    display->y = y;
}

void SetAbilityDisplayEnemy(AbilityDisplay_t* display, Enemy_t* enemy) {
    if (!display) return;
    display->enemy = enemy;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * GetAbilityColor - Get color for ability card background
 */
static aColor_t GetAbilityColor(EnemyAbility_t ability) {
    switch (ability) {
        case ABILITY_THE_HOUSE_REMEMBERS:    return (aColor_t){222, 158, 65, 255};   // Warm yellow-orange
        case ABILITY_IRREGULARITY_DETECTED:  return (aColor_t){122, 62, 123, 255};   // Deep purple
        case ABILITY_SYSTEM_OVERRIDE:        return (aColor_t){207, 87, 60, 255};    // Burnt orange-red
        default:                             return (aColor_t){100, 100, 100, 255};  // Gray
    }
}

/**
 * GetAbilityAbbreviation - Get 2-letter icon for ability (fits in vertical card)
 */
static const char* GetAbilityAbbreviation(EnemyAbility_t ability) {
    switch (ability) {
        case ABILITY_THE_HOUSE_REMEMBERS:    return "HR";  // House Remembers
        case ABILITY_IRREGULARITY_DETECTED:  return "ID";  // Irregularity Detected
        case ABILITY_SYSTEM_OVERRIDE:        return "SO";  // System Override
        default:                             return "??";
    }
}

/**
 * GetAbilityBadgeText - Get badge text for ability state (reusable for all ability types)
 * @param ability - Ability data
 * @param out_buffer - Output buffer for badge text (must be at least 16 chars)
 * @return bool - true if badge should be shown, false otherwise
 */
static bool GetAbilityBadgeText(const AbilityData_t* ability, char* out_buffer) {
    if (!ability || !out_buffer) return false;

    switch (ability->trigger) {
        case TRIGGER_COUNTER: {
            // Show remaining count (countdown: 5, 4, 3, 2, 1, 0)
            int remaining = ability->counter_max - ability->counter_current;
            snprintf(out_buffer, 16, "%d", remaining);
            return true;
        }

        case TRIGGER_HP_THRESHOLD:
            // Show READY or USED
            if (ability->has_triggered) {
                snprintf(out_buffer, 16, "USED");
            } else {
                snprintf(out_buffer, 16, "READY");
            }
            return true;

        case TRIGGER_ON_EVENT:
            // Always ready
            snprintf(out_buffer, 16, "READY");
            return true;

        default:
            return false;  // No badge for this type
    }
}

/**
 * GetAbilityBadgeColor - Get color for ability badge based on state
 */
static aColor_t GetAbilityBadgeColor(const AbilityData_t* ability, Uint8 alpha) {
    if (!ability) return (aColor_t){255, 255, 255, alpha};

    // Used/exhausted abilities = red tone
    if (ability->has_triggered && ability->trigger == TRIGGER_HP_THRESHOLD) {
        return (aColor_t){165, 48, 48, alpha};  // Palette red
    }

    // Active/ready abilities = warm yellow-orange
    return (aColor_t){222, 158, 65, alpha};  // Palette yellow-orange
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderAbilityDisplay(AbilityDisplay_t* display) {
    if (!display || !display->enemy) return;

    Enemy_t* enemy = display->enemy;
    if (!enemy->active_abilities || enemy->active_abilities->count == 0) return;

    // Get mouse position for hover detection
    int mouse_x = app.mouse.x;
    int mouse_y = app.mouse.y;
    display->hovered_index = -1;

    int base_x = display->x;
    int base_y = display->y;

    // Render each active ability (VERTICAL layout)
    for (size_t i = 0; i < enemy->active_abilities->count; i++) {
        const AbilityData_t* ability = (const AbilityData_t*)
            d_IndexDataFromArray(enemy->active_abilities, i);

        if (!ability) continue;

        // Apply shake offsets if ability is animating
        int current_x = base_x + (int)ability->shake_offset_x;
        int current_y = base_y + (int)ability->shake_offset_y;

        // Check if mouse is hovering over this card (use base position for hover detection)
        bool is_hovered = (mouse_x >= base_x && mouse_x < base_x + ABILITY_CARD_WIDTH &&
                          mouse_y >= base_y && mouse_y < base_y + ABILITY_CARD_HEIGHT);
        if (is_hovered) {
            display->hovered_index = (int)i;
        }

        // Determine card opacity based on trigger state
        Uint8 alpha = 255;
        if (ability->has_triggered && ability->trigger == TRIGGER_HP_THRESHOLD) {
            alpha = 120;  // Dimmed if already triggered (one-time abilities)
        }

        // Draw card background (color-coded by ability type)
        aColor_t bg_color = GetAbilityColor(ability->ability_id);
        bg_color.a = alpha;
        a_DrawFilledRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                        bg_color);

        // Draw yellow border if hovered, white otherwise
        if (is_hovered) {
            a_DrawRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                      (aColor_t){255, 255, 100, 255});  // Yellow border when hovered
        } else {
            Uint8 border_alpha = (Uint8)(alpha * 0.8f);
            a_DrawRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                      (aColor_t){255, 255, 255, border_alpha});  // White border normally
        }

        // Draw icon (2-letter abbreviation)
        const char* icon = GetAbilityAbbreviation(ability->ability_id);
        a_DrawText((char*)icon,
                  current_x + ABILITY_CARD_WIDTH / 2,
                  current_y + 18,
                  (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,255,alpha}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.2f, .padding=0});

        // Draw divider line (using filled rect as a thin line)
        a_DrawFilledRect((aRectf_t){current_x + 10, current_y + 48, ABILITY_CARD_WIDTH - 20, 1},
                        (aColor_t){255, 255, 255, (Uint8)(alpha * 0.5f)});

        // Draw badge (universal for all ability types)
        char badge_text[16];
        if (GetAbilityBadgeText(ability, badge_text)) {
            aColor_t badge_color = GetAbilityBadgeColor(ability, alpha);

            // Determine if badge is numeric (use larger font) or text (use smaller font)
            bool is_numeric = (ability->trigger == TRIGGER_COUNTER);

            // Calculate badge dimensions
            int badge_width = is_numeric ? 44 : 48;
            int badge_height = is_numeric ? 36 : 18;
            int badge_x = current_x + (ABILITY_CARD_WIDTH - badge_width) / 2;
            int badge_y = is_numeric ? current_y + 56 : current_y + 60;

            // Draw badge background (dark with transparency, like modal)
            a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_width, badge_height},
                            (aColor_t){20, 20, 30, (Uint8)(230 * (alpha / 255.0f))});

            // Draw badge border (white, like modal)
            a_DrawRect((aRectf_t){badge_x, badge_y, badge_width, badge_height},
                      (aColor_t){255, 255, 255, alpha});

            // Text Y position: move numeric text up for better centering
            int text_y_offset = is_numeric ? -6 : 2;

            a_DrawText(badge_text,
                      current_x + ABILITY_CARD_WIDTH / 2,
                      badge_y + text_y_offset,
                      (aTextStyle_t){.type=(is_numeric ? FONT_ENTER_COMMAND : FONT_GAME), .fg={badge_color.r,badge_color.g,badge_color.b,badge_color.a}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=(is_numeric ? 1.2f : 0.9f), .padding=0});
        }

        // Draw red flash overlay if ability is animating
        if (ability->flash_alpha > 0.0f) {
            a_DrawFilledRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                            (aColor_t){255, 0, 0, (Uint8)ability->flash_alpha});
        }

        // Move to next card (VERTICAL stack) - use base_y for consistent spacing
        base_y += ABILITY_CARD_HEIGHT + ABILITY_CARD_SPACING;
    }
}

// ============================================================================
// GETTERS
// ============================================================================

const AbilityData_t* GetHoveredAbilityData(const AbilityDisplay_t* display) {
    if (!display || !display->enemy || display->hovered_index < 0) return NULL;

    Enemy_t* enemy = display->enemy;
    if (!enemy->active_abilities || (size_t)display->hovered_index >= enemy->active_abilities->count) {
        return NULL;
    }

    return (const AbilityData_t*)d_IndexDataFromArray(enemy->active_abilities, (size_t)display->hovered_index);
}

bool GetHoveredAbilityPosition(const AbilityDisplay_t* display, int* out_x, int* out_y) {
    if (!display || display->hovered_index < 0 || !out_x || !out_y) return false;

    // Calculate position of hovered card
    *out_x = display->x;
    *out_y = display->y + (display->hovered_index * (ABILITY_CARD_HEIGHT + ABILITY_CARD_SPACING));
    return true;
}
