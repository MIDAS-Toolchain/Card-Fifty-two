# Game Core Design

## Overview
The core of Card-Fifty-Two is built on a **52-card standard deck** with extensible game logic. The baseline implementation focuses on **Blackjack**, with architecture supporting future games (Poker, Hearts, etc.).

## Core Concept

### Data-Driven Everything
Following the "everything's a table or array" philosophy:

- **Deck**: `dArray_t*` of 52 `Card_t` structs
- **Player Hands**: Each player has `dArray_t*` for their cards
- **Players**: `dTable_t*` keyed by `player_id` (int)
- **Textures**: `dTable_t*` mapping `card_id` to `SDL_Texture*`
- **Game State**: `dTable_t*` for dynamic state variables

**No naked pointers, no manual malloc for game data.**

## Standard 52-Card Deck

### Card Representation
```c
typedef enum {
    SUIT_HEARTS = 0,    // ♥
    SUIT_DIAMONDS,      // ♦
    SUIT_CLUBS,         // ♣
    SUIT_SPADES,        // ♠
    SUIT_MAX
} CardSuit_t;

typedef enum {
    RANK_ACE = 1,
    RANK_TWO, RANK_THREE, RANK_FOUR, RANK_FIVE,
    RANK_SIX, RANK_SEVEN, RANK_EIGHT, RANK_NINE,
    RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING,
    RANK_MAX
} CardRank_t;

typedef struct Card {
    CardSuit_t suit;
    CardRank_t rank;
    SDL_Texture* texture;  // Cached from global texture table
    int x, y;              // Screen position
    bool face_up;          // Visible to player?
    int card_id;           // Unique ID: 0-51
} Card_t;
```

### Card ID Mapping
```
card_id = (suit * 13) + (rank - 1)

Examples:
- Ace of Hearts:   (0 * 13) + (1 - 1) = 0
- King of Spades:  (3 * 13) + (13 - 1) = 51
- 7 of Diamonds:   (1 * 13) + (7 - 1) = 19
```

This ID maps directly to texture files: `resources/textures/cards/0.png` through `51.png`.

## Deck Management (dArray_t)

### Deck Structure
```c
typedef struct Deck {
    dArray_t* cards;        // Active cards (dealt from end)
    dArray_t* discard_pile; // Used cards
    size_t cards_remaining; // Quick count
} Deck_t;
```

### Smart Shuffling
Fisher-Yates shuffle ensures uniform distribution:
```c
void ShuffleDeck(Deck_t* deck) {
    srand(time(NULL));
    for (size_t i = deck->cards->count - 1; i > 0; i--) {
        size_t j = rand() % (i + 1);
        // Swap cards[i] and cards[j]
    }
}
```

### Dealing Mechanism

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

    // Get pointer to last card, copy by value, then remove from array
    Card_t* card_ptr = (Card_t*)d_IndexDataFromArray(deck->cards, deck->cards->count - 1);
    Card_t card = *card_ptr;  // Copy by value
    d_RemoveDataFromArray(deck->cards, deck->cards->count - 1);

    return card;
}
```

**Flow:**
1. Get last card from `deck->cards` (O(1) index access)
2. Copy card by value
3. Remove from array (O(1) pop)
4. Return card by value to caller

### Discard Pile

**Note: Cards passed by value**

```c
void DiscardCard(Deck_t* deck, Card_t card) {
    d_AppendDataToArray(deck->discard_pile, &card);  // Copy by value
}

void ResetDeck(Deck_t* deck) {
    // Move discard pile back to deck
    for (size_t i = 0; i < deck->discard_pile->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(deck->discard_pile, i);
        d_AppendDataToArray(deck->cards, card);  // Copy by value
    }
    d_ClearArray(deck->discard_pile);  // Clear discard pile
    deck->cards_remaining = deck->cards->count;
    ShuffleDeck(deck);
}
```

## Player System

### Player Structure

**Constitutional Pattern: dString_t for name, Hand_t embedded as value**

```c
typedef struct Player {
    dString_t* name;       // Constitutional: dString_t, not char[]
    int player_id;         // Unique ID for table lookup
    Hand_t hand;           // VALUE TYPE - embedded, not pointer
    int chips;             // Current balance
    int current_bet;       // Active wager
    bool is_dealer;        // Special role
    bool is_ai;            // Human vs AI
    PlayerState_t state;   // WAITING, PLAYING, BUST, etc.
} Player_t;

typedef struct Hand {
    dArray_t* cards;       // dArray of Card_t (value copies)
    int total_value;       // Cached score
    bool is_bust;
    bool is_blackjack;
} Hand_t;
```

### Player Registry (dTable_t)
```c
// Global table: Key = int (player_id), Value = Player_t*
dTable_t* g_players = d_InitTable(sizeof(int), sizeof(Player_t*),
                                   d_HashInt, d_CompareInt, 16);

// Add player
Player_t* alice = CreatePlayer("Alice", 1, false);
d_SetDataInTable(g_players, &alice->player_id, &alice);

// Lookup player
int id = 1;
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
Player_t* player = *player_ptr;
```

**Why dTable:**
- O(1) player lookup by ID
- Dynamic player count (1-6 players)
- Easy iteration for game logic

### Hand Operations

**Note: Hand_t is embedded value type in Player_t, use & to access**

```c
void AddCardToHand(Hand_t* hand, Card_t card) {
    d_AppendDataToArray(hand->cards, &card);  // Copy card by value
    hand->total_value = CalculateHandValue(hand);
    hand->is_bust = (hand->total_value > 21);
    hand->is_blackjack = (hand->cards->count == 2 && hand->total_value == 21);
}

void ClearHand(Hand_t* hand, Deck_t* deck) {
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

## Game State Management

### State Machine
```c
typedef enum {
    STATE_MENU,          // Main menu
    STATE_BETTING,       // Players place bets
    STATE_DEALING,       // Initial 2-card deal
    STATE_PLAYER_TURN,   // Player actions (hit/stand)
    STATE_DEALER_TURN,   // Dealer reveals and plays
    STATE_SHOWDOWN,      // Compare hands, payout
    STATE_GAME_OVER      // Round complete
} GameState_t;
```

### Game Context
```c
typedef struct GameContext {
    GameState_t current_state;
    GameState_t previous_state;
    dTable_t* bool_flags;       // Boolean flags (typed table, not void*)
    dTable_t* int_values;       // Integer values (typed table, not void*)
    dTable_t* dealer_phase;     // Dealer phase (typed table, not void*)
    dArray_t* active_players;   // Array of int (player IDs)
    int current_player_index;   // Whose turn?
    Deck_t* deck;               // The card deck
    float state_timer;          // Delta time accumulator
    int round_number;           // Current round counter
} GameContext_t;
```

**Constitutional Pattern Improvement:** Instead of using a single `dTable_t* state_data` with `void*` values (which violates type safety), the implementation uses **typed tables** - one table per value type. This provides:
- Type safety at compile time
- Clear semantic meaning for each table
- No `void*` casting chaos
- Better debugging and inspection

### Dynamic State Variables (Typed Tables)

**Example: Boolean Flags**
```c
// Store a boolean flag (using helper function)
SetBoolFlag(game, "player_stood", true);

// Retrieve (with default value)
bool stood = GetBoolFlag(game, "player_stood", false);

// Clear flag when done
ClearBoolFlag(game, "player_stood");
```

**Example: Integer Values**
```c
// Store an integer (direct table access)
int pot_value = 500;
const char* pot_key = "pot";
d_SetDataInTable(game->int_values, &pot_key, &pot_value);

// Retrieve
int* pot = (int*)d_GetDataFromTable(game->int_values, &pot_key);
if (pot) {
    d_LogInfoF("Pot value: %d", *pot);
}
```

**Example: Dealer Phase**
```c
// Store dealer phase (using helper function)
SetDealerPhase(game, DEALER_PHASE_DECIDE);

// Retrieve (with default)
DealerPhase_t phase = GetDealerPhase(game);

// Clear when dealer turn ends
ClearDealerPhase(game);
```

**Benefit:** Extend game state without modifying `GameContext_t` struct. Type-safe access via helper functions prevents `void*` casting errors.

## Turn-Based Flow

### Active Players Array
```c
// Track turn order (dArray of int player_ids)
dArray_t* active_players = game->active_players;

// Add players
int player1_id = 1;
int dealer_id = 0;
d_AppendDataToArray(active_players, &player1_id);
d_AppendDataToArray(active_players, &dealer_id);

// Iterate turns
for (size_t i = 0; i < active_players->count; i++) {
    int* id = (int*)d_IndexDataFromArray(active_players, i);
    Player_t** player = (Player_t**)d_GetDataFromTable(g_players, id);

    if (!(*player)->is_dealer) {
        ProcessPlayerTurn(*player);
    }
}
```

### Dealer Turn Logic
```c
void DealerTurn(GameContext_t* game) {
    // Find dealer
    int dealer_id = 0;
    Player_t** dealer_ptr = (Player_t**)d_GetDataFromTable(g_players, &dealer_id);
    Player_t* dealer = *dealer_ptr;

    // Reveal hidden card
    Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand->cards, 0);
    hidden->face_up = true;

    // Dealer must hit on 16 or less, stand on 17+
    while (dealer->hand->total_value < 17) {
        Card_t* card = DealCard(game->deck);
        card->face_up = true;
        AddCardToHand(dealer->hand, *card);
    }
}
```

## Texture Management (dTable)

### Texture Cache

**Constitutional Pattern: dString_t for path, not char[] buffer**

```c
// Global cache: Key = int (card_id), Value = SDL_Texture*
dTable_t* g_card_textures = d_InitTable(sizeof(int), sizeof(SDL_Texture*),
                                         d_HashInt, d_CompareInt, 64);

SDL_Texture* LoadCardTexture(int card_id) {
    // Check cache
    SDL_Texture** cached = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card_id);
    if (cached) return *cached;

    // Load from file (Constitutional: dString_t, not char[])
    dString_t* path = d_InitString();
    d_FormatString(path, "resources/textures/cards/%d.png", card_id);
    SDL_Texture* tex = a_LoadTexture(d_PeekString(path));
    d_DestroyString(path);

    // Store in cache
    d_SetDataInTable(g_card_textures, &card_id, &tex);
    return tex;
}
```

**Card Back Texture:**
```c
SDL_Texture* g_card_back_texture = a_LoadTexture("resources/textures/cards/back.png");
```

## Complete Round Flow (Blackjack Example)

### 1. Betting Phase
```c
STATE_BETTING:
    // Players place bets
    for each player in active_players:
        if player is human:
            Display bet UI (buttons: 10, 25, 50, 100)
        else if player is AI:
            PlaceBet(player, random(10, 100))

    Transition to STATE_DEALING
```

### 2. Dealing Phase

**Note: DealCard() returns by value, Hand_t is embedded value type**

```c
STATE_DEALING:
    // Deal 2 cards to each player
    for i in 0..1:
        for each player in active_players:
            Card_t card = DealCard(deck)  // Returns by value
            if (card.card_id == -1) continue;  // Deck empty check

            card.face_up = (i == 1 || !player.is_dealer)
            LoadCardTexture(&card);  // Load texture into card
            AddCardToHand(&player.hand, card)  // Hand_t is value type, use &

    // Check for blackjacks
    for each player:
        if player.hand.is_blackjack:
            player.state = PLAYER_STATE_BLACKJACK

    Transition to STATE_PLAYER_TURN
```

### 3. Player Turn Phase
```c
STATE_PLAYER_TURN:
    current_player = active_players[current_player_index]

    if current_player is dealer:
        current_player_index++
        if all players done:
            Transition to STATE_DEALER_TURN

    if current_player is human:
        Display actions: [Hit] [Stand] [Double] [Split]

        if clicked Hit:
            Card_t card = DealCard(deck)  // Returns by value
            if (card.card_id != -1) {
                card.face_up = true
                LoadCardTexture(&card);
                AddCardToHand(&current_player.hand, card)  // Hand_t is value type

                if current_player.hand.is_bust:
                    current_player.state = PLAYER_STATE_BUST
                    current_player_index++
            }

        if clicked Stand:
            current_player.state = PLAYER_STATE_STAND
            current_player_index++

    else if current_player is AI:
        action = BasicStrategyDecision(current_player.hand, dealer_upcard)
        // Execute action...
```

### 4. Dealer Turn Phase

**Note: Hand_t is embedded value type, access with &**

```c
STATE_DEALER_TURN:
    dealer = GetPlayerByID(0)

    // Reveal hidden card (access embedded hand with &)
    Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer.hand.cards, 0);
    hidden->face_up = true

    // Dealer hits on 16 or less
    while dealer.hand.total_value < 17:
        Card_t card = DealCard(deck)  // Returns by value
        if (card.card_id == -1) break;  // Deck empty

        card.face_up = true
        LoadCardTexture(&card);
        AddCardToHand(&dealer.hand, card)  // Hand_t is value type, use &

    Transition to STATE_SHOWDOWN
```

### 5. Showdown Phase
```c
STATE_SHOWDOWN:
    dealer_value = dealer.hand.total_value

    for each player in active_players:
        if player is dealer: continue

        if player.is_bust:
            LoseBet(player)
            Display "BUST"

        else if player.is_blackjack:
            if dealer.is_blackjack:
                ReturnBet(player)  // Push
                Display "PUSH"
            else:
                WinBet(player, 2.5)  // 3:2 payout
                Display "BLACKJACK!"

        else if dealer.is_bust:
            WinBet(player, 2.0)
            Display "DEALER BUST - YOU WIN"

        else if player.hand.total_value > dealer_value:
            WinBet(player, 2.0)
            Display "YOU WIN"

        else if player.hand.total_value < dealer_value:
            LoseBet(player)
            Display "DEALER WINS"

        else:
            ReturnBet(player)
            Display "PUSH"

    Wait 3 seconds
    Transition to STATE_BETTING (new round)
```

## Extensibility for Other Games

### Poker (Future)
- **5-card hands** - Same `Hand_t` structure, just deal 5 cards
- **Hand ranking** - New `PokerHandType_t` enum (Flush, Straight, etc.)
- **Community cards** - New `dArray_t* community_cards` in `GameContext_t`
- **Betting rounds** - New states: `STATE_PREFLOP`, `STATE_FLOP`, `STATE_TURN`, `STATE_RIVER`

### Hearts (Future)
- **Passing cards** - New state `STATE_PASSING`
- **Trick tracking** - `dArray_t* current_trick` in `GameContext_t`
- **Scoring** - Store per-round scores in `state_data` table

### Common Pattern
All card games share:
- 52-card deck (dArray)
- Player hands (dArray per player)
- Player registry (dTable)
- State machine with game-specific states
- Texture cache (dTable)

**Only game rules change - data structures remain the same.**

## Performance Considerations

- **Card dealing**: O(1) with `d_PopDataFromArray`
- **Player lookup**: O(1) average with hash table
- **Texture caching**: O(1) average, prevents re-loading
- **Turn iteration**: O(n) where n = active player count (2-6)

## Data Flow Diagram

```
┌─────────────┐
│    Deck     │
│  (dArray)   │◄────── Shuffle, Reset
└─────┬───────┘
      │ DealCard() (pop)
      ▼
┌─────────────┐
│    Card     │
│ (value type)│
└─────┬───────┘
      │ Copy to hand
      ▼
┌─────────────┐
│    Hand     │
│  (dArray)   │────► Calculate score
└─────┬───────┘
      │ Owned by
      ▼
┌─────────────┐
│   Player    │◄────► Stored in g_players
│  (Player_t) │       (dTable)
└─────────────┘
      │
      ▼
┌─────────────────────┐
│   GameContext       │
│  active_players     │────► Turn order (dArray of IDs)
│  state_data         │────► Dynamic state (dTable)
│  current_state      │────► State machine
└─────────────────────┘
```

## Testing Strategy

### Unit Tests
- Deck shuffling (distribution uniformity)
- Card ID mapping (bidirectional conversion)
- Hand value calculation (Blackjack rules)
- Bet placement (chip validation)

### Integration Tests
- Complete round simulation
- Multi-player scenarios
- Edge cases (all players bust, dealer blackjack)

### Stress Tests
- 1000 rounds (memory leak detection)
- 6-player games (max capacity)
- Multiple decks (8-deck shoe)
