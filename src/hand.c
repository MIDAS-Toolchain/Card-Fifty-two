/*
 * Card Fifty-Two - Hand System Implementation
 * Constitutional pattern: dArray_t for cards, Blackjack rules for scoring
 */

#include "hand.h"
#include "deck.h"  // For DiscardCard()
#include "cardTags.h"  // For HasCardTag(), RemoveCardTag()
#include "stats.h"  // For Stats_IncrementCardsDrawn()

// ============================================================================
// HAND LIFECYCLE
// ============================================================================

void InitHand(Hand_t* hand) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand) {
        d_LogError("InitHand: NULL hand pointer");
        return;
    }

    // Constitutional pattern: dArray_t for card storage (only heap allocation)
    // Initial capacity: 10 cards (typical max in Blackjack)
    // d_InitArray(capacity, element_size) - capacity FIRST!
    hand->cards = d_InitArray(10, sizeof(Card_t));
    if (!hand->cards) {
        d_LogFatal("InitHand: Failed to initialize card array");
        return;
    }

    // DEBUG: Verify element_size matches current sizeof(Card_t)
    d_LogInfoF("InitHand: Created array with element_size=%zu, sizeof(Card_t)=%zu",
               hand->cards->element_size, sizeof(Card_t));

    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;

    d_LogInfo("Hand initialized");
}

void CleanupHand(Hand_t* hand) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand) {
        d_LogError("CleanupHand: NULL hand pointer");
        return;
    }

    // Destroy Daedalus array (only heap-allocated resource)
    if (hand->cards) {
        d_DestroyArray(hand->cards);
        hand->cards = NULL;
    }

    // Reset state (hand itself not freed - it's a value type)
    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;

    d_LogInfo("Hand cleaned up");
}

// ============================================================================
// HAND OPERATIONS
// ============================================================================

void AddCardToHand(Hand_t* hand, Card_t card) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand) {
        d_LogError("AddCardToHand: NULL hand pointer");
        return;
    }

    // DEBUG: Verify sizes match before appending
    d_LogInfoF("AddCardToHand: array->element_size=%zu, sizeof(Card_t)=%zu, sizeof(card)=%zu",
               hand->cards->element_size, sizeof(Card_t), sizeof(card));

    // Constitutional pattern: Card_t copied by value into array
    d_AppendDataToArray(hand->cards, &card);

    // Track card drawn in global stats
    Stats_IncrementCardsDrawn();

    // DEBUG: What did we just store?
    d_LogInfoF("AddCardToHand: Stored card - rank=%d, suit=%d, card_id=%d",
               card.rank, card.suit, card.card_id);

    // Recalculate hand value
    CalculateHandValue(hand);

    d_LogInfoF("Added card to hand (%zu cards, value: %d)",
               hand->cards->count, hand->total_value);
}

void ClearHand(Hand_t* hand, Deck_t* deck) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand || !hand->cards) {
        d_LogError("ClearHand: NULL pointer passed");
        return;
    }

    // Remove DOUBLED tags from all cards before clearing (cleanup at end of turn)
    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
        if (card && HasCardTag(card->card_id, CARD_TAG_DOUBLED)) {
            RemoveCardTag(card->card_id, CARD_TAG_DOUBLED);
            d_LogInfoF("Removed DOUBLED tag from card %d", card->card_id);
        }
    }

    // If deck provided, discard all cards to deck's discard pile
    if (deck) {
        for (size_t i = 0; i < hand->cards->count; i++) {
            Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
            if (card) {
                DiscardCard(deck, *card);
            }
        }
        d_LogInfoF("Discarded %zu cards to deck", hand->cards->count);
    }

    // Clear array (doesn't free array itself, just removes elements)
    d_ClearArray(hand->cards);

    // Reset hand state
    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;

    d_LogInfo("Hand cleared");
}

int CalculateHandValue(Hand_t* hand) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand || !hand->cards) {
        d_LogError("CalculateHandValue: NULL pointer passed");
        return 0;
    }

    int total = 0;
    int num_aces = 0;

    // Sum up card values
    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
        if (!card) continue;

        // DEBUG: What are we reading back?
        d_LogInfoF("CalculateHandValue: Read card %zu - rank=%d, suit=%d, card_id=%d",
                   i, card->rank, card->suit, card->card_id);

        int value = 0;
        switch (card->rank) {
            case RANK_ACE:
                num_aces++;
                value = 11;  // Start with 11, adjust later if needed
                break;
            case RANK_TWO:   value = 2;  break;
            case RANK_THREE: value = 3;  break;
            case RANK_FOUR:  value = 4;  break;
            case RANK_FIVE:  value = 5;  break;
            case RANK_SIX:   value = 6;  break;
            case RANK_SEVEN: value = 7;  break;
            case RANK_EIGHT: value = 8;  break;
            case RANK_NINE:  value = 9;  break;
            case RANK_TEN:
            case RANK_JACK:
            case RANK_QUEEN:
            case RANK_KING:
                value = 10;
                break;
            default:
                d_LogErrorF("CalculateHandValue: Invalid rank %d", card->rank);
                value = 0;
                break;
        }

        // Check for DOUBLED tag (Degenerate's Gambit active)
        if (HasCardTag(card->card_id, CARD_TAG_DOUBLED)) {
            int original_value = value;
            value *= 2;
            d_LogInfoF("Card %d doubled: %d → %d", card->card_id, original_value, value);

            // Keep tag for visual feedback - will be removed when hand is cleared
            // RemoveCardTag(card->card_id, CARD_TAG_DOUBLED);
        }

        total += value;
    }

    // Optimize aces (convert 11 → 1 if bust)
    while (total > 21 && num_aces > 0) {
        total -= 10;  // Convert one ace from 11 to 1
        num_aces--;
    }

    // Update hand state
    hand->total_value = total;
    hand->is_bust = (total > 21);
    hand->is_blackjack = (total == 21 && hand->cards->count == 2);

    return total;
}

int CalculateVisibleHandValue(const Hand_t* hand) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand || !hand->cards) {
        d_LogError("CalculateVisibleHandValue: NULL pointer passed");
        return 0;
    }

    int total = 0;
    int num_aces = 0;

    // Sum up ONLY face-up card values
    for (size_t i = 0; i < hand->cards->count; i++) {
        const Card_t* card = (const Card_t*)d_IndexDataFromArray(hand->cards, i);
        if (!card) continue;

        // Skip face-down cards
        if (!card->face_up) continue;

        int value = 0;
        switch (card->rank) {
            case RANK_ACE:
                num_aces++;
                value = 11;  // Start with 11, adjust later if needed
                break;
            case RANK_TWO:   value = 2;  break;
            case RANK_THREE: value = 3;  break;
            case RANK_FOUR:  value = 4;  break;
            case RANK_FIVE:  value = 5;  break;
            case RANK_SIX:   value = 6;  break;
            case RANK_SEVEN: value = 7;  break;
            case RANK_EIGHT: value = 8;  break;
            case RANK_NINE:  value = 9;  break;
            case RANK_TEN:
            case RANK_JACK:
            case RANK_QUEEN:
            case RANK_KING:
                value = 10;
                break;
            default:
                d_LogErrorF("CalculateVisibleHandValue: Invalid rank %d", card->rank);
                value = 0;
                break;
        }

        total += value;
    }

    // Optimize aces (convert 11 → 1 if bust)
    while (total > 21 && num_aces > 0) {
        total -= 10;  // Convert one ace from 11 to 1
        num_aces--;
    }

    return total;
}

// ============================================================================
// HAND QUERIES
// ============================================================================

size_t GetHandSize(const Hand_t* hand) {
    if (!hand || !hand->cards) {
        return 0;
    }
    return hand->cards->count;
}

const Card_t* GetCardFromHand(const Hand_t* hand, size_t index) {
    if (!hand || !hand->cards) {
        return NULL;
    }

    if (index >= hand->cards->count) {
        d_LogErrorF("GetCardFromHand: Index %zu out of bounds (size: %zu)",
                    index, hand->cards->count);
        return NULL;
    }

    return (const Card_t*)d_IndexDataFromArray(hand->cards, index);
}

bool IsHandBust(const Hand_t* hand) {
    if (!hand) {
        return false;
    }
    return hand->is_bust;
}

bool IsHandBlackjack(const Hand_t* hand) {
    if (!hand) {
        return false;
    }
    return hand->is_blackjack;
}

// ============================================================================
// ACE VALUE QUERIES
// ============================================================================

int GetAceValue(const Hand_t* hand, size_t ace_index) {
    // Check if this is actually an ace
    if (!hand || !hand->cards || ace_index >= hand->cards->count) {
        return 0;
    }

    const Card_t* ace = (const Card_t*)d_IndexDataFromArray(hand->cards, ace_index);
    if (!ace || ace->rank != RANK_ACE) {
        return 0;  // Not an ace
    }

    // Calculate hand value WITHOUT this specific ace
    int total_without_ace = 0;
    int other_aces = 0;

    for (size_t i = 0; i < hand->cards->count; i++) {
        if (i == ace_index) continue;  // Skip the ace we're checking

        const Card_t* card = (const Card_t*)d_IndexDataFromArray(hand->cards, i);
        if (!card) continue;

        int value = 0;
        switch (card->rank) {
            case RANK_ACE:
                other_aces++;
                value = 11;
                break;
            case RANK_TWO:   value = 2;  break;
            case RANK_THREE: value = 3;  break;
            case RANK_FOUR:  value = 4;  break;
            case RANK_FIVE:  value = 5;  break;
            case RANK_SIX:   value = 6;  break;
            case RANK_SEVEN: value = 7;  break;
            case RANK_EIGHT: value = 8;  break;
            case RANK_NINE:  value = 9;  break;
            case RANK_TEN:
            case RANK_JACK:
            case RANK_QUEEN:
            case RANK_KING:
                value = 10;
                break;
            default:
                value = 0;
                break;
        }

        // Check for DOUBLED tag
        if (HasCardTag(card->card_id, CARD_TAG_DOUBLED)) {
            value *= 2;
        }

        total_without_ace += value;
    }

    // Optimize other aces
    while (total_without_ace > 21 && other_aces > 0) {
        total_without_ace -= 10;
        other_aces--;
    }

    // Now check if THIS ace can be 11 or must be 1
    if (total_without_ace + 11 <= 21) {
        return 11;  // This ace counts as 11
    } else {
        return 1;   // This ace must be 1 to avoid bust
    }
}

// ============================================================================
// HAND UTILITIES
// ============================================================================

void HandToString(const Hand_t* hand, dString_t* out) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!hand || !out) {
        d_LogError("HandToString: NULL pointer passed");
        return;
    }

    // Format: "Cards: Ace of Hearts, 10 of Spades | Value: 21"
    d_StringAppend(out, "Cards: ", 7);

    if (hand->cards->count == 0) {
        d_StringAppend(out, "(empty)", 7);
    } else {
        for (size_t i = 0; i < hand->cards->count; i++) {
            const Card_t* card = GetCardFromHand(hand, i);
            if (card) {
                dString_t* card_str = d_StringInit();
                CardToString(card, card_str);
                d_StringAppend(out, d_StringPeek(card_str),
                                 d_StringGetLength(card_str));
                d_StringDestroy(card_str);

                // Add comma separator (except for last card)
                if (i < hand->cards->count - 1) {
                    d_StringAppend(out, ", ", 2);
                }
            }
        }
    }

    // Add hand value
    d_StringAppend(out, " | Value: ", 10);
    d_StringAppendInt(out, hand->total_value);

    // Add status flags
    if (hand->is_blackjack) {
        d_StringAppend(out, " (BLACKJACK!)", 13);
    } else if (hand->is_bust) {
        d_StringAppend(out, " (BUST)", 7);
    }
}
