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

    return ui;
}

void DestroyTrinketUI(TrinketUI_t** ui) {
    if (!ui || !*ui) return;
    free(*ui);
    *ui = NULL;
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void HandleTrinketUIInput(TrinketUI_t* ui, Player_t* player, GameContext_t* game) {
    if (!ui || !player || !game || !player->trinket_slots) return;

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
        slot_x = CLASS_TRINKET_X;
        slot_y = CLASS_TRINKET_Y;
        slot_size = CLASS_TRINKET_SIZE;
    } else {
        int row = slot_index / 3;
        int col = slot_index % 3;
        slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
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
        d_StringSet(passive_text, "", 0);
        return passive_text;
    }

    // 1. Format trigger ("On Win:", "On Blackjack:", etc.)
    const char* trigger_str = NULL;
    switch (trigger) {
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
            const char* stat = template->passive_stack_stat ? d_StringPeek(template->passive_stack_stat) : "???";
            // Show current stacks / max stacks if instance exists
            if (instance && instance->trinket_stacks > 0) {
                d_StringFormat(passive_text, "%s: +%d%% %s (%d/%d stacks)",
                              trigger_str, template->passive_stack_value, stat,
                              instance->trinket_stacks, template->passive_stack_max);
            } else {
                d_StringFormat(passive_text, "%s: +%d%% %s (max %d stacks)",
                              trigger_str, template->passive_stack_value, stat,
                              template->passive_stack_max);
            }
            break;
        }

        case TRINKET_EFFECT_ADD_DAMAGE_FLAT:
            d_StringFormat(passive_text, "%s: +%d damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_DAMAGE_MULTIPLIER:
            d_StringFormat(passive_text, "%s: ×%d%% damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_PUSH_DAMAGE_PERCENT:
            d_StringFormat(passive_text, "%s: Deal %d%% damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_ADD_TAG_TO_CARDS: {
            const char* tag = template->passive_tag ? d_StringPeek(template->passive_tag) : "???";
            d_StringFormat(passive_text, "%s: Add %s tag to %d cards",
                          trigger_str, tag, template->passive_tag_count);
            break;
        }

        case TRINKET_EFFECT_BUFF_TAG_DAMAGE: {
            const char* tag = template->passive_tag ? d_StringPeek(template->passive_tag) : "???";
            d_StringFormat(passive_text, "%s: %s cards deal +%d damage",
                          trigger_str, tag, template->passive_tag_buff_value);
            break;
        }

        case TRINKET_EFFECT_NONE:
        default:
            d_StringFormat(passive_text, "%s: ???", trigger_str);
            break;
    }

    return passive_text;
}

/**
 * RenderTrinketTooltip (DUF Version) - Display tooltip for TrinketInstance_t
 *
 * Shows:
 * - Base trinket name, rarity, flavor
 * - Passive trigger/effect
 * - All rolled affixes
 */
static void RenderTrinketTooltip(const TrinketTemplate_t* template, TrinketInstance_t* instance, int slot_index) {
    if (!template || !instance) return;

    // Calculate tooltip position
    int row = slot_index / 3;
    int col = slot_index % 3;
    int slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
    int slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

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

    // Add passive description height
    tooltip_height += 20;  // "Passive:" label
    // TODO: Format passive trigger + effect

    // Add affix heights
    for (int i = 0; i < instance->affix_count; i++) {
        tooltip_height += 20;  // Each affix line
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
        .scale = 0.9f
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
        a_DrawText((char*)d_StringPeek(passive_text), content_x, current_y, passive_config);
        current_y += 20;
    }
    if (passive_text) d_StringDestroy(passive_text);

    // Passive description (secondary, if exists)
    if (template->passive_trigger_2 != 0) {
        dString_t* passive_text_2 = FormatTrinketPassive(template, instance, true);
        if (passive_text_2 && d_StringGetLength(passive_text_2) > 0) {
            a_DrawText((char*)d_StringPeek(passive_text_2), content_x, current_y, passive_config);
            current_y += 20;
        }
        if (passive_text_2) d_StringDestroy(passive_text_2);
    }

    current_y += 8;  // Spacing before affixes

    // Affixes
    aTextStyle_t affix_config = {
        .type = FONT_GAME,
        .fg = {100, 150, 255, 255},  // Blue
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };

    for (int i = 0; i < instance->affix_count; i++) {
        const char* stat_key = d_StringPeek(instance->affixes[i].stat_key);
        int value = instance->affixes[i].rolled_value;

        dString_t* affix_text = d_StringInit();
        d_StringFormat(affix_text, "+%d %s", value, stat_key);
        a_DrawText((char*)d_StringPeek(affix_text), content_x, current_y, affix_config);
        d_StringDestroy(affix_text);

        current_y += 20;
    }
}

void RenderTrinketTooltips(TrinketUI_t* ui, Player_t* player) {
    if (!ui || !player || !player->trinket_slots) return;

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
            const TrinketTemplate_t* template = GetTrinketTemplate(d_StringPeek(instance->base_trinket_key));
            if (template) {
                RenderTrinketTooltip(template, instance, ui->hovered_trinket_slot);
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
    bool class_hovered = (mouse_x >= CLASS_TRINKET_X &&
                          mouse_x < CLASS_TRINKET_X + CLASS_TRINKET_SIZE &&
                          mouse_y >= CLASS_TRINKET_Y &&
                          mouse_y < CLASS_TRINKET_Y + CLASS_TRINKET_SIZE);
    if (class_hovered) {
        ui->hovered_class_trinket = true;
        return;  // Early exit if class trinket hovered
    }

    // Check regular trinket slots (2 rows × 3 columns)
    for (int slot_index = 0; slot_index < 6; slot_index++) {
        int row = slot_index / 3;
        int col = slot_index % 3;
        int slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

        bool slot_hovered = (mouse_x >= slot_x &&
                            mouse_x < slot_x + TRINKET_SLOT_SIZE &&
                            mouse_y >= slot_y &&
                            mouse_y < slot_y + TRINKET_SLOT_SIZE);

        if (slot_hovered) {
            ui->hovered_trinket_slot = slot_index;
            return;  // Early exit if slot hovered
        }
    }
}

// ============================================================================
// RENDERING - TRINKET SLOTS
// ============================================================================

void RenderTrinketUI(TrinketUI_t* ui, Player_t* player) {
    if (!ui || !player || !player->trinket_slots) {
        return;
    }

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
        int shake_x = CLASS_TRINKET_X + (int)class_trinket->shake_offset_x;
        int shake_y = CLASS_TRINKET_Y + (int)class_trinket->shake_offset_y;

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

        int indicators_y = CLASS_TRINKET_Y + CLASS_TRINKET_SIZE + indicator_y_offset;
        int left_indicator_x = CLASS_TRINKET_X + (CLASS_TRINKET_SIZE / 2) - indicator_width - (indicator_gap / 2);
        int right_indicator_x = CLASS_TRINKET_X + (CLASS_TRINKET_SIZE / 2) + (indicator_gap / 2);

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
        a_DrawRect((aRectf_t){CLASS_TRINKET_X, CLASS_TRINKET_Y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE},
                  (aColor_t){100, 90, 25, 128});  // Dimmed gold
    }

    // ========================================================================
    // RENDER REGULAR TRINKET GRID (3x2 grid, RIGHT SIDE)
    // ========================================================================
    // 3x2 grid layout (slots 0-2 top row, 3-5 bottom row)
    for (int slot_index = 0; slot_index < 6; slot_index++) {
        int row = slot_index / 3;  // 0 or 1
        int col = slot_index % 3;  // 0, 1, or 2

        int slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

        // Use persisted hover state from UpdateTrinketUIHover()
        bool is_hovered = (ui->hovered_trinket_slot == slot_index);

        // Use new TrinketInstance_t system
        bool has_trinket = player->trinket_slot_occupied[slot_index];

        if (has_trinket) {
            TrinketInstance_t* instance = &player->trinket_slots[slot_index];
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
        } else {
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
