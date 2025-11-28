/*
 * Ability Display Component - Updated for data-driven Ability_t system
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
    display->hovered_index = -1;

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
 * GetAbilityColor - Get color for ability card based on trigger type
 */
static aColor_t GetAbilityColor(TriggerType_t trigger_type) {
    switch (trigger_type) {
        case TRIGGER_ON_EVENT:      return (aColor_t){222, 158, 65, 255};  // Warm yellow-orange
        case TRIGGER_COUNTER:       return (aColor_t){122, 62, 123, 255};  // Deep purple
        case TRIGGER_HP_THRESHOLD:  return (aColor_t){207, 87, 60, 255};   // Burnt orange-red
        case TRIGGER_RANDOM:        return (aColor_t){91, 110, 225, 255};  // Blue
        case TRIGGER_ON_ACTION:     return (aColor_t){76, 175, 80, 255};   // Green
        case TRIGGER_PASSIVE:
        default:                    return (aColor_t){100, 100, 100, 255}; // Gray
    }
}

/**
 * GetAbilityAbbreviation - Get 2-letter icon from ability name
 */
static void GetAbilityAbbreviation(const char* name, char* out_buffer, size_t buffer_size) {
    if (!name || !out_buffer || buffer_size < 3) {
        if (out_buffer && buffer_size >= 3) {
            out_buffer[0] = '?';
            out_buffer[1] = '?';
            out_buffer[2] = '\0';
        }
        return;
    }

    // Extract first letters of first two words
    size_t len = strlen(name);
    int char_count = 0;
    bool new_word = true;

    for (size_t i = 0; i < len && char_count < 2; i++) {
        if (name[i] == ' ' || name[i] == '-') {
            new_word = true;
        } else if (new_word && name[i] != ' ') {
            out_buffer[char_count++] = name[i];
            new_word = false;
        }
    }

    // Fallback if we didn't get 2 chars
    if (char_count < 2) {
        out_buffer[0] = name[0];
        out_buffer[1] = (len > 1) ? name[1] : '?';
    }

    out_buffer[2] = '\0';
}

/**
 * GetAbilityBadgeText - Get badge text for ability state
 */
static bool GetAbilityBadgeText(const Ability_t* ability, char* out_buffer, size_t buffer_size) {
    if (!ability || !out_buffer) return false;

    switch (ability->trigger.type) {
        case TRIGGER_COUNTER: {
            int remaining = ability->trigger.counter_max - ability->counter_current;
            snprintf(out_buffer, buffer_size, "%d", remaining);
            return true;
        }

        case TRIGGER_HP_THRESHOLD:
            if (ability->has_triggered) {
                snprintf(out_buffer, buffer_size, "USED");
            } else {
                snprintf(out_buffer, buffer_size, "READY");
            }
            return true;

        case TRIGGER_ON_EVENT:
            snprintf(out_buffer, buffer_size, "READY");
            return true;

        case TRIGGER_RANDOM:
            snprintf(out_buffer, buffer_size, "%.0f%%", ability->trigger.chance * 100.0f);
            return true;

        default:
            return false;
    }
}

/**
 * GetAbilityBadgeColor - Get badge color based on state
 */
static aColor_t GetAbilityBadgeColor(const Ability_t* ability, Uint8 alpha) {
    if (!ability) return (aColor_t){255, 255, 255, alpha};

    if (ability->has_triggered && ability->trigger.type == TRIGGER_HP_THRESHOLD) {
        return (aColor_t){165, 48, 48, alpha};  // Red for used
    }

    return (aColor_t){222, 158, 65, alpha};  // Yellow-orange for ready
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderAbilityDisplay(AbilityDisplay_t* display) {
    if (!display || !display->enemy) return;

    Enemy_t* enemy = display->enemy;
    if (!enemy->abilities || enemy->abilities->count == 0) return;

    int mouse_x = app.mouse.x;
    int mouse_y = app.mouse.y;
    display->hovered_index = -1;

    int base_x = display->x;
    int base_y = display->y;

    // Render each ability (VERTICAL layout)
    for (size_t i = 0; i < enemy->abilities->count; i++) {
        Ability_t** ability_ptr = (Ability_t**)d_ArrayGet(enemy->abilities, i);
        if (!ability_ptr || !*ability_ptr) continue;

        Ability_t* ability = *ability_ptr;

        // Apply shake offsets if animating
        int current_x = base_x + (int)ability->shake_offset_x;
        int current_y = base_y + (int)ability->shake_offset_y;

        // Check hover
        bool is_hovered = (mouse_x >= base_x && mouse_x < base_x + ABILITY_CARD_WIDTH &&
                          mouse_y >= base_y && mouse_y < base_y + ABILITY_CARD_HEIGHT);
        if (is_hovered) {
            display->hovered_index = (int)i;
        }

        // Determine opacity
        Uint8 alpha = 255;
        if (ability->has_triggered && ability->trigger.type == TRIGGER_HP_THRESHOLD) {
            alpha = 120;  // Dimmed if already used
        }

        // Draw card background (color by trigger type)
        aColor_t bg_color = GetAbilityColor(ability->trigger.type);
        bg_color.a = alpha;
        a_DrawFilledRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                        bg_color);

        // Draw border
        if (is_hovered) {
            a_DrawRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                      (aColor_t){255, 255, 100, 255});
        } else {
            Uint8 border_alpha = (Uint8)(alpha * 0.8f);
            a_DrawRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                      (aColor_t){255, 255, 255, border_alpha});
        }

        // Draw icon (2-letter abbreviation from name)
        char icon[3];
        GetAbilityAbbreviation(d_StringPeek(ability->name), icon, sizeof(icon));
        a_DrawText(icon,
                  current_x + ABILITY_CARD_WIDTH / 2,
                  current_y + 18,
                  (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,255,alpha}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.2f, .padding=0});

        // Draw divider line
        a_DrawFilledRect((aRectf_t){current_x + 10, current_y + 48, ABILITY_CARD_WIDTH - 20, 1},
                        (aColor_t){255, 255, 255, (Uint8)(alpha * 0.5f)});

        // Draw badge (state indicator) - matches old working version
        char badge_text[16];
        if (GetAbilityBadgeText(ability, badge_text, sizeof(badge_text))) {
            aColor_t badge_color = GetAbilityBadgeColor(ability, alpha);
            bool is_numeric = (ability->trigger.type == TRIGGER_COUNTER);

            // Calculate badge dimensions
            int badge_width = is_numeric ? 44 : 48;
            int badge_height = is_numeric ? 36 : 18;
            int badge_x = current_x + (ABILITY_CARD_WIDTH - badge_width) / 2;
            int badge_y = is_numeric ? current_y + 56 : current_y + 60;

            // Draw badge background (dark with transparency)
            a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_width, badge_height},
                            (aColor_t){20, 20, 30, (Uint8)(230 * (alpha / 255.0f))});

            // Draw badge border (white)
            a_DrawRect((aRectf_t){badge_x, badge_y, badge_width, badge_height},
                      (aColor_t){255, 255, 255, alpha});

            // Text Y position: move numeric text up for better centering
            int text_y_offset = is_numeric ? -6 : 2;

            a_DrawText(badge_text,
                      current_x + ABILITY_CARD_WIDTH / 2,
                      badge_y + text_y_offset,
                      (aTextStyle_t){
                          .type = (is_numeric ? FONT_ENTER_COMMAND : FONT_GAME),
                          .fg = {badge_color.r, badge_color.g, badge_color.b, badge_color.a},
                          .bg = {0, 0, 0, 0},
                          .align = TEXT_ALIGN_CENTER,
                          .wrap_width = 0,
                          .scale = (is_numeric ? 1.2f : 0.9f),
                          .padding = 0
                      });
        }

        // Draw red flash overlay if animating
        if (ability->flash_alpha > 0.0f) {
            a_DrawFilledRect((aRectf_t){current_x, current_y, ABILITY_CARD_WIDTH, ABILITY_CARD_HEIGHT},
                            (aColor_t){255, 0, 0, (Uint8)ability->flash_alpha});
        }

        // Move to next card position (vertical stack)
        base_y += ABILITY_CARD_HEIGHT + ABILITY_CARD_SPACING;
    }
}

// ============================================================================
// QUERIES
// ============================================================================

const Ability_t* GetHoveredAbilityData(const AbilityDisplay_t* display) {
    if (!display || !display->enemy) return NULL;

    Enemy_t* enemy = display->enemy;
    if (!enemy->abilities || (size_t)display->hovered_index >= enemy->abilities->count) {
        return NULL;
    }

    Ability_t** ability_ptr = (Ability_t**)d_ArrayGet(enemy->abilities, (size_t)display->hovered_index);
    return (ability_ptr && *ability_ptr) ? *ability_ptr : NULL;
}

bool GetHoveredAbilityPosition(const AbilityDisplay_t* display, int* out_x, int* out_y) {
    if (!display || display->hovered_index < 0) {
        if (out_x) *out_x = 0;
        if (out_y) *out_y = 0;
        return false;
    }

    // Calculate position of hovered card
    int card_y = display->y + (display->hovered_index * (ABILITY_CARD_HEIGHT + ABILITY_CARD_SPACING));

    if (out_x) *out_x = display->x;
    if (out_y) *out_y = card_y;
    return true;
}
