# Deck Management Design

## Deck Structure (dArray-based)

```c
typedef struct Deck {
    dArray_t* cards;        // dArray of Card_t (active deck)
    dArray_t* discard_pile; // dArray of Card_t (used cards)
    size_t cards_remaining; // Cached count
} Deck_t;
```

**Constitutional Pattern: Stack-allocated singleton, uses Init/Cleanup**

## Deck Operations

### Initialization (Stack-Allocated Pattern)
```c
void InitDeck(Deck_t* deck, int num_decks) {
    if (!deck) {
        d_LogFatal("InitDeck: NULL deck pointer");
        return;
    }

    deck->cards = d_InitArray(sizeof(Card_t), 52 * num_decks);
    deck->discard_pile = d_InitArray(sizeof(Card_t), 52 * num_decks);

    for (int d = 0; d < num_decks; d++) {
        for (int suit = 0; suit < SUIT_MAX; suit++) {
            for (int rank = 1; rank <= RANK_KING; rank++) {
                Card_t card = CreateCard(suit, rank);
                d_AppendDataToArray(deck->cards, &card);
            }
        }
    }

    deck->cards_remaining = deck->cards->count;
    ShuffleDeck(deck);
}

// Usage: Stack-allocated singleton
static Deck_t g_test_deck;
InitDeck(&g_test_deck, 1);  // Single 52-card deck
```

### Shuffling (Fisher-Yates)
```c
void ShuffleDeck(Deck_t* deck) {
    srand(time(NULL));

    for (size_t i = deck->cards->count - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);

        // Swap cards[i] and cards[j]
        Card_t* a = (Card_t*)d_IndexDataFromArray(deck->cards, i);
        Card_t* b = (Card_t*)d_IndexDataFromArray(deck->cards, j);

        Card_t temp = *a;
        *a = *b;
        *b = temp;
    }
}
```

### Dealing (Returns by Value)

**Constitutional Pattern: Returns Card_t by value, not pointer**

```c
Card_t DealCard(Deck_t* deck) {
    if (deck->cards_remaining == 0) {
        d_LogWarning("Deck empty - cannot deal");
        Card_t invalid = {0};
        invalid.card_id = -1;
        return invalid;
    }

    deck->cards_remaining--;

    // Get pointer to last card, copy by value, then remove
    Card_t* card_ptr = (Card_t*)d_IndexDataFromArray(deck->cards, deck->cards->count - 1);
    Card_t card = *card_ptr;  // Copy by value
    d_RemoveDataFromArray(deck->cards, deck->cards->count - 1);  // O(1) pop

    return card;
}
```

### Discard & Reset

**Note: Cards passed by value**

```c
void DiscardCard(Deck_t* deck, Card_t card) {
    d_AppendDataToArray(deck->discard_pile, &card);  // Copy by value
}

void ResetDeck(Deck_t* deck) {
    // Move discard pile back to main deck
    for (size_t i = 0; i < deck->discard_pile->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(deck->discard_pile, i);
        d_AppendDataToArray(deck->cards, card);  // Copy by value
    }
    d_ClearArray(deck->discard_pile);

    deck->cards_remaining = deck->cards->count;
    ShuffleDeck(deck);
    d_LogInfoF("Deck reset: %zu cards", deck->cards_remaining);
}
```

### Cleanup (Stack-Allocated Pattern)

**Constitutional Pattern: Cleanup, not Destroy (no free)**

```c
void CleanupDeck(Deck_t* deck) {
    if (!deck) return;

    if (deck->cards) {
        d_DestroyArray(deck->cards);
        deck->cards = NULL;
    }

    if (deck->discard_pile) {
        d_DestroyArray(deck->discard_pile);
        deck->discard_pile = NULL;
    }
    // NOTE: Does NOT free deck pointer (stack-allocated)
}
```

## Multi-Deck Support (Blackjack Shoe)

### 6-Deck Shoe

**Constitutional Pattern: Stack-allocated**

```c
void InitBlackjackShoe(Deck_t* shoe) {
    InitDeck(shoe, 6);  // 312 cards total
}

// Usage:
static Deck_t g_blackjack_shoe;
InitBlackjackShoe(&g_blackjack_shoe);
```

### Cut Card Logic
```c
#define CUT_CARD_POSITION 0.75  // 75% through deck

bool ShouldReshuffle(Deck_t* deck) {
    size_t total = deck->cards->count + deck->discard_pile->count;
    return deck->cards_remaining < (total * (1.0 - CUT_CARD_POSITION));
}
```

## Deck Rendering

```c
void RenderDeck(const Deck_t* deck, int x, int y) {
    if (deck->cards_remaining == 0) {
        // Draw empty deck placeholder
        a_DrawRect(x, y, CARD_WIDTH, CARD_HEIGHT, 100, 100, 100, 255);
        return;
    }

    // Draw stack with offset shadow effect
    for (int i = 0; i < 3 && i < (int)deck->cards_remaining; i++) {
        SDL_Rect dest = {x + i*2, y + i*2, CARD_WIDTH, CARD_HEIGHT};
        SDL_RenderCopy(app.renderer, g_card_back_texture, NULL, &dest);
    }

    // Draw card count (Constitutional: dString_t, not char[])
    dString_t* count_text = d_InitString();
    d_FormatString(count_text, "%zu", deck->cards_remaining);
    a_DrawText(d_PeekString(count_text), x + CARD_WIDTH/2, y + CARD_HEIGHT + 10,
               255, 255, 255, FONT_GAME, TEXT_ALIGN_CENTER, 0);
    d_DestroyString(count_text);
}
```

## Performance Notes

- **Deal**: O(1) with `d_PopDataFromArray`
- **Shuffle**: O(n)
- **Reset**: O(n) where n = discard pile size
- **Memory**: ~1.6KB per 52-card deck (value types in dArray)
