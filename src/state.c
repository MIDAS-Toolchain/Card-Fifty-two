/*
 * State Machine Module - Implementation
 * Extracted from game.c god object for better architecture
 */

#include "state.h"
#include "stateStorage.h"
#include "game.h"
#include "player.h"
#include "enemy.h"
#include "random.h"
#include "trinket.h"
#include "cardTags.h"
#include "hand.h"
#include "stats.h"
#include <Daedalus.h>

// ============================================================================
// INITIALIZATION AND CLEANUP
// ============================================================================

void State_InitContext(GameContext_t* game, Deck_t* deck) {
    if (!game) {
        d_LogFatal("State_InitContext: NULL game pointer");
        return;
    }

    if (!deck) {
        d_LogFatal("State_InitContext: NULL deck pointer");
        return;
    }

    // Initialize state machine
    game->current_state = STATE_BETTING;
    game->previous_state = STATE_BETTING;
    game->state_timer = 0.0f;
    game->round_number = 1;
    game->current_player_index = 0;
    game->deck = deck;

    // Initialize state storage (Constitutional: typed tables via stateStorage module)
    if (!StateData_Init(&game->state_data)) {
        d_LogFatal("State_InitContext: Failed to initialize state storage");
        return;
    }

    // Initialize active players array (holds int player_ids)
    game->active_players = d_ArrayInit(8, sizeof(int));
    if (!game->active_players) {
        d_LogFatal("State_InitContext: Failed to initialize active_players array");
        StateData_Cleanup(&game->state_data);
        return;
    }

    // Initialize combat system
    game->current_enemy = NULL;
    game->is_combat_mode = false;
    game->next_enemy_hp_multiplier = 1.0f;  // Normal HP (no modification initially)

    // Initialize event preview system
    game->event_reroll_base_cost = 50;  // Base cost (configurable)
    game->event_reroll_cost = 50;       // Current cost (reset each preview)
    game->event_rerolls_used = 0;       // Reroll counter
    game->event_preview_timer = 0.0f;   // Timer (set to 3.0 when entering STATE_EVENT_PREVIEW)

    d_LogInfoF("GameContext initialized - event_reroll_base_cost=%d", game->event_reroll_base_cost);
}

void State_CleanupContext(GameContext_t* game) {
    if (!game) {
        d_LogError("State_CleanupContext: NULL game pointer");
        return;
    }

    // Destroy state storage
    StateData_Cleanup(&game->state_data);

    // Destroy active players array
    if (game->active_players) {
        d_ArrayDestroy(game->active_players);
        game->active_players = NULL;
    }

    d_LogInfo("GameContext cleaned up");
}

// ============================================================================
// STATE MACHINE
// ============================================================================

const char* State_ToString(GameState_t state) {
    switch (state) {
        case STATE_MENU: return "MENU";
        case STATE_INTRO_NARRATIVE: return "INTRO_NARRATIVE";
        case STATE_BETTING: return "BETTING";
        case STATE_DEALING: return "DEALING";
        case STATE_PLAYER_TURN: return "PLAYER_TURN";
        case STATE_DEALER_TURN: return "DEALER_TURN";
        case STATE_SHOWDOWN: return "SHOWDOWN";
        case STATE_ROUND_END: return "ROUND_END";
        case STATE_COMBAT_VICTORY: return "COMBAT_VICTORY";
        case STATE_REWARD_SCREEN: return "REWARD_SCREEN";
        case STATE_COMBAT_PREVIEW: return "COMBAT_PREVIEW";
        case STATE_EVENT_PREVIEW: return "EVENT_PREVIEW";
        case STATE_EVENT: return "EVENT";
        case STATE_TARGETING: return "TARGETING";
        default: return "UNKNOWN";
    }
}

void State_Transition(GameContext_t* game, GameState_t new_state) {
    if (!game) {
        d_LogError("State_Transition: NULL game pointer");
        return;
    }

    // Warn on suspicious no-op transitions
    if (game->current_state == new_state) {
        d_LogWarningF("State_Transition: No-op transition %s -> %s",
                      State_ToString(game->current_state),
                      State_ToString(new_state));
    }

    d_LogInfoF("State transition: %s -> %s (timer was %.2f)",
               State_ToString(game->current_state),
               State_ToString(new_state),
               game->state_timer);

    game->previous_state = game->current_state;
    game->current_state = new_state;
    game->state_timer = 0.0f;

    // State entry actions
    switch (new_state) {
        case STATE_BETTING:
            // Reset player states
            for (size_t i = 0; i < game->active_players->count; i++) {
                int* id = (int*)d_ArrayGet(game->active_players, i);
                Player_t* player = Game_GetPlayerByID(*id);
                if (player && !player->is_dealer) {
                    player->state = PLAYER_STATE_BETTING;
                }
            }
            break;

        case STATE_DEALING:
            Game_DealInitialHands(game);
            break;

        case STATE_PLAYER_TURN:
            game->current_player_index = 0;
            break;

        case STATE_DEALER_TURN:
            d_LogInfo("STATE_DEALER_TURN entry: About to clear dealer state");

            // Validate tables before clearing (SUSSY check)
            if (!game->state_data.dealer_phase) {
                d_LogError("STATE_DEALER_TURN entry: SUSSY - dealer_phase table is NULL!");
            }
            if (!game->state_data.bool_flags) {
                d_LogError("STATE_DEALER_TURN entry: SUSSY - bool_flags table is NULL!");
            }

            // Clear dealer phase state from previous round
            StateData_ClearPhase(&game->state_data);
            d_LogDebug("STATE_DEALER_TURN entry: Cleared dealer phase");

            StateData_ClearBool(&game->state_data, "dealer_action_hit");
            d_LogDebug("STATE_DEALER_TURN entry: Cleared dealer_action_hit");

            // Keep player_stood flag - needed for reveal logic
            d_LogInfo("STATE_DEALER_TURN entry: Done clearing state");
            break;

        case STATE_SHOWDOWN:
            Game_ResolveRound(game);
            break;

        case STATE_ROUND_END:
            // Display results for 2 seconds
            break;

        case STATE_COMBAT_VICTORY:
            // Play victory sound
            a_AudioPlayEffect(&g_victory_sound);
            d_LogInfo("Victory sound played!");
            break;

        case STATE_REWARD_SCREEN:
            // Reward modal handled in sceneBlackjack.c
            break;

        default:
            break;
    }
}

void State_UpdateLogic(GameContext_t* game, float dt) {
    if (!game) {
        return;
    }

    game->state_timer += dt;

    switch (game->current_state) {
        case STATE_MENU:
            // Handled by sceneMenu.c
            break;

        case STATE_BETTING:
            State_UpdateBetting(game);
            break;

        case STATE_DEALING:
            State_UpdateDealing(game);
            break;

        case STATE_PLAYER_TURN:
            State_UpdatePlayerTurn(game, dt);
            break;

        case STATE_DEALER_TURN:
            State_UpdateDealerTurn(game, dt);
            break;

        case STATE_SHOWDOWN:
            State_UpdateShowdown(game);
            break;

        case STATE_ROUND_END:
            State_UpdateRoundEnd(game);
            break;

        case STATE_COMBAT_VICTORY:
            State_UpdateCombatVictory(game);
            break;

        case STATE_REWARD_SCREEN:
            State_UpdateRewardScreen(game);
            break;

        case STATE_EVENT_PREVIEW:
            State_UpdateEventPreview(game, dt);
            break;

        case STATE_EVENT:
            State_UpdateEvent(game);
            break;

        case STATE_TARGETING:
            // Handled entirely by sceneBlackjack.c (HandleTargetingInput)
            // No state machine updates needed - just wait for user to select target or cancel
            break;

        case STATE_INTRO_NARRATIVE:
            // Handled by introNarrativeModal.c
            break;

        case STATE_COMBAT_PREVIEW:
            // Handled by combatPreviewModal.c
            break;
    }
}

// ============================================================================
// STATE UPDATE FUNCTIONS
// ============================================================================

void State_UpdateBetting(GameContext_t* game) {
    bool all_bets_placed = true;

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_ArrayGet(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        if (player->current_bet == 0) {
            all_bets_placed = false;

            if (player->is_ai) {
                // AI places random bet (high-quality RNG)
                int bet = GetRandomInt(10, 100);
                PlaceBet(player, bet);
            }
            // Human player waits for UI input
        }
    }

    if (all_bets_placed) {
        State_Transition(game, STATE_DEALING);
    }
}

void State_UpdateDealing(GameContext_t* game) {
    // Wait for animation delay
    if (game->state_timer > 1.0f) {
        State_Transition(game, STATE_PLAYER_TURN);
    }
}

void State_UpdatePlayerTurn(GameContext_t* game, float dt) {
    (void)dt;

    if (!game) {
        d_LogError("State_UpdatePlayerTurn: NULL game!");
        return;
    }

    if (game->current_player_index >= (int)game->active_players->count) {
        d_LogInfoF("State_UpdatePlayerTurn: All players done (index=%d >= count=%zu)",
                   game->current_player_index, game->active_players->count);
        State_Transition(game, STATE_DEALER_TURN);
        return;
    }

    int* current_id = (int*)d_ArrayGet(game->active_players,
                                                   game->current_player_index);
    if (!current_id) {
        d_LogWarningF("State_UpdatePlayerTurn: NULL player ID at index %d", game->current_player_index);
        game->current_player_index++;
        return;
    }

    Player_t* player = Game_GetPlayerByID(*current_id);
    if (!player) {
        d_LogErrorF("State_UpdatePlayerTurn: Player ID %d not found in g_players table", *current_id);
        game->current_player_index++;
        return;
    }

    // Skip dealer
    if (player->is_dealer) {
        game->current_player_index++;
        if (game->current_player_index >= (int)game->active_players->count) {
            State_Transition(game, STATE_DEALER_TURN);
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
            if (!StateData_GetBool(&game->state_data, "player_stood", false)) {
                StateData_SetBool(&game->state_data, "player_stood", true);
                d_LogInfo("Set player_stood flag for dealer reveal logic");
            }
        }

        // Wait for delay before advancing
        if (!StateData_GetBool(&game->state_data, "waiting_for_dealer", false)) {
            // Just finished action, start wait timer
            StateData_SetBool(&game->state_data, "waiting_for_dealer", true);
            game->state_timer = 0.0f;
            d_LogInfoF("Player action complete - waiting %.2fs before next turn", PLAYER_ACTION_DELAY);
        } else if (game->state_timer >= PLAYER_ACTION_DELAY) {
            // Wait complete, advance to next player or dealer
            d_LogInfoF("Wait complete (%.2fs) - advancing to next player", game->state_timer);
            game->current_player_index++;
            StateData_ClearBool(&game->state_data, "waiting_for_dealer");

            if (game->current_player_index >= (int)game->active_players->count) {
                State_Transition(game, STATE_DEALER_TURN);
            }
        }
        return;
    }

    // AI decision (human waits for UI input)
    if (player->is_ai && game->state_timer > 1.0f) {
        const Card_t* dealer_upcard = Game_GetDealerUpcard(game);
        if (dealer_upcard) {
            PlayerAction_t action = Game_GetAIAction(player, dealer_upcard);
            Game_ExecutePlayerAction(game, player, action);
        }
    }
}

void State_UpdateDealerTurn(GameContext_t* game, float dt) {
    (void)dt;

    if (!game) {
        d_LogError("State_UpdateDealerTurn: NULL game!");
        return;
    }

    Player_t* dealer = Game_GetPlayerByID(0);
    if (!dealer) {
        d_LogError("State_UpdateDealerTurn: Dealer not found");
        State_Transition(game, STATE_SHOWDOWN);
        return;
    }

    // Get current phase (default to CHECK_REVEAL)
    DealerPhase_t phase = StateData_GetPhase(&game->state_data);

    const char* phase_names[] = {"CHECK_REVEAL", "DECIDE", "ACTION", "WAIT"};
    d_LogRateLimitedF(D_LOG_RATE_LIMIT_FLAG_HASH_FORMAT_STRING, D_LOG_LEVEL_INFO,
                      1, 1.0,  // 1 log per 1 second
                      "State_UpdateDealerTurn: phase=%s, dealer_total=%d, timer=%.2f",
                      phase_names[phase],
                      dealer->hand.total_value,
                      game->state_timer);

    switch (phase) {
        case DEALER_PHASE_CHECK_REVEAL:
            // Check if all players busted
            {
                bool all_busted = true;
                for (size_t i = 0; i < game->active_players->count; i++) {
                    int* id = (int*)d_ArrayGet(game->active_players, i);
                    Player_t* p = Game_GetPlayerByID(*id);
                    if (p && !p->is_dealer && p->state != PLAYER_STATE_BUST) {
                        all_busted = false;
                        break;
                    }
                }

                if (all_busted) {
                    // Everyone busted - reveal card for counting, then end
                    if (dealer->hand.cards->count > 0) {
                        Card_t* hidden = (Card_t*)d_ArrayGet(dealer->hand.cards, 0);
                        if (hidden && !hidden->face_up) {
                            hidden->face_up = true;
                            d_LogInfo("Dealer reveals hidden card (all players busted - card counting)");
                        }
                    }
                    State_Transition(game, STATE_SHOWDOWN);
                    return;
                }

                // Check if player stood (triggers immediate reveal)
                if (StateData_GetBool(&game->state_data, "player_stood", false)) {
                    // Wait for reveal animation time
                    if (game->state_timer >= CARD_REVEAL_DELAY) {
                        if (dealer->hand.cards->count > 0) {
                            Card_t* hidden = (Card_t*)d_ArrayGet(dealer->hand.cards, 0);
                            if (hidden && !hidden->face_up) {
                                hidden->face_up = true;
                                d_LogInfoF("Dealer reveals hidden card (player stood) - total: %d",
                                          dealer->hand.total_value);

                                // Process card tag effects for revealed card
                                ProcessCardTagEffects(hidden, game, dealer);
                            }
                        }

                        // Move to decide phase
                        StateData_SetPhase(&game->state_data, DEALER_PHASE_DECIDE);
                        game->state_timer = 0.0f;
                    }
                } else {
                    // Player didn't stand (hit/busted), card stays hidden for now
                    // Skip straight to decide
                    StateData_SetPhase(&game->state_data, DEALER_PHASE_DECIDE);
                    game->state_timer = 0.0f;
                }
            }
            break;

        case DEALER_PHASE_DECIDE:
            // Dealer decides based on their total (knows hidden card value)
            if (dealer->hand.total_value < 17) {
                d_LogInfoF("Dealer has %d - will HIT", dealer->hand.total_value);
                StateData_SetPhase(&game->state_data, DEALER_PHASE_ACTION);
                StateData_SetBool(&game->state_data, "dealer_action_hit", true);
            } else {
                d_LogInfoF("Dealer has %d - will STAND", dealer->hand.total_value);
                StateData_SetPhase(&game->state_data, DEALER_PHASE_ACTION);
                StateData_SetBool(&game->state_data, "dealer_action_hit", false);
            }
            game->state_timer = 0.0f;
            break;

        case DEALER_PHASE_ACTION:
            {
                if (StateData_GetBool(&game->state_data, "dealer_action_hit", false)) {
                    // HIT: Deal a card with animation
                    if (Game_DealCardWithAnimation(game->deck, &dealer->hand, dealer, true)) {
                        d_LogInfoF("Dealer hits - new total: %d", dealer->hand.total_value);

                        // Process card tag effects for dealer draws
                        const Card_t* last_card = GetCardFromHand(&dealer->hand, dealer->hand.cards->count - 1);
                        if (last_card && last_card->face_up) {
                            ProcessCardTagEffects(last_card, game, dealer);
                        }
                    }
                } else {
                    // STAND: Reveal hidden card if not already revealed
                    if (dealer->hand.cards->count > 0) {
                        Card_t* hidden = (Card_t*)d_ArrayGet(dealer->hand.cards, 0);
                        if (hidden && !hidden->face_up) {
                            hidden->face_up = true;
                            d_LogInfo("Dealer reveals hidden card on STAND");

                            // Process card tag effects for revealed card
                            ProcessCardTagEffects(hidden, game, dealer);
                        }
                    }
                }

                // Move to wait
                StateData_SetPhase(&game->state_data, DEALER_PHASE_WAIT);
                game->state_timer = 0.0f;
            }
            break;

        case DEALER_PHASE_WAIT:
            if (game->state_timer >= DEALER_ACTION_DELAY) {
                if (!StateData_GetBool(&game->state_data, "dealer_action_hit", false)) {
                    // Dealer stood - end turn
                    d_LogInfo("Dealer finished turn (stood)");
                    State_Transition(game, STATE_SHOWDOWN);
                } else if (dealer->hand.is_bust) {
                    // Dealer busted - reveal hole card to show bust score
                    if (dealer->hand.cards->count > 0) {
                        Card_t* hidden = (Card_t*)d_ArrayGet(dealer->hand.cards, 0);
                        if (hidden && !hidden->face_up) {
                            hidden->face_up = true;
                            d_LogInfo("Dealer reveals hidden card on BUST");
                        }
                    }
                    d_LogInfoF("Dealer busts with %d", dealer->hand.total_value);
                    State_Transition(game, STATE_SHOWDOWN);
                } else {
                    // Continue playing
                    StateData_SetPhase(&game->state_data, DEALER_PHASE_DECIDE);
                    game->state_timer = 0.0f;
                }
            }
            break;
    }
}

void State_UpdateShowdown(GameContext_t* game) {
    // Showdown executed in State_Transition entry action

    // Check if enemy was defeated in combat
    // Also check HP directly as safety (in case is_defeated flag wasn't set)
    if (game->is_combat_mode && game->current_enemy) {
        if (game->current_enemy->is_defeated || game->current_enemy->current_hp <= 0) {
            // Force is_defeated flag if HP is 0 (safety check)
            if (!game->current_enemy->is_defeated && game->current_enemy->current_hp <= 0) {
                d_LogWarning("Enemy at 0 HP but is_defeated not set! Forcing victory.");
                game->current_enemy->is_defeated = true;
                Stats_RecordCombatWon();
            }
            State_Transition(game, STATE_COMBAT_VICTORY);
            return;
        }
    }

    // Normal round end
    State_Transition(game, STATE_ROUND_END);
}

void State_UpdateRoundEnd(GameContext_t* game) {
    // Display results for THIS round (2 seconds)
    if (game->state_timer > 2.0f) {
        if (game->is_combat_mode && game->current_enemy) {
            // Check both is_defeated flag AND HP (safety check)
            if (game->current_enemy->is_defeated || game->current_enemy->current_hp <= 0) {
                // Force is_defeated flag if HP is 0 (safety check)
                if (!game->current_enemy->is_defeated && game->current_enemy->current_hp <= 0) {
                    d_LogWarning("ROUND_END: Enemy at 0 HP but is_defeated not set! Forcing victory.");
                    game->current_enemy->is_defeated = true;
                    Stats_RecordCombatWon();
                }
                // Enemy defeated - transition to victory celebration
                State_Transition(game, STATE_COMBAT_VICTORY);
            } else {
                // Combat continues - next round
                Game_StartNewRound(game);
                State_Transition(game, STATE_BETTING);
            }
        } else {
            // Practice mode - next round
            Game_StartNewRound(game);
            State_Transition(game, STATE_BETTING);
        }
    }
}

void State_UpdateCombatVictory(GameContext_t* game) {
    // Display victory celebration for 2 seconds
    if (game->state_timer > 2.0f) {
        // Transition to reward screen
        State_Transition(game, STATE_REWARD_SCREEN);
    }
}

void State_UpdateRewardScreen(GameContext_t* game) {
    // Reward modal handles its own lifecycle and transitions via sceneBlackjack.c
    // This state just waits for the modal to close and trigger the next state
    (void)game;  // No auto-advance - modal controls transition
}

void State_UpdateEventPreview(GameContext_t* game, float dt) {
    // Event preview timer countdown
    if (game->event_preview_timer > 0.0f) {
        game->event_preview_timer -= dt;

        // Auto-proceed when timer hits 0
        if (game->event_preview_timer <= 0.0f) {
            game->event_preview_timer = 0.0f;
            d_LogInfo("Event preview timer expired - auto-proceeding to event");
            State_Transition(game, STATE_EVENT);
        }
    }

    // Event preview modal handles button clicks (reroll/continue) in sceneBlackjack.c
    // This state just manages the auto-proceed timer
}

void State_UpdateEvent(GameContext_t* game) {
    // Event logic handled by sceneBlackjack.c
    // Events are rendered and input is processed there
    // This state just waits for the event to complete and trigger transition
    (void)game;  // No auto-advance - event scene controls transition
}
