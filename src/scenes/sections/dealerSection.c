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

    // Create card tooltip modal
    section->card_tooltip = CreateCardTooltipModal();
    if (!section->card_tooltip) {
        DestroyAbilityTooltipModal(&section->ability_tooltip);
        DestroyAbilityDisplay(&section->ability_display);
        DestroyEnemyHealthBar(&section->enemy_hp_bar);
        free(section);
        d_LogFatal("Failed to create card tooltip modal");
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

    if ((*section)->card_tooltip) {
        DestroyCardTooltipModal(&(*section)->card_tooltip);
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

        // Only allow hovering face-up cards
        if (!card->face_up) continue;

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

        // Draw card tags in column from top-right (universal tag system)
        if (card->face_up) {
            const dArray_t* tags = GetCardTags(card->card_id);

            if (tags && tags->count > 0) {
                const int badge_w = 64;  // Fixed width (prevents covering card rank)
                const int badge_h = 25;

                for (size_t t = 0; t < tags->count; t++) {
                    CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray((dArray_t*)tags, t);
                    if (!tag) continue;

                    // Position from top-right, offset by 12px right and 24px down
                    int badge_x = x + CARD_WIDTH - badge_w - 3 + 12;
                    int badge_y = y_pos - badge_h - 3 + 24 + ((int)t * (badge_h + 2));  // Stack vertically

                    // Get tag color
                    int r, g, b;
                    GetCardTagColor(*tag, &r, &g, &b);
                    a_DrawFilledRect(badge_x, badge_y, badge_w, badge_h, r, g, b, 255);
                    a_DrawRect(badge_x, badge_y, badge_w, badge_h, 0, 0, 0, 255);

                    // Truncate tag text to first 3 letters
                    const char* full_tag_text = GetCardTagName(*tag);
                    char truncated[4] = {0};
                    strncpy(truncated, full_tag_text, 3);
                    truncated[3] = '\0';

                    // Black text, centered
                    aFontConfig_t tag_config = {
                        .type = FONT_ENTER_COMMAND,
                        .color = {0, 0, 0, 255},
                        .align = TEXT_ALIGN_CENTER,
                        .scale = 0.7f
                    };
                    a_DrawTextStyled(truncated, badge_x + badge_w / 2, badge_y + 3, &tag_config);
                }
            }
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

            // Draw card tags on hovered card (scaled, universal tag system)
            if (card->face_up) {
                const dArray_t* tags = GetCardTags(card->card_id);

                if (tags && tags->count > 0) {
                    const int badge_w = (int)(64 * scale);  // Scaled width
                    const int badge_h = (int)(25 * scale);

                    for (size_t t = 0; t < tags->count; t++) {
                        CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray((dArray_t*)tags, t);
                        if (!tag) continue;

                        // Position from top-right, offset by 12px right and 24px down (scaled)
                        int badge_x = scaled_x + scaled_width - badge_w - (int)(3 * scale) + (int)(12 * scale);
                        int badge_y = scaled_y - badge_h - (int)(3 * scale) + (int)(24 * scale) + ((int)t * (badge_h + (int)(2 * scale)));

                        // Get tag color
                        int r, g, b;
                        GetCardTagColor(*tag, &r, &g, &b);
                        a_DrawFilledRect(badge_x, badge_y, badge_w, badge_h, r, g, b, 255);
                        a_DrawRect(badge_x, badge_y, badge_w, badge_h, 0, 0, 0, 255);

                        // Truncate tag text to first 3 letters
                        const char* full_tag_text = GetCardTagName(*tag);
                        char truncated[4] = {0};
                        strncpy(truncated, full_tag_text, 3);
                        truncated[3] = '\0';

                        // Black text, centered
                        aFontConfig_t tag_config = {
                            .type = FONT_ENTER_COMMAND,
                            .color = {0, 0, 0, 255},
                            .align = TEXT_ALIGN_CENTER,
                            .scale = 0.7f * scale
                        };
                        a_DrawTextStyled(truncated, badge_x + badge_w / 2, badge_y + (int)(3 * scale), &tag_config);
                    }
                }
            }

            // Show tooltip for hovered card (positioned at scaled card location)
            // NOTE: Tooltip is shown but NOT rendered here - rendering happens separately
            // in RenderDealerSectionTooltip() for proper z-ordering above trinket menu
            if (section->card_tooltip) {
                ShowCardTooltipModal(section->card_tooltip, card, scaled_x, scaled_y);
            }
        }
    }

    // Hide tooltip if no card is hovered
    if (hovered_index < 0 && section->card_tooltip) {
        HideCardTooltipModal(section->card_tooltip);
    }
}

void RenderDealerSectionTooltip(DealerSection_t* section) {
    if (!section || !section->card_tooltip) return;

    // Render tooltip on top of everything (including trinket menu)
    RenderCardTooltipModal(section->card_tooltip);
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

    // Check if hovering over the middle ability (index 1)
    if (section->ability_display && section->ability_display->hovered_index == 1) {
        // Update timer
        section->ability_hover_timer += dt;

        // After 0.3 seconds of hovering the middle ability, trigger tutorial
        if (section->ability_hover_timer >= 0.3f) {
            d_LogInfo("Tutorial: Middle ability (index 1) hovered for 0.3s");
            section->ability_tutorial_completed = true;  // Mark as completed
            section->ability_hover_timer = 0.0f;
            return true;  // Trigger event once
        }
    } else {
        // Reset timer if not hovering middle ability
        section->ability_hover_timer = 0.0f;
    }

    return false;
}
