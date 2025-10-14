# Card System Design

## Card Structure

### Complete Definition
```c
typedef struct Card {
    CardSuit_t suit;       // 0-3: Hearts, Diamonds, Clubs, Spades
    CardRank_t rank;       // 1-13: Ace through King
    SDL_Texture* texture;  // Cached texture pointer (not owned)
    int x, y;              // Screen coordinates
    bool face_up;          // Visibility flag
    int card_id;           // Unique identifier: 0-51
} Card_t;
```

### Design Rationale

**Why include position in Card_t?**
- Cards move frequently during rendering
- Avoids separate position tracking structure
- Enables simple animation updates

**Why include texture pointer?**
- Fast rendering without repeated table lookups
- Texture loaded once, cached globally
- Pointer invalidation handled by global cache

**Why card_id?**
- Direct mapping to texture files (0.png - 51.png)
- Fast hash key for texture cache
- Unique identifier for debugging

## Card Creation

### Factory Function
```c
Card_t CreateCard(CardSuit_t suit, CardRank_t rank) {
    Card_t card = {
        .suit = suit,
        .rank = rank,
        .texture = NULL,  // Loaded lazily
        .x = 0,
        .y = 0,
        .face_up = false,
        .card_id = CardToID(suit, rank)
    };
    return card;
}
```

**Returns by value** - Card_t is a value type, not heap-allocated

### ID Calculation
```c
int CardToID(CardSuit_t suit, CardRank_t rank) {
    return (suit * 13) + (rank - 1);
}

void IDToCard(int card_id, CardSuit_t* suit, CardRank_t* rank) {
    *suit = card_id / 13;
    *rank = (card_id % 13) + 1;
}
```

**Examples:**
| Card | Suit | Rank | Calculation | ID |
|------|------|------|-------------|----|
| Ace of Hearts | 0 | 1 | (0×13)+(1-1) | 0 |
| 2 of Hearts | 0 | 2 | (0×13)+(2-1) | 1 |
| King of Hearts | 0 | 13 | (0×13)+(13-1) | 12 |
| Ace of Diamonds | 1 | 1 | (1×13)+(1-1) | 13 |
| King of Spades | 3 | 13 | (3×13)+(13-1) | 51 |

## Card Comparison

### Equality
```c
bool CardsEqual(const Card_t* a, const Card_t* b) {
    return a->card_id == b->card_id;
}
```

### Ordering (for sorting)
```c
int CompareCards(const Card_t* a, const Card_t* b) {
    // Sort by suit first, then rank
    if (a->suit != b->suit) {
        return a->suit - b->suit;
    }
    return a->rank - b->rank;
}
```

**Usage with qsort:**
```c
void SortHand(Hand_t* hand) {
    qsort(hand->cards->data, hand->cards->count, sizeof(Card_t),
          (int (*)(const void*, const void*))CompareCards);
}
```

## Card Display

### String Representation

**Constitutional Pattern: Caller provides dString_t output parameter**

```c
void CardToString(const Card_t* card, dString_t* out) {
    const char* suits[] = {"♥", "♦", "♣", "♠"};
    const char* ranks[] = {
        "A", "2", "3", "4", "5", "6", "7",
        "8", "9", "10", "J", "Q", "K"
    };

    d_FormatString(out, "%s%s",
                   ranks[card->rank - 1],
                   suits[card->suit]);
}

// Usage:
dString_t* card_name = d_InitString();
CardToString(&card, card_name);
d_LogInfo(d_PeekString(card_name));
d_DestroyString(card_name);
```

**Output examples:**
- `A♥` - Ace of Hearts
- `10♠` - Ten of Spades
- `K♦` - King of Diamonds

### Debug String

**Constitutional Pattern: Caller provides dString_t output parameter**

```c
void CardToDebugString(const Card_t* card, dString_t* out) {
    const char* suit_names[] = {"Hearts", "Diamonds", "Clubs", "Spades"};
    const char* rank_names[] = {
        "Ace", "Two", "Three", "Four", "Five", "Six", "Seven",
        "Eight", "Nine", "Ten", "Jack", "Queen", "King"
    };

    d_FormatString(out, "%s of %s (ID: %d, Pos: %d,%d, Face: %s)",
                   rank_names[card->rank - 1],
                   suit_names[card->suit],
                   card->card_id,
                   card->x, card->y,
                   card->face_up ? "up" : "down");
}

// Usage:
dString_t* debug_str = d_InitString();
CardToDebugString(&card, debug_str);
d_LogDebug(d_PeekString(debug_str));
d_DestroyString(debug_str);
```

**Output:** `Ace of Hearts (ID: 0, Pos: 100,200, Face: up)`

## Texture Management

### Global Texture Cache
```c
// Defined in main.c
dTable_t* g_card_textures = NULL;     // Key: int (card_id), Value: SDL_Texture*
SDL_Texture* g_card_back_texture = NULL;
```

### Texture Loading (Lazy)

**Constitutional Pattern: dString_t for path, not char[] buffer**

```c
SDL_Texture* LoadCardTexture(int card_id) {
    // Check cache first
    SDL_Texture** cached = (SDL_Texture**)d_GetDataFromTable(
        g_card_textures, &card_id
    );
    if (cached) {
        return *cached;
    }

    // Load from file (Constitutional: dString_t, not char[])
    dString_t* path = d_InitString();
    d_FormatString(path, "resources/textures/cards/%d.png", card_id);

    SDL_Texture* texture = a_LoadTexture(d_PeekString(path));
    if (!texture) {
        d_LogErrorF("Failed to load card texture: %s", d_PeekString(path));
        d_DestroyString(path);
        return NULL;
    }

    d_DestroyString(path);

    // Cache for future use
    d_SetDataInTable(g_card_textures, &card_id, &texture);

    return texture;
}
```

### Eager Loading (All Cards)
```c
void LoadAllCardTextures(void) {
    for (int id = 0; id < 52; id++) {
        LoadCardTexture(id);
    }

    // Load card back
    g_card_back_texture = a_LoadTexture("resources/textures/cards/back.png");
    if (!g_card_back_texture) {
        d_LogError("Failed to load card back texture");
    }
}
```

### Texture Cleanup
```c
void FreeAllCardTextures(void) {
    // Free card faces
    dArray_t* ids = d_GetAllKeysFromTable(g_card_textures);
    for (size_t i = 0; i < ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(ids, i);
        SDL_Texture** tex = (SDL_Texture**)d_GetDataFromTable(
            g_card_textures, id
        );
        SDL_DestroyTexture(*tex);
    }
    d_DestroyArray(ids);
    d_DestroyTable(&g_card_textures);

    // Free card back
    SDL_DestroyTexture(g_card_back_texture);
}
```

## Card Rendering

### Basic Render
```c
void RenderCard(const Card_t* card) {
    // Choose texture (face or back)
    SDL_Texture* texture = card->face_up ?
        card->texture : g_card_back_texture;

    if (!texture) {
        d_LogErrorF("Missing texture for card ID %d", card->card_id);
        return;
    }

    SDL_Rect dest = {
        card->x,
        card->y,
        CARD_WIDTH,
        CARD_HEIGHT
    };

    SDL_RenderCopy(app.renderer, texture, NULL, &dest);
}
```

### Render with Scale
```c
void RenderCardScaled(const Card_t* card, float scale) {
    SDL_Texture* texture = card->face_up ?
        card->texture : g_card_back_texture;

    if (!texture) return;

    int width = (int)(CARD_WIDTH * scale);
    int height = (int)(CARD_HEIGHT * scale);

    SDL_Rect dest = {
        card->x - (width - CARD_WIDTH) / 2,   // Center on original position
        card->y - (height - CARD_HEIGHT) / 2,
        width,
        height
    };

    SDL_RenderCopy(app.renderer, texture, NULL, &dest);
}
```

### Render with Highlight
```c
void RenderCardHighlighted(const Card_t* card, bool highlighted) {
    RenderCard(card);

    if (highlighted) {
        // Draw yellow border
        a_DrawRect(card->x, card->y, CARD_WIDTH, CARD_HEIGHT,
                   255, 255, 0, 255);
    }
}
```

## Card Utilities

### Copy Card (Deep Copy)
```c
Card_t CopyCard(const Card_t* source) {
    Card_t copy = *source;  // Struct copy
    return copy;
}
```

**Note:** Texture pointer is shallow-copied (points to same global cache entry)

### Flip Card
```c
void FlipCard(Card_t* card) {
    card->face_up = !card->face_up;
}

void SetCardFaceUp(Card_t* card, bool face_up) {
    card->face_up = face_up;
}
```

### Move Card
```c
void MoveCard(Card_t* card, int x, int y) {
    card->x = x;
    card->y = y;
}

void OffsetCard(Card_t* card, int dx, int dy) {
    card->x += dx;
    card->y += dy;
}
```

## Card Validation

### Is Valid Card
```c
bool IsValidCard(const Card_t* card) {
    if (card->suit < SUIT_HEARTS || card->suit >= SUIT_MAX) {
        return false;
    }
    if (card->rank < RANK_ACE || card->rank > RANK_KING) {
        return false;
    }
    if (card->card_id != CardToID(card->suit, card->rank)) {
        return false;
    }
    return true;
}
```

### Is Face Card
```c
bool IsFaceCard(const Card_t* card) {
    return card->rank >= RANK_JACK && card->rank <= RANK_KING;
}
```

### Is Number Card
```c
bool IsNumberCard(const Card_t* card) {
    return card->rank >= RANK_TWO && card->rank <= RANK_TEN;
}
```

## Texture File Naming Convention

### Directory Structure
```
resources/textures/cards/
├── 0.png      # Ace of Hearts
├── 1.png      # 2 of Hearts
├── ...
├── 12.png     # King of Hearts
├── 13.png     # Ace of Diamonds
├── ...
├── 51.png     # King of Spades
└── back.png   # Card back design
```

### Mapping Table
| ID Range | Suit | Cards |
|----------|------|-------|
| 0-12 | Hearts (♥) | A-K |
| 13-25 | Diamonds (♦) | A-K |
| 26-38 | Clubs (♣) | A-K |
| 39-51 | Spades (♠) | A-K |

### Alternative: Sprite Atlas
For future optimization, use a single texture atlas:
```c
SDL_Texture* g_card_atlas;
SDL_Rect g_card_rects[52];  // Source rectangles

void RenderCardFromAtlas(const Card_t* card) {
    SDL_Rect src = g_card_rects[card->card_id];
    SDL_Rect dest = {card->x, card->y, CARD_WIDTH, CARD_HEIGHT};
    SDL_RenderCopy(app.renderer, g_card_atlas, &src, &dest);
}
```

**Benefits:**
- Single texture load
- Reduced GPU state changes
- Better performance for many cards

## Game-Specific Card Values

### Blackjack Value
```c
int GetBlackjackValue(const Card_t* card, bool ace_as_eleven) {
    if (card->rank >= RANK_JACK && card->rank <= RANK_KING) {
        return 10;  // Face cards
    }
    if (card->rank == RANK_ACE) {
        return ace_as_eleven ? 11 : 1;
    }
    return card->rank;  // 2-10
}
```

### Poker Rank Value
```c
int GetPokerRankValue(const Card_t* card) {
    // Ace is highest in poker
    if (card->rank == RANK_ACE) return 14;
    return card->rank;
}
```

## Memory Considerations

### Size of Card_t
```c
sizeof(Card_t) = sizeof(CardSuit_t)      // 4 bytes (enum/int)
               + sizeof(CardRank_t)      // 4 bytes (enum/int)
               + sizeof(SDL_Texture*)    // 8 bytes (64-bit pointer)
               + sizeof(int) * 2         // 8 bytes (x, y)
               + sizeof(bool)            // 1 byte (face_up)
               + sizeof(int)             // 4 bytes (card_id)
               + padding                 // 3 bytes (alignment)
               = 32 bytes
```

**Deck memory:** 52 cards × 32 bytes = 1,664 bytes (~1.6 KB)

### Value Type Benefits
- No heap allocations per card
- Cache-friendly (contiguous in dArray)
- No pointer chasing
- Simple copying

### Texture Pointer Lifetime
- Textures owned by `g_card_textures` table
- Card's `texture` field is **non-owning pointer**
- Valid until `FreeAllCardTextures()` called
- Never `SDL_DestroyTexture(card->texture)` directly

## Testing

### Unit Tests
```c
void test_card_id_mapping(void) {
    // Test all 52 cards
    for (int suit = 0; suit < SUIT_MAX; suit++) {
        for (int rank = 1; rank <= RANK_KING; rank++) {
            int id = CardToID(suit, rank);

            // Verify range
            assert(id >= 0 && id < 52);

            // Verify inverse
            CardSuit_t out_suit;
            CardRank_t out_rank;
            IDToCard(id, &out_suit, &out_rank);

            assert(out_suit == suit);
            assert(out_rank == rank);
        }
    }
}

void test_card_creation(void) {
    Card_t ace = CreateCard(SUIT_HEARTS, RANK_ACE);

    assert(ace.suit == SUIT_HEARTS);
    assert(ace.rank == RANK_ACE);
    assert(ace.card_id == 0);
    assert(ace.face_up == false);
    assert(ace.texture == NULL);
}

void test_card_comparison(void) {
    Card_t card1 = CreateCard(SUIT_HEARTS, RANK_FIVE);
    Card_t card2 = CreateCard(SUIT_HEARTS, RANK_FIVE);
    Card_t card3 = CreateCard(SUIT_SPADES, RANK_FIVE);

    assert(CardsEqual(&card1, &card2) == true);
    assert(CardsEqual(&card1, &card3) == false);

    assert(CompareCards(&card1, &card3) < 0);  // Hearts < Spades
}
```

## Best Practices

✅ **Always use CreateCard()** - Don't manually initialize Card_t
✅ **Check texture != NULL before rendering** - Handle missing textures gracefully
✅ **Use card_id for lookups** - Fast texture cache access
✅ **Cards are value types** - Copy freely, no manual free
✅ **Lazy load textures** - Only load when needed
✅ **Use dString_t for string operations** - Caller creates/destroys dString_t
✅ **Use d_FormatString instead of snprintf** - No buffer overflow risks

❌ **Don't malloc Card_t** - Use value semantics
❌ **Don't modify suit/rank after creation** - Immutable card identity
❌ **Don't free card->texture** - Not owned by card
❌ **Don't store Card_t* in arrays** - Use Card_t directly
❌ **Don't use static buffers in functions** - Use dString_t output parameters
❌ **Don't use snprintf with char[]** - Use d_FormatString with dString_t
