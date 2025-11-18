/*
 * Dealer Section Implementation
 */

#include "../../../include/scenes/sections/dealerSection.h"
#include "../../../include/hand.h"
#include "../../../include/card.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/cardAnimation.h"
#include "../../../include/stateStorage.h"
#include "../../../include/cardTags.h"

// External globals
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;
extern GameContext_t g_game;

// ============================================================================
// HELPER FUNCTIONS (internal to this section)
// ============================================================================

/**
 * IsCardHovered - Check if mouse is over a card
 */
static bool IsCardHovered(int card_x, int card_y) {
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    return (mouse_x >= card_x && mouse_x < card_x + CARD_WIDTH &&
            mouse_y >= card_y && mouse_y < card_y + CARD_HEIGHT);
}

// ============================================================================
// DEALER SECTION LIFECYCLE
// ============================================================================

DealerSection_t* CreateDealerSection(void) {
    DealerSection_t* section = malloc(sizeof(DealerSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate DealerSection");
        return NULL;
    }

    section->layout = NULL;  // Reserved for future FlexBox refinement

    // Initialize hover state
    section->hover_state.hovered_card_index = -1;  // No card hovered
    section->hover_state.hover_amount = 0.0f;      // No hover effect

    // Initialize ability hover tracking (for tutorial)
    section->ability_hover_timer = 0.0f;
    section->abilities_hovered_count = 0;
    section->ability_tutorial_completed = false;
    for (int i = 0; i < 8; i++) {
        section->abilities_hovered[i] = false;
    }

    // Create enemy health bar (will be positioned and updated in render)
    section->enemy_hp_bar = CreateEnemyHealthBar(0, 0, NULL);
    if (!section->enemy_hp_bar) {
        free(section);
        d_LogFatal("Failed to create enemy health bar");
        return NULL;
    }

    // Create ability display (will be positioned and updated in render)
    section->ability_display = CreateAbilityDisplay(NULL);
    if (!section->ability_display) {
        DestroyEnemyHealthBar(&section->enemy_hp_bar);
        free(section);
        d_LogFatal("Failed to create ability display");
        return NULL;
    }

    // Create ability tooltip modal
    section->ability_tooltip = CreateAbilityTooltipModal();
    if (!section->ability_tooltip) {
        DestroyAbilityDisplay(&section->ability_display);
        DestroyEnemyHealthBar(&section->enemy_hp_bar);
        free(section);
        d_LogFatal("Failed to create ability tooltip modal");
        return NULL;
    }

    d_LogInfo("DealerSection created");
    return section;
}

void DestroyDealerSection(DealerSection_t** section) {
    if (!section || !*section) return;

    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);
    }

    if ((*section)->enemy_hp_bar) {
        DestroyEnemyHealthBar(&(*section)->enemy_hp_bar);
    }

    if ((*section)->ability_display) {
        DestroyAbilityDisplay(&(*section)->ability_display);
    }

    if ((*section)->ability_tooltip) {
        DestroyAbilityTooltipModal(&(*section)->ability_tooltip);
    }

    free(*section);
    *section = NULL;
    d_LogInfo("DealerSection destroyed");
}

// ============================================================================
// DEALER SECTION RENDERING
// ============================================================================

void RenderDealerSection(DealerSection_t* section, Player_t* dealer, Enemy_t* enemy, int y) {
    if (!section || !dealer) return;

    // Section bounds: y to (y + DEALER_AREA_HEIGHT)
    // Layout: [SECTION_PADDING] name_text [HP_BAR] [ELEMENT_GAP] cards

    // Check if dealer has hidden cards
    bool has_hidden_card = false;
    for (size_t i = 0; i < GetHandSize(&dealer->hand); i++) {
        const Card_t* card = GetCardFromHand(&dealer->hand, i);
        if (card && !card->face_up) {
            has_hidden_card = true;
            break;
        }
    }

    // Dealer name and score - calculated position (no magic numbers)
    dString_t* info = d_StringInit();
    if (has_hidden_card) {
        int visible_value = CalculateVisibleHandValue(&dealer->hand);
        d_StringFormat(info, "%s: %d + ?", GetPlayerName(dealer), visible_value);
    } else {
        d_StringFormat(info, "%s: %d%s",
                       GetPlayerName(dealer),
                       dealer->hand.total_value,
                       dealer->hand.is_blackjack ? " (BLACKJACK!)" :
                       dealer->hand.is_bust ? " (BUST)" : "");
    }

    // Position calculation: padding from top
    int name_y = y + SECTION_PADDING;

    // Draw dealer text (left-aligned to make room for HP bar)
    int text_x = SCREEN_WIDTH / 2 - 100;  // Offset left from center
    a_DrawText((char*)d_StringPeek(info), text_x, name_y,
               255, 100, 100, FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

    d_StringDestroy(info);

    // Render enemy health bar (centered horizontally, slightly below dealer section top)
    if (enemy && section->enemy_hp_bar) {
        // Position using constants from sceneBlackjack.h
        int bar_x = (SCREEN_WIDTH / 2) - (ENEMY_HP_BAR_WIDTH / 2) + ENEMY_HP_BAR_X_OFFSET;
        int bar_y = ENEMY_HP_BAR_Y;

        SetEnemyHealthBarEnemy(section->enemy_hp_bar, enemy);
        SetEnemyHealthBarPosition(section->enemy_hp_bar, bar_x, bar_y);
        RenderEnemyHealthBar(section->enemy_hp_bar);
    }

    // Render enemy abilities if in combat mode (VERTICAL column right of sidebar)
    if (enemy && section->ability_display && enemy->active_abilities && enemy->active_abilities->count > 0) {
        // Position: Right of sidebar (280px), starting near top bar
        int abilities_x = 290;  // SIDEBAR_WIDTH (280) + 10px margin
        int abilities_y = 70;   // Below top bar (45px) + 25px margin

        SetAbilityDisplayEnemy(section->ability_display, enemy);
        SetAbilityDisplayPosition(section->ability_display, abilities_x, abilities_y);
        RenderAbilityDisplay(section->ability_display);

        // Show tooltip modal if hovering over an ability
        const AbilityData_t* hovered_ability = GetHoveredAbilityData(section->ability_display);
        if (hovered_ability && section->ability_tooltip) {
            int card_x, card_y;
            if (GetHoveredAbilityPosition(section->ability_display, &card_x, &card_y)) {
                ShowAbilityTooltipModal(section->ability_tooltip, hovered_ability, card_x, card_y);
            }
        } else if (section->ability_tooltip) {
            HideAbilityTooltipModal(section->ability_tooltip);
        }

        // Render tooltip modal (must be after ability cards for z-ordering)
        if (section->ability_tooltip) {
            RenderAbilityTooltipModal(section->ability_tooltip);
        }
    }

    // Cards position: after padding + text + gap
    int cards_y = y + SECTION_PADDING + TEXT_LINE_HEIGHT + ELEMENT_GAP;

    // Hover detection and two-pass rendering (same as player section)
    Hand_t* hand = &dealer->hand;
    if (!hand->cards || hand->cards->count == 0) return;

    size_t hand_size = hand->cards->count;
    CardTransitionManager_t* anim_mgr = GetCardTransitionManager();
    int new_hovered_index = -1;

    // Detect which card is hovered (reverse order for z-index priority)
    for (int i = (int)hand_size - 1; i >= 0; i--) {
        const Card_t* card = GetCardFromHand(hand, (size_t)i);
        if (!card) continue;

        // Get card position
        int x, y_pos;
        CardTransition_t* trans = GetCardTransition(anim_mgr, hand, (size_t)i);
        if (trans && trans->active) {
            x = (int)trans->current_x;
            y_pos = (int)trans->current_y;
        } else {
            CalculateCardFanPosition((size_t)i, hand_size, cards_y, &x, &y_pos);
        }

        if (IsCardHovered(x, y_pos)) {
            new_hovered_index = i;
            break;  // Take first hovered (topmost in fan)
        }
    }

    // Update hover state with tween system (constitutional pattern)
    if (new_hovered_index != section->hover_state.hovered_card_index) {
        section->hover_state.hovered_card_index = new_hovered_index;

        TweenManager_t* tween_mgr = GetTweenManager();

        // Stop any existing hover tweens
        StopTweensForTarget(tween_mgr, &section->hover_state.hover_amount);

        // Start new tween
        float target_hover = (new_hovered_index >= 0) ? 1.0f : 0.0f;
        TweenFloat(
            tween_mgr,
            &section->hover_state.hover_amount,
            target_hover,                       // To target (1.0 or 0.0)
            0.15f,                              // Duration: 150ms
            TWEEN_EASE_OUT_CUBIC                // Smooth easing
        );
    }

    // Two-pass rendering: non-hovered cards first, then hovered card on top
    int hovered_index = section->hover_state.hovered_card_index;

    // Pass 1: Render all non-hovered cards
    for (size_t i = 0; i < hand_size; i++) {
        if ((int)i == hovered_index) continue;  // Skip hovered card

        const Card_t* card = GetCardFromHand(hand, i);
        if (!card) continue;

        int x, y_pos;
        CardTransition_t* trans = GetCardTransition(anim_mgr, hand, i);
        if (trans && trans->active) {
            x = (int)trans->current_x;
            y_pos = (int)trans->current_y;
        } else {
            CalculateCardFanPosition(i, hand_size, cards_y, &x, &y_pos);
        }

        SDL_Rect dst = {x, y_pos, CARD_WIDTH, CARD_HEIGHT};

        if (card->face_up && card->texture) {
            SDL_RenderCopy(app.renderer, card->texture, NULL, &dst);
        } else if (!card->face_up && g_card_back_texture) {
            SDL_RenderCopy(app.renderer, g_card_back_texture, NULL, &dst);
        } else {
            a_DrawFilledRect(x, y_pos, CARD_WIDTH, CARD_HEIGHT, 200, 200, 200, 255);
            a_DrawRect(x, y_pos, CARD_WIDTH, CARD_HEIGHT, 100, 100, 100, 255);
        }

        // Draw ace value text (1 or 11) on the card itself
        if (card->face_up && card->rank == RANK_ACE) {
            int ace_value = GetAceValue(&dealer->hand, i);
            dString_t* ace_text = d_StringInit();
            d_StringFormat(ace_text, "(%d)", ace_value);

            // Top-left corner, 20px down from top
            int text_x = x + 4;
            int text_y = y_pos + 20;

            // Lighter blue text (dodger blue)
            a_DrawText((char*)d_StringPeek(ace_text), text_x, text_y,
                      30, 144, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
            d_StringDestroy(ace_text);
        }

        // Targeting highlight overlay (only if in targeting mode AND card is face-up)
        if (g_game.current_state == STATE_TARGETING && card->face_up) {
            int targeting_trinket_slot = StateData_GetInt(&g_game.state_data, "targeting_trinket_slot", -999);

            // Check both class trinket (-1) and regular trinkets (0-5)
            if (targeting_trinket_slot >= -1 && targeting_trinket_slot < 6) {
                // Use centralized validity check from sceneBlackjack.h
                bool is_valid = IsCardValidTarget(card, targeting_trinket_slot);

                if (is_valid) {
                    // Green overlay for valid targets
                    a_DrawFilledRect(x, y_pos, CARD_WIDTH, CARD_HEIGHT, 0, 255, 0, 60);
                    a_DrawRect(x, y_pos, CARD_WIDTH, CARD_HEIGHT, 0, 255, 0, 180);
                } else {
                    // Grey overlay for invalid targets
                    a_DrawFilledRect(x, y_pos, CARD_WIDTH, CARD_HEIGHT, 100, 100, 100, 100);
                    a_DrawRect(x, y_pos, CARD_WIDTH, CARD_HEIGHT, 100, 100, 100, 180);
                }
            }
        }

        // Draw card tags in column from top-right (flexible, stackable design)
        if (card->face_up) {
            int tag_y_offset = 4;  // Start 4px from top
            const int tag_padding = 3;
            const int tag_spacing = 2;

            // DOUBLED tag
            if (HasCardTag(card->card_id, CARD_TAG_DOUBLED)) {
                const char* tag_text = "DOUBLED";

                // Measure text to get tag width (approximate: 7px per char at scale 0.6)
                int text_width = (int)(strlen(tag_text) * 7 * 0.6f);
                int tag_width = text_width + (tag_padding * 2);
                int tag_height = 16;

                // Position from top-right
                int tag_x = x + CARD_WIDTH - tag_width - 4;
                int tag_y = y_pos + tag_y_offset;

                // Gold background with dark border
                a_DrawFilledRect(tag_x, tag_y, tag_width, tag_height, 0, 0, 0, 200);  // Dark border
                a_DrawFilledRect(tag_x + 1, tag_y + 1, tag_width - 2, tag_height - 2, 255, 215, 0, 240);  // Gold fill

                // Black text, left-aligned within tag
                aFontConfig_t tag_config = {
                    .type = FONT_ENTER_COMMAND,
                    .color = {0, 0, 0, 255},
                    .align = TEXT_ALIGN_LEFT,
                    .scale = 0.6f
                };
                a_DrawTextStyled(tag_text, tag_x + tag_padding, tag_y + 2, &tag_config);

                // Move Y offset down for next tag
                tag_y_offset += tag_height + tag_spacing;
            }

            // Future tags would go here (e.g., BURNING, FROZEN, etc.)
            // Each tag increments tag_y_offset to stack vertically
        }
    }

    // Pass 2: Render hovered card with scale + lift (using Archimedes scaled blit)
    if (hovered_index >= 0 && section->hover_state.hover_amount > 0.01f) {
        const Card_t* card = GetCardFromHand(hand, (size_t)hovered_index);
        if (card) {
            int x, y_pos;
            CardTransition_t* trans = GetCardTransition(anim_mgr, hand, (size_t)hovered_index);
            if (trans && trans->active) {
                x = (int)trans->current_x;
                y_pos = (int)trans->current_y;
            } else {
                CalculateCardFanPosition((size_t)hovered_index, hand_size, cards_y, &x, &y_pos);
            }

            // Apply hover effects
            float hover_t = section->hover_state.hover_amount;
            float scale = 1.0f + (0.15f * hover_t);  // 1.0 → 1.15
            int lift = (int)(-20.0f * hover_t);      // 0 → -20px

            int scaled_width = (int)(CARD_WIDTH * scale);
            int scaled_height = (int)(CARD_HEIGHT * scale);
            int scaled_x = x - (scaled_width - CARD_WIDTH) / 2;  // Center scaling
            int scaled_y = y_pos + lift - (scaled_height - CARD_HEIGHT) / 2;

            // Use Archimedes scaled blit (constitutional pattern)
            if (card->face_up && card->texture) {
                a_BlitTextureScaled(card->texture, scaled_x, scaled_y, scaled_width, scaled_height);
            } else if (!card->face_up && g_card_back_texture) {
                a_BlitTextureScaled(g_card_back_texture, scaled_x, scaled_y, scaled_width, scaled_height);
            } else {
                a_DrawFilledRect(scaled_x, scaled_y, scaled_width, scaled_height, 200, 200, 200, 255);
                a_DrawRect(scaled_x, scaled_y, scaled_width, scaled_height, 100, 100, 100, 255);
            }

            // Draw ace value text (1 or 11) on hovered card
            if (card->face_up && card->rank == RANK_ACE) {
                int ace_value = GetAceValue(&dealer->hand, (size_t)hovered_index);
                dString_t* ace_text = d_StringInit();
                d_StringFormat(ace_text, "(%d)", ace_value);

                // Top-left corner, 20px down from top (scaled)
                int text_x = scaled_x + (int)(4 * scale);
                int text_y = scaled_y + (int)(20 * scale);

                // Lighter blue text (dodger blue)
                a_DrawText((char*)d_StringPeek(ace_text), text_x, text_y,
                          30, 144, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
                d_StringDestroy(ace_text);
            }

            // ADR-11: Draw targeting highlight on hovered dealer card (bright border shows selection preview)
            if (g_game.current_state == STATE_TARGETING) {
                int trinket_slot = StateData_GetInt(&g_game.state_data, "targeting_trinket_slot", -999);
                if (trinket_slot >= -1 && trinket_slot < 6) {
                    // Only show targeting highlight on face-up cards
                    if (card->face_up) {
                        bool is_valid = IsCardValidTarget(card, trinket_slot);

                        // Draw bright border to indicate this card will be selected on click
                        int border_thickness = (int)(3 * scale);  // Thicker border for visibility
                        if (is_valid) {
                            // Bright green border (valid target + hovered = selectable)
                            for (int i = 0; i < border_thickness; i++) {
                                a_DrawRect(scaled_x - i, scaled_y - i,
                                          scaled_width + (i * 2), scaled_height + (i * 2),
                                          0, 255, 0, 255 - (i * 40));  // Fade outer layers
                            }
                        } else {
                            // Red border (invalid target + hovered = not selectable)
                            for (int i = 0; i < border_thickness; i++) {
                                a_DrawRect(scaled_x - i, scaled_y - i,
                                          scaled_width + (i * 2), scaled_height + (i * 2),
                                          255, 50, 50, 255 - (i * 40));  // Fade outer layers
                            }
                        }
                    }
                }
            }
        }
    }
}

// ============================================================================
// HOVER TRACKING (for tutorial)
// ============================================================================

bool UpdateDealerAbilityHoverTracking(DealerSection_t* section, Enemy_t* enemy, float dt) {
    if (!section || !enemy || !enemy->active_abilities) return false;

    // If tutorial event already triggered, don't trigger again
    if (section->ability_tutorial_completed) return false;

    size_t ability_count = enemy->active_abilities->count;
    if (ability_count == 0) return false;

    // Check if hovering over any ability
    if (section->ability_display && section->ability_display->hovered_index >= 0) {
        int hovered_idx = section->ability_display->hovered_index;

        // Update timer
        section->ability_hover_timer += dt;

        // After 0.3 seconds of hovering, mark this ability as hovered
        if (section->ability_hover_timer >= 0.3f && hovered_idx < 8) {
            if (!section->abilities_hovered[hovered_idx]) {
                section->abilities_hovered[hovered_idx] = true;
                section->abilities_hovered_count++;
                d_LogInfoF("Tutorial: Ability %d hovered (%d/%zu abilities total)",
                          hovered_idx, section->abilities_hovered_count, ability_count);
            }
            section->ability_hover_timer = 0.0f;  // Reset for next ability
        }
    } else {
        // Reset timer if not hovering
        section->ability_hover_timer = 0.0f;
    }

    // Check if all abilities have been hovered
    if (section->abilities_hovered_count >= (int)ability_count) {
        section->ability_tutorial_completed = true;  // Mark as completed
        return true;  // Trigger event once
    }

    return false;
}
