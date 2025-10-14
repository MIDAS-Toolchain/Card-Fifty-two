/*
 * Card Fifty-Two - Deck Management Implementation
 * Constitutional pattern: dArray_t for cards, Fisher-Yates shuffle
 */

#include "deck.h"
#include <stdlib.h>
#include <time.h>

// ============================================================================
// DECK LIFECYCLE
// ============================================================================

void InitDeck(Deck_t* deck, int num_decks) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!deck) {
        d_LogError("InitDeck: NULL deck pointer");
        return;
    }

    if (num_decks <= 0) {
        d_LogErrorF("InitDeck: Invalid num_decks %d (must be > 0)", num_decks);
        return;
    }

    // Constitutional pattern: Use dArray_t for card storage (only heap allocation)
    // Initial capacity: 52 cards per deck
    // d_InitArray(capacity, element_size) - capacity FIRST!
    deck->cards = d_InitArray(52 * num_decks, sizeof(Card_t));
    deck->discard_pile = d_InitArray(52 * num_decks, sizeof(Card_t));

    if (!deck->cards || !deck->discard_pile) {
        d_LogFatal("InitDeck: Failed to initialize card arrays");
        if (deck->cards) d_DestroyArray(deck->cards);
        if (deck->discard_pile) d_DestroyArray(deck->discard_pile);
        return;
    }

    // Initialize deck with cards
    // Standard deck: 4 suits × 13 ranks = 52 cards
    for (int d = 0; d < num_decks; d++) {
        for (int suit = 0; suit < SUIT_MAX; suit++) {
            for (int rank = 1; rank <= RANK_KING; rank++) {
                Card_t card = CreateCard((CardSuit_t)suit, (CardRank_t)rank);
                d_AppendDataToArray(deck->cards, &card);
            }
        }
    }

    deck->cards_remaining = deck->cards->count;

    d_LogInfoF("Initialized deck with %d card(s) (%d × 52)",
               (int)deck->cards_remaining, num_decks);
}

void CleanupDeck(Deck_t* deck) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!deck) {
        d_LogError("CleanupDeck: NULL deck pointer");
        return;
    }

    // Destroy Daedalus arrays (only heap-allocated resources)
    if (deck->cards) {
        d_DestroyArray(deck->cards);
        deck->cards = NULL;
    }
    if (deck->discard_pile) {
        d_DestroyArray(deck->discard_pile);
        deck->discard_pile = NULL;
    }

    // Reset state (deck itself not freed - it's a value type)
    deck->cards_remaining = 0;

    d_LogInfo("Deck cleaned up");
}

// ============================================================================
// DECK OPERATIONS
// ============================================================================

void ShuffleDeck(Deck_t* deck) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!deck || !deck->cards) {
        d_LogError("ShuffleDeck: NULL pointer passed");
        return;
    }

    size_t n = deck->cards->count;
    if (n <= 1) {
        return;  // Nothing to shuffle
    }

    // Seed RNG (only once per program run)
    static bool seeded = false;
    if (!seeded) {
        srand((unsigned int)time(NULL));
        seeded = true;
    }

    // Fisher-Yates shuffle algorithm (O(n))
    // Constitutional pattern: Direct array access via Daedalus
    for (size_t i = n - 1; i > 0; i--) {
        // Random index from 0 to i (inclusive)
        size_t j = rand() % (i + 1);

        // Swap cards[i] and cards[j]
        Card_t* card_i = (Card_t*)d_IndexDataFromArray(deck->cards, i);
        Card_t* card_j = (Card_t*)d_IndexDataFromArray(deck->cards, j);

        // Constitutional pattern: Card_t is value type, safe to copy
        Card_t temp = *card_i;
        *card_i = *card_j;
        *card_j = temp;
    }

    deck->cards_remaining = deck->cards->count;

    d_LogInfoF("Shuffled deck (%d cards)", (int)n);
}

Card_t DealCard(Deck_t* deck) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!deck || !deck->cards) {
        d_LogError("DealCard: NULL pointer passed");
        // Return invalid card
        Card_t invalid = {.card_id = -1};
        return invalid;
    }

    if (deck->cards->count == 0) {
        d_LogError("DealCard: Deck is empty");
        // Return invalid card
        Card_t invalid = {.card_id = -1};
        return invalid;
    }

    // Constitutional pattern: Pop from dArray_t (O(1) from end)
    // d_PopDataFromArray returns void* to the popped data
    Card_t* card_ptr = (Card_t*)d_PopDataFromArray(deck->cards);
    if (!card_ptr) {
        d_LogError("DealCard: Failed to pop card from array");
        Card_t invalid = {.card_id = -1};
        return invalid;
    }

    // Copy card before pointer becomes invalid
    Card_t card = *card_ptr;

    deck->cards_remaining = deck->cards->count;

    return card;  // Return by value (constitutional pattern)
}

void DiscardCard(Deck_t* deck, Card_t card) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!deck || !deck->discard_pile) {
        d_LogError("DiscardCard: NULL pointer passed");
        return;
    }

    // Constitutional pattern: Card_t copied by value into array
    d_AppendDataToArray(deck->discard_pile, &card);
}

void ResetDeck(Deck_t* deck) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!deck || !deck->cards || !deck->discard_pile) {
        d_LogError("ResetDeck: NULL pointer passed");
        return;
    }

    // Clear both deck and discard pile
    d_ClearArray(deck->cards);
    d_ClearArray(deck->discard_pile);

    // Regenerate all 52 cards from scratch
    // Standard deck: 4 suits × 13 ranks = 52 cards
    for (int suit = 0; suit < SUIT_MAX; suit++) {
        for (int rank = 1; rank <= RANK_KING; rank++) {
            Card_t card = CreateCard((CardSuit_t)suit, (CardRank_t)rank);
            d_AppendDataToArray(deck->cards, &card);
        }
    }

    deck->cards_remaining = deck->cards->count;

    // Shuffle the replenished deck
    ShuffleDeck(deck);

    d_LogInfoF("Deck reset (%d cards regenerated and shuffled)", (int)deck->cards_remaining);
}

// ============================================================================
// DECK QUERIES
// ============================================================================

size_t GetDeckSize(const Deck_t* deck) {
    if (!deck || !deck->cards) {
        return 0;
    }
    return deck->cards->count;
}

size_t GetDiscardSize(const Deck_t* deck) {
    if (!deck || !deck->discard_pile) {
        return 0;
    }
    return deck->discard_pile->count;
}

bool IsDeckEmpty(const Deck_t* deck) {
    if (!deck || !deck->cards) {
        return true;
    }
    return deck->cards->count == 0;
}
