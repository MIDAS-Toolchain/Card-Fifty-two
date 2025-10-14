# Game State Machine Design

## State Definitions

```c
typedef enum {
    STATE_MENU,          // Main menu / game select
    STATE_BETTING,       // Players place bets
    STATE_DEALING,       // Initial card deal
    STATE_PLAYER_TURN,   // Player actions (hit/stand/etc.)
    STATE_DEALER_TURN,   // Dealer plays by rules
    STATE_SHOWDOWN,      // Compare hands, payout
    STATE_ROUND_END      // Display results, prepare next round
} GameState_t;
```

## Game Context

**Constitutional Pattern: Stack-allocated singleton, owned by scene**

```c
typedef struct GameContext {
    GameState_t current_state;
    GameState_t previous_state;
    dTable_t* state_data;      // Owned: destroyed with context
    dArray_t* active_players;  // Owned: array of int (player IDs in turn order)
    int current_player_index;
    Deck_t* deck;              // Pointer to scene's stack-allocated deck
    float state_timer;         // For timed transitions
    int round_number;          // Current round counter
} GameContext_t;

// Usage: Stack-allocated singleton
static GameContext_t g_game;
InitGameContext(&g_game, &g_test_deck);
```

## State Transitions

```c
void TransitionState(GameContext_t* game, GameState_t new_state) {
    d_LogInfoF("State: %s -> %s",
               StateToString(game->current_state),
               StateToString(new_state));

    game->previous_state = game->current_state;
    game->current_state = new_state;
    game->state_timer = 0.0f;

    // State entry actions
    switch (new_state) {
        case STATE_BETTING:
            // Reset player states
            break;
        case STATE_DEALING:
            DealInitialHands(game);
            break;
        case STATE_PLAYER_TURN:
            game->current_player_index = 0;
            break;
        default:
            break;
    }
}
```

## State Update Loop

```c
void UpdateGameLogic(GameContext_t* game, float dt) {
    game->state_timer += dt;

    switch (game->current_state) {
        case STATE_MENU:
            UpdateMenuState(game);
            break;

        case STATE_BETTING:
            UpdateBettingState(game);
            break;

        case STATE_DEALING:
            if (game->state_timer > 1.0f) {  // Animation delay
                TransitionState(game, STATE_PLAYER_TURN);
            }
            break;

        case STATE_PLAYER_TURN:
            UpdatePlayerTurnState(game, dt);
            break;

        case STATE_DEALER_TURN:
            UpdateDealerTurnState(game, dt);
            break;

        case STATE_SHOWDOWN:
            UpdateShowdownState(game);
            break;

        case STATE_ROUND_END:
            if (game->state_timer > 3.0f) {  // Display results for 3s
                TransitionState(game, STATE_BETTING);
            }
            break;
    }
}
```

## State Implementations

### Betting State
```c
void UpdateBettingState(GameContext_t* game) {
    bool all_bets_placed = true;

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player->is_dealer) continue;

        if (player->current_bet == 0) {
            all_bets_placed = false;

            if (player->is_ai) {
                int bet = 10 + (rand() % 91);  // 10-100
                PlaceBet(player, bet);
            }
            // Human player waits for UI input
        }
    }

    if (all_bets_placed) {
        TransitionState(game, STATE_DEALING);
    }
}
```

### Player Turn State
```c
void UpdatePlayerTurnState(GameContext_t* game, float dt) {
    int* current_id = (int*)d_IndexDataFromArray(
        game->active_players, game->current_player_index
    );
    Player_t* player = GetPlayerByID(*current_id);

    // Skip dealer
    if (player->is_dealer) {
        game->current_player_index++;
        if (game->current_player_index >= (int)game->active_players->count) {
            TransitionState(game, STATE_DEALER_TURN);
        }
        return;
    }

    // Skip if already finished
    if (player->state == PLAYER_STATE_BUST ||
        player->state == PLAYER_STATE_STAND ||
        player->state == PLAYER_STATE_BLACKJACK) {

        game->current_player_index++;
        if (game->current_player_index >= (int)game->active_players->count) {
            TransitionState(game, STATE_DEALER_TURN);
        }
        return;
    }

    // AI decision
    if (player->is_ai) {
        if (game->state_timer > 1.0f) {  // Delay for realism
            PlayerAction_t action = GetAIAction(player, GetDealerUpcard(game));
            ExecutePlayerAction(game, player, action);
        }
    }
    // Human player waits for input (handled in input.c)
}
```

### Dealer Turn State

**Note: Hand_t is embedded value type, DealCard returns by value**

```c
void UpdateDealerTurnState(GameContext_t* game, float dt) {
    Player_t* dealer = GetPlayerByID(0);

    // Reveal hidden card (access embedded hand with &)
    if (game->state_timer < 0.5f) {
        Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
        hidden->face_up = true;
        return;
    }

    // Dealer hits on 16 or less
    if (dealer->hand.total_value < 17) {
        if (game->state_timer > 1.0f) {  // Delay between hits
            Card_t card = DealCard(game->deck);  // Returns by value
            if (card.card_id == -1) {
                TransitionState(game, STATE_SHOWDOWN);
                return;
            }

            card.face_up = true;
            LoadCardTexture(&card);
            AddCardToHand(&dealer->hand, card);  // Hand_t is value type, use &
            game->state_timer = 0.5f;  // Reset for next hit
        }
    } else {
        TransitionState(game, STATE_SHOWDOWN);
    }
}
```

### Showdown State

**Note: Hand_t is embedded value type, access with .**

```c
void UpdateShowdownState(GameContext_t* game) {
    Player_t* dealer = GetPlayerByID(0);
    int dealer_value = dealer->hand.total_value;  // Value type access
    bool dealer_bust = dealer->hand.is_bust;

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player->is_dealer) continue;

        // Determine outcome (Hand_t is embedded, use . access)
        if (player->hand.is_bust) {
            LoseBet(player);
        } else if (player->hand.is_blackjack && !dealer->hand.is_blackjack) {
            WinBet(player, 2.5f);  // 3:2 payout
        } else if (dealer_bust) {
            WinBet(player, 2.0f);
        } else if (player->hand.total_value > dealer_value) {
            WinBet(player, 2.0f);
        } else if (player->hand.total_value < dealer_value) {
            LoseBet(player);
        } else {
            ReturnBet(player);  // Push
        }
    }

    TransitionState(game, STATE_ROUND_END);
}
```

## Dynamic State Variables (dTable)

```c
// Store game-specific state
void SetStateVariable(GameContext_t* game, const char* key, void* value, size_t size) {
    d_SetDataInTable(game->state_data, (void*)&key, value);
}

void* GetStateVariable(GameContext_t* game, const char* key) {
    return d_GetDataFromTable(game->state_data, (void*)&key);
}

// Example usage
char* msg_key = "message";
char* msg_val = "Place your bets!";
SetStateVariable(game, msg_key, &msg_val, sizeof(char*));

char** msg = (char**)GetStateVariable(game, msg_key);
printf("%s\n", *msg);
```

## State Diagram

```
        ┌─────────┐
        │  MENU   │
        └────┬────┘
             │
             ▼
        ┌─────────┐
        │ BETTING │◄────────────┐
        └────┬────┘             │
             │                  │
             ▼                  │
        ┌─────────┐             │
        │ DEALING │             │
        └────┬────┘             │
             │                  │
             ▼                  │
     ┌──────────────┐           │
     │ PLAYER_TURN  │           │
     └───────┬──────┘           │
             │                  │
             ▼                  │
     ┌──────────────┐           │
     │ DEALER_TURN  │           │
     └───────┬──────┘           │
             │                  │
             ▼                  │
       ┌──────────┐             │
       │ SHOWDOWN │             │
       └────┬─────┘             │
            │                   │
            ▼                   │
       ┌──────────┐             │
       │ ROUND_END├─────────────┘
       └──────────┘
```
