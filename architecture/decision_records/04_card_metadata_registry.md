# ADR-005: Card Metadata as Separate Registry Over Embedded Fields

## Context

Cards need gameplay-affecting properties (tags, rarity, effects) beyond their suit/rank. Could embed these fields directly in Card_t or maintain separate metadata registry.

**Alternatives Considered:**

1. **Separate metadata registry** (global dTable_t keyed by card_id)
   - Pros: Card_t stays 32 bytes, metadata lazily allocated, independent lifecycle
   - Cons: Extra lookup cost, pointer indirection, manual memory management
   - Cost: O(1) table lookup per query, manual cleanup required

2. **Embed metadata in Card_t** (add tags array, rarity, flavor to struct)
   - Pros: Direct access, zero lookup cost, automatic cleanup
   - Cons: Card_t bloats to 128+ bytes, all cards pay cost, breaks value semantics
   - Cost: 4KB wasted memory (52 cards × metadata padding)

3. **Parallel arrays indexed by card_id** (g_card_tags[52], g_card_rarity[52])
   - Pros: Simple indexing, cache-friendly
   - Cons: Multiple global arrays, not extensible, wastes memory
   - Cost: Fixed 52-element arrays even if unused

4. **Card_t points to metadata** (add CardMetadata_t* to Card_t)
   - Pros: Cheap access, Card_t only grows 8 bytes
   - Cons: Breaks value semantics, lifetime coupling, NULL checks everywhere
   - Cost: Card_t no longer copyable by value

## Decision

**Use separate metadata registry (dTable_t) storing CardMetadata_t* keyed by card_id. Metadata lazily allocated on first tag assignment.**

Card_t remains lightweight 32-byte value type. CardMetadata_t is heap-allocated and lives in global table. Lookup via `GetOrCreateMetadata(card_id)`.

**Justification:**

1. **Preserves Card_t Value Semantics**: Card_t remains 32 bytes, copyable, no pointers
2. **Pay-Per-Use**: Only cards with tags allocate metadata (most cards are vanilla)
3. **Independent Lifecycle**: Metadata outlives individual Card_t copies (tags persist across deck shuffles)
4. **Extensible**: Add new metadata fields without touching Card_t struct
5. **Type Safety**: CardTag_t enum prevents invalid tag values

## Consequences

**Positive:**
- ✅ Card_t stays 32 bytes (no bloat, fits in cache lines)
- ✅ Metadata only allocated for ~10-20 cards (tags are rare rewards)
- ✅ Tags persist across rounds (metadata independent of card copies)
- ✅ Future-proof (add new metadata fields without Card_t refactor)
- ✅ Grep-able (all metadata access goes through cardTags.h functions)

**Negative:**
- ❌ Lookup cost (table lookup + pointer dereference vs direct access)
- ❌ Manual cleanup (must iterate 52 cards in CleanupCardMetadata)
- ❌ NULL checks (GetCardTags can return NULL if no metadata)

**Accepted Trade-offs:**
- ✅ Accept O(1) lookup cost for memory efficiency (32 bytes vs 128+ bytes per card)
- ✅ Accept manual cleanup for lazy allocation (don't pay for unused cards)
- ❌ NOT accepting: Card_t* metadata pointer - breaks value semantics

**Pattern Used:**

**Card data** (suit, rank, texture) in Card_t:
```c
typedef struct Card {
    SDL_Texture* texture;
    int card_id;
    CardSuit_t suit;
    CardRank_t rank;
    // ... 32 bytes total
} Card_t;
```

**Card metadata** (tags, rarity, effects) in separate registry:
```c
typedef struct CardMetadata {
    int card_id;
    dArray_t* tags;           // Array of CardTag_t (value types)
    CardRarity_t rarity;
    dString_t* flavor_text;
} CardMetadata_t;

extern dTable_t* g_card_metadata;  // card_id → CardMetadata_t*
```

**Access pattern:**
```c
// Query metadata
if (HasCardTag(card->card_id, CARD_TAG_LUCKY)) {
    // O(1) table lookup + O(n) tag array scan
}

// Modify metadata
AddCardTag(card->card_id, CARD_TAG_BLESSED);
SetCardRarity(card->card_id, CARD_RARITY_RARE);
```

## Implementation Evidence

**Lazy allocation** (cardTags.c:23-46):
```c
static CardMetadata_t* GetOrCreateMetadata(int card_id) {
    CardMetadata_t** meta_ptr = d_TableGet(g_card_metadata, &card_id);
    if (meta_ptr) return *meta_ptr;  // Already exists
    
    // Create on first use
    CardMetadata_t* meta = malloc(sizeof(CardMetadata_t));
    // ... initialize ...
    d_TableSet(g_card_metadata, &card_id, &meta);  // Store pointer
    return meta;
}
```

**Manual cleanup** (cardTags.c:72-97):
```c
void CleanupCardMetadata(void) {
    // Must iterate all 52 cards because table stores pointers
    for (int card_id = 0; card_id < 52; card_id++) {
        CardMetadata_t** meta_ptr = d_TableGet(g_card_metadata, &card_id);
        if (meta_ptr && *meta_ptr) {
            // Free nested resources
            d_ArrayDestroy((*meta_ptr)->tags);
            d_StringDestroy((*meta_ptr)->flavor_text);
            free(*meta_ptr);  // Manual free because heap-allocated
        }
    }
    d_TableDestroy(&g_card_metadata);
}
```

## Success Metrics

- Card_t remains exactly 32 bytes (verified via `sizeof(Card_t)`)
- Metadata only allocated for cards with tags (check table size vs 52)
- Fitness function: No CardMetadata_t fields in Card_t struct
- Valgrind shows zero memory leaks after full run