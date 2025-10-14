# Memory Management Strategy

## Philosophy

**All dynamic memory is managed through Daedalus containers (`dArray_t`, `dTable_t`, `dString_t`).** This eliminates manual `malloc`/`free` for data storage, reducing memory leaks and simplifying lifecycle management.

## Ownership Model

### 1. Global Resources (Program Lifetime)
**Owner:** main.c
**Lifetime:** Application start → Application shutdown

```c
// Global tables (owned by main.c)
dTable_t* g_players = NULL;           // Player registry
dTable_t* g_card_textures = NULL;     // Texture cache
SDL_Texture* g_card_back_texture = NULL;

// Initialized in main()
void Initialize(void) {
    g_players = d_InitTable(sizeof(int), sizeof(Player_t*),
                            d_HashInt, d_CompareInt, 16);
    g_card_textures = d_InitTable(sizeof(int), sizeof(SDL_Texture*),
                                   d_HashInt, d_CompareInt, 64);
    g_card_back_texture = a_LoadTexture("resources/textures/cards/back.png");
}

// Destroyed in Cleanup()
void Cleanup(void) {
    // 1. Free players (stored as pointers)
    dArray_t* player_ids = d_GetAllKeysFromTable(g_players);
    for (size_t i = 0; i < player_ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(player_ids, i);
        Player_t** player = (Player_t**)d_GetDataFromTable(g_players, id);
        DestroyPlayer(*player);  // Frees Player_t and its Hand_t
    }
    d_DestroyArray(player_ids);
    d_DestroyTable(&g_players);

    // 2. Free textures
    dArray_t* tex_ids = d_GetAllKeysFromTable(g_card_textures);
    for (size_t i = 0; i < tex_ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(tex_ids, i);
        SDL_Texture** tex = (SDL_Texture**)d_GetDataFromTable(g_card_textures, id);
        SDL_DestroyTexture(*tex);
    }
    d_DestroyArray(tex_ids);
    d_DestroyTable(&g_card_textures);

    SDL_DestroyTexture(g_card_back_texture);
}
```

**Rule:** Global tables are **never** destroyed mid-game, only at shutdown.

---

### 2. Game Context (Scene Lifetime)
**Owner:** Scene (e.g., sceneBlackjack.c)
**Lifetime:** InitGameContext() → CleanupGameContext()

**Constitutional Pattern: Stack-allocated singleton (like Deck_t)**

```c
typedef struct GameContext {
    GameState_t current_state;
    GameState_t previous_state;
    dTable_t* state_data;      // Owned: destroyed with context
    dArray_t* active_players;  // Owned: destroyed with context
    int current_player_index;
    Deck_t* deck;              // Pointer to scene's deck
    float state_timer;
    int round_number;
} GameContext_t;

// Scene-local storage (stack-allocated singleton)
static GameContext_t g_game;

void InitGameContext(GameContext_t* game, Deck_t* deck) {
    if (!game || !deck) {
        d_LogFatal("InitGameContext: NULL pointer");
        return;
    }

    game->current_state = STATE_BETTING;
    game->previous_state = STATE_BETTING;
    game->state_timer = 0.0f;
    game->round_number = 1;
    game->current_player_index = 0;
    game->deck = deck;  // Points to scene's deck

    game->state_data = d_InitTable(sizeof(char*), sizeof(void*),
                                    d_HashString, d_CompareString, 16);
    game->active_players = d_InitArray(8, sizeof(int));
}

void CleanupGameContext(GameContext_t* game) {
    if (!game) return;

    d_DestroyTable(&game->state_data);     // Frees all state variables
    d_DestroyArray(game->active_players);  // Frees player ID array
    // NOTE: Does NOT free game pointer (caller owns memory)
}
```

**Rule:** `GameContext_t` is a **stack-allocated singleton** (one per scene). It owns `state_data` and `active_players` (dTable/dArray). It does **not** own `deck` (scene owns it) or players (those are in `g_players`).

---

### 3. Players (Explicit Lifetime)
**Owner:** `g_players` table
**Lifetime:** CreatePlayer() → DestroyPlayer()

**Constitutional Pattern: dString_t for name, Hand_t embedded as value**

```c
typedef struct Player {
    dString_t* name;        // Constitutional: dString_t, not char[]
    int player_id;
    Hand_t hand;            // VALUE TYPE - embedded, not pointer
    int chips;
    int current_bet;
    bool is_dealer;
    bool is_ai;
    PlayerState_t state;
} Player_t;

Player_t* CreatePlayer(const char* name, int id, bool is_dealer) {
    Player_t* player = malloc(sizeof(Player_t));  // Heap-allocated

    // Constitutional pattern: dString_t for name
    player->name = d_InitString();
    d_SetString(player->name, name);

    player->player_id = id;

    // Hand_t is VALUE TYPE - initialize in place
    InitHand(&player->hand);

    player->chips = STARTING_CHIPS;
    player->current_bet = 0;
    player->is_dealer = is_dealer;
    player->is_ai = is_dealer;
    player->state = PLAYER_STATE_WAITING;

    return player;
}

void DestroyPlayer(Player_t** player_ptr) {
    if (!player_ptr || !*player_ptr) return;
    Player_t* player = *player_ptr;

    // Cleanup dString_t name
    if (player->name) {
        d_DestroyString(player->name);
    }

    // Cleanup embedded hand (value type)
    CleanupHand(&player->hand);

    // Free the Player_t struct
    free(player);
    *player_ptr = NULL;
}
```

**Storage:**
```c
// Add to global table
Player_t* player = CreatePlayer("Alice", 1, false);
d_SetDataInTable(g_players, &player->player_id, &player);

// Retrieve
int id = 1;
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
Player_t* player = *player_ptr;

// Remove and free
d_RemoveDataFromTable(g_players, &id);  // Removes from table
DestroyPlayer(player);                  // Frees memory
```

**Rule:** Players are heap-allocated and stored as **pointers** in `g_players`. Caller must manually free with `DestroyPlayer()`.

---

### 4. Hands (Embedded in Player)
**Owner:** Player_t (embedded as value type)
**Lifetime:** InitHand() → CleanupHand()

**Constitutional Pattern: Hand_t is VALUE TYPE, not pointer**

```c
typedef struct Hand {
    dArray_t* cards;      // dArray of Card_t (value types)
    int total_value;      // Cached hand value
    bool is_bust;         // true if total_value > 21
    bool is_blackjack;    // true if natural 21
} Hand_t;

void InitHand(Hand_t* hand) {
    if (!hand) {
        d_LogFatal("InitHand: NULL hand pointer");
        return;
    }

    hand->cards = d_InitArray(sizeof(Card_t), 10);  // Cards stored by value
    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}

void CleanupHand(Hand_t* hand) {
    if (!hand) return;

    if (hand->cards) {
        d_DestroyArray(hand->cards);  // Frees dArray (cards are value types)
        hand->cards = NULL;
    }
    // NOTE: Does NOT free hand pointer (embedded value type)
}

void ClearHand(Hand_t* hand, Deck_t* deck) {
    if (!hand) return;

    // Discard cards back to deck
    if (deck && hand->cards) {
        for (size_t i = 0; i < hand->cards->count; i++) {
            Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
            DiscardCard(deck, *card);
        }
    }

    d_ClearArray(hand->cards);  // Empties array without freeing
    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}
```

**Rule:** `Hand_t` is **embedded as value type** in Player_t. It owns its `cards` dArray. When player is destroyed, CleanupHand() is called to free the dArray.

---

### 5. Deck (Owned by Scene)
**Owner:** Scene (e.g., sceneBlackjack.c or main.c)
**Lifetime:** InitDeck() → CleanupDeck()

**Constitutional Pattern: Stack-allocated singleton (like GameContext_t)**

```c
typedef struct Deck {
    dArray_t* cards;        // dArray of Card_t (value types)
    dArray_t* discard_pile; // dArray of Card_t (discarded cards)
    size_t cards_remaining; // Quick count (cached)
} Deck_t;

// Scene-local storage (stack-allocated)
static Deck_t g_test_deck;

void InitDeck(Deck_t* deck, int num_decks) {
    if (!deck) {
        d_LogFatal("InitDeck: NULL deck pointer");
        return;
    }

    deck->cards = d_InitArray(sizeof(Card_t), 52 * num_decks);
    deck->discard_pile = d_InitArray(sizeof(Card_t), 52 * num_decks);
    deck->cards_remaining = 0;

    // Populate with cards (stored by value)
    for (int d = 0; d < num_decks; d++) {
        for (int suit = 0; suit < SUIT_MAX; suit++) {
            for (int rank = 1; rank <= RANK_KING; rank++) {
                Card_t card = CreateCard(suit, rank);
                d_AppendDataToArray(deck->cards, &card);  // Value copy
                deck->cards_remaining++;
            }
        }
    }

    ShuffleDeck(deck);
}

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

**Rule:** `Deck_t` is **stack-allocated singleton**. It owns two dArrays. Cards are stored **by value** (copied when dealt).

---

### 6. Cards (Value Types, No Ownership)
**Owner:** None (copied everywhere)
**Lifetime:** Created on stack or copied into arrays

```c
// Cards are value types (not heap-allocated)
Card_t CreateCard(CardSuit_t suit, CardRank_t rank) {
    Card_t card = {  // Stack allocation
        .suit = suit,
        .rank = rank,
        .texture = NULL,
        .x = 0, .y = 0,
        .face_up = false,
        .card_id = CardToID(suit, rank)
    };
    return card;  // Returned by value
}

// Used in deck
Card_t card = CreateCard(SUIT_HEARTS, RANK_ACE);
d_AppendDataToArray(deck->cards, &card);  // Copies card into array

// Used in hand
Card_t* dealt_card = DealCard(deck);      // Returns pointer from deck array
AddCardToHand(player->hand, *dealt_card); // Copies card value
```

**Rule:** Cards are **never** heap-allocated. Always copied by value.

---

## Memory Allocation Summary

| Type | Allocation | Storage | Freed By |
|------|-----------|---------|----------|
| **Card_t** | Stack/value | dArray (by value) | d_DestroyArray (automatic) |
| **Hand_t** | Stack/value | Embedded in Player_t | `CleanupHand()` |
| **Player_t** | `malloc` | dTable (as pointer) | `DestroyPlayer()` |
| **Deck_t** | Stack/value | Scene static/global | `CleanupDeck()` |
| **GameContext_t** | Stack/value | Scene static | `CleanupGameContext()` |
| **dArray_t** | `d_InitArray` | Embedded in structs | `d_DestroyArray()` |
| **dTable_t** | `d_InitTable` | Global/struct | `d_DestroyTable()` |
| **dString_t** | `d_InitString` | Player_t, temp vars | `d_DestroyString()` |
| **SDL_Texture** | `a_LoadTexture` | dTable (as pointer) | `SDL_DestroyTexture()` |

**Key Pattern:**
- **Singletons** (Deck_t, GameContext_t, Hand_t) = Stack-allocated, use Init/Cleanup
- **Multiple instances** (Player_t) = Heap-allocated, use Create/Destroy
- **Collections** (dArray_t, dTable_t, dString_t) = Heap-allocated by Daedalus, use Init/Destroy

## Leak Prevention Checklist

### Initialization Phase
✅ Create global tables (`g_players`, `g_card_textures`)
✅ Create game context
✅ Load textures into cache
✅ Create initial players and add to `g_players`

### During Gameplay
✅ Cards are **value types** (no manual free)
✅ Hands are cleared with `ClearHand()` (no deallocation)
✅ Players remain in `g_players` until explicitly removed

### Round Reset
✅ Clear player hands with `ClearHand()` (preserves dArray)
✅ Reset deck with `ResetDeck()` (moves discard → deck)
✅ **Do not** destroy/recreate players

### Shutdown Phase
1. ✅ Free players (iterate `g_players`, call `DestroyPlayer()`)
2. ✅ Destroy `g_players` table
3. ✅ Free textures (iterate `g_card_textures`, call `SDL_DestroyTexture()`)
4. ✅ Destroy `g_card_textures` table
5. ✅ Free card back texture
6. ✅ Destroy game context (frees deck, state_data, active_players)
7. ✅ Call `a_Quit()` (Archimedes cleanup)

## Common Patterns

### Adding a Player
```c
Player_t* player = CreatePlayer("Bob", 2, false);  // malloc
d_SetDataInTable(g_players, &player->player_id, &player);  // Store pointer
AddPlayerToGame(game, player);  // Adds ID to active_players array
```

### Removing a Player
```c
int id = 2;
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
Player_t* player = *player_ptr;

// Remove from game
for (size_t i = 0; i < game->active_players->count; i++) {
    int* active_id = (int*)d_IndexDataFromArray(game->active_players, i);
    if (*active_id == id) {
        d_RemoveDataFromArray(game->active_players, i);
        break;
    }
}

// Remove from registry and free
d_RemoveDataFromTable(g_players, &id);
DestroyPlayer(player);
```

### Dealing Cards
```c
// Deal from deck (pop returns pointer to card in deck array)
Card_t* card = DealCard(deck);  // Pops from deck->cards

// Copy to player's hand (value copy)
AddCardToHand(player->hand, *card);  // Copies card into hand->cards array
```

### Texture Caching
```c
// First access: load and cache
SDL_Texture* tex = a_LoadTexture("card_5.png");
int card_id = 5;
d_SetDataInTable(g_card_textures, &card_id, &tex);  // Store pointer

// Subsequent access: retrieve from cache
SDL_Texture** cached = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card_id);
SDL_RenderCopy(app.renderer, *cached, NULL, &dest);

// Cleanup: free all textures at shutdown
dArray_t* ids = d_GetAllKeysFromTable(g_card_textures);
for (size_t i = 0; i < ids->count; i++) {
    int* id = (int*)d_IndexDataFromArray(ids, i);
    SDL_Texture** tex = (SDL_Texture**)d_GetDataFromTable(g_card_textures, id);
    SDL_DestroyTexture(*tex);
}
d_DestroyArray(ids);
d_DestroyTable(&g_card_textures);
```

## Memory Debugging

### Valgrind Check
```bash
valgrind --leak-check=full --show-leak-kinds=all ./bin/card-fifty-two
```

**Expected output (no leaks):**
```
==12345== HEAP SUMMARY:
==12345==     in use at exit: 0 bytes in 0 blocks
==12345==   total heap usage: 1,234 allocs, 1,234 frees, 456,789 bytes allocated
==12345==
==12345== All heap blocks were freed -- no leaks are possible
```

### Address Sanitizer
```bash
make CFLAGS="-fsanitize=address -g"
./bin/card-fifty-two
```

### Daedalus Logging
```c
// Enable memory tracking (if available in Daedalus)
#define D_LOG_MIN_LEVEL D_LOG_LEVEL_DEBUG

// Log allocations
d_LogDebugF("Created player: %s (id=%d)", player->name, player->player_id);
d_LogDebugF("Destroyed player: %s", player->name);
```

## Anti-Patterns (Avoid)

### ❌ Manual malloc for game data
```c
// WRONG
Card_t* card = malloc(sizeof(Card_t));  // Never do this!
```

**Instead:** Use value types or Daedalus containers
```c
// RIGHT
Card_t card = CreateCard(suit, rank);  // Stack allocation
d_AppendDataToArray(array, &card);     // Copied into dArray
```

### ❌ Forgetting to free heap-allocated structs
```c
// WRONG
Player_t* player = CreatePlayer(...);
d_SetDataInTable(g_players, &id, &player);
// Never freed!
```

**Instead:** Always pair Create/Destroy
```c
// RIGHT
Player_t* player = CreatePlayer(...);
d_SetDataInTable(g_players, &id, &player);
// ... later ...
DestroyPlayer(player);
```

### ❌ Double-freeing dArray/dTable
```c
// WRONG
d_DestroyArray(hand->cards);
d_DestroyArray(hand->cards);  // CRASH!
```

**Instead:** Only destroy once, nullify pointer
```c
// RIGHT
d_DestroyArray(hand->cards);
hand->cards = NULL;
```

### ❌ Using pointers after table modification
```c
// WRONG
Card_t* card = (Card_t*)d_IndexDataFromArray(deck->cards, 0);
ShuffleDeck(deck);  // May realloc array!
card->x = 100;      // DANGER: pointer may be invalid
```

**Instead:** Re-fetch pointer after modifications
```c
// RIGHT
ShuffleDeck(deck);
Card_t* card = (Card_t*)d_IndexDataFromArray(deck->cards, 0);
card->x = 100;
```

## Best Practices

1. **Prefer value types** - Store Cards, Hands as values in arrays
2. **Use dArray/dTable exclusively** - No manual malloc for collections
3. **Clear ownership** - Each struct has one owner responsible for freeing
4. **Pair Create/Destroy** - Every CreateX() has a matching DestroyX()
5. **Free in reverse order** - Last allocated, first freed
6. **Check for NULL** - Always validate pointers from dTable/dArray
7. **Test with sanitizers** - Run Valgrind and ASAN regularly
