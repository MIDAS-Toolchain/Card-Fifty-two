/*
 * Card Fifty-Two - Game State Machine Implementation
 * Constitutional pattern: dTable_t for state data, dArray_t for player list
 */

#include "../include/common.h"
#include "../include/game.h"
#include "../include/player.h"
#include "../include/deck.h"
#include "../include/hand.h"

// External globals
extern dTable_t* g_players;

// ============================================================================
// GAME CONTEXT LIFECYCLE
// ============================================================================

void InitGameContext(GameContext_t* game, Deck_t* deck) {
    if (!game) {
        d_LogFatal("InitGameContext: NULL game pointer");
        return;
    }

    if (!deck) {
        d_LogFatal("InitGameContext: NULL deck pointer");
        return;
    }

    // Initialize state machine
    game->current_state = STATE_BETTING;
    game->previous_state = STATE_BETTING;
    game->state_timer = 0.0f;
    game->round_number = 1;
    game->current_player_index = 0;
    game->deck = deck;

    // Initialize typed state tables (Constitutional: explicit types, no void*)
    game->bool_flags = d_InitTable(sizeof(char*), sizeof(bool),
                                    d_HashString, d_CompareString, 16);
    game->int_values = d_InitTable(sizeof(char*), sizeof(int),
                                    d_HashString, d_CompareString, 8);
    game->dealer_phase = d_InitTable(sizeof(char*), sizeof(DealerPhase_t),
                                      d_HashString, d_CompareString, 2);

    if (!game->bool_flags || !game->int_values || !game->dealer_phase) {
        d_LogFatal("InitGameContext: Failed to initialize state tables");
        if (game->bool_flags) d_DestroyTable(&game->bool_flags);
        if (game->int_values) d_DestroyTable(&game->int_values);
        if (game->dealer_phase) d_DestroyTable(&game->dealer_phase);
        return;
    }

    d_LogInfoF("InitGameContext: Tables initialized (bool_flags=%p, int_values=%p, dealer_phase=%p)",
               (void*)game->bool_flags,
               (void*)game->int_values,
               (void*)game->dealer_phase);

    // Initialize active players array (holds int player_ids)
    game->active_players = d_InitArray(8, sizeof(int));
    if (!game->active_players) {
        d_LogFatal("InitGameContext: Failed to initialize active_players array");
        d_DestroyTable(&game->bool_flags);
        d_DestroyTable(&game->int_values);
        d_DestroyTable(&game->dealer_phase);
        return;
    }

    d_LogInfo("GameContext initialized successfully");
}

void CleanupGameContext(GameContext_t* game) {
    if (!game) {
        d_LogError("CleanupGameContext: NULL game pointer");
        return;
    }

    // Destroy typed state tables
    if (game->bool_flags) {
        d_DestroyTable(&game->bool_flags);
        game->bool_flags = NULL;
    }

    if (game->int_values) {
        d_DestroyTable(&game->int_values);
        game->int_values = NULL;
    }

    if (game->dealer_phase) {
        d_DestroyTable(&game->dealer_phase);
        game->dealer_phase = NULL;
    }

    // Destroy active players array
    if (game->active_players) {
        d_DestroyArray(game->active_players);
        game->active_players = NULL;
    }

    d_LogInfo("GameContext cleaned up");
}

// ============================================================================
// STATE MACHINE
// ============================================================================

const char* StateToString(GameState_t state) {
    switch (state) {
        case STATE_MENU:        return "MENU";
        case STATE_BETTING:     return "BETTING";
        case STATE_DEALING:     return "DEALING";
        case STATE_PLAYER_TURN: return "PLAYER_TURN";
        case STATE_DEALER_TURN: return "DEALER_TURN";
        case STATE_SHOWDOWN:    return "SHOWDOWN";
        case STATE_ROUND_END:   return "ROUND_END";
        default:                return "UNKNOWN";
    }
}

void TransitionState(GameContext_t* game, GameState_t new_state) {
    if (!game) {
        d_LogError("TransitionState: NULL game pointer");
        return;
    }

    // Warn on suspicious no-op transitions
    if (game->current_state == new_state) {
        d_LogWarningF("TransitionState: No-op transition %s -> %s",
                      StateToString(game->current_state),
                      StateToString(new_state));
    }

    d_LogInfoF("State transition: %s -> %s (timer was %.2f)",
               StateToString(game->current_state),
               StateToString(new_state),
               game->state_timer);

    game->previous_state = game->current_state;
    game->current_state = new_state;
    game->state_timer = 0.0f;

    // State entry actions
    switch (new_state) {
        case STATE_BETTING:
            // Reset player states
            for (size_t i = 0; i < game->active_players->count; i++) {
                int* id = (int*)d_IndexDataFromArray(game->active_players, i);
                Player_t* player = GetPlayerByID(*id);
                if (player && !player->is_dealer) {
                    player->state = PLAYER_STATE_BETTING;
                }
            }
            break;

        case STATE_DEALING:
            DealInitialHands(game);
            break;

        case STATE_PLAYER_TURN:
            game->current_player_index = 0;
            break;

        case STATE_DEALER_TURN:
            d_LogInfo("STATE_DEALER_TURN entry: About to clear dealer state");

            // Validate tables before clearing (SUSSY check)
            if (!game->dealer_phase) {
                d_LogError("STATE_DEALER_TURN entry: SUSSY - dealer_phase table is NULL!");
            }
            if (!game->bool_flags) {
                d_LogError("STATE_DEALER_TURN entry: SUSSY - bool_flags table is NULL!");
            }

            // Clear dealer phase state from previous round
            ClearDealerPhase(game);
            d_LogDebug("STATE_DEALER_TURN entry: Cleared dealer phase");

            ClearBoolFlag(game, "dealer_action_hit");
            d_LogDebug("STATE_DEALER_TURN entry: Cleared dealer_action_hit");

            // Keep player_stood flag - needed for reveal logic
            d_LogInfo("STATE_DEALER_TURN entry: Done clearing state");
            break;

        case STATE_SHOWDOWN:
            ResolveRound(game);
            break;

        case STATE_ROUND_END:
            // Display results for 3 seconds
            break;

        default:
            break;
    }
}

void UpdateGameLogic(GameContext_t* game, float dt) {
    if (!game) {
        return;
    }

    game->state_timer += dt;

    switch (game->current_state) {
        case STATE_MENU:
            // Handled by sceneMenu.c
            break;

        case STATE_BETTING:
            UpdateBettingState(game);
            break;

        case STATE_DEALING:
            UpdateDealingState(game);
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
            UpdateRoundEndState(game);
            break;
    }
}

// ============================================================================
// STATE UPDATE FUNCTIONS
// ============================================================================

void UpdateBettingState(GameContext_t* game) {
    bool all_bets_placed = true;

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        if (player->current_bet == 0) {
            all_bets_placed = false;

            if (player->is_ai) {
                // AI places random bet
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

void UpdateDealingState(GameContext_t* game) {
    // Wait for animation delay
    if (game->state_timer > 1.0f) {
        TransitionState(game, STATE_PLAYER_TURN);
    }
}

void UpdatePlayerTurnState(GameContext_t* game, float dt) {
    (void)dt;

    if (!game) {
        d_LogError("UpdatePlayerTurnState: NULL game!");
        return;
    }

    if (game->current_player_index >= (int)game->active_players->count) {
        d_LogInfoF("UpdatePlayerTurnState: All players done (index=%d >= count=%zu)",
                   game->current_player_index, game->active_players->count);
        TransitionState(game, STATE_DEALER_TURN);
        return;
    }

    int* current_id = (int*)d_IndexDataFromArray(game->active_players,
                                                   game->current_player_index);
    if (!current_id) {
        d_LogWarningF("UpdatePlayerTurnState: NULL player ID at index %d", game->current_player_index);
        game->current_player_index++;
        return;
    }

    Player_t* player = GetPlayerByID(*current_id);
    if (!player) {
        d_LogErrorF("UpdatePlayerTurnState: Player ID %d not found in g_players table", *current_id);
        game->current_player_index++;
        return;
    }

    // Skip dealer
    if (player->is_dealer) {
        game->current_player_index++;
        if (game->current_player_index >= (int)game->active_players->count) {
            TransitionState(game, STATE_DEALER_TURN);
        }
        return;
    }

    // Check if player is done with their turn
    if (player->state == PLAYER_STATE_BUST ||
        player->state == PLAYER_STATE_STAND ||
        player->state == PLAYER_STATE_BLACKJACK) {

        d_LogInfoF("Player %d is done (state=%d, blackjack=%d, bust=%d, stand=%d)",
                   *current_id,
                   player->state,
                   player->state == PLAYER_STATE_BLACKJACK,
                   player->state == PLAYER_STATE_BUST,
                   player->state == PLAYER_STATE_STAND);

        // Track if player stood (for dealer reveal logic)
        if (player->state == PLAYER_STATE_STAND) {
            if (!GetBoolFlag(game, "player_stood", false)) {
                SetBoolFlag(game, "player_stood", true);
                d_LogInfo("Set player_stood flag for dealer reveal logic");
            }
        }

        // Wait for delay before advancing
        if (!GetBoolFlag(game, "waiting_for_dealer", false)) {
            // Just finished action, start wait timer
            SetBoolFlag(game, "waiting_for_dealer", true);
            game->state_timer = 0.0f;
            d_LogInfoF("Player action complete - waiting %.2fs before next turn", PLAYER_ACTION_DELAY);
        } else if (game->state_timer >= PLAYER_ACTION_DELAY) {
            // Wait complete, advance to next player or dealer
            d_LogInfoF("Wait complete (%.2fs) - advancing to next player", game->state_timer);
            game->current_player_index++;
            ClearBoolFlag(game, "waiting_for_dealer");

            if (game->current_player_index >= (int)game->active_players->count) {
                TransitionState(game, STATE_DEALER_TURN);
            }
        }
        return;
    }

    // AI decision (human waits for UI input)
    if (player->is_ai && game->state_timer > 1.0f) {
        const Card_t* dealer_upcard = GetDealerUpcard(game);
        if (dealer_upcard) {
            PlayerAction_t action = GetAIAction(player, dealer_upcard);
            ExecutePlayerAction(game, player, action);
        }
    }
}

void UpdateDealerTurnState(GameContext_t* game, float dt) {
    (void)dt;

    if (!game) {
        d_LogError("UpdateDealerTurnState: NULL game!");
        return;
    }

    Player_t* dealer = GetPlayerByID(0);
    if (!dealer) {
        d_LogError("UpdateDealerTurnState: Dealer not found");
        TransitionState(game, STATE_SHOWDOWN);
        return;
    }

    // Get current phase (default to CHECK_REVEAL)
    DealerPhase_t phase = GetDealerPhase(game);

    const char* phase_names[] = {"CHECK_REVEAL", "DECIDE", "ACTION", "WAIT"};
    d_LogInfoF("UpdateDealerTurnState: phase=%s, dealer_total=%d, timer=%.2f",
               phase_names[phase],
               dealer->hand.total_value,
               game->state_timer);

    switch (phase) {
        case DEALER_PHASE_CHECK_REVEAL:
            // Check if all players busted
            {
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

                // Check if player stood (triggers immediate reveal)
                if (GetBoolFlag(game, "player_stood", false)) {
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
                        SetDealerPhase(game, DEALER_PHASE_DECIDE);
                        game->state_timer = 0.0f;
                    }
                } else {
                    // Player didn't stand (hit/busted), card stays hidden for now
                    // Skip straight to decide
                    SetDealerPhase(game, DEALER_PHASE_DECIDE);
                    game->state_timer = 0.0f;
                }
            }
            break;

        case DEALER_PHASE_DECIDE:
            // Dealer decides based on their total (knows hidden card value)
            if (dealer->hand.total_value < 17) {
                d_LogInfoF("Dealer has %d - will HIT", dealer->hand.total_value);
                SetDealerPhase(game, DEALER_PHASE_ACTION);
                SetBoolFlag(game, "dealer_action_hit", true);
            } else {
                d_LogInfoF("Dealer has %d - will STAND", dealer->hand.total_value);
                SetDealerPhase(game, DEALER_PHASE_ACTION);
                SetBoolFlag(game, "dealer_action_hit", false);
            }
            game->state_timer = 0.0f;
            break;

        case DEALER_PHASE_ACTION:
            {
                if (GetBoolFlag(game, "dealer_action_hit", false)) {
                    // HIT: Deal a card
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

                // Move to wait
                SetDealerPhase(game, DEALER_PHASE_WAIT);
                game->state_timer = 0.0f;
            }
            break;

        case DEALER_PHASE_WAIT:
            if (game->state_timer >= DEALER_ACTION_DELAY) {
                if (!GetBoolFlag(game, "dealer_action_hit", false)) {
                    // Dealer stood - end turn
                    d_LogInfo("Dealer finished turn (stood)");
                    TransitionState(game, STATE_SHOWDOWN);
                } else if (dealer->hand.is_bust) {
                    // Dealer busted - reveal hole card to show bust score
                    if (dealer->hand.cards->count > 0) {
                        Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
                        if (hidden && !hidden->face_up) {
                            hidden->face_up = true;
                            d_LogInfo("Dealer reveals hidden card on BUST");
                        }
                    }
                    d_LogInfoF("Dealer busts with %d", dealer->hand.total_value);
                    TransitionState(game, STATE_SHOWDOWN);
                } else {
                    // Continue playing
                    SetDealerPhase(game, DEALER_PHASE_DECIDE);
                    game->state_timer = 0.0f;
                }
            }
            break;
    }
}

void UpdateShowdownState(GameContext_t* game) {
    // Showdown executed in TransitionState entry action
    // Immediately move to round end
    TransitionState(game, STATE_ROUND_END);
}

void UpdateRoundEndState(GameContext_t* game) {
    // Display results for 3 seconds
    if (game->state_timer > 3.0f) {
        StartNewRound(game);
        TransitionState(game, STATE_BETTING);
    }
}

// ============================================================================
// PLAYER ACTIONS
// ============================================================================

void ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action) {
    if (!game || !player) {
        d_LogError("ExecutePlayerAction: NULL pointer");
        return;
    }

    switch (action) {
        case ACTION_HIT: {
            Card_t card = DealCard(game->deck);
            if (card.card_id != -1) {
                card.face_up = true;
                LoadCardTexture(&card);
                AddCardToHand(&player->hand, card);

                d_LogInfoF("%s hits (hand value: %d)",
                          GetPlayerName(player), player->hand.total_value);

                if (player->hand.is_bust) {
                    player->state = PLAYER_STATE_BUST;
                    game->current_player_index++;
                    d_LogInfoF("%s busts!", GetPlayerName(player));
                } else if (player->hand.total_value == 21) {
                    player->state = PLAYER_STATE_STAND;
                    game->current_player_index++;
                    d_LogInfoF("%s reaches 21 (auto-stand)", GetPlayerName(player));
                }
            }
            break;
        }

        case ACTION_STAND:
            player->state = PLAYER_STATE_STAND;
            game->current_player_index++;
            d_LogInfoF("%s stands (hand value: %d)",
                      GetPlayerName(player), player->hand.total_value);
            break;

        case ACTION_DOUBLE:
            if (CanAffordBet(player, player->current_bet)) {
                PlaceBet(player, player->current_bet);  // Double the bet
                Card_t card = DealCard(game->deck);
                if (card.card_id != -1) {
                    card.face_up = true;
                    LoadCardTexture(&card);
                    AddCardToHand(&player->hand, card);
                }
                player->state = player->hand.is_bust ? PLAYER_STATE_BUST : PLAYER_STATE_STAND;
                game->current_player_index++;
                d_LogInfoF("%s doubles down", GetPlayerName(player));
            }
            break;

        case ACTION_SPLIT:
            // Future implementation
            d_LogInfo("Split not yet implemented");
            break;
    }
}

PlayerAction_t GetAIAction(const Player_t* player, const Card_t* dealer_upcard) {
    if (!player || !dealer_upcard) {
        return ACTION_STAND;
    }

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

// ============================================================================
// INPUT PROCESSING (UI → Game Logic Bridge)
// ============================================================================

bool ProcessBettingInput(GameContext_t* game, Player_t* player, int bet_amount) {
    // Constitutional pattern: NULL checks
    if (!game || !player) {
        d_LogError("ProcessBettingInput: NULL pointer");
        return false;
    }

    // Validate game state
    if (game->current_state != STATE_BETTING) {
        d_LogWarning("ProcessBettingInput: Not in BETTING state");
        return false;
    }

    // Validate bet amount
    if (!CanAffordBet(player, bet_amount)) {
        d_LogWarningF("ProcessBettingInput: Player cannot afford bet %d (has %d chips)",
                     bet_amount, GetPlayerChips(player));
        return false;
    }

    // Execute bet
    if (PlaceBet(player, bet_amount)) {
        d_LogInfoF("Player bet %d chips", bet_amount);
        // State machine will handle transition to DEALING
        return true;
    }

    return false;
}

bool ProcessPlayerTurnInput(GameContext_t* game, Player_t* player, PlayerAction_t action) {
    // Constitutional pattern: NULL checks
    if (!game || !player) {
        d_LogError("ProcessPlayerTurnInput: NULL pointer");
        return false;
    }

    // Validate game state
    if (game->current_state != STATE_PLAYER_TURN) {
        d_LogWarning("ProcessPlayerTurnInput: Not in PLAYER_TURN state");
        return false;
    }

    // Validate it's this player's turn
    Player_t* current = GetCurrentPlayer(game);
    if (current != player) {
        d_LogWarning("ProcessPlayerTurnInput: Not current player's turn");
        return false;
    }

    // Validate action-specific rules
    if (action == ACTION_DOUBLE) {
        // Can only double on first decision (2 cards)
        if (player->hand.cards->count != 2) {
            d_LogWarning("ProcessPlayerTurnInput: Cannot double after hitting");
            return false;
        }
        // Must have chips to double bet
        if (!CanAffordBet(player, player->current_bet)) {
            d_LogWarningF("ProcessPlayerTurnInput: Cannot afford to double (bet: %d, chips: %d)",
                         player->current_bet, GetPlayerChips(player));
            return false;
        }
    }

    // Execute action
    ExecutePlayerAction(game, player, action);
    return true;
}

// ============================================================================
// DEALER LOGIC
// ============================================================================

void DealerTurn(GameContext_t* game) {
    Player_t* dealer = GetPlayerByID(0);
    if (!dealer) {
        d_LogError("DealerTurn: Dealer not found");
        return;
    }

    // Reveal hidden card
    if (dealer->hand.cards->count > 0) {
        Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
        if (hidden) {
            hidden->face_up = true;
        }
    }

    // Dealer hits on 16 or less
    while (dealer->hand.total_value < 17) {
        Card_t card = DealCard(game->deck);
        if (card.card_id != -1) {
            card.face_up = true;
            LoadCardTexture(&card);
            AddCardToHand(&dealer->hand, card);
        }
    }

    d_LogInfoF("Dealer finishes with %d", dealer->hand.total_value);
}

const Card_t* GetDealerUpcard(const GameContext_t* game) {
    if (!game) return NULL;

    Player_t* dealer = GetPlayerByID(0);
    if (!dealer || dealer->hand.cards->count < 2) {
        return NULL;
    }

    // Second card is the upcard (first is hidden)
    return (const Card_t*)d_IndexDataFromArray(dealer->hand.cards, 1);
}

// ============================================================================
// ROUND MANAGEMENT
// ============================================================================

void DealInitialHands(GameContext_t* game) {
    if (!game) return;

    d_LogInfo("Dealing initial hands...");

    // Deal 2 cards to each player
    for (int round = 0; round < 2; round++) {
        for (size_t i = 0; i < game->active_players->count; i++) {
            int* id = (int*)d_IndexDataFromArray(game->active_players, i);
            Player_t* player = GetPlayerByID(*id);

            if (!player) continue;

            Card_t card = DealCard(game->deck);
            if (card.card_id == -1) {
                d_LogError("Deck empty during dealing!");
                return;
            }

            // Dealer's first card is face down
            card.face_up = !(player->is_dealer && round == 0);
            LoadCardTexture(&card);
            AddCardToHand(&player->hand, card);
        }
    }

    // Check for blackjacks
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player && player->hand.is_blackjack) {
            player->state = PLAYER_STATE_BLACKJACK;
            d_LogInfoF("%s has BLACKJACK!", GetPlayerName(player));
        }
    }
}

void ResolveRound(GameContext_t* game) {
    if (!game) return;

    Player_t* dealer = GetPlayerByID(0);
    if (!dealer) {
        d_LogError("ResolveRound: Dealer not found");
        return;
    }

    int dealer_value = dealer->hand.total_value;
    bool dealer_bust = dealer->hand.is_bust;

    d_LogInfoF("Resolving round - Dealer: %d%s",
               dealer_value, dealer_bust ? " (BUST)" : "");

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        const char* outcome = "";

        if (player->hand.is_bust) {
            LoseBet(player);
            player->state = PLAYER_STATE_LOST;
            outcome = "BUST - LOSE";
        } else if (player->hand.is_blackjack) {
            if (dealer->hand.is_blackjack) {
                ReturnBet(player);
                player->state = PLAYER_STATE_PUSH;
                outcome = "PUSH";
            } else {
                WinBet(player, 1.5f);  // 3:2 payout
                player->state = PLAYER_STATE_WON;
                outcome = "BLACKJACK - WIN 3:2";
            }
        } else if (dealer_bust) {
            WinBet(player, 1.0f);  // 1:1 payout
            player->state = PLAYER_STATE_WON;
            outcome = "DEALER BUST - WIN";
        } else if (player->hand.total_value > dealer_value) {
            WinBet(player, 1.0f);
            player->state = PLAYER_STATE_WON;
            outcome = "WIN";
        } else if (player->hand.total_value < dealer_value) {
            LoseBet(player);
            player->state = PLAYER_STATE_LOST;
            outcome = "LOSE";
        } else {
            ReturnBet(player);
            player->state = PLAYER_STATE_PUSH;
            outcome = "PUSH";
        }

        d_LogInfoF("%s: %s", GetPlayerName(player), outcome);
    }
}

void StartNewRound(GameContext_t* game) {
    if (!game) return;

    d_LogInfo("Starting new round...");

    // Clear all hands
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        if (player) {
            ClearHand(&player->hand, game->deck);
            player->current_bet = 0;
            player->state = PLAYER_STATE_WAITING;
        }
    }

    // Check if deck needs reshuffling
    if (GetDeckSize(game->deck) < 20) {
        d_LogInfo("Reshuffling deck...");
        ResetDeck(game->deck);
    }

    game->round_number++;
    game->current_player_index = 0;
}

// ============================================================================
// PLAYER MANAGEMENT
// ============================================================================

void AddPlayerToGame(GameContext_t* game, int player_id) {
    if (!game) {
        d_LogError("AddPlayerToGame: NULL game pointer");
        return;
    }

    d_AppendDataToArray(game->active_players, &player_id);
    d_LogInfoF("Added player %d to game", player_id);
}

Player_t* GetCurrentPlayer(GameContext_t* game) {
    if (!game || game->current_player_index >= (int)game->active_players->count) {
        return NULL;
    }

    int* id = (int*)d_IndexDataFromArray(game->active_players,
                                          game->current_player_index);
    if (!id) return NULL;

    return GetPlayerByID(*id);
}

Player_t* GetPlayerByID(int player_id) {
    if (!g_players) return NULL;

    // Constitutional pattern: Table stores Player_t by value, return direct pointer
    return (Player_t*)d_GetDataFromTable(g_players, &player_id);
}

// ============================================================================
// STATE VARIABLE HELPERS (Constitutional: Typed tables, no void*)
// ============================================================================

// Static storage for bool values (only 2 possible values, persistent memory)
static const bool BOOL_TRUE = true;
static const bool BOOL_FALSE = false;

void SetBoolFlag(GameContext_t* game, const char* key, bool value) {
    if (!game) {
        d_LogErrorF("SetBoolFlag: NULL game pointer (key='%s')", key ? key : "NULL");
        return;
    }
    if (!key) {
        d_LogError("SetBoolFlag: NULL key");
        return;
    }
    if (!game->bool_flags) {
        d_LogFatalF("SetBoolFlag: game->bool_flags is NULL! Memory corruption? (key='%s')", key);
        return;
    }

    // Store pointer to static bool (persistent memory, not stack!)
    d_SetDataInTable(game->bool_flags, &key, value ? &BOOL_TRUE : &BOOL_FALSE);

    // Verify storage (SUSSY checks)
    bool* verify = (bool*)d_GetDataFromTable(game->bool_flags, &key);
    if (!verify) {
        d_LogWarningF("SetBoolFlag: SUSSY - Failed to store key='%s', value=%d", key, value);
    } else if (*verify != value) {
        d_LogErrorF("SetBoolFlag: SUSSY - Stored value mismatch! Expected %d, got %d (key='%s')",
                    value, *verify, key);
    }
}

bool GetBoolFlag(const GameContext_t* game, const char* key, bool default_value) {
    if (!game || !key) {
        return default_value;
    }

    bool* value_ptr = (bool*)d_GetDataFromTable(game->bool_flags, &key);
    return value_ptr ? *value_ptr : default_value;
}

void ClearBoolFlag(GameContext_t* game, const char* key) {
    if (!game || !key) {
        d_LogWarningF("ClearBoolFlag: NULL pointer (game=%p, key=%p)",
                      (void*)game, (void*)key);
        return;
    }

    if (!game->bool_flags) {
        d_LogErrorF("ClearBoolFlag: game->bool_flags is NULL! (key='%s')", key);
        return;
    }

    // Check if key exists before removing (not an error if it doesn't)
    bool* exists = (bool*)d_GetDataFromTable(game->bool_flags, &key);
    if (exists) {
        d_RemoveDataFromTable(game->bool_flags, &key);
        d_LogDebugF("ClearBoolFlag: Removed key='%s' (was %d)", key, *exists);
    } else {
        d_LogDebugF("ClearBoolFlag: Key='%s' not found (already clear)", key);
    }
}

// Static storage for dealer phases (4 possible values, persistent memory)
static const DealerPhase_t PHASE_CHECK_REVEAL = DEALER_PHASE_CHECK_REVEAL;
static const DealerPhase_t PHASE_DECIDE = DEALER_PHASE_DECIDE;
static const DealerPhase_t PHASE_ACTION = DEALER_PHASE_ACTION;
static const DealerPhase_t PHASE_WAIT = DEALER_PHASE_WAIT;

void SetDealerPhase(GameContext_t* game, DealerPhase_t phase) {
    if (!game) {
        d_LogError("SetDealerPhase: NULL game pointer");
        return;
    }
    if (!game->dealer_phase) {
        d_LogFatal("SetDealerPhase: game->dealer_phase is NULL! Memory corruption?");
        return;
    }

    // Store pointer to static phase value (persistent memory)
    const DealerPhase_t* phase_ptr = NULL;
    switch (phase) {
        case DEALER_PHASE_CHECK_REVEAL: phase_ptr = &PHASE_CHECK_REVEAL; break;
        case DEALER_PHASE_DECIDE:       phase_ptr = &PHASE_DECIDE; break;
        case DEALER_PHASE_ACTION:       phase_ptr = &PHASE_ACTION; break;
        case DEALER_PHASE_WAIT:         phase_ptr = &PHASE_WAIT; break;
    }

    if (phase_ptr) {
        const char* key = "phase";
        d_SetDataInTable(game->dealer_phase, &key, phase_ptr);

        // Verify storage (SUSSY checks)
        DealerPhase_t* verify = (DealerPhase_t*)d_GetDataFromTable(game->dealer_phase, &key);
        if (!verify) {
            d_LogWarningF("SetDealerPhase: SUSSY - Failed to store phase=%d", phase);
        } else if (*verify != phase) {
            d_LogErrorF("SetDealerPhase: SUSSY - Stored phase mismatch! Expected %d, got %d",
                        phase, *verify);
        }
    } else {
        d_LogErrorF("SetDealerPhase: Invalid phase value %d", phase);
    }
}

DealerPhase_t GetDealerPhase(const GameContext_t* game) {
    if (!game) {
        return DEALER_PHASE_CHECK_REVEAL;
    }

    const char* key = "phase";
    DealerPhase_t* phase_ptr = (DealerPhase_t*)d_GetDataFromTable(game->dealer_phase, &key);
    return phase_ptr ? *phase_ptr : DEALER_PHASE_CHECK_REVEAL;
}

void ClearDealerPhase(GameContext_t* game) {
    if (!game) {
        d_LogWarning("ClearDealerPhase: NULL game pointer!");
        return;
    }

    if (!game->dealer_phase) {
        d_LogError("ClearDealerPhase: game->dealer_phase is NULL!");
        return;
    }

    // SUSSY: Log table state before accessing
    d_LogInfoF("ClearDealerPhase: game=%p, dealer_phase table=%p",
               (void*)game, (void*)game->dealer_phase);

    // Check if phase exists before removing (not an error if it doesn't)
    const char* key_str = "phase";
    d_LogInfoF("ClearDealerPhase: About to call d_GetDataFromTable with key='%s' at %p",
               key_str, (void*)key_str);

    DealerPhase_t* exists = (DealerPhase_t*)d_GetDataFromTable(game->dealer_phase, &key_str);

    d_LogInfo("ClearDealerPhase: d_GetDataFromTable returned");

    if (exists) {
        d_RemoveDataFromTable(game->dealer_phase, &key_str);
        d_LogDebugF("ClearDealerPhase: Removed phase (was %d)", *exists);
    } else {
        d_LogDebug("ClearDealerPhase: Phase not found (already clear)");
    }
}
