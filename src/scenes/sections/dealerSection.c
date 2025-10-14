/*
 * Dealer Section Implementation
 */

#include "../../../include/scenes/sections/dealerSection.h"
#include "../../../include/hand.h"
#include "../../../include/card.h"
#include "../../../include/scenes/sceneBlackjack.h"

// External globals
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;

// ============================================================================
// HELPER FUNCTIONS (internal to this section)
// ============================================================================

/**
 * CalculateCenteredHandX - Calculate X position to center a hand of cards
 */
static int CalculateCenteredHandX(size_t hand_size) {
    if (hand_size == 0) return SCREEN_WIDTH / 2;

    int total_width = (hand_size * CARD_WIDTH) + ((hand_size - 1) * CARD_SPACING);
    return (SCREEN_WIDTH / 2) - (total_width / 2);
}

/**
 * RenderHand - Render cards centered on screen
 */
static void RenderHand(const Hand_t* hand, int start_y) {
    if (!hand || !hand->cards || hand->cards->count == 0) {
        return;
    }

    int start_x = CalculateCenteredHandX(hand->cards->count);

    for (size_t i = 0; i < hand->cards->count; i++) {
        const Card_t* card = GetCardFromHand(hand, i);
        if (!card) continue;

        int x = start_x + (i * CARD_SPACING);
        int y = start_y;

        SDL_Rect dst = {x, y, CARD_WIDTH, CARD_HEIGHT};

        if (card->face_up && card->texture) {
            SDL_RenderCopy(app.renderer, card->texture, NULL, &dst);
        } else if (!card->face_up && g_card_back_texture) {
            SDL_RenderCopy(app.renderer, g_card_back_texture, NULL, &dst);
        } else {
            // Fallback
            a_DrawFilledRect(x, y, CARD_WIDTH, CARD_HEIGHT, 200, 200, 200, 255);
            a_DrawRect(x, y, CARD_WIDTH, CARD_HEIGHT, 100, 100, 100, 255);
        }
    }
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

    d_LogInfo("DealerSection created");
    return section;
}

void DestroyDealerSection(DealerSection_t** section) {
    if (!section || !*section) return;

    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);
    }

    free(*section);
    *section = NULL;
    d_LogInfo("DealerSection destroyed");
}

// ============================================================================
// DEALER SECTION RENDERING
// ============================================================================

void RenderDealerSection(DealerSection_t* section, Player_t* dealer, int y) {
    if (!section || !dealer) return;

    // Section bounds: y to (y + DEALER_AREA_HEIGHT)
    // Layout: [SECTION_PADDING] name_text [ELEMENT_GAP] cards

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
    dString_t* info = d_InitString();
    if (has_hidden_card) {
        int visible_value = CalculateVisibleHandValue(&dealer->hand);
        d_FormatString(info, "%s: %d + ?", GetPlayerName(dealer), visible_value);
    } else {
        d_FormatString(info, "%s: %d%s",
                       GetPlayerName(dealer),
                       dealer->hand.total_value,
                       dealer->hand.is_blackjack ? " (BLACKJACK!)" :
                       dealer->hand.is_bust ? " (BUST)" : "");
    }

    // Position calculation: padding from top
    int name_y = y + SECTION_PADDING;

    a_DrawText((char*)d_PeekString(info), SCREEN_WIDTH / 2, name_y,
               255, 100, 100, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_DestroyString(info);

    // Cards position: after padding + text + gap
    // Math: y + 10 (padding) + 20 (text height) + 10 (gap) = y + 40
    int cards_y = y + SECTION_PADDING + TEXT_LINE_HEIGHT + ELEMENT_GAP;
    RenderHand(&dealer->hand, cards_y);
}
