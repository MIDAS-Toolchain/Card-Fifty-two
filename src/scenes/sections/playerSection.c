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
extern SDL_Texture* g_card_back_texture;
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

    d_LogInfo("PlayerSection created (deck panel removed)");
    return section;
}

void DestroyPlayerSection(PlayerSection_t** section) {
    if (!section || !*section) return;

    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);
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
               255, 255, 0, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

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

        SDL_Rect dst = {x, y, CARD_WIDTH, CARD_HEIGHT};

        if (card->face_up && card->texture) {
            SDL_RenderCopy(app.renderer, card->texture, NULL, &dst);
        } else if (!card->face_up && g_card_back_texture) {
            SDL_RenderCopy(app.renderer, g_card_back_texture, NULL, &dst);
        } else {
            a_DrawFilledRect(x, y, CARD_WIDTH, CARD_HEIGHT, 200, 200, 200, 255);
            a_DrawRect(x, y, CARD_WIDTH, CARD_HEIGHT, 100, 100, 100, 255);
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
                      30, 144, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
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
                    a_DrawFilledRect(x, y, CARD_WIDTH, CARD_HEIGHT, 0, 255, 0, 80);
                } else {
                    // Dimmed overlay for invalid targets
                    a_DrawFilledRect(x, y, CARD_WIDTH, CARD_HEIGHT, 128, 128, 128, 80);
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
                int tag_y = y + tag_y_offset;

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
                int ace_value = GetAceValue(hand, (size_t)hovered_index);
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

            // Draw card tags on hovered card (scaled, column from top-right)
            if (card->face_up) {
                int tag_y_offset = (int)(4 * scale);
                const int tag_padding = (int)(3 * scale);
                const int tag_spacing = (int)(2 * scale);

                // DOUBLED tag
                if (HasCardTag(card->card_id, CARD_TAG_DOUBLED)) {
                    const char* tag_text = "DOUBLED";

                    // Measure text (scaled)
                    int text_width = (int)(strlen(tag_text) * 7 * 0.6f * scale);
                    int tag_width = text_width + (tag_padding * 2);
                    int tag_height = (int)(16 * scale);

                    // Position from top-right (scaled)
                    int tag_x = scaled_x + scaled_width - tag_width - (int)(4 * scale);
                    int tag_y = scaled_y + tag_y_offset;

                    // Gold background with dark border
                    a_DrawFilledRect(tag_x, tag_y, tag_width, tag_height, 0, 0, 0, 200);
                    a_DrawFilledRect(tag_x + 1, tag_y + 1, tag_width - 2, tag_height - 2, 255, 215, 0, 240);

                    // Black text, left-aligned
                    aFontConfig_t tag_config = {
                        .type = FONT_ENTER_COMMAND,
                        .color = {0, 0, 0, 255},
                        .align = TEXT_ALIGN_LEFT,
                        .scale = 0.6f * scale
                    };
                    a_DrawTextStyled(tag_text, tag_x + tag_padding, tag_y + (int)(2 * scale), &tag_config);

                    tag_y_offset += tag_height + tag_spacing;
                }
            }
        }
    }
}
