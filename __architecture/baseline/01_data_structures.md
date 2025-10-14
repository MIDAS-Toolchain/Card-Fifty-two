# Core Data Structures

## Philosophy: Everything's a Table or Array

Following Daedalus design principles, **all game data is stored in dynamic arrays (`dArray_t`), hash tables (`dTable_t`), or dynamic strings (`dString_t`)**. No raw pointers, no manual memory management beyond the provided APIs.

### Core Daedalus Types
- **`dArray_t`** - Dynamic arrays (replaces raw pointers, malloc/realloc)
- **`dTable_t`** - Hash tables (key-value lookups, O(1) average)
- **`dString_t`** - Dynamic strings (replaces char[], snprintf, manual buffers)

### String Handling Pattern
**NO raw char arrays, NO snprintf - use `dString_t` for ALL string operations:**

```c
// ❌ WRONG - Manual buffer management
char buffer[128];
snprintf(buffer, sizeof(buffer), "Deck: %d cards", count);

// ✅ CORRECT - Daedalus dString_t
dString_t* str = d_InitString();
d_FormatString(str, "Deck: %d cards", count);
const char* result = d_PeekString(str);  // Get const char* for APIs
d_DestroyString(str);
```

**Why dString_t?**
- No buffer overflow risks
- Automatic memory management
- Dynamic resizing
- Thread-safe (no static buffers)
- Consistent with "everything's a table or array" philosophy

## Primary Data Structures

### 1. Card Structure
```c
typedef enum {
    SUIT_HEARTS = 0,
    SUIT_DIAMONDS,
    SUIT_CLUBS,
    SUIT_SPADES,
    SUIT_MAX
} CardSuit_t;

typedef enum {
    RANK_ACE = 1,
    RANK_TWO,
    RANK_THREE,
    RANK_FOUR,
    RANK_FIVE,
    RANK_SIX,
    RANK_SEVEN,
    RANK_EIGHT,
    RANK_NINE,
    RANK_TEN,
    RANK_JACK,
    RANK_QUEEN,
    RANK_KING,
    RANK_MAX
} CardRank_t;

typedef struct {
    CardSuit_t suit;
    CardRank_t rank;
    SDL_Texture* texture;  // Cached from texture table
    int x, y;              // Screen position for rendering
    bool face_up;          // Visible to player?
    int card_id;           // Unique ID: (suit * 13) + (rank - 1)
} Card_t;
```

**Rationale:**
- Fixed-size struct, easily stored in `dArray_t`
- `card_id` enables fast texture lookups: `0-51` for standard deck
- Position data lives with card for rendering convenience
- `face_up` flag controls card back/front rendering

### 2. Deck - Dynamic Array
```c
typedef struct {
    dArray_t* cards;        // dArray of Card_t
    dArray_t* discard_pile; // dArray of Card_t
    size_t cards_remaining; // Quick count without iteration
} Deck_t;
```

**Operations:**
- `Deck_t* CreateDeck()` - Initializes `dArray_t` with 52 cards
- `void ShuffleDeck(Deck_t*)` - Fisher-Yates shuffle
- `Card_t* DealCard(Deck_t*)` - Pops from deck, decrements count
- `void DiscardCard(Deck_t*, Card_t*)` - Appends to discard pile
- `void ResetDeck(Deck_t*)` - Moves discard to deck, reshuffles

**Why dArray:**
- Dynamic size (handles multiple decks)
- Pop operation for dealing: `O(1)` with `d_PopDataFromArray`
- Easy shuffling with array indexing

### 3. Hand - Dynamic Array per Player
```c
typedef struct {
    dArray_t* cards;      // dArray of Card_t
    int total_value;      // Cached score (Blackjack)
    bool is_bust;         // Game-specific flag
    bool is_blackjack;    // Natural 21
} Hand_t;
```

**Operations:**
- `Hand_t* CreateHand()` - Initializes empty `dArray_t`
- `void AddCardToHand(Hand_t*, Card_t)` - Appends card
- `void ClearHand(Hand_t*)` - Clears array, resets flags
- `int CalculateHandValue(Hand_t*)` - Game-specific scoring

**Why dArray:**
- Hands grow dynamically (unknown size)
- Easy iteration for rendering/scoring
- Automatic memory management

### 4. Player - Hash Table Entry

**Constitutional Pattern: dString_t for name, Hand_t embedded as value**

```c
typedef struct {
    dString_t* name;       // Player name (Constitutional: dString_t, not char[])
    int player_id;         // Unique ID
    Hand_t hand;           // Current hand (VALUE TYPE - embedded, not pointer)
    int chips;             // Currency/score
    int current_bet;       // Active wager
    bool is_dealer;        // Special role flag
    bool is_ai;            // Human vs AI
    PlayerState_t state;   // WAITING, PLAYING, BUST, etc.
} Player_t;
```

**Storage:** Hash table keyed by `player_id` (int)
```c
// Global player registry
dTable_t* g_players;  // Key: int (player_id), Value: Player_t*

// Initialize
g_players = d_InitTable(sizeof(int), sizeof(Player_t*),
                        d_HashInt, d_CompareInt, 16);

// Add player
Player_t* player = CreatePlayer("Alice", 1, false);
d_SetDataInTable(g_players, &player->player_id, &player);

// Lookup
int id = 1;
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
```

**Why dTable:**
- Fast O(1) player lookup by ID
- Dynamic player count (multiplayer support)
- Easy iteration for turn-based logic

### 5. Texture Cache - Hash Table
```c
// Global texture registry
dTable_t* g_card_textures;  // Key: int (card_id), Value: SDL_Texture*

// Initialize
g_card_textures = d_InitTable(sizeof(int), sizeof(SDL_Texture*),
                               d_HashInt, d_CompareInt, 64);

// Load texture once
SDL_Texture* LoadCardTexture(int card_id) {
    SDL_Texture** cached = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card_id);
    if (cached) return *cached;

    // Constitutional pattern: dString_t, not char[]
    dString_t* path = d_InitString();
    d_FormatString(path, "resources/textures/cards/%d.png", card_id);
    SDL_Texture* tex = a_LoadTexture(d_PeekString(path));
    d_DestroyString(path);

    d_SetDataInTable(g_card_textures, &card_id, &tex);
    return tex;
}
```

**Why dTable:**
- Prevents duplicate texture loads
- Fast texture retrieval during rendering
- Automatic cache with first-access loading

### 6. Game State - Hash Table
```c
typedef enum {
    STATE_MENU,
    STATE_BETTING,
    STATE_DEALING,
    STATE_PLAYER_TURN,
    STATE_DEALER_TURN,
    STATE_SHOWDOWN,
    STATE_GAME_OVER
} GameState_t;

typedef struct {
    GameState_t current_state;
    GameState_t previous_state;
    dTable_t* state_data;      // Key: char* (variable name), Value: void*
    dArray_t* active_players;  // Array of int (player IDs)
    int current_player_index;
    Deck_t* deck;
    float state_timer;         // Delta accumulator for timed events
} GameContext_t;
```

**State Data Table Usage:**
```c
// Store arbitrary state variables
char* pot_key = "pot";
int pot_value = 500;
d_SetDataInTable(game->state_data, &pot_key, &pot_value);

// Retrieve
int* pot = (int*)d_GetDataFromTable(game->state_data, &pot_key);

// String values
char* msg_key = "message";
char* msg_val = "Place your bets!";
d_SetDataInTable(game->state_data, &msg_key, &msg_val);
```

**Why dTable for state:**
- Flexible key-value storage for dynamic state
- Easy serialization for save/load
- No struct modification for new variables

### 7. Active Players - Dynamic Array
```c
// Track turn order
dArray_t* active_players;  // Array of int (player_ids)

// Add player to turn order
int player_id = 2;
d_AppendDataToArray(active_players, &player_id);

// Iterate turns
for (size_t i = 0; i < active_players->count; i++) {
    int* id = (int*)d_IndexDataFromArray(active_players, i);
    Player_t** player = (Player_t**)d_GetDataFromTable(g_players, id);
    ProcessPlayerTurn(*player);
}
```

**Why dArray:**
- Turn order is sequential
- Easy rotation for multi-round games
- Can shuffle for random seating

## Data Flow Example: Dealing Cards

**Constitutional Pattern: Hand_t is embedded value, not pointer**

```c
void DealInitialHands(GameContext_t* game) {
    // Get active player IDs
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* player_id = (int*)d_IndexDataFromArray(game->active_players, i);

        // Lookup player from table
        Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, player_id);
        Player_t* player = *player_ptr;

        // Deal 2 cards from deck
        for (int j = 0; j < 2; j++) {
            Card_t card = DealCard(game->deck);  // Returns by value
            card.face_up = (j == 1 || !player->is_dealer);  // Dealer's first card hidden

            // Load texture from cache (dTable lookup)
            LoadCardTexture(&card);

            // Add to player's hand (Hand_t is VALUE TYPE, use & to access)
            AddCardToHand(&player->hand, card);
        }
    }
}
```

## Memory Ownership Rules

1. **Deck is stack-allocated singleton** - Owned by scene, uses Init/Cleanup pattern
2. **Cards are value types** - Copied when dealt, no ownership
3. **Players are heap-allocated** - Stored as pointers in `g_players` table
4. **Hands are embedded in players** - VALUE TYPE, not pointer; use CleanupHand()
5. **Textures are globally cached** - Freed on shutdown
6. **Game context is stack-allocated** - Owned by scene, uses Init/Cleanup pattern

## Initialization Order

**Constitutional Pattern: Stack-allocated singletons for Deck_t and GameContext_t**

```c
// Scene-local singletons (stack-allocated)
static Deck_t g_test_deck;
static GameContext_t g_game;

void InitScene(void) {
    // 1. Initialize deck (stack-allocated singleton)
    InitDeck(&g_test_deck, 1);  // Single 52-card deck
    ShuffleDeck(&g_test_deck);

    // 2. Initialize game context (stack-allocated singleton)
    InitGameContext(&g_game, &g_test_deck);

    // 3. Create players (heap-allocated, stored in g_players)
    Player_t* player1 = CreatePlayer("Player 1", 1, false);
    Player_t* dealer = CreatePlayer("Dealer", 0, true);
    d_SetDataInTable(g_players, &player1->player_id, &player1);
    d_SetDataInTable(g_players, &dealer->player_id, &dealer);

    // 4. Set up turn order
    AddPlayerToGame(&g_game, player1->player_id);
    AddPlayerToGame(&g_game, dealer->player_id);

    // 5. Start game
    TransitionState(&g_game, STATE_BETTING);
}

void CleanupScene(void) {
    // 1. Cleanup game context (frees internal tables/arrays)
    CleanupGameContext(&g_game);

    // 2. Cleanup deck (frees internal arrays)
    CleanupDeck(&g_test_deck);

    // 3. Destroy players (from g_players table)
    dArray_t* player_ids = d_GetAllKeysFromTable(g_players);
    for (size_t i = 0; i < player_ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(player_ids, i);
        Player_t** player = (Player_t**)d_GetDataFromTable(g_players, id);
        DestroyPlayer(player);  // Frees Player_t and embedded Hand_t
    }
    d_DestroyArray(player_ids);
}
```

## Cleanup Order

**See CleanupScene() above for scene cleanup pattern.**

**Main cleanup (application shutdown):**

```c
void Cleanup(void) {
    // 1. Cleanup scene singletons (if not already done)
    CleanupGameContext(&g_game);  // Frees internal tables/arrays
    CleanupDeck(&g_test_deck);    // Frees internal arrays

    // 2. Free players (from global table)
    dArray_t* player_ids = d_GetAllKeysFromTable(g_players);
    for (size_t i = 0; i < player_ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(player_ids, i);
        Player_t** player = (Player_t**)d_GetDataFromTable(g_players, id);
        DestroyPlayer(player);  // Frees Player_t and embedded Hand_t
    }
    d_DestroyArray(player_ids);
    d_DestroyTable(&g_players);

    // 3. Free textures (from cache)
    dArray_t* tex_ids = d_GetAllKeysFromTable(g_card_textures);
    for (size_t i = 0; i < tex_ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(tex_ids, i);
        SDL_Texture** tex = (SDL_Texture**)d_GetDataFromTable(g_card_textures, id);
        SDL_DestroyTexture(*tex);
    }
    d_DestroyArray(tex_ids);
    d_DestroyTable(&g_card_textures);

    // 4. Quit Archimedes
    a_Quit();
}
```

## Performance Characteristics

| Structure | Operation | Complexity | Notes |
|-----------|-----------|------------|-------|
| Deck (dArray) | Deal card | O(1) | Pop from end |
| Deck (dArray) | Shuffle | O(n) | Fisher-Yates |
| Hand (dArray) | Add card | O(1) amortized | Append |
| Hand (dArray) | Iterate | O(n) | Render/score |
| Players (dTable) | Lookup by ID | O(1) average | Hash table |
| Players (dTable) | Iterate all | O(n) | Via GetAllKeys |
| Textures (dTable) | Get texture | O(1) average | Cache hit |
| State (dTable) | Get/Set var | O(1) average | Dynamic state |

## Benefits of This Architecture

✅ **No manual memory leaks** - Daedalus handles allocations
✅ **Type-safe containers** - Compile-time size checks
✅ **Scalable** - Easy to add players, decks, games
✅ **Cache-friendly** - Arrays provide locality
✅ **Debuggable** - Daedalus logging integration
✅ **Portable** - Pure C, works on web (Emscripten)
✅ **Testable** - Clear data flow, mockable tables
