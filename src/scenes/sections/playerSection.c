/*
 * Player Section Component Implementation
 */

#include "../../../include/scenes/sections/playerSection.h"
#include "../../../include/hand.h"
#include "../../../include/card.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/cardAnimation.h"

// External globals
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;

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
        }
    }
}
