/*
 * Player Section Component Implementation
 */

#include "../../../include/scenes/sections/playerSection.h"
#include "../../../include/scenes/sections/deckViewPanel.h"
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
// PLAYER SECTION LIFECYCLE
// ============================================================================

PlayerSection_t* CreatePlayerSection(DeckButton_t** deck_buttons, Deck_t* deck) {
    PlayerSection_t* section = malloc(sizeof(PlayerSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate PlayerSection");
        return NULL;
    }

    section->layout = NULL;  // Reserved for future FlexBox refinement

    // Create deck view panel
    section->deck_panel = CreateDeckViewPanel(deck_buttons, NUM_DECK_BUTTONS, deck);
    if (!section->deck_panel) {
        d_LogError("Failed to create DeckViewPanel");
        free(section);
        return NULL;
    }

    d_LogInfo("PlayerSection created");
    return section;
}

void DestroyPlayerSection(PlayerSection_t** section) {
    if (!section || !*section) return;

    // Destroy deck panel
    if ((*section)->deck_panel) {
        DestroyDeckViewPanel(&(*section)->deck_panel);
    }

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
    // Layout: [SECTION_PADDING] name_text [ELEMENT_GAP] chips_text [ELEMENT_GAP] cards

    // Player name and score - calculated position (no magic numbers)
    dString_t* info = d_InitString();
    d_FormatString(info, "%s: %d%s",
                   GetPlayerName(player),
                   player->hand.total_value,
                   player->hand.is_blackjack ? " (BLACKJACK!)" :
                   player->hand.is_bust ? " (BUST)" : "");

    // Position calculation: padding from top
    int name_y = y + SECTION_PADDING;

    // Cast safe: a_DrawText is read-only, dString_t used for dynamic formatting
    a_DrawText((char*)d_PeekString(info), SCREEN_WIDTH / 2, name_y,
               255, 255, 0, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_DestroyString(info);

    // Chips position: after padding + name_text + gap
    // Math: y + 10 (padding) + 20 (name height) + 10 (gap) = y + 40
    int chips_y = name_y + TEXT_LINE_HEIGHT + ELEMENT_GAP;

    dString_t* chips_info = d_InitString();
    d_FormatString(chips_info, "Chips: %d | Bet: %d",
                   GetPlayerChips(player), player->current_bet);
    // Cast safe: a_DrawText is read-only, dString_t used for dynamic formatting
    a_DrawText((char*)d_PeekString(chips_info), SCREEN_WIDTH / 2, chips_y,
               200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_DestroyString(chips_info);

    // Cards position: after padding + name_text + gap + chips_text + gap
    // Math: y + 10 + 20 + 10 + 20 + 10 = y + 70
    int cards_y = chips_y + TEXT_LINE_HEIGHT + ELEMENT_GAP;
    RenderHand(&player->hand, cards_y);

    // Render deck view panel at bottom of player section
    // Position it below the cards with some spacing
    if (section->deck_panel) {
        int panel_y = cards_y + CARD_HEIGHT / 2 + ELEMENT_GAP;
        RenderDeckViewPanel(section->deck_panel, panel_y);
    }
}
