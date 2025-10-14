#ifndef CARD_H
#define CARD_H

#include "common.h"

// ============================================================================
// CARD CREATION
// ============================================================================

/**
 * CreateCard - Factory function for creating a new card
 *
 * @param suit Card suit (SUIT_HEARTS, SUIT_DIAMONDS, etc.)
 * @param rank Card rank (RANK_ACE through RANK_KING)
 * @return Card_t by value (not heap-allocated)
 *
 * Constitutional pattern: Returns by value, not pointer
 * Texture pointer is NULL initially, loaded lazily from global cache
 */
Card_t CreateCard(CardSuit_t suit, CardRank_t rank);

// ============================================================================
// CARD ID SYSTEM
// ============================================================================

/**
 * CardToID - Convert suit/rank to unique card ID
 *
 * @param suit Card suit
 * @param rank Card rank
 * @return Unique card ID in range [0, 51]
 *
 * Formula: card_id = (suit * 13) + (rank - 1)
 *
 * Examples:
 *   Ace of Hearts:   (0 * 13) + (1 - 1) = 0
 *   King of Hearts:  (0 * 13) + (13 - 1) = 12
 *   Ace of Diamonds: (1 * 13) + (1 - 1) = 13
 *   King of Spades:  (3 * 13) + (13 - 1) = 51
 */
int CardToID(CardSuit_t suit, CardRank_t rank);

/**
 * IDToCard - Convert card ID back to suit/rank
 *
 * @param card_id Unique card ID in range [0, 51]
 * @param suit Output parameter for suit
 * @param rank Output parameter for rank
 *
 * Formula:
 *   suit = card_id / 13
 *   rank = (card_id % 13) + 1
 */
void IDToCard(int card_id, CardSuit_t* suit, CardRank_t* rank);

// ============================================================================
// CARD COMPARISON
// ============================================================================

/**
 * CardsEqual - Check if two cards are identical
 *
 * @param a First card
 * @param b Second card
 * @return true if cards have same card_id, false otherwise
 *
 * Note: Ignores position, face_up state - only compares identity
 */
bool CardsEqual(const Card_t* a, const Card_t* b);

// ============================================================================
// CARD UTILITIES
// ============================================================================

/**
 * CardToString - Convert card to human-readable string
 *
 * @param card The card to convert
 * @param out Output dString_t (caller must create and destroy)
 *
 * Format: "Rank of Suit" (e.g., "Ace of Hearts", "10 of Spades")
 *
 * Constitutional pattern: Uses dString_t, not char[] buffer
 */
void CardToString(const Card_t* card, dString_t* out);

/**
 * GetSuitString - Get suit name as string
 *
 * @param suit Card suit
 * @return Static string ("Hearts", "Diamonds", "Clubs", "Spades")
 */
const char* GetSuitString(CardSuit_t suit);

/**
 * GetRankString - Get rank name as string
 *
 * @param rank Card rank
 * @return Static string ("Ace", "2", "3", ..., "Jack", "Queen", "King")
 */
const char* GetRankString(CardRank_t rank);

/**
 * LoadCardTexture - Load texture for card from global cache
 *
 * @param card Pointer to card (texture field will be updated)
 *
 * Constitutional pattern: Uses global g_card_textures table
 * If texture not in cache, logs error and sets texture to NULL
 */
void LoadCardTexture(Card_t* card);

#endif // CARD_H
