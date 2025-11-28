/*
 * Player Section Component Implementation
 */

#include "../../../include/scenes/sections/playerSection.h"
#include "../../../include/hand.h"
#include "../../../include/card.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/cardAnimation.h"
#include "../../../include/trinket.h"
#include "../../../include/stateStorage.h"
#include "../../../include/cardTags.h"

// External globals
extern dTable_t* g_players;
extern dTable_t* g_card_textures;
extern SDL_Surface* g_card_back_texture;
extern GameContext_t g_game;  // For checking targeting state

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
// PLAYER SECTION LIFECYCLE
// ============================================================================

PlayerSection_t* CreatePlayerSection(DeckButton_t** deck_buttons, Deck_t* deck) {
    (void)deck_buttons;  // DEPRECATED: Now handled by LeftSidebarSection
    (void)deck;          // DEPRECATED: Now handled by LeftSidebarSection

    PlayerSection_t* section = malloc(sizeof(PlayerSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate PlayerSection");
        return NULL;
    }

    section->layout = NULL;  // Reserved for future FlexBox refinement

    // Initialize hover state
    section->hover_state.hovered_card_index = -1;  // No card hovered
    section->hover_state.hover_amount = 0.0f;      // No hover effect

    // Create tooltip for hand cards
    section->tooltip = CreateCardTooltipModal();
    if (!section->tooltip) {
        free(section);
        d_LogError("Failed to create CardTooltipModal for player section");
        return NULL;
    }

    d_LogInfo("PlayerSection created (deck panel removed)");
    return section;
}

void DestroyPlayerSection(PlayerSection_t** section) {
    if (!section || !*section) return;

    if ((*section)->layout) {
        a_FlexBoxDestroy(&(*section)->layout);
    }

    // Destroy tooltip
    if ((*section)->tooltip) {
        DestroyCardTooltipModal(&(*section)->tooltip);
    }

    free(*section);
    *section = NULL;
    d_LogInfo("PlayerSection destroyed");
}

// ============================================================================
// PLAYER SECTION RENDERING
// ============================================================================

void RenderPlayerSection(PlayerSection_t* section, Player_t* player, int y) {
    if (!section || !player) return;

    // Section bounds: y to (y + PLAYER_AREA_HEIGHT)
    // Layout: [SECTION_PADDING] name_text [ELEMENT_GAP] cards
    // NOTE: Chips/bet info now in LeftSidebarSection

    // Player name and score - centered in game area
    dString_t* info = d_StringInit();
    d_StringFormat(info, "%s: %d%s",
                   GetPlayerName(player),
                   player->hand.total_value,
                   player->hand.is_blackjack ? " (BLACKJACK!)" :
                   player->hand.is_bust ? " (BUST)" : "");

    int name_y = y + SECTION_PADDING;
    int center_x = GAME_AREA_X + (GAME_AREA_WIDTH / 2);

    // Draw main score (yellow)
    a_DrawText((char*)d_StringPeek(info), center_x, name_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,0,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    d_StringDestroy(info);

    // Cards position: after name + gap
    int cards_y = name_y + TEXT_LINE_HEIGHT + ELEMENT_GAP;

    // Hover detection and state management
    Hand_t* hand = &player->hand;
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
        int x, y;
        CardTransition_t* trans = GetCardTransition(anim_mgr, hand, (size_t)i);
        if (trans && trans->active) {
            x = (int)trans->current_x;
            y = (int)trans->current_y;
        } else {
            CalculateCardFanPosition((size_t)i, hand_size, cards_y, &x, &y);
        }

        if (IsCardHovered(x, y)) {
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

        int x, y;
        CardTransition_t* trans = GetCardTransition(anim_mgr, hand, i);
        if (trans && trans->active) {
            x = (int)trans->current_x;
            y = (int)trans->current_y;
        } else {
            CalculateCardFanPosition(i, hand_size, cards_y, &x, &y);
        }

        if (card->face_up && card->texture) {
            a_BlitSurfaceRect(card->texture, (aRectf_t){x, y, CARD_WIDTH, CARD_HEIGHT}, 1);
        } else if (!card->face_up && g_card_back_texture) {
            a_BlitSurfaceRect(g_card_back_texture, (aRectf_t){x, y, CARD_WIDTH, CARD_HEIGHT}, 1);
        } else {
            a_DrawFilledRect((aRectf_t){x, y, CARD_WIDTH, CARD_HEIGHT}, (aColor_t){200, 200, 200, 255});
            a_DrawRect((aRectf_t){x, y, CARD_WIDTH, CARD_HEIGHT}, (aColor_t){100, 100, 100, 255});
        }

        // Draw ace value text (1 or 11) on the card itself
        if (card->face_up && card->rank == RANK_ACE) {
            int ace_value = GetAceValue(hand, i);
            dString_t* ace_text = d_StringInit();
            d_StringFormat(ace_text, "(%d)", ace_value);

            // Top-left corner, 20px down from top
            int text_x = x + 4;
            int text_y = y + 20;

            // Lighter blue text (dodger blue)
            a_DrawText((char*)d_StringPeek(ace_text), text_x, text_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={30,144,255,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
            d_StringDestroy(ace_text);
        }

        // Draw targeting highlight overlay (if in targeting mode)
        if (g_game.current_state == STATE_TARGETING) {
            // Get active trinket slot
            int trinket_slot = StateData_GetInt(&g_game.state_data, "targeting_trinket_slot", -999);

            // Check both class trinket (-1) and regular trinkets (0-5)
            if (trinket_slot >= -1 && trinket_slot < 6) {
                // Use centralized validity check from sceneBlackjack.h
                bool is_valid = IsCardValidTarget(card, trinket_slot);

                if (is_valid) {
                    // Green overlay for valid targets
                    a_DrawFilledRect((aRectf_t){x, y, CARD_WIDTH, CARD_HEIGHT}, (aColor_t){0, 255, 0, 80});
                } else {
                    // Dimmed overlay for invalid targets
                    a_DrawFilledRect((aRectf_t){x, y, CARD_WIDTH, CARD_HEIGHT}, (aColor_t){128, 128, 128, 80});
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
                    CardTag_t* tag = (CardTag_t*)d_ArrayGet((dArray_t*)tags, t);
                    if (!tag) continue;

                    // Position from top-right, offset by 12px right and 24px down
                    int badge_x = x + CARD_WIDTH - badge_w - 3 + 12;
                    int badge_y = y - badge_h - 3 + 24 + ((int)t * (badge_h + 2));  // Stack vertically

                    // Get tag color
                    int r, g, b;
                    GetCardTagColor(*tag, &r, &g, &b);
                    a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){r, g, b, 255});
                    a_DrawRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){0, 0, 0, 255});

                    // Truncate tag text to first 3 letters
                    const char* full_tag_text = GetCardTagName(*tag);
                    char truncated[4] = {0};
                    strncpy(truncated, full_tag_text, 3);
                    truncated[3] = '\0';

                    // Black text, centered
                    aTextStyle_t tag_config = {
                        .type = FONT_ENTER_COMMAND,
                        .fg = {0, 0, 0, 255},
                        .align = TEXT_ALIGN_CENTER,
                        .scale = 0.7f
                    };
                    a_DrawText(truncated, badge_x + badge_w / 2, badge_y - 3, tag_config);
                }
            }
        }
    }

    // Pass 2: Render hovered card with scale + lift (using Archimedes scaled blit)
    if (hovered_index >= 0 && section->hover_state.hover_amount > 0.01f) {
        const Card_t* card = GetCardFromHand(hand, (size_t)hovered_index);
        if (card) {
            int x, y;
            CardTransition_t* trans = GetCardTransition(anim_mgr, hand, (size_t)hovered_index);
            if (trans && trans->active) {
                x = (int)trans->current_x;
                y = (int)trans->current_y;
            } else {
                CalculateCardFanPosition((size_t)hovered_index, hand_size, cards_y, &x, &y);
            }

            // Apply hover effects
            float hover_t = section->hover_state.hover_amount;
            float scale = 1.0f + (0.15f * hover_t);  // 1.0 → 1.15
            int lift = (int)(-20.0f * hover_t);      // 0 → -20px

            int scaled_width = (int)(CARD_WIDTH * scale);
            int scaled_height = (int)(CARD_HEIGHT * scale);
            int scaled_x = x - (scaled_width - CARD_WIDTH) / 2;  // Center scaling
            int scaled_y = y + lift - (scaled_height - CARD_HEIGHT) / 2;

            // Use Archimedes surface blitting
            if (card->face_up && card->texture) {
                a_BlitSurfaceRect(card->texture, (aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, 1);
            } else if (!card->face_up && g_card_back_texture) {
                a_BlitSurfaceRect(g_card_back_texture, (aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, 1);
            } else {
                a_DrawFilledRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, (aColor_t){200, 200, 200, 255});
                a_DrawRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, (aColor_t){100, 100, 100, 255});
            }

            // Draw ace value text (1 or 11) on hovered card
            if (card->face_up && card->rank == RANK_ACE) {
                int ace_value = GetAceValue(hand, (size_t)hovered_index);
                dString_t* ace_text = d_StringInit();
                d_StringFormat(ace_text, "(%d)", ace_value);

                // Top-left corner, 20px down from top (scaled)
                int text_x = scaled_x + (int)(4 * scale);
                int text_y = scaled_y + (int)(20 * scale);

                // Lighter blue text (dodger blue)
                a_DrawText((char*)d_StringPeek(ace_text), text_x, text_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={30,144,255,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
                d_StringDestroy(ace_text);
            }

            // ADR-11: Draw targeting highlight on hovered card (bright border shows selection preview)
            if (g_game.current_state == STATE_TARGETING) {
                int trinket_slot = StateData_GetInt(&g_game.state_data, "targeting_trinket_slot", -999);
                if (trinket_slot >= -1 && trinket_slot < 6) {
                    bool is_valid = IsCardValidTarget(card, trinket_slot);

                    // Draw bright border to indicate this card will be selected on click
                    int border_thickness = (int)(3 * scale);  // Thicker border for visibility
                    if (is_valid) {
                        // Bright green border (valid target + hovered = selectable)
                        for (int i = 0; i < border_thickness; i++) {
                            a_DrawRect((aRectf_t){scaled_x - i, scaled_y - i, scaled_width + (i * 2), scaled_height + (i * 2)},
                                      (aColor_t){0, 255, 0, 255 - (i * 40)});  // Fade outer layers
                        }
                    } else {
                        // Red border (invalid target + hovered = not selectable)
                        for (int i = 0; i < border_thickness; i++) {
                            a_DrawRect((aRectf_t){scaled_x - i, scaled_y - i, scaled_width + (i * 2), scaled_height + (i * 2)},
                                      (aColor_t){255, 50, 50, 255 - (i * 40)});  // Fade outer layers
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
                        CardTag_t* tag = (CardTag_t*)d_ArrayGet((dArray_t*)tags, t);
                        if (!tag) continue;

                        // Position from top-right, offset by 12px right and 24px down (scaled)
                        int badge_x = scaled_x + scaled_width - badge_w - (int)(3 * scale) + (int)(12 * scale);
                        int badge_y = scaled_y - badge_h - (int)(3 * scale) + (int)(24 * scale) + ((int)t * (badge_h + (int)(2 * scale)));

                        // Get tag color
                        int r, g, b;
                        GetCardTagColor(*tag, &r, &g, &b);
                        a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){r, g, b, 255});
                        a_DrawRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){0, 0, 0, 255});

                        // Truncate tag text to first 3 letters
                        const char* full_tag_text = GetCardTagName(*tag);
                        char truncated[4] = {0};
                        strncpy(truncated, full_tag_text, 3);
                        truncated[3] = '\0';

                        // Black text, centered
                        aTextStyle_t tag_config = {
                            .type = FONT_ENTER_COMMAND,
                            .fg = {0, 0, 0, 255},
                            .align = TEXT_ALIGN_CENTER,
                            .scale = 0.7f * scale
                        };
                        a_DrawText(truncated, badge_x + badge_w / 2, badge_y + (int)(3 * scale) - 8, tag_config);
                    }
                }
            }

            // Show tooltip for hovered card (positioned at scaled card location)
            // NOTE: Tooltip is shown but NOT rendered here - rendering happens separately
            // in RenderPlayerSectionTooltip() for proper z-ordering above trinket menu
            if (section->tooltip) {
                ShowCardTooltipModal(section->tooltip, card, scaled_x, scaled_y);
            }
        }
    }

    // Hide tooltip if no card is hovered
    if (hovered_index < 0 && section->tooltip) {
        HideCardTooltipModal(section->tooltip);
    }
}

void RenderPlayerSectionTooltip(PlayerSection_t* section) {
    if (!section || !section->tooltip) return;

    // Render tooltip on top of everything (including trinket menu)
    RenderCardTooltipModal(section->tooltip);
}
