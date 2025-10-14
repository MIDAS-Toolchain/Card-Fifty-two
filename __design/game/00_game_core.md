# Game Core - State Machine Architecture

**Source:** `src/game.c`
**Last Updated:** 2025-10-07

## Overview

The game core implements a finite state machine that manages the flow of any card game. This document describes the generic state machine architecture that can be used for Blackjack, Poker, or other card games.

## GameContext_t Structure

The central game state container:

```c
typedef struct GameContext {
    GameState_t current_state;      // Current state in the FSM
    GameState_t previous_state;     // Previous state (for debugging/transitions)
    dTable_t* bool_flags;           // Boolean flags (key: char*, value: bool)
    dTable_t* int_values;           // Integer values (key: char*, value: int)
    dTable_t* dealer_phase;         // Dealer phase (key: char*, value: DealerPhase_t)
    dArray_t* active_players;       // Array of int player_ids participating in current game
    int current_player_index;       // Index into active_players for turn-based games
    Deck_t* deck;                   // Pointer to active deck
    float state_timer;              // Time spent in current state
    int round_number;               // Current round counter
} GameContext_t;
```

### Constitutional Pattern Compliance

- **Everything's a table or array**: `bool_flags`, `int_values`, `dealer_phase` use `dTable_t`, `active_players` uses `dArray_t`
- **No raw pointers to collections**: All collections managed by Daedalus
- **Value type for context**: `GameContext_t` itself is stack-allocated in scene code
- **Pointer types for complex objects**: `Deck_t*`, `Player_t*` are heap-allocated
- **Typed tables instead of void***: Using separate typed tables (`bool_flags`, `int_values`, `dealer_phase`) instead of a single `void*` table provides type safety and prevents casting chaos

## Game State Machine

### States

```c
typedef enum {
    STATE_MENU,         // Main menu (handled by sceneMenu.c)
    STATE_BETTING,      // Players place bets
    STATE_DEALING,      // Initial cards dealt
    STATE_PLAYER_TURN,  // Player making decisions
    STATE_DEALER_TURN,  // Dealer playing hand
    STATE_SHOWDOWN,     // Comparing hands
    STATE_ROUND_END     // Displaying results, cleanup
} GameState_t;
```

### State Transitions

```c
void TransitionState(GameContext_t* game, GameState_t new_state)
```

**Entry Actions by State:**

| State | Entry Action |
|-------|-------------|
| `STATE_BETTING` | Reset player states to `PLAYER_STATE_BETTING` |
| `STATE_DEALING` | Call `DealInitialHands()` |
| `STATE_PLAYER_TURN` | Reset `current_player_index` to 0 |
| `STATE_DEALER_TURN` | Dealer AI begins (handled in update) |
| `STATE_SHOWDOWN` | Call `ResolveRound()` |
| `STATE_ROUND_END` | Start 3-second result display timer |

**State Timer:**
- Resets to 0.0f on every state transition
- Incremented by delta time each frame
- Used for timed transitions and delays

## Lifecycle Functions

### Initialization

```c
void InitGameContext(GameContext_t* game, Deck_t* deck)
```

- Initializes state machine to `STATE_BETTING`
- Creates `state_data` table (16 buckets, string keys)
- Creates `active_players` array (capacity 8)
- Links to provided deck pointer

### Cleanup

```c
void CleanupGameContext(GameContext_t* game)
```

- Destroys `state_data` table
- Destroys `active_players` array
- Does NOT destroy deck (scene owns deck)
- Does NOT destroy players (global `g_players` table owns them)

## Player Management

### Active Players Array

```c
dArray_t* active_players;  // Stores int player_ids
```

- Contains player IDs participating in current game
- Indexed by `current_player_index` for turn-based gameplay
- Players added via `AddPlayerToGame(game, player_id)`

### Player Lookup

```c
Player_t* GetPlayerByID(int player_id)
```

- Looks up player in global `g_players` table
- Returns NULL if player not found
- NULL checks required before use

### Current Player

```c
Player_t* GetCurrentPlayer(GameContext_t* game)
```

- Returns player at `current_player_index`
- Used in turn-based states (PLAYER_TURN)
- Advances index when player completes action

## Round Management

### Deal Initial Hands

```c
void DealInitialHands(GameContext_t* game)
```

Called at `STATE_DEALING` entry:

1. Deals 2 rounds of cards to all active players
2. Dealer's first card dealt face-down (hidden)
3. All other cards dealt face-up
4. Checks for natural blackjacks
5. Sets player states accordingly

### Resolve Round

```c
void ResolveRound(GameContext_t* game)
```

Called at `STATE_SHOWDOWN` entry:

1. Compares all player hands against dealer
2. Applies payout rules (win/loss/push)
3. Updates player chip counts
4. Sets final player states (WON/LOST/PUSH/BLACKJACK)

### Start New Round

```c
void StartNewRound(GameContext_t* game)
```

Called at end of `STATE_ROUND_END`:

1. Clears all hands (returns cards to discard pile)
2. Resets player bets to 0
3. Resets player states to `PLAYER_STATE_WAITING`
4. Checks if deck needs reshuffling (< 20 cards)
5. Increments `round_number`
6. Resets `current_player_index` to 0
7. Transitions back to `STATE_BETTING`

## Deck and Discard Management

### Card Counting Considerations

**Critical Rule:** ALL cards that enter the discard pile MUST be visible to the player before discarding.

**Why:** Card counting is a core strategic mechanic. Players need to track:
- Which cards have been played
- Remaining deck composition
- Probability of drawing specific cards

**Implementation Requirements:**

1. **Deck Count Visible:** Show remaining cards in deck at all times
2. **Discard Count Visible:** Show cards in discard pile
3. **All Reveals Before Discard:**
   - Player busts � Dealer reveals hidden card before discard
   - Round ends � All cards revealed before cleanup
   - Hidden dealer card � ALWAYS revealed during dealer turn
4. **Discard Pile Access:** Player can inspect discard pile contents

### Reshuffle Trigger

```c
if (GetDeckSize(game->deck) < 20) {
    d_LogInfo("Reshuffling deck...");
    ResetDeck(game->deck);
}
```

- Triggers when deck falls below 20 cards
- Moves all discard cards back to deck
- Reshuffles entire deck
- Happens at start of new round (not mid-round)

## State Variable Storage

### Typed Tables (Improved Pattern)

Instead of using `void*` values (error-prone), the implementation uses **typed tables**:

```c
dTable_t* bool_flags;    // Key: char*, Value: bool
dTable_t* int_values;    // Key: char*, Value: int
dTable_t* dealer_phase;  // Key: char*, Value: DealerPhase_t
```

### Type-Safe Helper Functions

**Boolean Flags:**
```c
// Set a boolean flag
void SetBoolFlag(GameContext_t* game, const char* key, bool value);

// Get a boolean flag (with default)
bool GetBoolFlag(const GameContext_t* game, const char* key, bool default_value);

// Clear a flag
void ClearBoolFlag(GameContext_t* game, const char* key);
```

**Dealer Phase:**
```c
// Set dealer turn phase
void SetDealerPhase(GameContext_t* game, DealerPhase_t phase);

// Get dealer phase (defaults to DEALER_PHASE_CHECK_REVEAL)
DealerPhase_t GetDealerPhase(const GameContext_t* game);

// Clear dealer phase
void ClearDealerPhase(GameContext_t* game);
```

**Example Use Cases:**
- Store player action flag: `SetBoolFlag(game, "player_stood", true)`
- Store dealer hidden value: `d_SetDataInTable(game->int_values, &key, &value)`
- Store dealer phase: `SetDealerPhase(game, DEALER_PHASE_DECIDE)`

**Constitutional Compliance (Improved):**
- Uses `dTable_t` for storage (not raw hash map)
- String keys for readability
- **Typed values instead of void*** (prevents casting errors)
- Helper functions provide type-safe access
- NULL checks built into helpers

## Update Loop

```c
void UpdateGameLogic(GameContext_t* game, float dt)
```

Called every frame by scene logic:

1. Increments `state_timer` by `dt`
2. Dispatches to state-specific update function:
   - `STATE_BETTING` � `UpdateBettingState()`
   - `STATE_DEALING` � `UpdateDealingState()`
   - `STATE_PLAYER_TURN` � `UpdatePlayerTurnState()`
   - `STATE_DEALER_TURN` � `UpdateDealerTurnState()`
   - `STATE_SHOWDOWN` � `UpdateShowdownState()`
   - `STATE_ROUND_END` � `UpdateRoundEndState()`

### State Update Functions

Each state has its own update function that:
- Checks conditions for state transition
- Updates timers and counters
- Processes player/AI actions
- Calls `TransitionState()` when ready to advance

## Input Processing

### Betting Input

```c
bool ProcessBettingInput(GameContext_t* game, Player_t* player, int bet_amount)
```

- Validates game is in `STATE_BETTING`
- Checks player has sufficient chips
- Places bet via `PlaceBet(player, bet_amount)`
- Returns true on success

### Player Turn Input

```c
bool ProcessPlayerTurnInput(GameContext_t* game, Player_t* player, PlayerAction_t action)
```

- Validates game is in `STATE_PLAYER_TURN`
- Validates it's the player's turn
- Validates action-specific rules (e.g., can only double with 2 cards)
- Executes action via `ExecutePlayerAction()`
- Returns true on success

## Logging and Debug

### State Transitions

```c
d_LogInfoF("State transition: %s -> %s",
           StateToString(game->current_state),
           StateToString(new_state));
```

Every transition logged for debugging.

### Player Actions

```c
d_LogInfoF("%s hits (hand value: %d)", GetPlayerName(player), player->hand.total_value);
d_LogInfoF("%s stands (hand value: %d)", GetPlayerName(player), player->hand.total_value);
d_LogInfoF("%s doubles down", GetPlayerName(player));
```

All player decisions logged.

### Round Results

```c
d_LogInfoF("Resolving round - Dealer: %d%s", dealer_value, dealer_bust ? " (BUST)" : "");
d_LogInfoF("%s: WIN", GetPlayerName(player));
d_LogInfoF("%s: BLACKJACK - WIN 3:2", GetPlayerName(player));
```

Round outcomes logged for verification.

## Integration with Scenes

The game state machine is used by scene code (e.g., `sceneBlackjack.c`):

```c
// Scene-local game context (stack-allocated)
static GameContext_t g_game;

// Initialize at scene start
InitGameContext(&g_game, &g_test_deck);
AddPlayerToGame(&g_game, dealer_id);
AddPlayerToGame(&g_game, player_id);
TransitionState(&g_game, STATE_BETTING);

// Update every frame
UpdateGameLogic(&g_game, dt);

// Process input
if (bet_button_clicked) {
    ProcessBettingInput(&g_game, player, bet_amount);
}

// Cleanup at scene end
CleanupGameContext(&g_game);
```

## Future Enhancements

1. **Animation System Integration:**
   - Add animation phase tracking to state_data
   - Coordinate state transitions with animation completion
   - Support for smooth card dealing/revealing animations

2. **Multi-Player Support:**
   - Extend `active_players` to handle multiple human players
   - Add turn indicator for current player
   - Network synchronization hooks

3. **Save/Load State:**
   - Serialize GameContext_t to disk
   - Resume interrupted games
   - Replay system for debugging

4. **Performance Metrics:**
   - Track average time per state
   - Log state transition frequencies
   - Identify performance bottlenecks

## Constitutional Adherence

 **Everything's a table or array:** `state_data` (dTable_t), `active_players` (dArray_t)
 **Value types for simple data:** `GameContext_t` stack-allocated in scenes
 **Pointer types for complex objects:** `Deck_t*`, `Player_t*`
 **NULL checks before dereferencing:** All player lookups checked
 **Logging at appropriate severity:** Info for state changes, Error for failures
 **No magic numbers:** State timers defined, reshuffle threshold constant

---

**Next Document:** `01_blackjack_core.md` - Blackjack-specific game loop and rules
