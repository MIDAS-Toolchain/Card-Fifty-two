# Module Organization and Boundaries

## Module Structure

The codebase is organized into focused modules, each responsible for a specific domain. Dependencies flow downward (no circular dependencies).

```
┌─────────────────────────────────────────┐
│             main.c                      │  ← Entry point
│  (Initialization, game loop)            │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│            game.c/h                     │  ← Game state machine
│  (State transitions, game logic)        │
└─────────────────────────────────────────┘
       ↓              ↓              ↓
┌──────────┐  ┌──────────────┐  ┌──────────┐
│ player.c │  │   rules.c    │  │ render.c │
│ player.h │  │   rules.h    │  │ render.h │
└──────────┘  └──────────────┘  └──────────┘
       ↓              ↓              ↓
┌─────────────────────────────────────────┐
│            deck.c/h                     │  ← Deck management
│  (Shuffle, deal, discard)               │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│            card.c/h                     │  ← Card primitives
│  (Card creation, comparison)            │
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│          common.h                       │  ← Shared types/constants
└─────────────────────────────────────────┘
              ↓
┌─────────────────────────────────────────┐
│  Archimedes + Daedalus (external libs)  │
└─────────────────────────────────────────┘
```

## Module Definitions

### 1. common.h (Shared Types)
**Purpose:** Shared enums, constants, and global declarations

**Contents:**
```c
#ifndef COMMON_H
#define COMMON_H

#include <Archimedes.h>
#include <Daedalus.h>

// Screen configuration
#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// Card dimensions
#define CARD_WIDTH 100
#define CARD_HEIGHT 140

// Game constants
#define MAX_PLAYERS 6
#define STARTING_CHIPS 1000

// Enums (from 01_data_structures.md)
typedef enum { ... } CardSuit_t;
typedef enum { ... } CardRank_t;
typedef enum { ... } GameState_t;
typedef enum { ... } PlayerState_t;

// Forward declarations
typedef struct Card Card_t;
typedef struct Deck Deck_t;
typedef struct Hand Hand_t;
typedef struct Player Player_t;
typedef struct GameContext GameContext_t;

// Global tables (declared in main.c)
extern dTable_t* g_players;
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;

#endif
```

**Dependencies:** Archimedes, Daedalus
**Used by:** All modules

---

### 2. card.c/h (Card Primitives)
**Purpose:** Card creation, ID calculation, basic operations

**Header (card.h):**
```c
#ifndef CARD_H
#define CARD_H

#include "common.h"

// Card structure (full definition from 01_data_structures.md)
typedef struct Card {
    CardSuit_t suit;
    CardRank_t rank;
    SDL_Texture* texture;
    int x, y;
    bool face_up;
    int card_id;
} Card_t;

// Constructor
Card_t CreateCard(CardSuit_t suit, CardRank_t rank);

// Utilities
int CardToID(CardSuit_t suit, CardRank_t rank);
void IDToCard(int card_id, CardSuit_t* suit, CardRank_t* rank);
const char* CardToString(const Card_t* card);
int CompareCards(const Card_t* a, const Card_t* b);

#endif
```

**Implementation (card.c):**
```c
#include "card.h"

Card_t CreateCard(CardSuit_t suit, CardRank_t rank) {
    Card_t card = {
        .suit = suit,
        .rank = rank,
        .texture = NULL,  // Loaded on demand
        .x = 0, .y = 0,
        .face_up = false,
        .card_id = CardToID(suit, rank)
    };
    return card;
}

int CardToID(CardSuit_t suit, CardRank_t rank) {
    return (suit * 13) + (rank - 1);
}

void IDToCard(int card_id, CardSuit_t* suit, CardRank_t* rank) {
    *suit = card_id / 13;
    *rank = (card_id % 13) + 1;
}

// Constitutional pattern: dString_t, not static buffer
void CardToString(const Card_t* card, dString_t* out) {
    d_FormatString(out, "%s of %s",
                   GetRankString(card->rank),
                   GetSuitString(card->suit));
}

const char* GetSuitString(CardSuit_t suit) {
    switch (suit) {
        case SUIT_HEARTS:   return "Hearts";
        case SUIT_DIAMONDS: return "Diamonds";
        case SUIT_CLUBS:    return "Clubs";
        case SUIT_SPADES:   return "Spades";
        default:            return "Unknown";
    }
}

const char* GetRankString(CardRank_t rank) {
    switch (rank) {
        case RANK_ACE:   return "Ace";
        case RANK_TWO:   return "2";
        // ... (rest of ranks)
        case RANK_KING:  return "King";
        default:         return "Unknown";
    }
}

int CompareCards(const Card_t* a, const Card_t* b) {
    if (a->suit != b->suit) return a->suit - b->suit;
    return a->rank - b->rank;
}
```

**Dependencies:** common.h
**Used by:** deck.c, rules.c, render.c
**Status:** ✅ Implemented

---

### 3. deck.c/h (Deck Management)
**Purpose:** Deck initialization, shuffling, dealing, discard pile

**Constitutional Pattern: Stack-allocated singleton, uses Init/Cleanup**

**Header (deck.h):**
```c
#ifndef DECK_H
#define DECK_H

#include "common.h"
#include "card.h"

typedef struct Deck {
    dArray_t* cards;        // dArray of Card_t (value types)
    dArray_t* discard_pile; // dArray of Card_t (discarded cards)
    size_t cards_remaining; // Quick count (cached)
} Deck_t;

// Lifecycle (Init/Cleanup for stack-allocated singletons)
void InitDeck(Deck_t* deck, int num_decks);
void CleanupDeck(Deck_t* deck);

// Operations
void ShuffleDeck(Deck_t* deck);
Card_t DealCard(Deck_t* deck);  // Returns by value
void DiscardCard(Deck_t* deck, Card_t card);  // Takes by value
void ResetDeck(Deck_t* deck);

// Queries
size_t GetDeckSize(const Deck_t* deck);
size_t GetDiscardSize(const Deck_t* deck);
bool IsDeckEmpty(const Deck_t* deck);

#endif
```

**Implementation Highlights:**
- Fisher-Yates shuffle (O(n))
- `DealCard()` returns Card_t by value
- Card_t copied by value when dealing/discarding
- `InitDeck()` / `CleanupDeck()` pattern (stack-allocated singleton)
- CleanupDeck() frees internal arrays but NOT the Deck_t pointer

**Example Usage:**
```c
// Scene-local storage (stack-allocated)
static Deck_t g_test_deck;

void InitScene(void) {
    InitDeck(&g_test_deck, 1);  // Single 52-card deck
    ShuffleDeck(&g_test_deck);
}

void CleanupScene(void) {
    CleanupDeck(&g_test_deck);  // Frees internal arrays
}
```

**Dependencies:** common.h, card.h, Daedalus (dArray)
**Used by:** main.c, game.c, scenes/sceneBlackjack.c
**Status:** ✅ Implemented

---

### 4. player.c/h (Player Management)
**Purpose:** Player creation, hand management, chip operations

**Constitutional Pattern: Hand_t embedded as value, dString_t for name**

**Header (player.h):**
```c
#ifndef PLAYER_H
#define PLAYER_H

#include "hand.h"  // Hand_t definition
#include "deck.h"

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

// Player lifecycle (Create/Destroy for heap-allocated instances)
Player_t* CreatePlayer(const char* name, int id, bool is_dealer);
void DestroyPlayer(Player_t** player_ptr);  // Nulls pointer

// Chip operations
bool PlaceBet(Player_t* player, int amount);
void WinBet(Player_t* player, float multiplier);
void LoseBet(Player_t* player);
bool CanAffordBet(const Player_t* player, int amount);

// Queries
const char* GetPlayerName(const Player_t* player);

#endif
```

**Header (hand.h) - Separate module:**
```c
#ifndef HAND_H
#define HAND_H

#include "card.h"

typedef struct Hand {
    dArray_t* cards;      // dArray of Card_t (value types)
    int total_value;      // Cached hand value
    bool is_bust;         // true if total_value > 21
    bool is_blackjack;    // true if natural 21
} Hand_t;

// Hand lifecycle (Init/Cleanup for embedded value types)
void InitHand(Hand_t* hand);
void CleanupHand(Hand_t* hand);

// Hand operations
void AddCardToHand(Hand_t* hand, Card_t card);
void ClearHand(Hand_t* hand, Deck_t* deck);

// Queries
const Card_t* GetCardFromHand(const Hand_t* hand, size_t index);
size_t GetHandSize(const Hand_t* hand);

#endif
```

**Implementation Note:**
- Hand_t is VALUE TYPE - embedded directly in Player_t
- InitHand() called when creating player
- CleanupHand() called when destroying player
- Hand_t does NOT use malloc/free

**Dependencies:** hand.h, deck.h, Daedalus (dArray, dString)
**Used by:** game.c, scenes/sceneBlackjack.c

---

### 5. rules.c/h (Game Rules)
**Purpose:** Game-specific logic (Blackjack scoring, win conditions)

**Header (rules.h):**
```c
#ifndef RULES_H
#define RULES_H

#include "player.h"

// Blackjack rules
int CalculateBlackjackValue(const Hand_t* hand);
bool IsBlackjack(const Hand_t* hand);
bool IsBust(const Hand_t* hand);
int CompareHands(const Hand_t* player, const Hand_t* dealer);

// AI logic
PlayerAction_t DealerDecision(const Hand_t* dealer_hand);
PlayerAction_t BasicStrategyDecision(const Hand_t* player_hand,
                                      const Card_t* dealer_upcard);

#endif
```

**Dependencies:** player.h
**Used by:** game.c

---

### 6. render.c/h (Rendering)
**Purpose:** All Archimedes rendering calls

**Header (render.h):**
```c
#ifndef RENDER_H
#define RENDER_H

#include "game.h"

// Initialization
void LoadAllTextures(void);
void FreeAllTextures(void);

// Rendering
void RenderGame(GameContext_t* game, float dt);
void RenderCard(const Card_t* card);
void RenderHand(const Hand_t* hand, int base_x, int base_y);
void RenderMenu(void);
void RenderUI(GameContext_t* game);

#endif
```

**Dependencies:** game.h, Archimedes
**Used by:** main.c

---

### 7. game.c/h (Game State Machine)
**Purpose:** State transitions, turn management, game flow

**Constitutional Pattern: Stack-allocated singleton, uses Init/Cleanup**

**Header (game.h):**
```c
#ifndef GAME_H
#define GAME_H

#include "player.h"

typedef struct GameContext {
    GameState_t current_state;
    GameState_t previous_state;
    dTable_t* state_data;       // Owned: destroyed with context
    dArray_t* active_players;   // Owned: destroyed with context
    int current_player_index;
    Deck_t* deck;               // Pointer to scene's deck
    float state_timer;
    int round_number;
} GameContext_t;

// Lifecycle (Init/Cleanup for stack-allocated singletons)
void InitGameContext(GameContext_t* game, Deck_t* deck);
void CleanupGameContext(GameContext_t* game);

// State machine
void UpdateGameLogic(GameContext_t* game, float dt);
void TransitionState(GameContext_t* game, GameState_t new_state);
const char* StateToString(GameState_t state);

// Player management
void AddPlayerToGame(GameContext_t* game, int player_id);
Player_t* GetCurrentPlayer(GameContext_t* game);
Player_t* GetPlayerByID(int player_id);

// Game actions
void StartNewRound(GameContext_t* game);
void DealInitialHands(GameContext_t* game);
void ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action);
void DealerTurn(GameContext_t* game);
void ResolveRound(GameContext_t* game);

// State variable helpers
void SetStateVariable(GameContext_t* game, const char* key, void* value);
void* GetStateVariable(const GameContext_t* game, const char* key);

#endif
```

**Example Usage:**
```c
// Scene-local storage (stack-allocated)
static GameContext_t g_game;

void InitScene(void) {
    InitGameContext(&g_game, &g_test_deck);
    AddPlayerToGame(&g_game, 0);  // Dealer
    AddPlayerToGame(&g_game, 1);  // Player
    TransitionState(&g_game, STATE_BETTING);
}

void CleanupScene(void) {
    CleanupGameContext(&g_game);  // Frees internal tables/arrays
}
```

**Dependencies:** player.h, hand.h, deck.h, Daedalus (dTable, dArray)
**Used by:** scenes/sceneBlackjack.c, main.c

---

### 8. main.c (Entry Point)
**Purpose:** Initialization, main loop, cleanup

**Implementation:**
```c
#include "game.h"
#include "render.h"
#include "input.h"

// Global tables
dTable_t* g_players = NULL;
dTable_t* g_card_textures = NULL;
SDL_Texture* g_card_back_texture = NULL;

GameContext_t* g_game = NULL;

void Initialize(void) {
    a_Init(SCREEN_WIDTH, SCREEN_HEIGHT, "Card Fifty-Two");

    g_players = d_InitTable(sizeof(int), sizeof(Player_t*),
                            d_HashInt, d_CompareInt, 16);
    g_card_textures = d_InitTable(sizeof(int), sizeof(SDL_Texture*),
                                   d_HashInt, d_CompareInt, 64);

    LoadAllTextures();
    g_game = CreateGameContext();

    // Create initial players
    Player_t* player = CreatePlayer("Player 1", 1, false);
    Player_t* dealer = CreatePlayer("Dealer", 0, true);
    d_SetDataInTable(g_players, &player->player_id, &player);
    d_SetDataInTable(g_players, &dealer->player_id, &dealer);

    AddPlayerToGame(g_game, player);
    AddPlayerToGame(g_game, dealer);

    StartNewRound(g_game);
}

void Cleanup(void) {
    // Free players
    dArray_t* ids = d_GetAllKeysFromTable(g_players);
    for (size_t i = 0; i < ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(ids, i);
        Player_t** p = (Player_t**)d_GetDataFromTable(g_players, id);
        DestroyPlayer(*p);
    }
    d_DestroyArray(ids);
    d_DestroyTable(&g_players);

    FreeAllTextures();
    d_DestroyTable(&g_card_textures);

    DestroyGameContext(g_game);
    a_Quit();
}

void mainloop(void) {
    float dt = a_GetDeltaTime();

    a_DoInput();
    HandleInput(g_game);

    UpdateGameLogic(g_game, dt);

    a_PrepareScene();
    RenderGame(g_game, dt);
    a_PresentScene();

    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.running = 0;
    }
}

int main(void) {
    Initialize();

    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(mainloop, 0, 1);
    #else
        while (app.running) {
            mainloop();
        }
    #endif

    Cleanup();
    return 0;
}
```

**Dependencies:** All modules
**Used by:** None (entry point)

---

## Dependency Rules

### Allowed Dependencies
✅ Lower-level modules can be used by higher-level modules
✅ Modules can depend on `common.h`
✅ All modules can use Archimedes and Daedalus

### Forbidden Dependencies
❌ No circular dependencies (A depends on B, B depends on A)
❌ Lower-level modules cannot depend on higher-level ones
❌ No direct dependencies between `render.c` and `rules.c`

### Dependency Graph
```
main.c → game.h → player.h → deck.h → card.h → common.h
       ↘ render.h ↗       ↘ rules.h ↗
```

## File Organization

```
include/
├── common.h       # Shared types
├── card.h         # Card primitives
├── deck.h         # Deck management
├── player.h       # Player/hand management
├── rules.h        # Game rules
├── game.h         # Game state machine
├── render.h       # Rendering
└── input.h        # Input handling

src/
├── card.c
├── deck.c
├── player.c
├── rules.c
├── game.c
├── render.c
├── input.c
└── main.c
```

## Interface vs Implementation

### Public API (Header)
- Type definitions
- Function declarations
- Constants relevant to users

### Private Implementation (Source)
- Static helper functions
- Internal state
- Implementation details

**Example:**
```c
// player.h (public)
Player_t* CreatePlayer(const char* name, int id, bool is_dealer);

// player.c (private)
static void InitializePlayerStats(Player_t* player) {
    // Internal helper, not exposed
}
```

## Testing Strategy per Module

| Module | Unit Tests | Integration Tests |
|--------|-----------|-------------------|
| card.c | ✅ CardToID, IDToCard, CompareCards | - |
| deck.c | ✅ Shuffle, Deal, Discard | ✅ Multi-deck behavior |
| player.c | ✅ Bet placement, chip operations | - |
| rules.c | ✅ Blackjack scoring, win conditions | ✅ Full round simulation |
| game.c | - | ✅ State transitions |
| render.c | - | ✅ Visual regression |
| main.c | - | ✅ Full game flow |

## Compilation Order

1. **common.h** (header only)
2. **card.c** → card.o
3. **deck.c** → deck.o
4. **player.c** → player.o
5. **rules.c** → rules.o
6. **game.c** → game.o
7. **render.c** → render.o
8. **input.c** → input.o
9. **main.c** → main.o
10. **Link all** → bin/card-fifty-two
