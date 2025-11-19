/*
 * Trinket UI Component Implementation
 */

#include "../../../include/scenes/components/trinketUI.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/trinket.h"
#include "../../../include/state.h"
#include "../../../include/stateStorage.h"
#include "../../../include/tutorial/tutorialSystem.h"

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

            Trinket_t* trinket = GetEquippedTrinket(player, ui->hovered_trinket_slot);

            if (trinket && trinket->active_cooldown_current == 0 && trinket->active_target_type == TRINKET_TARGET_CARD) {
                // Enter targeting mode (store state in int_values table)
                StateData_SetInt(&game->state_data, "targeting_trinket_slot", ui->hovered_trinket_slot);
                StateData_SetInt(&game->state_data, "targeting_player_id", player->player_id);
                State_Transition(game, STATE_TARGETING);

                d_LogInfoF("Entered targeting mode for trinket slot %d (player ID %d)",
                          ui->hovered_trinket_slot, player->player_id);
            }
        }
    }
}

// ============================================================================
// RENDERING - TOOLTIPS
// ============================================================================

static void RenderTrinketTooltip(const Trinket_t* trinket, int slot_index) {
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

    tooltip_height += name_height + 10;
    tooltip_height += desc_height + 12;
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

    tooltip_height += padding;

    // Clamp to screen
    if (tooltip_y + tooltip_height > SCREEN_HEIGHT - 10) {
        tooltip_y = SCREEN_HEIGHT - tooltip_height - 10;
    }
    if (tooltip_y < TOP_BAR_HEIGHT + 10) {
        tooltip_y = TOP_BAR_HEIGHT + 10;
    }

    // Background
    a_DrawFilledRect(tooltip_x, tooltip_y, tooltip_width, tooltip_height,
                    20, 20, 30, 230);
    a_DrawRect(tooltip_x, tooltip_y, tooltip_width, tooltip_height,
              255, 255, 255, 255);

    // Render content
    int content_x = tooltip_x + padding;
    int current_y = tooltip_y + padding;

    // Class trinket header (if applicable)
    if (is_class_trinket) {
        aFontConfig_t class_header_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {200, 180, 50, 255},  // Gold
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 0.6f
        };
        a_DrawTextStyled("CLASS TRINKET", content_x, current_y, &class_header_config);
        current_y += 20;
    }

    // Title
    aFontConfig_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .color = {232, 193, 112, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)name, content_x, current_y, &title_config);
    current_y += name_height + 10;

    // Description
    aFontConfig_t desc_config = {
        .type = FONT_GAME,
        .color = {180, 180, 180, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)description, content_x, current_y, &desc_config);
    current_y += desc_height + 12;

    // Divider
    a_DrawFilledRect(content_x, current_y, content_width, 1, 100, 100, 100, 200);
    current_y += 12;

    // Passive
    aFontConfig_t passive_config = {
        .type = FONT_GAME,
        .color = {168, 202, 88, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)passive_desc, content_x, current_y, &passive_config);
    current_y += passive_height + 8;

    // Active
    aFontConfig_t active_config = {
        .type = FONT_GAME,
        .color = {207, 87, 60, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)active_desc, content_x, current_y, &active_config);
    current_y += active_height + 8;

    // Cooldown status
    if (trinket->active_cooldown_current > 0) {
        dString_t* cd_text = d_StringInit();
        d_StringFormat(cd_text, "Cooldown: %d turns remaining", trinket->active_cooldown_current);

        aFontConfig_t cd_config = {
            .type = FONT_GAME,
            .color = {255, 100, 100, 255},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(cd_text), content_x, current_y, &cd_config);
        d_StringDestroy(cd_text);
        current_y += 20;
    }

    // Total damage dealt (if any)
    if (trinket->total_damage_dealt > 0) {
        dString_t* dmg_text = d_StringInit();
        d_StringFormat(dmg_text, "Total Damage Dealt: %d", trinket->total_damage_dealt);

        aFontConfig_t dmg_config = {
            .type = FONT_GAME,
            .color = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(dmg_text), content_x, current_y, &dmg_config);
        d_StringDestroy(dmg_text);
        current_y += 20;
    }
}

void RenderTrinketTooltips(TrinketUI_t* ui, Player_t* player) {
    if (!ui || !player || !player->trinket_slots) return;

    // Show tooltip for hovered class trinket
    if (ui->hovered_class_trinket) {
        Trinket_t* trinket = GetClassTrinket(player);
        if (trinket) {
            RenderTrinketTooltip(trinket, -1);  // -1 = class trinket
        }
    }
    // Show tooltip for hovered regular trinket
    else if (ui->hovered_trinket_slot >= 0) {
        Trinket_t* trinket = GetEquippedTrinket(player, ui->hovered_trinket_slot);
        if (trinket) {
            RenderTrinketTooltip(trinket, ui->hovered_trinket_slot);
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

    // Reset hover state
    ui->hovered_trinket_slot = -1;
    ui->hovered_class_trinket = false;

    // Get mouse position for hover detection
    int mouse_x = app.mouse.x;
    int mouse_y = app.mouse.y;

    // ========================================================================
    // RENDER CLASS TRINKET (LEFT SIDE, BIGGER SLOT)
    // ========================================================================
    Trinket_t* class_trinket = GetClassTrinket(player);

    // Check if mouse is hovering over class trinket slot
    bool class_hovered = (mouse_x >= CLASS_TRINKET_X && mouse_x < CLASS_TRINKET_X + CLASS_TRINKET_SIZE &&
                          mouse_y >= CLASS_TRINKET_Y && mouse_y < CLASS_TRINKET_Y + CLASS_TRINKET_SIZE);
    if (class_hovered) {
        ui->hovered_class_trinket = true;
    }

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
        a_DrawFilledRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                       bg_r, bg_g, bg_b, 255);

        // Gold border (thicker, 2px)
        a_DrawRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                  border_r, border_g, border_b, 255);  // Gold
        a_DrawRect(shake_x + 1, shake_y + 1, CLASS_TRINKET_SIZE - 2, CLASS_TRINKET_SIZE - 2,
                  border_r, border_g, border_b, 255);  // Double border for emphasis

        // Hover glow effect (subtle overlay)
        if (class_hovered && is_clickable) {
            a_DrawFilledRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                           255, 255, 200, 30);  // Subtle yellow glow
        }

        // Draw red flash overlay when trinket procs
        if (class_trinket->flash_alpha > 0.0f) {
            Uint8 flash = (Uint8)class_trinket->flash_alpha;
            a_DrawFilledRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                           255, 0, 0, flash);
        }

        // "CLASS" label at top
        aFontConfig_t class_label_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {200, 180, 50, 255},  // Gold text
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.5f
        };
        a_DrawTextStyled("CLASS",
                       shake_x + CLASS_TRINKET_SIZE / 2,
                       shake_y + 6,
                       &class_label_config);

        // Draw trinket name
        const char* trinket_name = GetTrinketName(class_trinket);
        aFontConfig_t name_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {255, 255, 255, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = CLASS_TRINKET_SIZE - 8,
            .scale = 0.6f
        };
        a_DrawTextStyled((char*)trinket_name,
                       shake_x + CLASS_TRINKET_SIZE / 2,
                       shake_y + 24,
                       &name_config);

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
            a_DrawFilledRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                           60, 20, 20, 255);
            a_DrawRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                      255, 100, 100, 255);

            char cooldown_text[8];
            snprintf(cooldown_text, sizeof(cooldown_text), "%d", class_trinket->active_cooldown_current);

            aFontConfig_t cd_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {255, 150, 150, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 0.8f
            };
            a_DrawTextStyled(cooldown_text,
                           left_indicator_x + indicator_width / 2,
                           indicators_y + 3,
                           &cd_config);
        } else {
            // Dimmed background when ready
            a_DrawFilledRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                           30, 30, 30, 255);
            a_DrawRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                      80, 80, 80, 255);

            aFontConfig_t ready_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {100, 255, 100, 255},  // Green for ready
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 0.8f
            };
            a_DrawTextStyled("0",
                           left_indicator_x + indicator_width / 2,
                           indicators_y + 3,
                           &ready_config);
        }

        // Right indicator: Damage bonus (shows scaling bonus, not base damage)
        // Green background for damage
        a_DrawFilledRect(right_indicator_x, indicators_y, indicator_width, indicator_height,
                       20, 60, 20, 255);
        a_DrawRect(right_indicator_x, indicators_y, indicator_width, indicator_height,
                  100, 255, 100, 255);

        char bonus_text[16];
        snprintf(bonus_text, sizeof(bonus_text), "+%d", class_trinket->passive_damage_bonus);

        aFontConfig_t bonus_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {150, 255, 150, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.8f
        };
        a_DrawTextStyled(bonus_text,
                       right_indicator_x + indicator_width / 2,
                       indicators_y + 3,
                       &bonus_config);
    } else {
        // Empty class trinket slot - draw dimmed gold outline
        a_DrawRect(CLASS_TRINKET_X, CLASS_TRINKET_Y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                  100, 90, 25, 128);  // Dimmed gold
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

        // Check if mouse is hovering over this slot
        bool is_hovered = (mouse_x >= slot_x && mouse_x < slot_x + TRINKET_SLOT_SIZE &&
                          mouse_y >= slot_y && mouse_y < slot_y + TRINKET_SLOT_SIZE);
        if (is_hovered) {
            ui->hovered_trinket_slot = slot_index;
        }

        Trinket_t* trinket = GetEquippedTrinket(player, slot_index);

        if (trinket) {
            // Trinket equipped - draw with styling
            int bg_r = 40, bg_g = 40, bg_b = 50;
            int border_r = 150, border_g = 150, border_b = 200;

            // Check if on cooldown
            bool on_cooldown = trinket->active_cooldown_current > 0;
            bool is_clickable = !on_cooldown && trinket->active_target_type == TRINKET_TARGET_CARD;

            if (on_cooldown) {
                // Red tint
                bg_r = 60; bg_g = 20; bg_b = 20;
                border_r = 255; border_g = 100; border_b = 100;
            }

            // Hover effect (brighter if ready to use)
            if (is_hovered && is_clickable) {
                bg_r += 20; bg_g += 20; bg_b += 20;
                border_r = 255; border_g = 255; border_b = 100;  // Yellow border
            }

            // Apply shake offsets (tweened during proc)
            int shake_x = slot_x + (int)trinket->shake_offset_x;
            int shake_y = slot_y + (int)trinket->shake_offset_y;

            // Background
            a_DrawFilledRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                           bg_r, bg_g, bg_b, 255);

            // Border
            a_DrawRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                      border_r, border_g, border_b, 255);

            // Hover glow effect (subtle overlay)
            if (is_hovered && is_clickable) {
                a_DrawFilledRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                               255, 255, 200, 30);  // Subtle yellow glow
            }

            // Draw red flash overlay when trinket procs
            if (trinket->flash_alpha > 0.0f) {
                Uint8 flash = (Uint8)trinket->flash_alpha;
                a_DrawFilledRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                               255, 0, 0, flash);
            }

            // Draw trinket name (small, wrapped)
            const char* trinket_name = GetTrinketName(trinket);
            aFontConfig_t name_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {255, 255, 255, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = TRINKET_SLOT_SIZE - 4,
                .scale = 0.5f
            };
            a_DrawTextStyled((char*)trinket_name,
                           shake_x + TRINKET_SLOT_SIZE / 2,
                           shake_y + 6,
                           &name_config);

            // Cooldown overlay (centered countdown)
            if (on_cooldown) {
                char cd_text[8];
                snprintf(cd_text, sizeof(cd_text), "%d", trinket->active_cooldown_current);

                aFontConfig_t cd_config = {
                    .type = FONT_ENTER_COMMAND,
                    .color = {255, 150, 150, 255},
                    .align = TEXT_ALIGN_CENTER,
                    .wrap_width = 0,
                    .scale = 1.5f
                };
                a_DrawTextStyled(cd_text,
                               shake_x + TRINKET_SLOT_SIZE / 2,
                               shake_y + TRINKET_SLOT_SIZE / 2 - 8,
                               &cd_config);
            }
        } else {
            // Empty slot - draw dimmed border
            a_DrawRect(slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                      80, 80, 80, 128);  // Dimmed gray

            // "EMPTY" label
            aFontConfig_t empty_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {100, 100, 100, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = TRINKET_SLOT_SIZE - 4,
                .scale = 0.5f
            };
            a_DrawTextStyled("EMPTY",
                           slot_x + TRINKET_SLOT_SIZE / 2,
                           slot_y + TRINKET_SLOT_SIZE / 2 - 6,
                           &empty_config);
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
