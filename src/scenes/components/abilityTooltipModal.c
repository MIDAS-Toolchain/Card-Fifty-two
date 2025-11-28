/*
 * Ability Tooltip Modal - Updated for data-driven Ability_t system
 */

#include "../../../include/scenes/components/abilityTooltipModal.h"
#include "../../../include/scenes/components/abilityDisplay.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include <stdlib.h>

// Forward declarations
static void GetTriggerDescription(const Ability_t* ability, const Enemy_t* enemy, char* buffer, size_t buffer_size);
static dString_t* GetEffectDescription(const Ability_t* ability);
static void GetStateDescription(const Ability_t* ability, char* buffer, size_t buffer_size);

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
    modal->enemy = NULL;

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
                             const Ability_t* ability,
                             const Enemy_t* enemy,
                             int card_x, int card_y) {
    if (!modal || !ability) return;

    modal->visible = true;
    modal->ability = ability;
    modal->enemy = enemy;

    // Position to right of card
    modal->x = card_x + ABILITY_CARD_WIDTH + 10;
    modal->y = card_y;

    // Flip to left if too close to screen edge
    if (modal->x + ABILITY_TOOLTIP_WIDTH > SCREEN_WIDTH - 10) {
        modal->x = card_x - ABILITY_TOOLTIP_WIDTH - 10;
    }

    // Clamp Y to screen bounds (conservative estimate - will be corrected during render)
    int estimated_height = ABILITY_TOOLTIP_MIN_HEIGHT;
    if (modal->y + estimated_height > SCREEN_HEIGHT - 10) {
        modal->y = SCREEN_HEIGHT - estimated_height - 10;
    }
    if (modal->y < TOP_BAR_HEIGHT + 10) {
        modal->y = TOP_BAR_HEIGHT + 10;
    }
}

void HideAbilityTooltipModal(AbilityTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
    modal->ability = NULL;
    modal->enemy = NULL;
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * GetTriggerDescription - Describe how ability triggers
 */
static void GetTriggerDescription(const Ability_t* ability, const Enemy_t* enemy, char* buffer, size_t buffer_size) {
    if (!ability || !buffer) return;

    switch (ability->trigger.type) {
        case TRIGGER_PASSIVE:
            snprintf(buffer, buffer_size, "Passive (always active)");
            break;

        case TRIGGER_ON_EVENT:
            snprintf(buffer, buffer_size, "Triggers on: %s", GameEventToString(ability->trigger.event));
            break;

        case TRIGGER_COUNTER:
            snprintf(buffer, buffer_size, "Every %d times: %s",
                    ability->trigger.counter_max, GameEventToString(ability->trigger.event));
            break;

        case TRIGGER_HP_THRESHOLD:
            if (enemy && enemy->max_hp > 0) {
                int threshold_hp = (int)(ability->trigger.threshold * enemy->max_hp);
                snprintf(buffer, buffer_size, "When enemy HP drops below %d HP (%.0f%%) ",
                        threshold_hp, ability->trigger.threshold * 100.0f);
            } else {
                snprintf(buffer, buffer_size, "When enemy HP drops below %.0f%% ",
                        ability->trigger.threshold * 100.0f);
            }
            break;

        case TRIGGER_RANDOM:
            snprintf(buffer, buffer_size, "%.0f%% chance on: %s",
                    ability->trigger.chance * 100.0f, GameEventToString(ability->trigger.event));
            break;

        case TRIGGER_ON_ACTION:
            snprintf(buffer, buffer_size, "When player uses: %s",
                    PlayerActionToString(ability->trigger.action));
            break;

        case TRIGGER_HP_SEGMENT:
            snprintf(buffer, buffer_size, "Every %d%% HP lost",
                    ability->trigger.segment_percent);
            break;

        case TRIGGER_DAMAGE_ACCUMULATOR: {
            int threshold = ability->trigger.damage_threshold;
            int accumulated = ability->trigger.damage_accumulated;
            int enemy_total = enemy ? enemy->total_damage_taken : 0;

            // Calculate progress toward next trigger
            int damage_since_last = enemy_total - accumulated;
            int remaining = threshold - damage_since_last;

            if (remaining < 0) remaining = 0;  // Already triggered, waiting for next check

            snprintf(buffer, buffer_size, "Every %d total damage dealt (%d/%d)",
                    threshold, damage_since_last, threshold);
            break;
        }

        default:
            snprintf(buffer, buffer_size, "Unknown trigger");
            break;
    }
}

/**
 * GetEffectDescription - Describe what ability does
 * Returns dString_t* that caller must destroy
 */
static dString_t* GetEffectDescription(const Ability_t* ability) {
    dString_t* desc = d_StringInit();
    if (!ability || !ability->effects) return desc;

    for (size_t i = 0; i < ability->effects->count; i++) {
        AbilityEffect_t* effect = (AbilityEffect_t*)d_ArrayGet(ability->effects, i);
        if (!effect) continue;

        if (d_StringGetLength(desc) > 0) {
            d_StringAppend(desc, "\n- ", 3);
        } else {
            d_StringAppend(desc, "- ", 2);
        }

        dString_t* temp = d_StringInit();
        switch (effect->type) {
            case EFFECT_APPLY_STATUS: {
                const char* status_name = GetStatusEffectName(effect->status);

                // Human-readable status descriptions
                if (effect->status == STATUS_GREED) {
                    // Greed is flat - doesn't use value field
                    d_StringFormat(temp, "Applies %s - Win only 50%% chips for %d rounds ",
                                 status_name, effect->duration);
                } else if (effect->status == STATUS_CHIP_DRAIN) {
                    if (effect->value == 0) {
                        d_StringFormat(temp, "Applies %s for %d rounds ", status_name, effect->duration);
                    } else {
                        d_StringFormat(temp, "Applies %s - Lose %d chips per round for %d rounds ",
                                     status_name, effect->value, effect->duration);
                    }
                } else if (effect->status == STATUS_RAKE) {
                    // RAKE: Stack-based (duration = stacks), value = percentage
                    d_StringFormat(temp, "Applies %s - Lose %d%% of damage as chips (%d stack%s) ",
                                 status_name, effect->value, effect->duration,
                                 effect->duration == 1 ? "" : "s");
                } else {
                    // Generic fallback for other status effects
                    if (effect->value == 0) {
                        d_StringFormat(temp, "Applies %s for %d rounds ", status_name, effect->duration);
                    } else {
                        d_StringFormat(temp, "Applies %s (value: %d) for %d rounds ",
                                     status_name, effect->value, effect->duration);
                    }
                }
                d_StringAppend(desc, d_StringPeek(temp), d_StringGetLength(temp));
                break;
            }

            case EFFECT_REMOVE_STATUS:
                d_StringFormat(temp, "Removes %s", GetStatusEffectName(effect->status));
                d_StringAppend(desc, d_StringPeek(temp), d_StringGetLength(temp));
                break;

            case EFFECT_HEAL:
                if (effect->target == TARGET_SELF) {
                    d_StringFormat(temp, "Heals self for %d HP", effect->value);
                } else {
                    d_StringFormat(temp, "Heals player for %d chips", effect->value);
                }
                d_StringAppend(desc, d_StringPeek(temp), d_StringGetLength(temp));
                break;

            case EFFECT_DAMAGE:
                if (effect->target == TARGET_SELF) {
                    d_StringFormat(temp, "Damages self for %d HP", effect->value);
                } else {
                    d_StringFormat(temp, "Player loses %d chips", effect->value);
                }
                d_StringAppend(desc, d_StringPeek(temp), d_StringGetLength(temp));
                break;

            case EFFECT_SHUFFLE_DECK:
                d_StringAppend(desc, "Forces deck reshuffle", 22);
                break;

            case EFFECT_DISCARD_HAND:
                d_StringAppend(desc, "Discards player's hand", 23);
                break;

            case EFFECT_FORCE_HIT:
                d_StringAppend(desc, "Forces player to draw", 22);
                break;

            case EFFECT_REVEAL_HOLE:
                d_StringAppend(desc, "Reveals dealer's hole card", 27);
                break;

            case EFFECT_MESSAGE:
                if (effect->message) {
                    d_StringAppend(desc, d_StringPeek(effect->message), d_StringGetLength(effect->message));
                }
                break;

            default:
                d_StringAppend(desc, "Unknown effect", 14);
                break;
        }
        d_StringDestroy(temp);
    }

    return desc;
}

/**
 * GetStateDescription - Describe current state
 */
static void GetStateDescription(const Ability_t* ability, char* buffer, size_t buffer_size) {
    if (!ability || !buffer) return;

    if (ability->trigger.type == TRIGGER_COUNTER) {
        int remaining = ability->trigger.counter_max - ability->counter_current;
        snprintf(buffer, buffer_size, "Progress: %d/%d (triggers in %d)",
                ability->counter_current, ability->trigger.counter_max, remaining);
    } else if (ability->trigger.type == TRIGGER_HP_THRESHOLD) {
        if (ability->has_triggered) {
            snprintf(buffer, buffer_size, "Status: Already used");
        } else {
            snprintf(buffer, buffer_size, "Status: Ready");
        }
    } else {
        snprintf(buffer, buffer_size, "Status: Active");
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderAbilityTooltipModal(const AbilityTooltipModal_t* modal) {
    if (!modal || !modal->visible || !modal->ability) return;

    const Ability_t* ability = modal->ability;
    int padding = 15;
    int wrap_width = ABILITY_TOOLTIP_WIDTH - (padding * 2);

    // Pre-calculate description strings
    char trigger_desc[256];
    char state_desc[128];
    GetTriggerDescription(ability, modal->enemy, trigger_desc, sizeof(trigger_desc));
    GetStateDescription(ability, state_desc, sizeof(state_desc));

    // Get effect description as dString (no truncation!)
    dString_t* effect_desc = GetEffectDescription(ability);

    // Measure text heights using Archimedes
    // NOTE: When text is scaled, wrap_width must be adjusted (divide by scale)
    int title_height = a_GetWrappedTextHeight((char*)d_StringPeek(ability->name), FONT_ENTER_COMMAND, wrap_width);

    int desc_height = 0;
    if (ability->description && d_StringGetLength(ability->description) > 0) {
        desc_height = a_GetWrappedTextHeight((char*)d_StringPeek(ability->description), FONT_GAME, wrap_width);
    }

    // These are rendered at scale 1.1, so measure with adjusted wrap width
    int scaled_wrap_width = (int)(wrap_width / 1.1f);
    int trigger_height = a_GetWrappedTextHeight(trigger_desc, FONT_GAME, scaled_wrap_width);
    int effects_height = a_GetWrappedTextHeight((char*)d_StringPeek(effect_desc), FONT_GAME, scaled_wrap_width);
    int state_height = a_GetWrappedTextHeight(state_desc, FONT_GAME, scaled_wrap_width);

    // Calculate modal height using current_y tracker pattern (matches old working version)
    int current_y = padding;
    current_y += title_height + 10;  // Title + margin
    if (desc_height > 0) {
        current_y += desc_height + 8;   // Description + spacing
    }
    current_y += 1 + 12;             // Divider + spacing
    current_y += trigger_height + 4; // Trigger + spacing
    current_y += state_height + 4;   // State + spacing
    current_y += 8;                  // Spacing before effect
    current_y += effects_height + padding;  // Effect + bottom padding

    int modal_height = current_y;

    // Draw background (dark semi-transparent)
    a_DrawFilledRect((aRectf_t){modal->x, modal->y, ABILITY_TOOLTIP_WIDTH, modal_height},
                    (aColor_t){20, 20, 25, 240});

    // Draw border
    a_DrawRect((aRectf_t){modal->x, modal->y, ABILITY_TOOLTIP_WIDTH, modal_height},
              (aColor_t){222, 158, 65, 255});

    // Render content (track position as we draw)
    int text_x = modal->x + padding;
    int text_y = modal->y + padding;

    // Draw ability name (title) - gold
    a_DrawText((char*)d_StringPeek(ability->name),
              text_x, text_y,
              (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={232,193,112,255}, .bg={0,0,0,0},
                            .align=TEXT_ALIGN_LEFT, .wrap_width=wrap_width, .scale=1.0f, .padding=0});
    text_y += title_height + 10;

    // Draw description (flavor text) - gray
    if (desc_height > 0) {
        a_DrawText((char*)d_StringPeek(ability->description),
                  text_x, text_y,
                  (aTextStyle_t){.type=FONT_GAME, .fg={180,180,180,255}, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_LEFT, .wrap_width=wrap_width, .scale=1.0f, .padding=0});
        text_y += desc_height + 8;
    }

    // Draw divider
    a_DrawFilledRect((aRectf_t){text_x, text_y, wrap_width, 1},
                    (aColor_t){100, 100, 100, 200});
    text_y += 12;

    // Draw trigger description - yellow-green
    a_DrawText(trigger_desc,
              text_x, text_y,
              (aTextStyle_t){.type=FONT_GAME, .fg={168,202,88,255}, .bg={0,0,0,0},
                            .align=TEXT_ALIGN_LEFT, .wrap_width=wrap_width, .scale=1.1f, .padding=0});
    text_y += trigger_height + 4;

    // Draw current state - orange
    a_DrawText(state_desc,
              text_x, text_y,
              (aTextStyle_t){.type=FONT_GAME, .fg={222,158,65,255}, .bg={0,0,0,0},
                            .align=TEXT_ALIGN_LEFT, .wrap_width=wrap_width, .scale=1.1f, .padding=0});
    text_y += state_height + 4;

    text_y += 8;  // Spacing before effect

    // Draw effect description - red-orange
    a_DrawText((char*)d_StringPeek(effect_desc),
              text_x, text_y,
              (aTextStyle_t){.type=FONT_GAME, .fg={207,87,60,255}, .bg={0,0,0,0},
                            .align=TEXT_ALIGN_LEFT, .wrap_width=wrap_width, .scale=1.1f, .padding=0});

    // Cleanup
    d_StringDestroy(effect_desc);
}
