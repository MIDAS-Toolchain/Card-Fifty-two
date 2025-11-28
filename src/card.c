/*
 * Card Fifty-Two - Card System Implementation
 * Constitutional pattern: Card_t as value type, Daedalus for collections
 */

#include "card.h"

// ============================================================================
// CARD CREATION
// ============================================================================

Card_t CreateCard(CardSuit_t suit, CardRank_t rank) {
    // Zero all bytes including padding to ensure deterministic struct contents
    Card_t card;
    memset(&card, 0, sizeof(Card_t));

    // Set only non-zero fields
    card.card_id = CardToID(suit, rank);
    card.suit = suit;
    card.rank = rank;
    // texture = NULL (already 0 from memset)
    // x, y = 0 (already 0 from memset)
    // face_up = false (already 0 from memset)
    // _padding = {0} (already 0 from memset)

    return card;  // Return by value (constitutional pattern)
}

// ============================================================================
// CARD ID SYSTEM
// ============================================================================

int CardToID(CardSuit_t suit, CardRank_t rank) {
    // Formula: card_id = (suit * 13) + (rank - 1)
    // Range: [0, 51] for standard 52-card deck
    return (suit * 13) + (rank - 1);
}

void IDToCard(int card_id, CardSuit_t* suit, CardRank_t* rank) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!suit || !rank) {
        d_LogError("IDToCard: NULL pointer passed");
        return;
    }

    // Reverse formula:
    // suit = card_id / 13
    // rank = (card_id % 13) + 1
    *suit = card_id / 13;
    *rank = (card_id % 13) + 1;
}

// ============================================================================
// CARD COMPARISON
// ============================================================================

bool CardsEqual(const Card_t* a, const Card_t* b) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!a || !b) {
        d_LogError("CardsEqual: NULL pointer passed");
        return false;
    }

    // Identity check: Only compare card_id (ignores position, face_up)
    return a->card_id == b->card_id;
}

// ============================================================================
// CARD UTILITIES
// ============================================================================

const char* GetSuitString(CardSuit_t suit) {
    switch (suit) {
        case SUIT_HEARTS:   return "Hearts";
        case SUIT_DIAMONDS: return "Diamonds";
        case SUIT_SPADES:   return "Spades";
        case SUIT_CLUBS:    return "Clubs";
        default:            return "Unknown";
    }
}

const char* GetRankString(CardRank_t rank) {
    switch (rank) {
        case RANK_ACE:   return "Ace";
        case RANK_TWO:   return "2";
        case RANK_THREE: return "3";
        case RANK_FOUR:  return "4";
        case RANK_FIVE:  return "5";
        case RANK_SIX:   return "6";
        case RANK_SEVEN: return "7";
        case RANK_EIGHT: return "8";
        case RANK_NINE:  return "9";
        case RANK_TEN:   return "10";
        case RANK_JACK:  return "Jack";
        case RANK_QUEEN: return "Queen";
        case RANK_KING:  return "King";
        default:         return "Unknown";
    }
}

void CardToString(const Card_t* card, dString_t* out) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!card || !out) {
        d_LogError("CardToString: NULL pointer passed");
        return;
    }

    // Format: "Rank of Suit" (e.g., "Ace of Hearts")
    // Constitutional pattern: dString_t, not snprintf
    d_StringFormat(out, "%s of %s",
                   GetRankString(card->rank),
                   GetSuitString(card->suit));
}

void LoadCardTexture(Card_t* card) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!card) {
        d_LogError("LoadCardTexture: NULL pointer passed");
        return;
    }

    // Constitutional pattern: Use global g_card_textures table
    SDL_Texture** tex_ptr = (SDL_Texture**)d_TableGet(
        g_card_textures,
        &card->card_id
    );

    if (tex_ptr && *tex_ptr) {
        card->texture = *tex_ptr;
        d_LogInfoF("LoadCardTexture: Loaded texture %p for card_id %d (%s of %s)",
                   (void*)card->texture, card->card_id,
                   GetRankString(card->rank), GetSuitString(card->suit));
    } else {
        // Texture not in cache - log error
        d_LogErrorF("LoadCardTexture: No texture for card_id %d (%s of %s) - tex_ptr=%p",
                    card->card_id,
                    GetRankString(card->rank),
                    GetSuitString(card->suit),
                    (void*)tex_ptr);
        card->texture = NULL;
    }
}
