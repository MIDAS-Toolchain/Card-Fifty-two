# Blackjack Core - Game Loop & Rules

**Source:** `src/game.c`, `src/hand.c`, `src/player.c`
**Last Updated:** 2025-10-07
**Status:** =' Needs Improvements (documented below)

## Overview

This document describes the Blackjack-specific game loop, rules implementation, and the improved timing/reveal mechanics needed for proper gameplay and card counting strategy.

## Current Implementation Issues

### L Problems with Current Game Loop

1. **Instant Dealer Turn:**
   - Dealer reveals card and hits in a `while` loop (instant, no animation)
   - No visual feedback for dealer decisions
   - Player can't follow dealer's reasoning

2. **No Action Delays:**
   - Player makes choice � immediately jumps to dealer turn
   - No pause between player action and dealer response
   - Feels abrupt and confusing

3. **Hidden Card Reveal Logic:**
   - Current code reveals hidden card at start of dealer turn
   - But doesn't handle player bust case properly
   - **Critical Bug:** If player busts, hidden card stays hidden � breaks card counting

4. **No Animation System:**
   - All card movements instant
   - No flip animations for reveals
   - No slide animations for dealing

## Improved Blackjack Game Loop

### Phase 1: Initial Deal (STATE_DEALING)

**Timing:** Handled in `DealInitialHands()`, triggered on `STATE_DEALING` entry

```c
void DealInitialHands(GameContext_t* game) {
    // Deal 2 rounds of cards to all players
    for (int round = 0; round < 2; round++) {
        for (size_t i = 0; i < game->active_players->count; i++) {
            int* id = (int*)d_IndexDataFromArray(game->active_players, i);
            Player_t* player = GetPlayerByID(*id);

            Card_t card = DealCard(game->deck);

            // Dealer's FIRST card is face-down (hidden)
            card.face_up = !(player->is_dealer && round == 0);

            LoadCardTexture(&card);
            AddCardToHand(&player->hand, card);
        }
    }

    // Check for natural blackjacks
    // Set player states to PLAYER_STATE_BLACKJACK if applicable
}
```

**Result:**
- Dealer: 2 cards (1 hidden, 1 face-up)
- Player: 2 cards (both face-up)
- Dealer knows their hidden card value for AI decisions
- Player cannot see hidden card yet

**Transition:** After 1.0s delay (`UpdateDealingState`), move to `STATE_PLAYER_TURN`

### Phase 2: Player Turn (STATE_PLAYER_TURN)

**Current Code Location:** `UpdatePlayerTurnState()` in `src/game.c:222`

**Player Actions:**

```c
typedef enum {
    ACTION_HIT,      // Take another card
    ACTION_STAND,    // End turn
    ACTION_DOUBLE,   // Double bet, take one card, end turn
    ACTION_SPLIT     // Split pairs (future)
} PlayerAction_t;
```

**Action Execution:**

```c
void ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action) {
    switch (action) {
        case ACTION_HIT:
            Card_t card = DealCard(game->deck);
            card.face_up = true;
            AddCardToHand(&player->hand, card);

            if (player->hand.is_bust) {
                player->state = PLAYER_STATE_BUST;
                game->current_player_index++;  // Move to next player
            }
            break;

        case ACTION_STAND:
            player->state = PLAYER_STATE_STAND;
            game->current_player_index++;
            break;

        case ACTION_DOUBLE:
            // Can only double on first decision (2 cards)
            if (player->hand.cards->count != 2) return;
            if (!CanAffordBet(player, player->current_bet)) return;

            PlaceBet(player, player->current_bet);  // Double the bet
            Card_t card = DealCard(game->deck);
            card.face_up = true;
            AddCardToHand(&player->hand, card);

            player->state = player->hand.is_bust ? PLAYER_STATE_BUST : PLAYER_STATE_STAND;
            game->current_player_index++;
            break;
    }
}
```

### � REQUIRED CHANGE: Add Delay After Player Action

**Current Problem:** Player action executes � immediately jumps to dealer turn

**Solution:** Add 0.5s delay after player action before transitioning to dealer turn

```c
// New timing constant
#define PLAYER_ACTION_DELAY 0.5f

// Modified UpdatePlayerTurnState
void UpdatePlayerTurnState(GameContext_t* game, float dt) {
    // ... existing player selection logic ...

    // Check if player just executed an action
    Player_t* current = GetCurrentPlayer(game);
    if (current && (current->state == PLAYER_STATE_BUST ||
                    current->state == PLAYER_STATE_STAND ||
                    current->state == PLAYER_STATE_BLACKJACK)) {

        // Start delay timer if just finished
        bool* awaiting_transition = GetStateVariable(game, "awaiting_dealer");
        if (!awaiting_transition || !*awaiting_transition) {
            game->state_timer = 0.0f;  // Reset timer
            bool flag = true;
            SetStateVariable(game, "awaiting_dealer", &flag);
        }

        // Wait for delay before transitioning
        if (game->state_timer >= PLAYER_ACTION_DELAY) {
            // All players done, move to dealer
            if (game->current_player_index >= (int)game->active_players->count) {
                TransitionState(game, STATE_DEALER_TURN);
            }
        }
    }
}
```

### Phase 3: Dealer Turn (STATE_DEALER_TURN) - NEEDS MAJOR REFACTOR

**Current Code:** `UpdateDealerTurnState()` in `src/game.c:273`

**Current Problems:**
- L Reveals card instantly
- L Hits in a `while` loop (no animation)
- L No delay between actions
- L Doesn't always reveal hidden card before discard (breaks card counting)

### =' REQUIRED: New Dealer Turn Implementation

**Critical Rule for Card Counting:** Dealer's hidden card MUST be revealed before cards go to discard pile. Reveal happens in one of these scenarios:
1. **Player stood last turn** → Reveal at start of dealer turn
2. **Dealer decides to stand** → Reveal when standing
3. **All players busted** → Reveal before showdown (for card counting)

**Hidden card stays hidden if:** Player hit/busted without standing (dealer wins immediately, but card revealed before discard).

**New Dealer Turn Phases:**

```c
typedef enum {
    DEALER_PHASE_CHECK_REVEAL,  // Check if we should reveal (player stood?)
    DEALER_PHASE_DECIDE,        // Make hit/stand decision
    DEALER_PHASE_ACTION,        // Execute action (hit or stand-with-reveal)
    DEALER_PHASE_WAIT           // Wait 0.5s before next decision
} DealerPhase_t;
```

**New Implementation:**

```c
#define DEALER_ACTION_DELAY 0.5f
#define CARD_REVEAL_DELAY   0.3f

void UpdateDealerTurnState(GameContext_t* game, float dt) {
    Player_t* dealer = GetPlayerByID(0);
    if (!dealer) {
        TransitionState(game, STATE_SHOWDOWN);
        return;
    }

    // Get current phase
    DealerPhase_t* phase_ptr = GetStateVariable(game, "dealer_phase");
    DealerPhase_t phase = phase_ptr ? *phase_ptr : DEALER_PHASE_CHECK_REVEAL;

    switch (phase) {
        case DEALER_PHASE_CHECK_REVEAL:
            // Check if all players busted
            bool all_busted = true;
            for (size_t i = 0; i < game->active_players->count; i++) {
                int* id = (int*)d_IndexDataFromArray(game->active_players, i);
                Player_t* p = GetPlayerByID(*id);
                if (p && !p->is_dealer && p->state != PLAYER_STATE_BUST) {
                    all_busted = false;
                    break;
                }
            }

            if (all_busted) {
                // Everyone busted - reveal card for counting, then end
                if (dealer->hand.cards->count > 0) {
                    Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
                    if (hidden && !hidden->face_up) {
                        hidden->face_up = true;
                        d_LogInfo("Dealer reveals hidden card (all players busted - card counting)");
                    }
                }
                TransitionState(game, STATE_SHOWDOWN);
                return;
            }

            // Check if player stood (triggers reveal)
            bool* player_stood = (bool*)GetStateVariable(game, "player_stood");
            if (player_stood && *player_stood) {
                // Wait for reveal animation time
                if (game->state_timer >= CARD_REVEAL_DELAY) {
                    if (dealer->hand.cards->count > 0) {
                        Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
                        if (hidden && !hidden->face_up) {
                            hidden->face_up = true;
                            d_LogInfoF("Dealer reveals hidden card (player stood) - total: %d",
                                      dealer->hand.total_value);
                        }
                    }
                    // Move to decide phase
                    DealerPhase_t next = DEALER_PHASE_DECIDE;
                    SetStateVariable(game, "dealer_phase", &next);
                    game->state_timer = 0.0f;
                }
            } else {
                // Player didn't stand (hit/busted), card stays hidden for now
                DealerPhase_t next = DEALER_PHASE_DECIDE;
                SetStateVariable(game, "dealer_phase", &next);
                game->state_timer = 0.0f;
            }
            break;

        case DEALER_PHASE_DECIDE:
            // Dealer decides: hit on <17, stand on >=17
            if (dealer->hand.total_value < 17) {
                d_LogInfoF("Dealer has %d - must hit", dealer->hand.total_value);

                // Move to action phase (HIT)
                DealerPhase_t next = DEALER_PHASE_ACTION;
                SetStateVariable(game, "dealer_phase", &next);
                bool hit = true;
                SetStateVariable(game, "dealer_action_hit", &hit);
            } else {
                d_LogInfoF("Dealer has %d - must stand", dealer->hand.total_value);

                // Move to action phase (STAND with reveal)
                DealerPhase_t next = DEALER_PHASE_ACTION;
                SetStateVariable(game, "dealer_phase", &next);
                bool hit = false;
                SetStateVariable(game, "dealer_action_hit", &hit);
            }
            game->state_timer = 0.0f;
            break;

        case DEALER_PHASE_ACTION:
            {
                bool* is_hit = (bool*)GetStateVariable(game, "dealer_action_hit");
                if (is_hit && *is_hit) {
                    // HIT: Deal card to dealer
                    Card_t card = DealCard(game->deck);
                    if (card.card_id != -1) {
                        card.face_up = true;
                        LoadCardTexture(&card);
                        AddCardToHand(&dealer->hand, card);
                        d_LogInfoF("Dealer hits - new total: %d", dealer->hand.total_value);
                    }
                } else {
                    // STAND: Reveal hidden card if not already revealed
                    if (dealer->hand.cards->count > 0) {
                        Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
                        if (hidden && !hidden->face_up) {
                            hidden->face_up = true;
                            d_LogInfo("Dealer reveals hidden card on STAND");
                        }
                    }
                }

                // Move to wait phase
                game->state_timer = 0.0f;
                DealerPhase_t next = DEALER_PHASE_WAIT;
                SetStateVariable(game, "dealer_phase", &next);
            }
            break;

        case DEALER_PHASE_WAIT:
            // Wait before next decision
            if (game->state_timer < DEALER_ACTION_DELAY) {
                break;
            }

            // Check if dealer busted or should stand
            if (dealer->hand.is_bust) {
                d_LogInfoF("Dealer busts with %d", dealer->hand.total_value);
                TransitionState(game, STATE_SHOWDOWN);
            } else if (dealer->hand.total_value >= 17) {
                // Dealer must stand at 17+
                TransitionState(game, STATE_SHOWDOWN);
            } else {
                // Continue deciding (dealer has < 17)
                game->state_timer = 0.0f;
                DealerPhase_t next = DEALER_PHASE_DECIDE;
                SetStateVariable(game, "dealer_phase", &next);
            }
            break;
    }
}
```

### Hidden Card Reveal Logic - Implementation Summary

**Three Reveal Scenarios:**

1. **Player Stood Last Turn**
   - `UpdatePlayerTurnState` sets `"player_stood"` flag when player stands
   - `DEALER_PHASE_CHECK_REVEAL` checks flag at dealer turn start
   - If true: waits CARD_REVEAL_DELAY (0.3s) then reveals hidden card
   - Card is face-up before dealer makes first decision

2. **Dealer Stands**
   - Dealer reaches 17+, decides to stand
   - `DEALER_PHASE_ACTION` with `dealer_action_hit = false`
   - Reveals hidden card when executing STAND action
   - Card visible before transitioning to SHOWDOWN

3. **All Players Busted**
   - `DEALER_PHASE_CHECK_REVEAL` checks all player states
   - If all PLAYER_STATE_BUST: reveals card immediately
   - Transitions to SHOWDOWN (dealer wins, but card counting preserved)

**Result:** Every card visible before entering discard pile, maintaining card counting strategic depth.

### Phase 4: Showdown (STATE_SHOWDOWN)

**Current Code:** `ResolveRound()` in `src/game.c:571`

**Process:**
1. Get dealer's final total
2. Compare each player's hand against dealer
3. Apply payout rules
4. Set player states (WON/LOST/PUSH/BLACKJACK)

**Payout Rules:**

| Outcome | Payout | Player State |
|---------|--------|--------------|
| Player busts | Lose bet | `PLAYER_STATE_LOST` |
| Player blackjack, dealer no blackjack | Bet � 2.5 (3:2) | `PLAYER_STATE_WON` |
| Both blackjack | Return bet | `PLAYER_STATE_PUSH` |
| Dealer busts, player didn't | Bet � 2 (1:1) | `PLAYER_STATE_WON` |
| Player total > dealer total | Bet � 2 (1:1) | `PLAYER_STATE_WON` |
| Player total < dealer total | Lose bet | `PLAYER_STATE_LOST` |
| Player total = dealer total | Return bet | `PLAYER_STATE_PUSH` |

```c
void ResolveRound(GameContext_t* game) {
    Player_t* dealer = GetPlayerByID(0);
    int dealer_value = dealer->hand.total_value;
    bool dealer_bust = dealer->hand.is_bust;

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player->is_dealer) continue;

        if (player->hand.is_bust) {
            LoseBet(player);
            player->state = PLAYER_STATE_LOST;
        } else if (player->hand.is_blackjack) {
            if (dealer->hand.is_blackjack) {
                ReturnBet(player);  // Push
                player->state = PLAYER_STATE_PUSH;
            } else {
                WinBet(player, 1.5f);  // 3:2 payout (returns bet + 1.5� bet)
                player->state = PLAYER_STATE_WON;
            }
        } else if (dealer_bust) {
            WinBet(player, 1.0f);  // 1:1 payout (returns bet + bet)
            player->state = PLAYER_STATE_WON;
        } else if (player->hand.total_value > dealer_value) {
            WinBet(player, 1.0f);
            player->state = PLAYER_STATE_WON;
        } else if (player->hand.total_value < dealer_value) {
            LoseBet(player);
            player->state = PLAYER_STATE_LOST;
        } else {
            ReturnBet(player);  // Push
            player->state = PLAYER_STATE_PUSH;
        }
    }
}
```

**Transition:** Immediately move to `STATE_ROUND_END`

### Phase 5: Round End (STATE_ROUND_END)

**Current Code:** `UpdateRoundEndState()` in `src/game.c:322`

**Display Results:** Show winner/loser for 3 seconds

```c
#define ROUND_END_DISPLAY 3.0f

void UpdateRoundEndState(GameContext_t* game) {
    if (game->state_timer > ROUND_END_DISPLAY) {
        StartNewRound(game);
        TransitionState(game, STATE_BETTING);
    }
}
```

**Cleanup:** `StartNewRound()` clears all hands

```c
void StartNewRound(GameContext_t* game) {
    // Clear all hands (returns cards to discard)
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player) {
            ClearHand(&player->hand, game->deck);  // Moves cards to discard
            player->current_bet = 0;
            player->state = PLAYER_STATE_WAITING;
        }
    }

    // Reshuffle if needed
    if (GetDeckSize(game->deck) < 20) {
        d_LogInfo("Reshuffling deck...");
        ResetDeck(game->deck);
    }

    game->round_number++;
    game->current_player_index = 0;
}
```

## Blackjack Rules Implementation

### Hand Value Calculation

```c
int CalculateBlackjackValue(const Hand_t* hand) {
    int value = 0;
    int aces = 0;

    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);

        if (card->rank == RANK_ACE) {
            aces++;
            value += 11;  // Start with 11
        } else if (card->rank >= RANK_JACK) {
            value += 10;  // Face cards = 10
        } else {
            value += card->rank;  // 2-10 = face value
        }
    }

    // Adjust aces from 11 to 1 if busting
    while (value > 21 && aces > 0) {
        value -= 10;  // Convert one ace from 11 to 1
        aces--;
    }

    return value;
}
```

### Natural Blackjack Detection

```c
bool IsBlackjack(const Hand_t* hand) {
    return hand->cards->count == 2 && hand->total_value == 21;
}
```

**Natural Blackjack:**
- Exactly 2 cards
- Total value 21
- Must be from initial deal
- Pays 3:2 (bet � 2.5)

**Regular 21:**
- More than 2 cards totaling 21
- Not a natural blackjack
- Pays 1:1 (bet � 2.0)

### Dealer AI Strategy

**House Rules:**
- Dealer MUST hit on 16 or less
- Dealer MUST stand on 17 or more
- No player-style decision making
- Dealer knows their hidden card value

```c
// Dealer's hidden card value calculation (at deal time)
void DealInitialHands(GameContext_t* game) {
    // ... dealing loop ...

    // After dealing, calculate dealer's hidden total for AI
    Player_t* dealer = GetPlayerByID(0);
    if (dealer && dealer->is_dealer) {
        int hidden_value = dealer->hand.total_value;
        SetStateVariable(game, "dealer_hidden_value", &hidden_value);
    }
}
```

### Player AI Strategy (Basic Strategy)

```c
PlayerAction_t GetAIAction(const Player_t* player, const Card_t* dealer_upcard) {
    int player_value = player->hand.total_value;
    int dealer_value = (dealer_upcard->rank >= RANK_JACK) ? 10 : dealer_upcard->rank;

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

## Timing Constants Summary

```c
// State transition delays
#define DEALING_ANIMATION_TIME  1.0f   // Time to show dealt cards
#define PLAYER_ACTION_DELAY     0.5f   // Delay after player action
#define DEALER_ACTION_DELAY     0.5f   // Delay between dealer actions
#define CARD_REVEAL_DELAY       0.3f   // Time to flip hidden card
#define ROUND_END_DISPLAY       3.0f   // Time to show results

// Deck management
#define RESHUFFLE_THRESHOLD     20     // Cards remaining before reshuffle
```

## TODO: Animation System

**Required for Archimedes framework:**

```c
// Tween/Animation system (to be added to Archimedes)
typedef struct Tween {
    float* target_value;        // Pointer to value being animated
    float start_value;
    float end_value;
    float duration;
    float elapsed;
    TweenEasing_t easing;      // LINEAR, EASE_IN, EASE_OUT, etc.
} Tween_t;

// Card-specific animations
typedef struct CardAnimation {
    Card_t* card;
    AnimationType_t type;      // DEAL, FLIP, SLIDE, etc.
    float duration;
    float elapsed;
    int start_x, start_y;
    int end_x, end_y;
} CardAnimation_t;
```

**Needed Animations:**
1. **Card Dealing:** Slide from deck to hand position
2. **Card Flipping:** Reveal hidden card with flip animation
3. **Chip Movement:** Bet placement and payout animations
4. **Result Display:** Fade in/out result text
5. **Hand Evaluation:** Highlight winning/losing hands

**Usage Example:**
```c
// Deal card with animation
Card_t card = DealCard(deck);
CardAnimation_t* anim = CreateCardAnimation(
    &card,
    ANIM_DEAL,
    /*from*/ deck_x, deck_y,
    /*to*/ player_hand_x, player_hand_y,
    /*duration*/ 0.3f
);
AddAnimation(g_animation_manager, anim);

// Reveal hidden card with flip
CardAnimation_t* flip = CreateCardAnimation(
    hidden_card,
    ANIM_FLIP,
    /*current*/ card->x, card->y,
    /*same*/ card->x, card->y,
    /*duration*/ CARD_REVEAL_DELAY
);
```

## Card Counting & Strategic Depth

### Visible Information

**Player Can See:**
- All face-up cards (current hands)
- Discard pile contents
- Deck size (cards remaining)
- Discard size (cards played)

**Player Cannot See:**
- Cards remaining in deck (until drawn)
- Dealer's hidden card (until dealer turn)

### Card Counting Requirements

1. **ALL cards revealed before discard** 
2. **Discard pile always accessible** 
3. **Deck/discard counts visible in UI** 
4. **Reshuffle indicator shown** 

### Implementation for Card Counting UI

```c
// Display deck/discard info
void RenderDeckInfo(const Deck_t* deck, int x, int y) {
    dString_t* info = d_InitString();

    int deck_remaining = GetDeckSize(deck);
    int discard_count = GetDiscardSize(deck);

    d_FormatString(info, "Deck: %d | Discard: %d",
                   deck_remaining, discard_count);

    a_DrawText(d_PeekString(info), x, y,
               COLOR_INFO_TEXT.r, COLOR_INFO_TEXT.g, COLOR_INFO_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

    // Warning if reshuffle coming
    if (deck_remaining < RESHUFFLE_THRESHOLD) {
        d_SetString(info, "� Reshuffle Soon", 0);
        a_DrawText(d_PeekString(info), x, y + 25,
                   255, 255, 0,  // Yellow warning
                   FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    }

    d_DestroyString(info);
}
```

## Implementation Checklist

###  Already Implemented
- [x] Basic state machine (BETTING � DEALING � PLAYER_TURN � DEALER_TURN � SHOWDOWN � ROUND_END)
- [x] Hand value calculation with ace handling
- [x] Blackjack detection
- [x] Player actions (Hit, Stand, Double)
- [x] Dealer basic AI (hit on <17, stand on >=17)
- [x] Payout resolution
- [x] Round cleanup and reshuffle

### L Needs Implementation
- [ ] Player action delay (0.5s before dealer turn)
- [ ] Dealer turn phase system (reveal � decide � action � wait)
- [ ] **CRITICAL:** Always reveal hidden card before discard (even if player busts)
- [ ] Incremental dealer actions with delays
- [ ] Animation system in Archimedes
- [ ] Card dealing animations
- [ ] Card flip animations
- [ ] Deck/discard count UI
- [ ] Discard pile viewer modal

### =' Needs Refactoring
- [ ] `UpdateDealerTurnState()` - replace while loop with phase-based system
- [ ] `UpdatePlayerTurnState()` - add delay mechanism
- [ ] State variable usage - standardize key names and types

## Performance Considerations

**Current Performance:**
- State updates: O(1) per frame
- Player iteration: O(n) where n = active players (typically 1-4)
- Card lookups: O(1) via array indexing
- Player lookups: O(1) via hash table

**With Animation System:**
- Animation updates: O(m) where m = active animations (typically 1-5)
- No impact on game logic performance
- Rendering remains separate concern

## Testing Requirements

### Unit Tests Needed
1. **Hand Value Calculation:**
   - Test ace soft/hard totals
   - Test face card values
   - Test bust detection

2. **State Transitions:**
   - Test each transition path
   - Test invalid transitions rejected
   - Test state timer resets

3. **Payout Resolution:**
   - Test all outcome scenarios
   - Test blackjack 3:2 payout
   - Test push returns bet
   - Test double down doubles stake

4. **Card Counting:**
   - **Test hidden card always revealed**
   - Test discard pile tracking
   - Test reshuffle resets discard
   - Verify no cards lost/duplicated

### Integration Tests Needed
1. Complete round simulation (betting � resolution)
2. Multi-round with reshuffle
3. Player bust scenario (verify hidden card revealed!)
4. Dealer bust scenario
5. Natural blackjack both sides (push)

## Future Enhancements

1. **Split Pairs:**
   - Allow splitting same-rank initial cards
   - Create two separate hands
   - Play each hand independently

2. **Insurance:**
   - Offer when dealer shows ace
   - Side bet pays 2:1 if dealer has blackjack

3. **Surrender:**
   - Forfeit half bet before playing hand
   - Only on first decision

4. **Soft 17 Rule Variant:**
   - Dealer hits on soft 17 (Ace + 6)
   - Configurable house rule

5. **Multi-Deck Shoe:**
   - Use 6 or 8 decks
   - Cut card placement
   - Penetration percentage

---

**Related Documents:**
- `00_game_core.md` - Core state machine architecture
- `10_ideas_for_main_game.md` - Future game design (tags, relics, damage system)
- `__design/baseline/05_blackjack_rules.md` - Original rules reference
