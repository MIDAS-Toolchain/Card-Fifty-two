/*
 * Trinket UI Component Implementation
 */

#include "../../../include/scenes/components/trinketUI.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/trinket.h"
#include "../../../include/state.h"
#include "../../../include/stateStorage.h"
#include "../../../include/tutorial/tutorialSystem.h"
#include "../../../include/loaders/trinketLoader.h"
#include "../../../include/loaders/affixLoader.h"

// External references
extern TutorialSystem_t* g_tutorial_system;

// ============================================================================
// LIFECYCLE
// ============================================================================

TrinketUI_t* CreateTrinketUI(void) {
    TrinketUI_t* ui = malloc(sizeof(TrinketUI_t));
    if (!ui) {
        d_LogError("Failed to allocate TrinketUI_t");
        return NULL;
    }

    ui->hovered_trinket_slot = -1;
    ui->hovered_class_trinket = false;

    // Initialize affix template cache
    for (int i = 0; i < 3; i++) {
        ui->cached_affix_templates[i] = NULL;
    }
    ui->cached_affix_count = 0;
    ui->cached_slot_index = -2;  // -2 = no cache

    return ui;
}

void DestroyTrinketUI(TrinketUI_t** ui) {
    if (!ui || !*ui) return;

    // Cleanup cached affix templates
    for (int i = 0; i < 3; i++) {
        if ((*ui)->cached_affix_templates[i]) {
            CleanupAffixTemplate((*ui)->cached_affix_templates[i]);
            free((*ui)->cached_affix_templates[i]);
            (*ui)->cached_affix_templates[i] = NULL;
        }
    }

    free(*ui);
    *ui = NULL;
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void HandleTrinketUIInput(TrinketUI_t* ui, Player_t* player, GameContext_t* game) {
    if (!ui || !player || !game) return;
    // Note: trinket_slots is a fixed-size array embedded in Player_t, not a pointer

    // Only allow trinket use during player turn
    if (game->current_state != STATE_PLAYER_TURN) return;

    // Check for mouse click (using pressed flag like buttons)
    if (app.mouse.pressed) {
        // Check if clicked on class trinket
        if (ui->hovered_class_trinket) {
            Trinket_t* trinket = GetClassTrinket(player);

            if (trinket && trinket->active_cooldown_current == 0 && trinket->active_target_type == TRINKET_TARGET_CARD) {
                // Trigger tutorial event (class trinket clicked)
                if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
                    TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_FUNCTION_CALL, (void*)0x1000);
                }

                // Enter targeting mode (use slot_index = -1 for class trinket)
                StateData_SetInt(&game->state_data, "targeting_trinket_slot", -1);  // -1 = class trinket
                StateData_SetInt(&game->state_data, "targeting_player_id", player->player_id);
                State_Transition(game, STATE_TARGETING);

                d_LogInfoF("Entered targeting mode for CLASS trinket (player ID %d)",
                          player->player_id);
            }
        }
        // Check if clicked on regular trinket slot
        else if (ui->hovered_trinket_slot >= 0) {
            // Defensive: Verify player exists
            if (!player) {
                d_LogError("HandleTrinketUIInput: player is NULL");
                return;
            }

            // DUF trinkets are passive-only (no active abilities, no targeting)
            // Class trinkets (hardcoded) handle their own targeting logic elsewhere
            // So we don't need to check for trinket clicks here
            (void)ui;  // Suppress unused warning
        }
    }
}

// ============================================================================
// RENDERING - TOOLTIPS
// ============================================================================

// Legacy version for hardcoded class trinkets (Trinket_t)
static void RenderClassTrinketTooltip(const Trinket_t* trinket, int slot_index) {
    if (!trinket) return;

    // Determine if this is a class trinket (slot_index == -1)
    bool is_class_trinket = (slot_index == -1);

    // Calculate tooltip position
    int slot_x, slot_y, slot_size;
    if (is_class_trinket) {
        slot_x = GetClassTrinketX();
        slot_y = GetClassTrinketY();
        slot_size = CLASS_TRINKET_SIZE;
    } else {
        int row = slot_index / 3;
        int col = slot_index % 3;
        slot_x = GetTrinketUIX() + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        slot_y = GetTrinketUIY() + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        slot_size = TRINKET_SLOT_SIZE;
    }

    int tooltip_width = 340;
    int tooltip_x = slot_x - tooltip_width - 10;  // Left of slot
    int tooltip_y = slot_y;

    // If too far left, show on right instead
    if (tooltip_x < 10) {
        tooltip_x = slot_x + slot_size + 10;
    }

    // Get trinket info
    const char* name = GetTrinketName(trinket);
    const char* description = GetTrinketDescription(trinket);
    const char* passive_desc = trinket->passive_description ? d_StringPeek(trinket->passive_description) : "No passive";
    const char* active_desc = trinket->active_description ? d_StringPeek(trinket->active_description) : "No active";

    // Get rarity color for border
    int rarity_r, rarity_g, rarity_b;
    GetTrinketRarityColor(trinket->rarity, &rarity_r, &rarity_g, &rarity_b);

    int padding = 16;
    int content_width = tooltip_width - (padding * 2);

    // Measure text heights
    int name_height = a_GetWrappedTextHeight((char*)name, FONT_ENTER_COMMAND, content_width);
    int desc_height = a_GetWrappedTextHeight((char*)description, FONT_GAME, content_width);
    int passive_height = a_GetWrappedTextHeight((char*)passive_desc, FONT_GAME, content_width);
    int active_height = a_GetWrappedTextHeight((char*)active_desc, FONT_GAME, content_width);

    // Calculate tooltip height
    int tooltip_height = padding;

    // Add space for "CLASS TRINKET" header if applicable
    if (is_class_trinket) {
        tooltip_height += 20;  // CLASS TRINKET header
    }

    tooltip_height += name_height + 8;
    tooltip_height += desc_height + 12;  // No rarity text
    tooltip_height += 1 + 12;  // Divider
    tooltip_height += passive_height + 8;
    tooltip_height += active_height + 8;

    // Show cooldown/status
    if (trinket->active_cooldown_current > 0) {
        tooltip_height += 20;
    }

    // Show damage counter (if any)
    if (trinket->total_damage_dealt > 0) {
        tooltip_height += 20;
    }

    // Show bonus chips (if any - for Elite Membership)
    if (trinket->total_bonus_chips > 0) {
        tooltip_height += 20;
    }

    // Show refunded chips (if any - for Elite Membership)
    if (trinket->total_refunded_chips > 0) {
        tooltip_height += 20;
    }

    tooltip_height += padding;

    // Clamp to screen
    if (tooltip_y + tooltip_height > SCREEN_HEIGHT - 10) {
        tooltip_y = SCREEN_HEIGHT - tooltip_height - 10;
    }
    if (tooltip_y < TOP_BAR_HEIGHT + 10) {
        tooltip_y = TOP_BAR_HEIGHT + 10;
    }

    // Background (dark panel)
    a_DrawFilledRect((aRectf_t){tooltip_x, tooltip_y, tooltip_width, tooltip_height},
                    (aColor_t){9, 10, 20, 240});  // COLOR_PANEL_BG

    // Rarity-colored double border (matches trinket slot and modal)
    a_DrawRect((aRectf_t){tooltip_x, tooltip_y, tooltip_width, tooltip_height},
              (aColor_t){rarity_r, rarity_g, rarity_b, 255});
    a_DrawRect((aRectf_t){tooltip_x + 1, tooltip_y + 1, tooltip_width - 2, tooltip_height - 2},
              (aColor_t){rarity_r, rarity_g, rarity_b, 255});

    // Render content
    int content_x = tooltip_x + padding;
    int current_y = tooltip_y + padding;

    // Class trinket header (if applicable)
    if (is_class_trinket) {
        aTextStyle_t class_header_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {200, 180, 50, 255},  // Gold
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 0.6f
        };
        a_DrawText("CLASS TRINKET", content_x, current_y, class_header_config);
        current_y += 20;
    }

    // Title
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {232, 193, 112, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)name, content_x, current_y, title_config);
    current_y += name_height + 8;

    // Rarity shown by border color, no text needed

    // Description
    aTextStyle_t desc_config = {
        .type = FONT_GAME,
        .fg = {180, 180, 180, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)description, content_x, current_y, desc_config);
    current_y += desc_height + 12;

    // Divider
    a_DrawFilledRect((aRectf_t){content_x, current_y, content_width, 1}, (aColor_t){100, 100, 100, 200});
    current_y += 12;

    // Passive
    aTextStyle_t passive_config = {
        .type = FONT_GAME,
        .fg = {168, 202, 88, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)passive_desc, content_x, current_y, passive_config);
    current_y += passive_height + 8;

    // Active
    aTextStyle_t active_config = {
        .type = FONT_GAME,
        .fg = {207, 87, 60, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)active_desc, content_x, current_y, active_config);
    current_y += active_height + 8;

    // Cooldown status
    if (trinket->active_cooldown_current > 0) {
        dString_t* cd_text = d_StringInit();
        d_StringFormat(cd_text, "Cooldown: %d turns remaining", trinket->active_cooldown_current);

        aTextStyle_t cd_config = {
            .type = FONT_GAME,
            .fg = {255, 100, 100, 255},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(cd_text), content_x, current_y, cd_config);
        d_StringDestroy(cd_text);
        current_y += 20;
    }

    // Total damage dealt (if any)
    if (trinket->total_damage_dealt > 0) {
        dString_t* dmg_text = d_StringInit();
        d_StringFormat(dmg_text, "Total Damage Dealt: %d", trinket->total_damage_dealt);

        aTextStyle_t dmg_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(dmg_text), content_x, current_y, dmg_config);
        d_StringDestroy(dmg_text);
        current_y += 20;
    }

    // Bonus chips won (if any - for Elite Membership)
    if (trinket->total_bonus_chips > 0) {
        dString_t* bonus_text = d_StringInit();
        d_StringFormat(bonus_text, "Bonus Chips Won: %d", trinket->total_bonus_chips);

        aTextStyle_t bonus_config = {
            .type = FONT_GAME,
            .fg = {100, 255, 100, 255},  // Green text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(bonus_text), content_x, current_y, bonus_config);
        d_StringDestroy(bonus_text);
        current_y += 20;
    }

    // Chips refunded (if any - for Elite Membership)
    if (trinket->total_refunded_chips > 0) {
        dString_t* refund_text = d_StringInit();
        d_StringFormat(refund_text, "Chips Refunded: %d", trinket->total_refunded_chips);

        aTextStyle_t refund_config = {
            .type = FONT_GAME,
            .fg = {100, 200, 255, 255},  // Blue text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(refund_text), content_x, current_y, refund_config);
        d_StringDestroy(refund_text);
        current_y += 20;
    }
}

/**
 * FormatTrinketPassive - Format passive description for display
 *
 * @param template - Trinket template
 * @param instance - Trinket instance (for stack tracking)
 * @param secondary - true for secondary passive (trigger_2), false for primary
 * @return dString_t* - Formatted passive string (caller must destroy)
 */
static dString_t* FormatTrinketPassive(const TrinketTemplate_t* template, const TrinketInstance_t* instance, bool secondary) {
    dString_t* passive_text = d_StringInit();
    if (!passive_text) return NULL;

    // Select which trigger/effect to format
    GameEvent_t trigger = secondary ? template->passive_trigger_2 : template->passive_trigger;
    TrinketEffectType_t effect_type = secondary ? template->passive_effect_type_2 : template->passive_effect_type;
    int effect_value = secondary ? template->passive_effect_value_2 : template->passive_effect_value;

    // Skip if no trigger set
    if (trigger == 0 && effect_type == TRINKET_EFFECT_NONE) {
        d_StringSet(passive_text, "");
        return passive_text;
    }

    // 1. Format trigger ("On Win:", "On Blackjack:", etc.)
    // Cast to int to allow non-enum value 999 (special ON_EQUIP trigger)
    const char* trigger_str = NULL;
    switch ((int)trigger) {
        case GAME_EVENT_COMBAT_START:     trigger_str = "On Combat Start"; break;
        case GAME_EVENT_PLAYER_WIN:       trigger_str = "On Win"; break;
        case GAME_EVENT_PLAYER_LOSS:      trigger_str = "On Loss"; break;
        case GAME_EVENT_PLAYER_BUST:      trigger_str = "On Bust"; break;
        case GAME_EVENT_PLAYER_BLACKJACK: trigger_str = "On Blackjack"; break;
        case GAME_EVENT_PLAYER_PUSH:      trigger_str = "On Push"; break;
        case GAME_EVENT_CARD_DRAWN:       trigger_str = "On Card Drawn"; break;
        case 999:                         trigger_str = "On Equip"; break; // Special ON_EQUIP
        default:                          trigger_str = "Passive"; break;
    }

    // 2. Format effect ("Gain 5 chips", "Apply 1 LUCKY_STREAK", etc.)
    switch (effect_type) {
        case TRINKET_EFFECT_ADD_CHIPS:
            d_StringFormat(passive_text, "%s: Gain %d chips", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_ADD_CHIPS_PERCENT:
            d_StringFormat(passive_text, "%s: Gain %d%% of bet", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_LOSE_CHIPS:
            d_StringFormat(passive_text, "%s: Lose %d chips", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_REFUND_CHIPS_PERCENT:
            d_StringFormat(passive_text, "%s: Refund %d%% of bet", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_APPLY_STATUS: {
            const char* status_key = secondary ?
                (template->passive_status_key_2 ? d_StringPeek(template->passive_status_key_2) : "???") :
                (template->passive_status_key ? d_StringPeek(template->passive_status_key) : "???");
            int status_stacks = secondary ? template->passive_status_stacks_2 : template->passive_status_stacks;
            d_StringFormat(passive_text, "%s: Gain %d %s", trigger_str, status_stacks, status_key);
            break;
        }

        case TRINKET_EFFECT_CLEAR_STATUS: {
            const char* status_key = secondary ?
                (template->passive_status_key_2 ? d_StringPeek(template->passive_status_key_2) : "???") :
                (template->passive_status_key ? d_StringPeek(template->passive_status_key) : "???");
            d_StringFormat(passive_text, "%s: Clear %s", trigger_str, status_key);
            break;
        }

        case TRINKET_EFFECT_TRINKET_STACK: {
            const char* stat_key = template->passive_stack_stat ? d_StringPeek(template->passive_stack_stat) : "???";

            // Format stat name for readability and detect flat vs percent stats
            const char* stat_name = stat_key;
            bool is_flat_stat = false;

            if (strcmp(stat_key, "crit_chance") == 0) {
                stat_name = "chance to crit";
            } else if (strcmp(stat_key, "damage_bonus_percent") == 0) {
                stat_name = "damage";
            } else if (strcmp(stat_key, "damage_flat") == 0) {
                stat_name = "flat damage";
                is_flat_stat = true;
            }

            // Check for loop mechanic (Broken Watch)
            const char* on_max_behavior = template->passive_stack_on_max ?
                                         d_StringPeek(template->passive_stack_on_max) : NULL;
            bool loops = (on_max_behavior && strcmp(on_max_behavior, "reset_to_one") == 0);

            // Show current stacks / max stacks if instance exists
            if (instance && instance->trinket_stacks > 0) {
                // Show current/max (0 = infinite)
                if (template->passive_stack_max == 0) {
                    if (is_flat_stat) {
                        d_StringFormat(passive_text, "%s: +%d %s (%d stacks)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      instance->trinket_stacks);
                    } else {
                        d_StringFormat(passive_text, "%s: +%d%% %s (%d stacks)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      instance->trinket_stacks);
                    }
                } else if (loops) {
                    if (is_flat_stat) {
                        d_StringFormat(passive_text, "%s: +%d %s (%d/%d, loops)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      instance->trinket_stacks, template->passive_stack_max);
                    } else {
                        d_StringFormat(passive_text, "%s: +%d%% %s (%d/%d, loops)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      instance->trinket_stacks, template->passive_stack_max);
                    }
                } else {
                    if (is_flat_stat) {
                        d_StringFormat(passive_text, "%s: +%d %s (%d/%d stacks)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      instance->trinket_stacks, template->passive_stack_max);
                    } else {
                        d_StringFormat(passive_text, "%s: +%d%% %s (%d/%d stacks)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      instance->trinket_stacks, template->passive_stack_max);
                    }
                }
            } else {
                // Show max stacks (0 = infinite)
                if (template->passive_stack_max == 0) {
                    if (is_flat_stat) {
                        d_StringFormat(passive_text, "%s: +%d %s (no limit)",
                                      trigger_str, template->passive_stack_value, stat_name);
                    } else {
                        d_StringFormat(passive_text, "%s: +%d%% %s (no limit)",
                                      trigger_str, template->passive_stack_value, stat_name);
                    }
                } else if (loops) {
                    if (is_flat_stat) {
                        d_StringFormat(passive_text, "%s: +%d %s (loops at %d)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      template->passive_stack_max);
                    } else {
                        d_StringFormat(passive_text, "%s: +%d%% %s (loops at %d)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      template->passive_stack_max);
                    }
                } else {
                    if (is_flat_stat) {
                        d_StringFormat(passive_text, "%s: +%d %s (max %d stacks)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      template->passive_stack_max);
                    } else {
                        d_StringFormat(passive_text, "%s: +%d%% %s (max %d stacks)",
                                      trigger_str, template->passive_stack_value, stat_name,
                                      template->passive_stack_max);
                    }
                }
            }
            break;
        }

        case TRINKET_EFFECT_TRINKET_STACK_RESET:
            d_StringFormat(passive_text, "%s: Reset stacks to 0", trigger_str);
            break;

        case TRINKET_EFFECT_ADD_DAMAGE_FLAT:
            d_StringFormat(passive_text, "%s: Deal %d damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_DAMAGE_MULTIPLIER:
            d_StringFormat(passive_text, "%s: ×%d%% damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_PUSH_DAMAGE_PERCENT:
            d_StringFormat(passive_text, "%s: Deal %d%% damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_ADD_TAG_TO_CARDS: {
            const char* tag = template->passive_tag ? d_StringPeek(template->passive_tag) : "???";
            d_StringFormat(passive_text, "%s: Add %s tag to %d card%s",
                          trigger_str, tag, template->passive_tag_count,
                          template->passive_tag_count == 1 ? "" : "s");
            break;
        }

        case TRINKET_EFFECT_BUFF_TAG_DAMAGE: {
            const char* tag = template->passive_tag ? d_StringPeek(template->passive_tag) : "???";
            d_StringFormat(passive_text, "%s: %s cards deal +%d damage",
                          trigger_str, tag, template->passive_tag_buff_value);
            break;
        }

        case TRINKET_EFFECT_BLOCK_DEBUFF:
            if (effect_value == 1) {
                d_StringFormat(passive_text, "%s: Block 1 debuff", trigger_str);
            } else {
                d_StringFormat(passive_text, "%s: Block %d debuffs", trigger_str, effect_value);
            }
            break;

        case TRINKET_EFFECT_PUNISH_HEAL:
            if (effect_value == 1) {
                d_StringFormat(passive_text, "%s: Punish 1 enemy heal", trigger_str);
            } else {
                d_StringFormat(passive_text, "%s: Punish %d enemy heals", trigger_str, effect_value);
            }
            break;

        case TRINKET_EFFECT_CHIP_COST_FLAT_DAMAGE:
            d_StringFormat(passive_text, "%s: Lose %d chips, gain +%d flat damage", trigger_str, effect_value, effect_value);
            break;

        case TRINKET_EFFECT_NONE:
            d_StringFormat(passive_text, "%s: (no effect)", trigger_str);
            break;

        default:
            d_LogWarningF("FormatTrinketPassive: Effect type %d not formatted in trinketUI.c", effect_type);
            d_StringFormat(passive_text, "%s: (effect %d not formatted)", trigger_str, effect_type);
            break;
    }

    return passive_text;
}

/**
 * RenderAffixTooltip - Render separate secondary tooltip for affixes
 *
 * Positioned to the left of the main tooltip with intelligent screen-edge detection.
 * Shows affixes in FlexBox 2-column layout with weight/quality color gradients.
 */
static void RenderAffixTooltip(TrinketUI_t* ui, TrinketInstance_t* instance, int slot_index,
                                 int main_tooltip_x, int main_tooltip_y, int main_tooltip_height) {
    if (!ui || !instance || ui->cached_affix_count == 0) return;
    (void)main_tooltip_height;  // Unused
    // Dimensions
    int affix_padding = 16;
    int affix_width = 320;  // Slightly narrower than main tooltip (340)
    int content_width = affix_width - (affix_padding * 2);

    // Calculate height dynamically based on affix count
    int affix_height = affix_padding;
    affix_height += (ui->cached_affix_count * 45);  // Each affix = 2 rows (name 20px + desc 20px + gap 5px)
    affix_height += affix_padding;  // Bottom padding

    // Position to left of main tooltip with 15px gap
    int affix_tooltip_x = main_tooltip_x - affix_width - 15;
    int affix_tooltip_y = main_tooltip_y;  // Align top with main tooltip

    // If too far left (would go offscreen), position to right of trinket slot instead
    if (affix_tooltip_x < 10) {
        int col = slot_index % 3;
        int slot_x = GetTrinketUIX() + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        affix_tooltip_x = slot_x + TRINKET_SLOT_SIZE + 15;
    }

    // Clamp Y to screen bounds
    if (affix_tooltip_y + affix_height > SCREEN_HEIGHT - 10) {
        affix_tooltip_y = SCREEN_HEIGHT - affix_height - 10;
    }
    if (affix_tooltip_y < TOP_BAR_HEIGHT + 10) {
        affix_tooltip_y = TOP_BAR_HEIGHT + 10;
    }

    // Background (dark panel, matching main tooltip)
    a_DrawFilledRect((aRectf_t){affix_tooltip_x, affix_tooltip_y, affix_width, affix_height},
                    (aColor_t){9, 10, 20, 240});  // COLOR_PANEL_BG

    // Blue border (to distinguish from main tooltip's rarity-colored border)
    a_DrawRect((aRectf_t){affix_tooltip_x, affix_tooltip_y, affix_width, affix_height},
              (aColor_t){100, 150, 255, 255});  // Blue
    a_DrawRect((aRectf_t){affix_tooltip_x + 1, affix_tooltip_y + 1, affix_width - 2, affix_height - 2},
              (aColor_t){100, 150, 255, 255});  // Double border

    // Render content
    int content_x = affix_tooltip_x + affix_padding;
    int current_y = affix_tooltip_y + affix_padding;

    // Render each affix with FlexBox 2-column layout (no header needed)
    for (int i = 0; i < instance->affix_count && i < ui->cached_affix_count; i++) {
        int value = instance->affixes[i].rolled_value;
        AffixTemplate_t* affix_template = ui->cached_affix_templates[i];

        if (!affix_template) {
            continue;
        }

        const char* affix_name = d_StringPeek(affix_template->name);
        int weight = affix_template->weight;

        // Calculate weight-based color gradient for affix name
        // Weight range: 25 (rare/orange) to 100 (common/white)
        float t = (weight - 25.0f) / 75.0f;  // 0.0 = rare, 1.0 = common
        t = fmaxf(0.0f, fminf(1.0f, t));     // Clamp to [0, 1]

        // Interpolate: Orange (255,140,0) -> White (200,200,200)
        uint8_t name_r = (uint8_t)(255.0f - (55.0f * t));   // 255 -> 200
        uint8_t name_g = (uint8_t)(140.0f + (60.0f * t));   // 140 -> 200
        uint8_t name_b = (uint8_t)(0.0f + (200.0f * t));    // 0 -> 200

        // Format description with rolled value (replace {value} placeholder)
        const char* desc_template = d_StringPeek(affix_template->description);
        dString_t* temp_desc = d_StringInit();
        const char* desc_str = desc_template;
        bool replaced = false;
        for (size_t j = 0; desc_str[j] != '\0'; j++) {
            if (desc_str[j] == '{' && desc_str[j+1] == 'v' && desc_str[j+2] == 'a' &&
                desc_str[j+3] == 'l' && desc_str[j+4] == 'u' && desc_str[j+5] == 'e' &&
                desc_str[j+6] == '}' && !replaced) {
                // Found {value}, replace with actual value
                d_StringAppendInt(temp_desc, value);
                j += 6;  // Skip past {value}
                replaced = true;
            } else {
                d_StringAppendChar(temp_desc, desc_str[j]);
            }
        }

        // Render affix name (row 1: bold, weight-colored, smaller, moved up)
        aTextStyle_t name_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {name_r, name_g, name_b, 255},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 0.9f  // Smaller (was 1.1f)
        };
        a_DrawText((char*)affix_name, content_x, current_y - 12, name_config);  // Move up 12px
        current_y += 20;  // Move down for description

        // Calculate roll quality percentage
        int min_val = affix_template->min_value;
        int max_val = affix_template->max_value;
        float roll_range = (float)(max_val - min_val);
        float roll_quality = (roll_range > 0) ? ((float)(value - min_val) / roll_range) : 0.5f;
        roll_quality = fmaxf(0.0f, fminf(1.0f, roll_quality));  // Clamp [0, 1]

        // Color gradient based on roll quality:
        // 0-30%: Gray (160, 160, 160)
        // 31-70%: Gray -> White (160->235)
        // 71-100%: White -> Green (235,235,235 -> 168,202,88 from palette)
        uint8_t desc_r, desc_g, desc_b;

        if (roll_quality <= 0.30f) {
            // Low roll: Gray
            desc_r = desc_g = desc_b = 160;
        } else if (roll_quality <= 0.70f) {
            // Mid roll: Gray -> White
            float t2 = (roll_quality - 0.30f) / 0.40f;  // 0.0 to 1.0
            desc_r = desc_g = desc_b = (uint8_t)(160 + (75 * t2));  // 160 -> 235
        } else {
            // High roll: White -> Green
            float t2 = (roll_quality - 0.70f) / 0.30f;  // 0.0 to 1.0
            desc_r = (uint8_t)(235 - (67 * t2));   // 235 -> 168 (palette green)
            desc_g = (uint8_t)(235 - (33 * t2));   // 235 -> 202 (palette green)
            desc_b = (uint8_t)(235 - (147 * t2));  // 235 -> 88 (palette green)
        }

        // Render description (row 2: roll-quality colored, left-aligned, moved down 4px)
        aTextStyle_t desc_config = {
            .type = FONT_GAME,
            .fg = {desc_r, desc_g, desc_b, 255},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,  // Allow wrapping
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(temp_desc), content_x, current_y + 4, desc_config);

        // Cleanup strings
        d_StringDestroy(temp_desc);

        current_y += 20 + 5;  // Description height + gap before next affix
    }
}

/**
 * RenderTrinketTooltip (DUF Version) - Display tooltip for TrinketInstance_t
 *
 * Shows:
 * - Base trinket name, rarity, flavor
 * - Passive trigger/effect
 * - Stats counters
 */
static void RenderTrinketTooltip(TrinketUI_t* ui, const TrinketTemplate_t* template, TrinketInstance_t* instance, Player_t* player, int slot_index) {
    if (!template || !instance || !player) {
        d_LogWarningF("RenderTrinketTooltip: NULL param (template=%p, instance=%p, player=%p)",
                      (void*)template, (void*)instance, (void*)player);
        return;
    }

    d_LogDebugF("RenderTrinketTooltip: slot %d, template name=%s",
                slot_index, d_StringPeek(template->name));

    // Check if this trinket has block_debuff effect (Warded Charm)
    bool has_block_debuff = (template->passive_effect_type == TRINKET_EFFECT_BLOCK_DEBUFF ||
                             template->passive_effect_type_2 == TRINKET_EFFECT_BLOCK_DEBUFF);

    // Calculate tooltip position
    int row = slot_index / 3;
    int col = slot_index % 3;
    int slot_x = GetTrinketUIX() + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
    int slot_y = GetTrinketUIY() + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

    int tooltip_width = 340;
    int tooltip_x = slot_x - tooltip_width - 10;  // Left of slot
    int tooltip_y = slot_y;

    // If too far left, show on right instead
    if (tooltip_x < 10) {
        tooltip_x = slot_x + TRINKET_SLOT_SIZE + 10;
    }

    int padding = 16;
    int content_width = tooltip_width - (padding * 2);

    // Get text content
    const char* name = d_StringPeek(template->name);
    const char* flavor = d_StringPeek(template->flavor);

    // Get rarity color for border
    int rarity_r, rarity_g, rarity_b;
    GetTrinketRarityColor(instance->rarity, &rarity_r, &rarity_g, &rarity_b);

    // Measure text heights
    int name_height = a_GetWrappedTextHeight((char*)name, FONT_ENTER_COMMAND, content_width);
    int flavor_height = a_GetWrappedTextHeight((char*)flavor, FONT_GAME, content_width);

    // Calculate tooltip height (no rarity text, color shows rarity)
    int tooltip_height = padding;
    tooltip_height += name_height + 8;
    tooltip_height += flavor_height + 12;
    tooltip_height += 1 + 12;  // Divider

    // Add passive description height (primary) - measured dynamically for text wrapping
    dString_t* primary_passive = FormatTrinketPassive(template, instance, false);
    if (primary_passive && d_StringGetLength(primary_passive) > 0) {
        int passive_height = a_GetWrappedTextHeight((char*)d_StringPeek(primary_passive),
                                                     FONT_GAME, content_width);
        tooltip_height += passive_height + 4;  // Measured height + small gap
    }
    if (primary_passive) d_StringDestroy(primary_passive);

    // Add secondary passive height (if exists)
    // NOTE: Can't check passive_trigger_2 != 0 because COMBAT_START == 0!
    if (template->passive_effect_type_2 != TRINKET_EFFECT_NONE) {
        dString_t* secondary_passive = FormatTrinketPassive(template, instance, true);
        if (secondary_passive && d_StringGetLength(secondary_passive) > 0) {
            int passive_height = a_GetWrappedTextHeight((char*)d_StringPeek(secondary_passive),
                                                         FONT_GAME, content_width);
            tooltip_height += passive_height + 4;  // Measured height + small gap
        }
        if (secondary_passive) d_StringDestroy(secondary_passive);
    }

    // Add spacing after passives (before bottom padding)
    tooltip_height += 8;

    // Affixes rendered in separate tooltip (no height needed here)

    // Show damage counter (if any)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_DAMAGE_DEALT) > 0) {
        tooltip_height += 20;
    }

    // Show chips lost counter (if any - for Blood Pact)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_CHIPS_LOST) > 0) {
        tooltip_height += 20;
    }

    // Show bonus chips (if any - for Elite Membership)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_BONUS_CHIPS) > 0) {
        tooltip_height += 20;
    }

    // Show refunded chips (if any - for Elite Membership)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_REFUNDED_CHIPS) > 0) {
        tooltip_height += 20;
    }

    // Show highest streak (if any - for Streak Counter)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_HIGHEST_STREAK) > 0) {
        tooltip_height += 20;
    }

    // Show debuffs blocked (if any - for Warded Charm)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_DEBUFFS_BLOCKED) > 0) {
        tooltip_height += 20;
    }

    // Show heal damage dealt (if any - for Bleeding Heart)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_HEAL_DAMAGE_DEALT) > 0) {
        tooltip_height += 20;
    }

    // Show current blocks remaining (if Warded Charm and has blocks)
    if (has_block_debuff && instance->debuff_blocks_remaining > 0) {
        tooltip_height += 20;  // "Blocks Remaining: X"
    }

    // Check if this trinket has punish_heal effect (Bleeding Heart)
    bool has_punish_heal = (template->passive_effect_type == TRINKET_EFFECT_PUNISH_HEAL ||
                            template->passive_effect_type_2 == TRINKET_EFFECT_PUNISH_HEAL);
    if (has_punish_heal && instance->heal_punishes_remaining > 0) {
        tooltip_height += 20;  // "Punishes Remaining: X"
    }

    tooltip_height += padding;

    // Clamp to screen
    if (tooltip_y + tooltip_height > SCREEN_HEIGHT - 10) {
        tooltip_y = SCREEN_HEIGHT - tooltip_height - 10;
    }
    if (tooltip_y < TOP_BAR_HEIGHT + 10) {
        tooltip_y = TOP_BAR_HEIGHT + 10;
    }

    // Background (dark panel)
    a_DrawFilledRect((aRectf_t){tooltip_x, tooltip_y, tooltip_width, tooltip_height},
                    (aColor_t){9, 10, 20, 240});  // COLOR_PANEL_BG

    // Rarity-colored double border (matches trinket slot and modal)
    a_DrawRect((aRectf_t){tooltip_x, tooltip_y, tooltip_width, tooltip_height},
              (aColor_t){rarity_r, rarity_g, rarity_b, 255});
    a_DrawRect((aRectf_t){tooltip_x + 1, tooltip_y + 1, tooltip_width - 2, tooltip_height - 2},
              (aColor_t){rarity_r, rarity_g, rarity_b, 255});

    // Render content
    int content_x = tooltip_x + padding;
    int current_y = tooltip_y + padding;

    // Title
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {232, 193, 112, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)name, content_x, current_y, title_config);
    current_y += name_height + 8;

    // Rarity shown by border color, no text needed

    // Flavor text
    aTextStyle_t flavor_config = {
        .type = FONT_GAME,
        .fg = {150, 150, 150, 255},  // Dimmed italic style
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)flavor, content_x, current_y, flavor_config);
    current_y += flavor_height + 12;

    // Divider
    a_DrawFilledRect((aRectf_t){content_x, current_y, content_width, 1}, (aColor_t){100, 100, 100, 200});
    current_y += 12;

    // Passive description (primary)
    aTextStyle_t passive_config = {
        .type = FONT_GAME,
        .fg = {168, 202, 88, 255},  // Green
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    dString_t* passive_text = FormatTrinketPassive(template, instance, false);
    if (passive_text && d_StringGetLength(passive_text) > 0) {
        int passive_height = a_GetWrappedTextHeight((char*)d_StringPeek(passive_text),
                                                     FONT_GAME, content_width);
        a_DrawText((char*)d_StringPeek(passive_text), content_x, current_y, passive_config);
        current_y += passive_height + 4;  // Dynamic height + small gap
    }
    if (passive_text) d_StringDestroy(passive_text);

    // Passive description (secondary, if exists)
    // NOTE: Can't check passive_trigger_2 != 0 because COMBAT_START == 0!
    if (template->passive_effect_type_2 != TRINKET_EFFECT_NONE) {
        dString_t* passive_text_2 = FormatTrinketPassive(template, instance, true);
        if (passive_text_2 && d_StringGetLength(passive_text_2) > 0) {
            int passive_height = a_GetWrappedTextHeight((char*)d_StringPeek(passive_text_2),
                                                         FONT_GAME, content_width);
            a_DrawText((char*)d_StringPeek(passive_text_2), content_x, current_y, passive_config);
            current_y += passive_height + 4;  // Dynamic height + small gap
        }
        if (passive_text_2) d_StringDestroy(passive_text_2);
    }

    // Show current blocks remaining (Warded Charm - live counter)
    if (has_block_debuff && instance->debuff_blocks_remaining > 0) {
        dString_t* blocks_text = d_StringInit();
        d_StringFormat(blocks_text, "Blocks Remaining: %d", instance->debuff_blocks_remaining);

        aTextStyle_t blocks_config = {
            .type = FONT_GAME,
            .fg = {100, 200, 255, 255},  // Light blue (active protection)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(blocks_text), content_x, current_y, blocks_config);
        d_StringDestroy(blocks_text);
        current_y += 20;
    }

    // Affixes rendered in separate tooltip (see RenderAffixTooltip below)

    // Total damage dealt (if any)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_DAMAGE_DEALT) > 0) {
        dString_t* dmg_text = d_StringInit();
        d_StringFormat(dmg_text, "Total Damage Dealt: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_DAMAGE_DEALT));

        aTextStyle_t dmg_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(dmg_text), content_x, current_y, dmg_config);
        d_StringDestroy(dmg_text);
        current_y += 20;
    }

    // Total chips lost (if any - for Blood Pact)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_CHIPS_LOST) > 0) {
        dString_t* chips_text = d_StringInit();
        d_StringFormat(chips_text, "Total Chips Lost: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_CHIPS_LOST));

        aTextStyle_t chips_config = {
            .type = FONT_GAME,
            .fg = {255, 100, 100, 255},  // Reddish text (cost)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(chips_text), content_x, current_y, chips_config);
        d_StringDestroy(chips_text);
        current_y += 20;
    }

    // Bonus chips won (if any - for Elite Membership)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_BONUS_CHIPS) > 0) {
        dString_t* bonus_text = d_StringInit();
        d_StringFormat(bonus_text, "Bonus Chips Won: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_BONUS_CHIPS));

        aTextStyle_t bonus_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(bonus_text), content_x, current_y, bonus_config);
        d_StringDestroy(bonus_text);
        current_y += 20;
    }

    // Chips refunded (if any - for Elite Membership)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_REFUNDED_CHIPS) > 0) {
        dString_t* refund_text = d_StringInit();
        d_StringFormat(refund_text, "Chips Refunded: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_REFUNDED_CHIPS));

        aTextStyle_t refund_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(refund_text), content_x, current_y, refund_config);
        d_StringDestroy(refund_text);
        current_y += 20;
    }

    // Highest streak (if any - for Streak Counter)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_HIGHEST_STREAK) > 0) {
        dString_t* streak_text = d_StringInit();
        d_StringFormat(streak_text, "Highest Streak: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_HIGHEST_STREAK));

        aTextStyle_t streak_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(streak_text), content_x, current_y, streak_config);
        d_StringDestroy(streak_text);
        current_y += 20;
    }

    // Debuffs blocked (if any - for Warded Charm)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_DEBUFFS_BLOCKED) > 0) {
        dString_t* blocked_text = d_StringInit();
        d_StringFormat(blocked_text, "Debuffs Blocked: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_DEBUFFS_BLOCKED));

        aTextStyle_t blocked_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text (matches other stats)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(blocked_text), content_x, current_y, blocked_config);
        d_StringDestroy(blocked_text);
        current_y += 20;
    }

    // Heal damage dealt (if any - for Bleeding Heart)
    if (TRINKET_GET_STAT(instance, TRINKET_STAT_HEAL_DAMAGE_DEALT) > 0) {
        dString_t* heal_dmg_text = d_StringInit();
        d_StringFormat(heal_dmg_text, "Heal Damage Dealt: %d", TRINKET_GET_STAT(instance, TRINKET_STAT_HEAL_DAMAGE_DEALT));

        aTextStyle_t heal_dmg_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 255, 255},  // White text (matches other stats)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(heal_dmg_text), content_x, current_y, heal_dmg_config);
        d_StringDestroy(heal_dmg_text);
        current_y += 20;
    }

    // Show current punishes remaining (Bleeding Heart - live counter)
    if (has_punish_heal && instance->heal_punishes_remaining > 0) {
        dString_t* punishes_text = d_StringInit();
        d_StringFormat(punishes_text, "Punishes Remaining: %d", instance->heal_punishes_remaining);

        aTextStyle_t punishes_config = {
            .type = FONT_GAME,
            .fg = {255, 100, 100, 255},  // Red/pink (punishing heals)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(punishes_text), content_x, current_y, punishes_config);
        d_StringDestroy(punishes_text);
        current_y += 20;
    }

    // Render separate affix tooltip (if affixes exist)
    if (ui->cached_affix_count > 0) {
        RenderAffixTooltip(ui, instance, slot_index, tooltip_x, tooltip_y, tooltip_height);
    }
}

void RenderTrinketTooltips(TrinketUI_t* ui, Player_t* player) {
    if (!ui || !player) return;
    // Note: trinket_slots is a fixed-size array embedded in Player_t, not a pointer

    // Don't show tooltips while in targeting mode (tooltip would block card selection)
    extern GameContext_t g_game;
    if (g_game.current_state == STATE_TARGETING) {
        return;
    }

    // Show tooltip for hovered class trinket
    if (ui->hovered_class_trinket) {
        Trinket_t* trinket = GetClassTrinket(player);
        if (trinket) {
            RenderClassTrinketTooltip(trinket, -1);  // -1 = class trinket
        }
    }
    // Show tooltip for hovered regular trinket
    else if (ui->hovered_trinket_slot >= 0) {
        // Use new TrinketInstance_t system
        if (player->trinket_slot_occupied[ui->hovered_trinket_slot]) {
            TrinketInstance_t* instance = &player->trinket_slots[ui->hovered_trinket_slot];

            // Safety check: base_trinket_key must be initialized
            if (!instance->base_trinket_key) {
                d_LogWarningF("Trinket in slot %d has NULL base_trinket_key (occupied but uninitialized)",
                              ui->hovered_trinket_slot);
                return;
            }

            const TrinketTemplate_t* template = GetTrinketTemplate(d_StringPeek(instance->base_trinket_key));
            if (template) {
                RenderTrinketTooltip(ui, template, instance, player, ui->hovered_trinket_slot);
            }
        }
    }
}

// ============================================================================
// HOVER STATE UPDATE
// ============================================================================

/**
 * UpdateTrinketUIHover - Update hover state based on mouse position
 *
 * MUST be called BEFORE HandleTrinketUIInput() to ensure input handler
 * sees current hover state (not stale data from previous frame).
 *
 * Called in BlackjackLogic before input handling.
 */
void UpdateTrinketUIHover(TrinketUI_t* ui, Player_t* player) {
    if (!ui || !player) return;

    // Reset hover state
    ui->hovered_trinket_slot = -1;
    ui->hovered_class_trinket = false;

    // Get mouse position
    int mouse_x = app.mouse.x;
    int mouse_y = app.mouse.y;

    // Check class trinket hover
    bool class_hovered = (mouse_x >= GetClassTrinketX() &&
                          mouse_x < GetClassTrinketX() + CLASS_TRINKET_SIZE &&
                          mouse_y >= GetClassTrinketY() &&
                          mouse_y < GetClassTrinketY() + CLASS_TRINKET_SIZE);
    if (class_hovered) {
        ui->hovered_class_trinket = true;
        return;  // Early exit if class trinket hovered
    }

    // Check regular trinket slots (2 rows × 3 columns)
    for (int slot_index = 0; slot_index < 6; slot_index++) {
        int row = slot_index / 3;
        int col = slot_index % 3;
        int slot_x = GetTrinketUIX() + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int slot_y = GetTrinketUIY() + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

        bool slot_hovered = (mouse_x >= slot_x &&
                            mouse_x < slot_x + TRINKET_SLOT_SIZE &&
                            mouse_y >= slot_y &&
                            mouse_y < slot_y + TRINKET_SLOT_SIZE);

        if (slot_hovered) {
            ui->hovered_trinket_slot = slot_index;

            // Cache affix templates if slot changed
            if (ui->cached_slot_index != slot_index) {
                // Clear old cache
                for (int i = 0; i < 3; i++) {
                    if (ui->cached_affix_templates[i]) {
                        CleanupAffixTemplate(ui->cached_affix_templates[i]);
                        free(ui->cached_affix_templates[i]);
                        ui->cached_affix_templates[i] = NULL;
                    }
                }
                ui->cached_affix_count = 0;
                ui->cached_slot_index = slot_index;

                // Load new templates
                TrinketInstance_t* instance = &player->trinket_slots[slot_index];

                // DEBUG: Log trinket state before accessing
                d_LogDebugF("UpdateTrinketUIHover: slot=%d, base_trinket_key=%p, occupied=%d",
                            slot_index,
                            (void*)instance->base_trinket_key,
                            player->trinket_slot_occupied[slot_index]);

                if (instance->base_trinket_key && d_StringGetLength(instance->base_trinket_key) > 0) {
                    for (int i = 0; i < instance->affix_count && i < 3; i++) {
                        const char* stat_key = d_StringPeek(instance->affixes[i].stat_key);
                        AffixTemplate_t* template = LoadAffixTemplateFromDUF(stat_key);
                        if (template) {
                            ui->cached_affix_templates[i] = template;
                            ui->cached_affix_count++;
                        }
                    }
                }
            }

            return;  // Early exit if slot hovered
        }
    }

    // No slot hovered - clear cache if it was set
    if (ui->cached_slot_index != -2) {
        for (int i = 0; i < 3; i++) {
            if (ui->cached_affix_templates[i]) {
                CleanupAffixTemplate(ui->cached_affix_templates[i]);
                free(ui->cached_affix_templates[i]);
                ui->cached_affix_templates[i] = NULL;
            }
        }
        ui->cached_affix_count = 0;
        ui->cached_slot_index = -2;
    }
}

// ============================================================================
// RENDERING - TRINKET SLOTS
// ============================================================================

void RenderTrinketUI(TrinketUI_t* ui, Player_t* player) {
    if (!ui || !player) {
        return;
    }
    // Note: trinket_slots is a fixed-size array embedded in Player_t, not a pointer

    // Hover state is now updated by UpdateTrinketUIHover() before input handling
    // (no longer reset or calculated here to avoid timing bug)

    // ========================================================================
    // RENDER CLASS TRINKET (LEFT SIDE, BIGGER SLOT)
    // ========================================================================
    Trinket_t* class_trinket = GetClassTrinket(player);

    // Use persisted hover state from UpdateTrinketUIHover()
    bool class_hovered = ui->hovered_class_trinket;

    if (class_trinket) {
        // Class trinket equipped - draw with gold styling
        int bg_r = 50, bg_g = 50, bg_b = 30;  // Slightly golden tint
        int border_r = 200, border_g = 180, border_b = 50;  // Gold border

        // Check if on cooldown
        bool on_cooldown = class_trinket->active_cooldown_current > 0;
        bool is_clickable = !on_cooldown && class_trinket->active_target_type == TRINKET_TARGET_CARD;

        if (on_cooldown) {
            // Red tint (same red as cooldown indicator box)
            bg_r = 60; bg_g = 20; bg_b = 20;
            border_r = 255; border_g = 100; border_b = 100;
        }

        // Hover effect (brighter if ready to use)
        if (class_hovered && is_clickable) {
            bg_r += 20; bg_g += 20; bg_b += 10;  // Brighten background
            border_r = 255; border_g = 220; border_b = 100;  // Brighter gold border
        }

        // Apply shake offsets (tweened during proc)
        int shake_x = GetClassTrinketX() + (int)class_trinket->shake_offset_x;
        int shake_y = GetClassTrinketY() + (int)class_trinket->shake_offset_y;

        // Background
        a_DrawFilledRect((aRectf_t){shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE},
                       (aColor_t){bg_r, bg_g, bg_b, 255});

        // Gold border (thicker, 2px)
        a_DrawRect((aRectf_t){shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE},
                  (aColor_t){border_r, border_g, border_b, 255});  // Gold
        a_DrawRect((aRectf_t){shake_x + 1, shake_y + 1, CLASS_TRINKET_SIZE - 2, CLASS_TRINKET_SIZE - 2},
                  (aColor_t){border_r, border_g, border_b, 255});  // Double border for emphasis

        // Hover glow effect (subtle overlay)
        if (class_hovered && is_clickable) {
            a_DrawFilledRect((aRectf_t){shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE},
                           (aColor_t){255, 255, 200, 30});  // Subtle yellow glow
        }

        // Draw red flash overlay when trinket procs
        if (class_trinket->flash_alpha > 0.0f) {
            Uint8 flash = (Uint8)class_trinket->flash_alpha;
            a_DrawFilledRect((aRectf_t){shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE},
                           (aColor_t){255, 0, 0, flash});
        }

        // "CLASS" label at top
        aTextStyle_t class_label_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {200, 180, 50, 255},  // Gold text
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.5f
        };
        a_DrawText("CLASS",
                       shake_x + CLASS_TRINKET_SIZE / 2,
                       shake_y + 6,
                       class_label_config);

        // Draw trinket name
        const char* trinket_name = GetTrinketName(class_trinket);
        aTextStyle_t name_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {255, 255, 255, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = CLASS_TRINKET_SIZE - 8,
            .scale = 0.6f
        };
        a_DrawText((char*)trinket_name,
                       shake_x + CLASS_TRINKET_SIZE / 2,
                       shake_y + 24,
                       name_config);

        // Draw indicator boxes below trinket (cooldown left, damage bonus right)
        const int indicator_width = 40;
        const int indicator_height = 34;
        const int indicator_gap = 6;
        const int indicator_y_offset = 6;  // Gap between trinket and indicators

        int indicators_y = GetClassTrinketY() + CLASS_TRINKET_SIZE + indicator_y_offset;
        int left_indicator_x = GetClassTrinketX() + (CLASS_TRINKET_SIZE / 2) - indicator_width - (indicator_gap / 2);
        int right_indicator_x = GetClassTrinketX() + (CLASS_TRINKET_SIZE / 2) + (indicator_gap / 2);

        // Left indicator: Cooldown
        if (on_cooldown) {
            // Red background for cooldown
            a_DrawFilledRect((aRectf_t){left_indicator_x, indicators_y, indicator_width, indicator_height},
                           (aColor_t){60, 20, 20, 255});
            a_DrawRect((aRectf_t){left_indicator_x, indicators_y, indicator_width, indicator_height},
                      (aColor_t){255, 100, 100, 255});

            char cooldown_text[8];
            snprintf(cooldown_text, sizeof(cooldown_text), "%d", class_trinket->active_cooldown_current);

            aTextStyle_t cd_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {255, 150, 150, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 0.8f
            };
            a_DrawText(cooldown_text,
                           left_indicator_x + indicator_width / 2,
                           indicators_y + 3,
                           cd_config);
        } else {
            // Dimmed background when ready
            a_DrawFilledRect((aRectf_t){left_indicator_x, indicators_y, indicator_width, indicator_height},
                           (aColor_t){30, 30, 30, 255});
            a_DrawRect((aRectf_t){left_indicator_x, indicators_y, indicator_width, indicator_height},
                      (aColor_t){80, 80, 80, 255});

            aTextStyle_t ready_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {100, 255, 100, 255},  // Green for ready
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 0.8f
            };
            a_DrawText("0",
                           left_indicator_x + indicator_width / 2,
                           indicators_y + 3,
                           ready_config);
        }

        // Right indicator: Damage bonus (shows scaling bonus, not base damage)
        // Green background for damage
        a_DrawFilledRect((aRectf_t){right_indicator_x, indicators_y, indicator_width, indicator_height},
                       (aColor_t){20, 60, 20, 255});
        a_DrawRect((aRectf_t){right_indicator_x, indicators_y, indicator_width, indicator_height},
                  (aColor_t){100, 255, 100, 255});

        char bonus_text[16];
        snprintf(bonus_text, sizeof(bonus_text), "+%d", class_trinket->passive_damage_bonus);

        aTextStyle_t bonus_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {150, 255, 150, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.8f
        };
        a_DrawText(bonus_text,
                       right_indicator_x + indicator_width / 2,
                       indicators_y + 3,
                       bonus_config);
    } else {
        // Empty class trinket slot - draw dimmed gold outline
        a_DrawRect((aRectf_t){GetClassTrinketX(), GetClassTrinketY(), CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE},
                  (aColor_t){100, 90, 25, 128});  // Dimmed gold
    }

    // ========================================================================
    // RENDER REGULAR TRINKET GRID (3x2 grid, RIGHT SIDE)
    // ========================================================================
    // 3x2 grid layout (slots 0-2 top row, 3-5 bottom row)
    for (int slot_index = 0; slot_index < 6; slot_index++) {
        int row = slot_index / 3;  // 0 or 1
        int col = slot_index % 3;  // 0, 1, or 2

        int slot_x = GetTrinketUIX() + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int slot_y = GetTrinketUIY() + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

        // Use persisted hover state from UpdateTrinketUIHover()
        bool is_hovered = (ui->hovered_trinket_slot == slot_index);

        // Use new TrinketInstance_t system
        bool has_trinket = player->trinket_slot_occupied[slot_index];

        if (has_trinket) {
            TrinketInstance_t* instance = &player->trinket_slots[slot_index];

            // Safety check: base_trinket_key must be initialized
            if (!instance->base_trinket_key) {
                d_LogWarningF("Trinket in slot %d has NULL base_trinket_key (occupied but uninitialized)", slot_index);
                // Draw empty slot instead
                has_trinket = false;
                goto draw_empty_slot;
            }

            const char* trinket_key = d_StringPeek(instance->base_trinket_key);
            const TrinketTemplate_t* template = GetTrinketTemplate(trinket_key);

            if (!template) {
                d_LogErrorF("Trinket UI: Failed to load template for key '%s' in slot %d",
                           trinket_key ? trinket_key : "NULL", slot_index);
            }

            if (template) {
                // Get rarity colors from palette
                int rarity_r, rarity_g, rarity_b;
                GetTrinketRarityColor(instance->rarity, &rarity_r, &rarity_g, &rarity_b);

                // Background: dark panel color
                int bg_r = 9, bg_g = 10, bg_b = 20;  // COLOR_PANEL_BG

                // Hover effect - brighten background slightly
                if (is_hovered) {
                    bg_r += 20; bg_g += 20; bg_b += 20;
                }

                // Background
                a_DrawFilledRect((aRectf_t){slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE},
                               (aColor_t){bg_r, bg_g, bg_b, 255});

                // Rarity-colored double border (like modal)
                a_DrawRect((aRectf_t){slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE},
                          (aColor_t){rarity_r, rarity_g, rarity_b, 255});
                a_DrawRect((aRectf_t){slot_x + 1, slot_y + 1, TRINKET_SLOT_SIZE - 2, TRINKET_SLOT_SIZE - 2},
                          (aColor_t){rarity_r, rarity_g, rarity_b, 255});

                // Hover glow effect (subtle overlay in rarity color)
                if (is_hovered) {
                    a_DrawFilledRect((aRectf_t){slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE},
                                   (aColor_t){rarity_r, rarity_g, rarity_b, 30});
                }

                // Draw trinket name (small, wrapped)
                const char* trinket_name = d_StringPeek(template->name);
                aTextStyle_t name_config = {
                    .type = FONT_ENTER_COMMAND,
                    .fg = {255, 255, 255, 255},
                    .align = TEXT_ALIGN_CENTER,
                    .wrap_width = TRINKET_SLOT_SIZE - 4,
                    .scale = 0.5f
                };
                a_DrawText((char*)trinket_name,
                               slot_x + TRINKET_SLOT_SIZE / 2,
                               slot_y + 6,
                               name_config);
            }
        }

draw_empty_slot:
        if (!has_trinket) {
            // Empty slot - draw dimmed border
            a_DrawRect((aRectf_t){slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE},
                      (aColor_t){80, 80, 80, 128});  // Dimmed gray

            // "EMPTY" label
            aTextStyle_t empty_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {100, 100, 100, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = TRINKET_SLOT_SIZE - 4,
                .scale = 0.5f
            };
            a_DrawText("EMPTY",
                           slot_x + TRINKET_SLOT_SIZE / 2,
                           slot_y + TRINKET_SLOT_SIZE / 2 - 6,
                           empty_config);
        }
    }
}

// ============================================================================
// STATE QUERIES
// ============================================================================

int GetHoveredTrinketSlot(const TrinketUI_t* ui) {
    return ui ? ui->hovered_trinket_slot : -1;
}

bool IsClassTrinketHovered(const TrinketUI_t* ui) {
    return ui ? ui->hovered_class_trinket : false;
}
