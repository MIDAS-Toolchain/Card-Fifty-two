/*
 * Dealer Section Implementation
 */

#include "../../../include/scenes/sections/dealerSection.h"
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

    // Create enemy health bar (will be positioned and updated in render)
    section->enemy_hp_bar = CreateEnemyHealthBar(0, 0, NULL);
    if (!section->enemy_hp_bar) {
        free(section);
        d_LogFatal("Failed to create enemy health bar");
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
        }
    }
}
