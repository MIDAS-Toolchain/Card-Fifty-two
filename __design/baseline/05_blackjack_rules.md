# Blackjack Rules Implementation

## Hand Value Calculation

```c
int CalculateBlackjackValue(const Hand_t* hand) {
    int value = 0;
    int aces = 0;

    // Sum all cards
    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);

        if (card->rank == RANK_ACE) {
            aces++;
            value += 11;  // Start with 11
        } else if (card->rank >= RANK_JACK) {
            value += 10;  // Face cards
        } else {
            value += card->rank;
        }
    }

    // Adjust aces from 11 to 1 if busting
    while (value > 21 && aces > 0) {
        value -= 10;
        aces--;
    }

    return value;
}
```

## Blackjack Detection

```c
bool IsBlackjack(const Hand_t* hand) {
    return hand->cards->count == 2 && hand->total_value == 21;
}

bool IsBust(const Hand_t* hand) {
    return hand->total_value > 21;
}
```

## Player Actions

```c
typedef enum {
    ACTION_HIT,      // Take another card
    ACTION_STAND,    // End turn
    ACTION_DOUBLE,   // Double bet, take one card, end turn
    ACTION_SPLIT     // Split pairs (future)
} PlayerAction_t;

void ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action) {
    switch (action) {
        case ACTION_HIT:
            {
                Card_t card = DealCard(game->deck);  // Returns by value
                if (card.card_id == -1) {
                    d_LogError("Failed to deal card - deck empty");
                    return;
                }
                card.face_up = true;
                AddCardToHand(&player->hand, card);  // Hand_t is embedded value type

                if (player->hand.is_bust) {  // Hand_t is value type, use .
                    player->state = PLAYER_STATE_BUST;
                    game->current_player_index++;
                }
            }
            break;

        case ACTION_STAND:
            player->state = PLAYER_STATE_STAND;
            game->current_player_index++;
            break;

        case ACTION_DOUBLE:
            if (player->chips >= player->current_bet) {
                player->chips -= player->current_bet;
                player->current_bet *= 2;

                Card_t card = DealCard(game->deck);  // Returns by value
                if (card.card_id == -1) {
                    d_LogError("Failed to deal card - deck empty");
                    return;
                }
                card.face_up = true;
                AddCardToHand(&player->hand, card);  // Hand_t is embedded value type

                player->state = player->hand.is_bust ? PLAYER_STATE_BUST : PLAYER_STATE_STAND;
                game->current_player_index++;
            }
            break;

        case ACTION_SPLIT:
            // Future implementation
            break;
    }
}
```

## Dealer Logic (Hits on 16, Stands on 17)

```c
void DealerTurn(GameContext_t* game) {
    Player_t* dealer = GetPlayerByID(0);

    // Reveal hidden card (Hand_t is embedded value type)
    Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
    hidden->face_up = true;

    // Dealer must hit on 16 or less
    while (dealer->hand.total_value < 17) {
        Card_t card = DealCard(game->deck);  // Returns by value
        if (card.card_id == -1) {
            d_LogError("Failed to deal card - deck empty");
            return;
        }
        card.face_up = true;
        AddCardToHand(&dealer->hand, card);  // Hand_t is embedded value type
    }
}
```

## Payout Rules

```c
void ResolveRound(GameContext_t* game) {
    Player_t* dealer = GetPlayerByID(0);
    int dealer_value = dealer->hand.total_value;  // Hand_t is embedded value type
    bool dealer_bust = dealer->hand.is_bust;

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player->is_dealer) continue;

        if (player->hand.is_bust) {  // Hand_t is value type, use .
            LoseBet(player);  // Lose bet
        } else if (player->hand.is_blackjack) {
            if (dealer->hand.is_blackjack) {
                ReturnBet(player);  // Push
            } else {
                WinBet(player, 2.5f);  // 3:2 payout for blackjack
            }
        } else if (dealer_bust) {
            WinBet(player, 2.0f);  // Win if dealer busts
        } else if (player->hand.total_value > dealer_value) {
            WinBet(player, 2.0f);  // Win if higher
        } else if (player->hand.total_value < dealer_value) {
            LoseBet(player);  // Lose if lower
        } else {
            ReturnBet(player);  // Push if equal
        }
    }
}
```

## Basic Strategy (AI)

```c
PlayerAction_t BasicStrategyDecision(const Hand_t* player_hand, const Card_t* dealer_upcard) {
    int player_value = player_hand->total_value;
    int dealer_value = GetBlackjackValue(dealer_upcard, true);

    // Always hit on 11 or less
    if (player_value <= 11) return ACTION_HIT;

    // Always stand on 17+
    if (player_value >= 17) return ACTION_STAND;

    // 12-16: Hit if dealer shows 7+ (strong), stand if 2-6 (weak)
    if (player_value >= 12 && player_value <= 16) {
        return (dealer_value >= 7) ? ACTION_HIT : ACTION_STAND;
    }

    return ACTION_STAND;
}
```

## Game Rules Summary

| Rule | Implementation |
|------|---------------|
| **Card Values** | 2-10 = face value, J/Q/K = 10, A = 1 or 11 |
| **Blackjack** | Ace + 10-value card on initial deal |
| **Dealer Hits** | Must hit on 16 or less |
| **Dealer Stands** | Must stand on 17+ |
| **Player Bust** | Over 21 = automatic loss |
| **Blackjack Payout** | 3:2 (bet × 2.5) |
| **Win Payout** | 1:1 (bet × 2.0) |
| **Push** | Tie = return bet |
