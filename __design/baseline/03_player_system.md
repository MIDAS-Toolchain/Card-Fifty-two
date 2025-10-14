# Player System Design

## Player Structure

**Constitutional Pattern: dString_t for name, Hand_t embedded as value**

```c
typedef struct Player {
    dString_t* name;       // Constitutional: dString_t, not char[]
    int player_id;         // Unique ID (0 = dealer, 1+ = players)
    Hand_t hand;           // VALUE TYPE - embedded, not pointer
    int chips;
    int current_bet;
    bool is_dealer;
    bool is_ai;
    PlayerState_t state;
} Player_t;

typedef enum {
    PLAYER_STATE_WAITING,
    PLAYER_STATE_BETTING,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_STAND,
    PLAYER_STATE_BUST,
    PLAYER_STATE_BLACKJACK,
    PLAYER_STATE_WON,
    PLAYER_STATE_LOST,
    PLAYER_STATE_PUSH
} PlayerState_t;
```

## Hand Structure

```c
typedef struct Hand {
    dArray_t* cards;       // dArray of Card_t (value copies)
    int total_value;       // Cached score
    bool is_bust;
    bool is_blackjack;
} Hand_t;
```

## Player Lifecycle

### Creation (Heap-Allocated)

**Constitutional Pattern: dString_t for name, InitHand for embedded Hand_t**

```c
Player_t* CreatePlayer(const char* name, int id, bool is_dealer) {
    Player_t* player = malloc(sizeof(Player_t));

    // Constitutional pattern: dString_t for name
    player->name = d_InitString();
    d_SetString(player->name, name);

    player->player_id = id;

    // Hand_t is VALUE TYPE - initialize in place
    InitHand(&player->hand);

    player->chips = STARTING_CHIPS;
    player->current_bet = 0;
    player->is_dealer = is_dealer;
    player->is_ai = is_dealer;  // Dealer is always AI
    player->state = PLAYER_STATE_WAITING;

    return player;
}
```

### Destruction

**Constitutional Pattern: Cleanup embedded Hand_t, destroy dString_t**

```c
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

## Hand Lifecycle

**Constitutional Pattern: Hand_t is embedded value type, uses Init/Cleanup**

```c
void InitHand(Hand_t* hand) {
    if (!hand) {
        d_LogFatal("InitHand: NULL hand pointer");
        return;
    }

    hand->cards = d_InitArray(sizeof(Card_t), 10);
    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}

void CleanupHand(Hand_t* hand) {
    if (!hand) return;

    if (hand->cards) {
        d_DestroyArray(hand->cards);
        hand->cards = NULL;
    }
    // NOTE: Does NOT free hand pointer (embedded value type)
}
```

## Hand Operations

### Add Card
```c
void AddCardToHand(Hand_t* hand, Card_t card) {
    d_AppendDataToArray(hand->cards, &card);
    hand->total_value = CalculateHandValue(hand);  // Game-specific
    hand->is_bust = (hand->total_value > 21);
    hand->is_blackjack = (hand->cards->count == 2 && hand->total_value == 21);
}
```

### Clear Hand

**Note: Can optionally discard cards back to deck**

```c
void ClearHand(Hand_t* hand, Deck_t* deck) {
    // Discard cards back to deck
    if (deck && hand->cards) {
        for (size_t i = 0; i < hand->cards->count; i++) {
            Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
            DiscardCard(deck, *card);
        }
    }

    d_ClearArray(hand->cards);  // Empties without freeing
    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}
```

## Chip Operations

### Place Bet
```c
bool PlaceBet(Player_t* player, int amount) {
    if (amount <= 0 || amount > player->chips) {
        d_LogWarningF("Invalid bet: %d (chips: %d)", amount, player->chips);
        return false;
    }

    player->current_bet = amount;
    player->chips -= amount;
    player->state = PLAYER_STATE_PLAYING;

    d_LogInfoF("%s bet %d chips (%d remaining)",
               player->name, amount, player->chips);
    return true;
}
```

### Win Bet
```c
void WinBet(Player_t* player, float multiplier) {
    int winnings = (int)(player->current_bet * multiplier);
    player->chips += winnings;
    player->state = PLAYER_STATE_WON;

    d_LogInfoF("%s won %d chips (total: %d)",
               player->name, winnings, player->chips);
}
```

### Lose Bet
```c
void LoseBet(Player_t* player) {
    player->state = PLAYER_STATE_LOST;
    d_LogInfoF("%s lost %d chips (%d remaining)",
               player->name, player->current_bet, player->chips);
    player->current_bet = 0;
}
```

### Return Bet (Push)
```c
void ReturnBet(Player_t* player) {
    player->chips += player->current_bet;
    player->state = PLAYER_STATE_PUSH;
    d_LogInfoF("%s push - returned %d chips", player->name, player->current_bet);
    player->current_bet = 0;
}
```

## Player Registry (dTable)

### Global Table
```c
// In main.c
dTable_t* g_players = NULL;  // Key: int (player_id), Value: Player_t*
```

### Add Player
```c
void RegisterPlayer(Player_t* player) {
    d_SetDataInTable(g_players, &player->player_id, &player);
    d_LogInfoF("Registered player: %s (ID: %d)", player->name, player->player_id);
}
```

### Lookup Player
```c
Player_t* GetPlayerByID(int player_id) {
    Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &player_id);
    return player_ptr ? *player_ptr : NULL;
}
```

### Iterate All Players

**Note: Hand_t is embedded value type, access with &**

```c
void ResetAllPlayers(Deck_t* deck) {
    dArray_t* ids = d_GetAllKeysFromTable(g_players);

    for (size_t i = 0; i < ids->count; i++) {
        int* id = (int*)d_IndexDataFromArray(ids, i);
        Player_t* player = GetPlayerByID(*id);

        ClearHand(&player->hand, deck);  // Hand_t is value type, use &
        player->current_bet = 0;
        player->state = PLAYER_STATE_WAITING;
    }

    d_DestroyArray(ids);
}
```

## Hand Rendering

```c
void RenderHand(const Hand_t* hand, int base_x, int base_y, bool fan_out) {
    const int CARD_SPACING = fan_out ? 40 : 20;
    const int CARD_OFFSET_Y = fan_out ? 15 : 0;

    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);

        card->x = base_x + (i * CARD_SPACING);
        card->y = base_y + (i * CARD_OFFSET_Y);

        RenderCard(card);
    }
}
```

## Player AI (Basic)

```c
PlayerAction_t GetAIAction(Player_t* player, const Card_t* dealer_upcard) {
    if (player->hand->total_value < 12) {
        return ACTION_HIT;
    }
    if (player->hand->total_value >= 17) {
        return ACTION_STAND;
    }

    // Basic strategy based on dealer's card
    int dealer_value = GetBlackjackValue(dealer_upcard, true);

    if (dealer_value >= 7) {
        return ACTION_HIT;  // Dealer strong, player must improve
    } else {
        return ACTION_STAND;  // Dealer weak, let dealer bust
    }
}
```
