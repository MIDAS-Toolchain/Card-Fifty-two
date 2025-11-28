#ifndef DECK_H
#define DECK_H

#include "common.h"
#include "card.h"

// ============================================================================
// DECK LIFECYCLE
// ============================================================================

/**
 * InitDeck - Initialize a deck in-place
 *
 * @param deck Pointer to deck to initialize (stack or heap allocated)
 * @param num_decks Number of 52-card decks to include (1-8 typical)
 *
 * Constitutional pattern:
 * - Deck_t is value type, no malloc
 * - Only allocates internal dArray_t* cards and discard_pile
 * - Caller must call CleanupDeck() when done
 * - Texture pointers remain NULL (loaded lazily)
 */
void InitDeck(Deck_t* deck, int num_decks);

/**
 * CleanupDeck - Free deck's internal resources
 *
 * @param deck Pointer to deck to cleanup
 *
 * Constitutional pattern: Destroys arrays, does NOT free deck itself
 * Use when deck is stack-allocated or embedded in a struct
 */
void CleanupDeck(Deck_t* deck);

// ============================================================================
// DECK OPERATIONS
// ============================================================================

/**
 * ShuffleDeck - Randomize card order
 *
 * @param deck Deck to shuffle
 *
 * Implementation: Fisher-Yates shuffle algorithm (O(n))
 * Constitutional pattern: Operates on dArray_t directly
 */
void ShuffleDeck(Deck_t* deck);

/**
 * DealCard - Remove and return top card from deck
 *
 * @param deck Deck to deal from
 * @return Card_t by value, or Card with card_id=-1 if deck empty
 *
 * Constitutional pattern: Returns Card_t by value (not pointer)
 * Uses d_ArrayPop for O(1) operation
 */
Card_t DealCard(Deck_t* deck);

/**
 * DiscardCard - Add card to discard pile
 *
 * @param deck Deck containing discard pile
 * @param card Card to discard (passed by value)
 *
 * Constitutional pattern: Card_t copied by value into discard pile
 */
void DiscardCard(Deck_t* deck, Card_t card);

/**
 * ResetDeck - Move all discarded cards back to deck and shuffle
 *
 * @param deck Deck to reset
 *
 * Use case: When deck runs out mid-game
 * Constitutional pattern: Array operations via Daedalus
 */
void ResetDeck(Deck_t* deck);

// ============================================================================
// DECK QUERIES
// ============================================================================

/**
 * GetDeckSize - Get number of cards remaining in deck
 *
 * @param deck Deck to query
 * @return Number of cards, or 0 if deck is NULL
 *
 * Constitutional pattern: Returns cached count (O(1))
 */
size_t GetDeckSize(const Deck_t* deck);

/**
 * GetDiscardSize - Get number of cards in discard pile
 *
 * @param deck Deck to query
 * @return Number of discarded cards, or 0 if deck is NULL
 */
size_t GetDiscardSize(const Deck_t* deck);

/**
 * IsDeckEmpty - Check if deck has no cards
 *
 * @param deck Deck to query
 * @return true if deck is empty or NULL, false otherwise
 */
bool IsDeckEmpty(const Deck_t* deck);

#endif // DECK_H
