#ifndef HAND_H
#define HAND_H

#include "common.h"
#include "card.h"

// ============================================================================
// HAND LIFECYCLE
// ============================================================================

/**
 * InitHand - Initialize a hand in-place
 *
 * @param hand Pointer to hand to initialize (stack or heap allocated)
 *
 * Constitutional pattern: Hand_t is value type, no malloc
 * Only allocates internal dArray_t* cards
 * Caller must call CleanupHand() when done
 */
void InitHand(Hand_t* hand);

/**
 * CleanupHand - Free hand's internal resources
 *
 * @param hand Pointer to hand to cleanup
 *
 * Constitutional pattern: Destroys cards array, does NOT free hand itself
 * Use when hand is embedded in Player_t (value type)
 */
void CleanupHand(Hand_t* hand);

// ============================================================================
// HAND OPERATIONS
// ============================================================================

/**
 * AddCardToHand - Add a card to the hand
 *
 * @param hand Hand to add card to
 * @param card Card to add (passed by value, will be copied)
 *
 * Constitutional pattern: Card_t passed by value (value type)
 * Automatically recalculates hand value and checks for bust/blackjack
 */
void AddCardToHand(Hand_t* hand, Card_t card);

/**
 * ClearHand - Remove all cards from hand and optionally discard them
 *
 * @param hand Hand to clear
 * @param deck Optional deck to discard cards to (NULL to just delete cards)
 *
 * Use case: Start of new round, reset hand state
 * If deck is provided, cards are moved to deck's discard pile
 */
void ClearHand(Hand_t* hand, Deck_t* deck);

/**
 * CalculateHandValue - Compute Blackjack hand value
 *
 * @param hand Hand to calculate
 * @return Total hand value (0-21+ with ace optimization)
 *
 * Blackjack rules:
 * - Number cards (2-10): face value
 * - Face cards (J, Q, K): 10
 * - Ace: 11 or 1 (automatically optimized to avoid bust)
 *
 * Updates hand->total_value, hand->is_bust, hand->is_blackjack
 */
int CalculateHandValue(Hand_t* hand);

/**
 * CalculateVisibleHandValue - Calculate hand value for face-up cards only
 *
 * @param hand Hand to calculate
 * @return Total value of visible (face-up) cards only
 *
 * Same calculation rules as CalculateHandValue(), but only counts cards
 * where card->face_up == true. Used for displaying dealer's partial hand
 * value when first card is hidden.
 *
 * Does NOT update hand state (read-only).
 */
int CalculateVisibleHandValue(const Hand_t* hand);

// ============================================================================
// HAND QUERIES
// ============================================================================

/**
 * GetHandSize - Get number of cards in hand
 *
 * @param hand Hand to query
 * @return Number of cards, or 0 if hand is NULL
 */
size_t GetHandSize(const Hand_t* hand);

/**
 * GetCardFromHand - Get card at specific index
 *
 * @param hand Hand to query
 * @param index Index of card (0-based)
 * @return Pointer to card, or NULL if index out of bounds
 *
 * Note: Returns pointer to card in array, do NOT free
 */
const Card_t* GetCardFromHand(const Hand_t* hand, size_t index);

/**
 * IsHandBust - Check if hand is bust (over 21)
 *
 * @param hand Hand to check
 * @return true if bust, false otherwise
 */
bool IsHandBust(const Hand_t* hand);

/**
 * IsHandBlackjack - Check if hand is natural blackjack
 *
 * @param hand Hand to check
 * @return true if blackjack (2 cards, total 21), false otherwise
 */
bool IsHandBlackjack(const Hand_t* hand);

// ============================================================================
// ACE VALUE QUERIES
// ============================================================================

/**
 * GetAceValue - Determine how an ace is currently counting in hand
 *
 * @param hand Hand containing the ace
 * @param ace_index Index of the ace in the hand (0-based)
 * @return 11 if ace counts as 11, 1 if optimized to 1, 0 if not an ace
 *
 * Use case: Visual indicator to show players whether ace is "soft" (11) or "hard" (1)
 * Calculates hand without this ace, then checks if adding 11 would bust
 */
int GetAceValue(const Hand_t* hand, size_t ace_index);

// ============================================================================
// HAND UTILITIES
// ============================================================================

/**
 * HandToString - Convert hand to human-readable string
 *
 * @param hand Hand to convert
 * @param out Output dString_t (caller must create and destroy)
 *
 * Format: "Cards: Ace of Hearts, 10 of Spades | Value: 21"
 *
 * Constitutional pattern: Uses dString_t, not char[] buffer
 */
void HandToString(const Hand_t* hand, dString_t* out);

#endif // HAND_H
