/*
 * Card Fifty-Two - Game State Machine Implementation
 * Constitutional pattern: dTable_t for state data, dArray_t for player list
 */

#include "../include/common.h"
#include "../include/game.h"
#include "../include/player.h"
#include "../include/deck.h"
#include "../include/hand.h"
#include "../include/enemy.h"
#include "../include/stats.h"
#include "../include/scenes/sceneBlackjack.h"
#include "../include/cardAnimation.h"
#include "../include/tween/tween.h"
#include "../include/cardTags.h"
#include "../include/stateStorage.h"
#include "../include/statusEffects.h"
#include "../include/trinket.h"

// External globals
extern dTable_t* g_players;
extern TweenManager_t g_tween_manager;  // From sceneBlackjack.c

// ============================================================================
// GAME EVENTS
// ============================================================================

const char* GameEventToString(GameEvent_t event) {
    switch (event) {
        case GAME_EVENT_COMBAT_START:        return "COMBAT_START";
        case GAME_EVENT_HAND_END:            return "HAND_END";
        case GAME_EVENT_PLAYER_WIN:          return "PLAYER_WIN";
        case GAME_EVENT_PLAYER_LOSS:         return "PLAYER_LOSS";
        case GAME_EVENT_PLAYER_PUSH:         return "PLAYER_PUSH";
        case GAME_EVENT_PLAYER_BUST:         return "PLAYER_BUST";
        case GAME_EVENT_PLAYER_BLACKJACK:    return "PLAYER_BLACKJACK";
        case GAME_EVENT_DEALER_BUST:         return "DEALER_BUST";
        case GAME_EVENT_CARD_DRAWN:          return "CARD_DRAWN";
        case GAME_EVENT_PLAYER_ACTION_END:   return "PLAYER_ACTION_END";
        case GAME_EVENT_CARD_TAG_CURSED:     return "CARD_TAG_CURSED";
        case GAME_EVENT_CARD_TAG_VAMPIRIC:   return "CARD_TAG_VAMPIRIC";
        default:                             return "UNKNOWN_EVENT";
    }
}

void Game_TriggerEvent(GameContext_t* game, GameEvent_t event) {
    if (!game) {
        d_LogError("Game_TriggerEvent: NULL game");
        return;
    }

    d_LogDebugF("Game event: %s", GameEventToString(event));

    // Check enemy abilities (if in combat)
    if (game->is_combat_mode && game->current_enemy) {
        CheckEnemyAbilityTriggers(game->current_enemy, event, game);
    }

    // Check player trinket passives (only for human player ID 1)
    Player_t* human_player = Game_GetPlayerByID(1);
    if (human_player) {
        CheckTrinketPassiveTriggers(human_player, event, game);
    }

    // Future: Check player status effects
    // if (game->human_player && game->human_player->status_effects) {
    //     ProcessStatusEffectTriggers(game->human_player->status_effects, event);
    // }
}

// ============================================================================
// PLAYER ACTIONS
// ============================================================================

void Game_ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action) {
    if (!game || !player) {
        d_LogError("ExecutePlayerAction: NULL pointer");
        return;
    }

    switch (action) {
        case ACTION_HIT: {
            // Deal card with animation
            if (Game_DealCardWithAnimation(game->deck, &player->hand, player, true)) {
                d_LogInfoF("%s hits (hand value: %d)",
                          GetPlayerName(player), player->hand.total_value);

                // ✅ NEW: Fire event
                Game_TriggerEvent(game, GAME_EVENT_CARD_DRAWN);

                // Process card tag effects (CURSED, VAMPIRIC)
                const Card_t* last_card = GetCardFromHand(&player->hand, player->hand.cards->count - 1);
                if (last_card && last_card->face_up) {
                    ProcessCardTagEffects(last_card, game, player);
                }

                // Check for enemy defeat from trinket/tag damage during card draw
                if (game->is_combat_mode && game->current_enemy && game->current_enemy->is_defeated) {
                    d_LogInfo("COMBAT VICTORY (mid-turn trinket/tag kill)!");
                    player->state = PLAYER_STATE_STAND;  // Force end player turn
                    game->current_player_index++;
                    Game_TriggerEvent(game, GAME_EVENT_PLAYER_ACTION_END);
                    return;
                }

                if (player->hand.is_bust) {
                    player->state = PLAYER_STATE_BUST;
                    game->current_player_index++;
                    d_LogInfoF("%s busts!", GetPlayerName(player));
                    Game_TriggerEvent(game, GAME_EVENT_PLAYER_ACTION_END);
                } else if (player->hand.total_value == 21) {
                    player->state = PLAYER_STATE_STAND;
                    game->current_player_index++;
                    d_LogInfoF("%s reaches 21 (auto-stand)", GetPlayerName(player));
                    Game_TriggerEvent(game, GAME_EVENT_PLAYER_ACTION_END);
                }
            }
            break;
        }

        case ACTION_STAND:
            player->state = PLAYER_STATE_STAND;
            game->current_player_index++;
            d_LogInfoF("%s stands (hand value: %d)",
                      GetPlayerName(player), player->hand.total_value);
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_ACTION_END);
            break;

        case ACTION_DOUBLE:
            if (CanAffordBet(player, player->current_bet)) {
                PlaceBet(player, player->current_bet);  // Double the bet
                Game_DealCardWithAnimation(game->deck, &player->hand, player, true);

                // ✅ NEW: Fire event (double also draws a card)
                Game_TriggerEvent(game, GAME_EVENT_CARD_DRAWN);

                // Process card tag effects (CURSED, VAMPIRIC)
                const Card_t* last_card = GetCardFromHand(&player->hand, player->hand.cards->count - 1);
                if (last_card && last_card->face_up) {
                    ProcessCardTagEffects(last_card, game, player);
                }

                // Check for enemy defeat from trinket/tag damage during double down
                if (game->is_combat_mode && game->current_enemy && game->current_enemy->is_defeated) {
                    d_LogInfo("COMBAT VICTORY (double down trinket/tag kill)!");
                    player->state = player->hand.is_bust ? PLAYER_STATE_BUST : PLAYER_STATE_STAND;
                    game->current_player_index++;
                    Game_TriggerEvent(game, GAME_EVENT_PLAYER_ACTION_END);
                    return;
                }

                player->state = player->hand.is_bust ? PLAYER_STATE_BUST : PLAYER_STATE_STAND;
                game->current_player_index++;
                d_LogInfoF("%s doubles down", GetPlayerName(player));
                Game_TriggerEvent(game, GAME_EVENT_PLAYER_ACTION_END);
            }
            break;

        case ACTION_SPLIT:
            // Future implementation
            d_LogInfo("Split not yet implemented");
            break;
    }
}

PlayerAction_t Game_GetAIAction(const Player_t* player, const Card_t* dealer_upcard) {
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

bool Game_ProcessBettingInput(GameContext_t* game, Player_t* player, int bet_amount) {
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

    // ✅ NEW: Apply status effect modifiers to bet
    if (player->status_effects) {
        // Check minimum bet restrictions (base minimum is 1 chip)
        int base_min = 1;
        int modified_min = GetMinimumBetWithEffects(player->status_effects, base_min);
        if (bet_amount < modified_min) {
            // Only mention status effects if they're actually raising the minimum
            if (modified_min > base_min) {
                d_LogWarningF("ProcessBettingInput: Bet %d below minimum %d (status effects active)",
                             bet_amount, modified_min);
            } else {
                d_LogWarningF("ProcessBettingInput: Bet %d below minimum %d",
                             bet_amount, modified_min);
            }
            return false;
        }

        // Modify bet amount (forced all-in, madness, etc.)
        bet_amount = ModifyBetWithEffects(player->status_effects, bet_amount, player);
        d_LogInfoF("Bet modified by status effects: %d", bet_amount);
    }

    // Validate bet amount
    if (!CanAffordBet(player, bet_amount)) {
        d_LogWarningF("ProcessBettingInput: Player cannot afford bet %d (has %d chips)",
                     bet_amount, GetPlayerChips(player));
        return false;
    }

    // Reset current_bet before placing new bet (in case previous hand didn't clean up)
    d_LogInfoF("🎲 ProcessBettingInput: Resetting current_bet from %d to 0 before placing %d",
               player->current_bet, bet_amount);
    player->current_bet = 0;

    // Execute bet
    if (PlaceBet(player, bet_amount)) {
        // Play chip sound effect
        d_LogInfo("Playing push_chips sound effect");
        a_PlaySoundEffect(&g_push_chips_sound);

        d_LogInfoF("Player bet %d chips", bet_amount);
        // State machine will handle transition to DEALING
        return true;
    }

    return false;
}

bool Game_ProcessPlayerTurnInput(GameContext_t* game, Player_t* player, PlayerAction_t action) {
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
    Player_t* current = Game_GetCurrentPlayer(game);
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
    Game_ExecutePlayerAction(game, player, action);
    return true;
}

// ============================================================================
// DEALER LOGIC
// ============================================================================

void Game_DealerTurn(GameContext_t* game) {
    Player_t* dealer = Game_GetPlayerByID(0);
    if (!dealer) {
        d_LogError("DealerTurn: Dealer not found");
        return;
    }

    // Reveal hidden card
    if (dealer->hand.cards->count > 0) {
        Card_t* hidden = (Card_t*)d_IndexDataFromArray(dealer->hand.cards, 0);
        if (hidden) {
            hidden->face_up = true;

            // Process card tag effects for flipped card (CURSED, VAMPIRIC)
            ProcessCardTagEffects(hidden, game, dealer);

            // Check for enemy defeat from tag damage (hidden card reveal)
            if (game->is_combat_mode && game->current_enemy && game->current_enemy->is_defeated) {
                d_LogInfo("COMBAT VICTORY (tag kill on dealer hidden card reveal)!");
                return;  // Early exit - enemy defeated
            }
        }
    }

    // Dealer hits on 16 or less (with animations)
    while (dealer->hand.total_value < 17) {
        Game_DealCardWithAnimation(game->deck, &dealer->hand, dealer, true);

        // Process card tag effects for dealer draws
        const Card_t* last_card = GetCardFromHand(&dealer->hand, dealer->hand.cards->count - 1);
        if (last_card && last_card->face_up) {
            d_LogInfoF("DealerTurn: Processing tags for dealer card %d (face_up=%d)",
                      last_card->card_id, last_card->face_up);
            ProcessCardTagEffects(last_card, game, dealer);

            // Check for enemy defeat from tag damage (dealer hit)
            if (game->is_combat_mode && game->current_enemy && game->current_enemy->is_defeated) {
                d_LogInfo("COMBAT VICTORY (tag kill during dealer hit)!");
                return;  // Early exit - enemy defeated
            }
        } else {
            d_LogInfoF("DealerTurn: Skipping tags - last_card=%p face_up=%d",
                      (void*)last_card, last_card ? last_card->face_up : -1);
        }
    }

    d_LogInfoF("Dealer finishes with %d", dealer->hand.total_value);
}

const Card_t* Game_GetDealerUpcard(const GameContext_t* game) {
    if (!game) return NULL;

    Player_t* dealer = Game_GetPlayerByID(0);
    if (!dealer || dealer->hand.cards->count < 2) {
        return NULL;
    }

    // Second card is the upcard (first is hidden)
    return (const Card_t*)d_IndexDataFromArray(dealer->hand.cards, 1);
}

// ============================================================================
// ROUND MANAGEMENT
// ============================================================================

// Deck position (center-right of screen, approximate)
#define DECK_POSITION_X 850.0f
#define DECK_POSITION_Y 300.0f
#define CARD_DEAL_DURATION 0.4f  // Seconds for card to slide from deck to hand

bool Game_DealCardWithAnimation(Deck_t* deck, Hand_t* hand, Player_t* player, bool face_up) {
    if (!deck || !hand || !player) return false;

    // Deal card from deck
    Card_t card = DealCard(deck);
    if (card.card_id == -1) {
        d_LogError("DealCardWithAnimation: Deck empty");
        return false;
    }

    // Set face state and load texture
    card.face_up = face_up;
    LoadCardTexture(&card);

    // Add to hand (this becomes the last card in hand)
    AddCardToHand(hand, card);
    size_t card_index = hand->cards->count - 1;
    size_t hand_size = hand->cards->count;

    // Calculate base Y position based on player type
    int base_y;
    if (player->is_dealer) {
        // Dealer: LAYOUT_TOP_MARGIN + SECTION_PADDING + TEXT_LINE_HEIGHT + ELEMENT_GAP
        base_y = LAYOUT_TOP_MARGIN + SECTION_PADDING + TEXT_LINE_HEIGHT + ELEMENT_GAP;
    } else {
        // Player: LAYOUT_TOP_MARGIN + DEALER_AREA_HEIGHT + BUTTON_AREA_HEIGHT +
        //         SECTION_PADDING + TEXT_LINE_HEIGHT + ELEMENT_GAP
        base_y = LAYOUT_TOP_MARGIN + DEALER_AREA_HEIGHT + BUTTON_AREA_HEIGHT +
                 SECTION_PADDING + TEXT_LINE_HEIGHT + ELEMENT_GAP;
    }

    // Calculate final position using shared fan layout
    int target_x_int, target_y_int;
    CalculateCardFanPosition(card_index, hand_size, base_y, &target_x_int, &target_y_int);

    float target_x = (float)target_x_int;
    float target_y = (float)target_y_int;

    // Get managers (from sceneBlackjack.c)
    CardTransitionManager_t* anim_mgr = GetCardTransitionManager();
    TweenManager_t* tween_mgr = GetTweenManager();

    // Start animation from deck to hand
    StartCardDealAnimation(anim_mgr, tween_mgr,
                           hand, card_index,
                           DECK_POSITION_X, DECK_POSITION_Y,
                           target_x, target_y,
                           CARD_DEAL_DURATION,
                           face_up);  // Flip face-up halfway if needed

    return true;
}

void Game_DealInitialHands(GameContext_t* game) {
    if (!game) return;

    d_LogInfo("Dealing initial hands...");

    // Deal 2 cards to each player (with animations)
    for (int round = 0; round < 2; round++) {
        for (size_t i = 0; i < game->active_players->count; i++) {
            int* id = (int*)d_IndexDataFromArray(game->active_players, i);
            Player_t* player = Game_GetPlayerByID(*id);

            if (!player) continue;

            // Dealer's first card is face down
            bool face_up = !(player->is_dealer && round == 0);

            // Deal card with animation
            if (!Game_DealCardWithAnimation(game->deck, &player->hand, player, face_up)) {
                d_LogError("Failed to deal card during initial hands!");
                return;
            }

            // Trigger CARD_DRAWN event for ability counters (only for non-dealer)
            if (!player->is_dealer) {
                Game_TriggerEvent(game, GAME_EVENT_CARD_DRAWN);
            }

            // Process card tag effects (CURSED, VAMPIRIC) for all players, only if face-up
            const Card_t* last_card = GetCardFromHand(&player->hand, player->hand.cards->count - 1);
            if (last_card && last_card->face_up) {
                ProcessCardTagEffects(last_card, game, player);

                // Check for enemy defeat from tag damage during initial deal (rare but possible)
                if (game->is_combat_mode && game->current_enemy && game->current_enemy->is_defeated) {
                    d_LogInfo("COMBAT VICTORY (tag kill during initial deal)!");
                    return;  // Early exit - enemy defeated
                }
            }
        }
    }

    // Check for blackjacks
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (player && player->hand.is_blackjack) {
            player->state = PLAYER_STATE_BLACKJACK;
            d_LogInfoF("%s has BLACKJACK!", GetPlayerName(player));

            // Trigger event immediately when blackjack is dealt
            if (!player->is_dealer) {
                Game_TriggerEvent(game, GAME_EVENT_PLAYER_BLACKJACK);
            }
        }
    }
}

void Game_ResolveRound(GameContext_t* game) {
    if (!game) return;

    Player_t* dealer = Game_GetPlayerByID(0);
    if (!dealer) {
        d_LogError("ResolveRound: Dealer not found");
        return;
    }

    // Process status effects at round start (chip drain, etc.)
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        if (player->status_effects) {
            ProcessStatusEffectsRoundStart(player->status_effects, player);
        }
    }

    int dealer_value = dealer->hand.total_value;
    bool dealer_bust = dealer->hand.is_bust;

    d_LogInfoF("Resolving round - Dealer: %d%s",
               dealer_value, dealer_bust ? " (BUST)" : "");

    // Fire dealer bust event
    if (dealer_bust) {
        Game_TriggerEvent(game, GAME_EVENT_DEALER_BUST);
    }

    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        const char* outcome = "";
        int damage_dealt = 0;  // Damage to enemy

        // Save bet amount BEFORE WinBet/LoseBet clears it
        int bet_amount = player->current_bet;
        int hand_value = player->hand.total_value;

        // Record bet in stats (do this BEFORE incrementing turn counter)
        Stats_RecordChipsBet(bet_amount);

        // Track turn played for stats
        Stats_RecordTurnPlayed();

        if (player->hand.is_bust) {
            LoseBet(player);

            // Apply status effect loss modifiers (TILT doubles losses)
            if (player->status_effects) {
                int additional_loss = ModifyLosses(player->status_effects, bet_amount);
                if (additional_loss > 0) {
                    player->chips -= additional_loss;
                    Stats_RecordChipsDrained(additional_loss);
                    Stats_UpdateChipsPeak(player->chips);
                    d_LogInfoF("Status effect additional loss: %d chips", additional_loss);
                }
            }

            player->state = PLAYER_STATE_LOST;
            outcome = "BUST - LOSE";

            // Track loss in stats
            Stats_RecordTurnLost();
            Stats_RecordChipsLost(bet_amount);

            // Fire event
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_BUST);

        } else if (player->hand.is_blackjack) {
            if (dealer->hand.is_blackjack) {
                ReturnBet(player);
                player->state = PLAYER_STATE_PUSH;
                outcome = "PUSH - Half Damage";

                // Push deals half damage (explicitly calculate half to avoid any issues)
                int full_damage = hand_value * bet_amount;
                damage_dealt = full_damage / 2;

                // Track push in stats
                Stats_RecordTurnPushed();

                // Fire event
                Game_TriggerEvent(game, GAME_EVENT_PLAYER_PUSH);
            } else {
                // Calculate base winnings (net profit only)
                int base_winnings = (int)(bet_amount * 1.5f);

                // Apply status effect win modifiers (GREED caps at 50% of bet)
                if (player->status_effects) {
                    base_winnings = ModifyWinnings(player->status_effects, base_winnings, bet_amount);
                }

                // Award net winnings only (bet was never deducted)
                player->chips += base_winnings;
                Stats_UpdateChipsPeak(player->chips);
                player->current_bet = 0;

                player->state = PLAYER_STATE_WON;
                outcome = "BLACKJACK - WIN 3:2";
                // Damage = hand_value × bet_amount
                damage_dealt = hand_value * bet_amount;

                // Track win in stats
                Stats_RecordTurnWon();
                Stats_RecordChipsWon(base_winnings);

                // Fire event
                Game_TriggerEvent(game, GAME_EVENT_PLAYER_BLACKJACK);
            }
        } else if (dealer_bust) {
            // Calculate base winnings (net profit only)
            int base_winnings = bet_amount;

            // Apply status effect win modifiers (GREED caps at 50% of bet)
            if (player->status_effects) {
                base_winnings = ModifyWinnings(player->status_effects, base_winnings, bet_amount);
            }

            // Award net winnings only (bet was never deducted)
            player->chips += base_winnings;
            player->current_bet = 0;

            player->state = PLAYER_STATE_WON;
            outcome = "DEALER BUST - WIN";
            damage_dealt = hand_value * bet_amount;

            // Track win in stats
            Stats_RecordTurnWon();
            Stats_RecordChipsWon(base_winnings);

            // ✅ NEW: Fire event
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_WIN);

        } else if (player->hand.total_value > dealer_value) {
            // Calculate base winnings (net profit only)
            int base_winnings = bet_amount;

            // ✅ NEW: Apply status effect win modifiers (GREED caps at 50% of bet)
            if (player->status_effects) {
                base_winnings = ModifyWinnings(player->status_effects, base_winnings, bet_amount);
            }

            // Award net winnings only (bet was never deducted)
            player->chips += base_winnings;
            player->current_bet = 0;

            player->state = PLAYER_STATE_WON;
            outcome = "WIN";
            damage_dealt = hand_value * bet_amount;

            // Track win in stats
            Stats_RecordTurnWon();
            Stats_RecordChipsWon(base_winnings);

            // ✅ NEW: Fire event
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_WIN);

        } else if (player->hand.total_value < dealer_value) {
            LoseBet(player);

            // ✅ NEW: Apply status effect loss modifiers (TILT doubles losses)
            if (player->status_effects) {
                int additional_loss = ModifyLosses(player->status_effects, bet_amount);
                if (additional_loss > 0) {
                    player->chips -= additional_loss;
                    Stats_RecordChipsDrained(additional_loss);
                    Stats_UpdateChipsPeak(player->chips);
                    d_LogInfoF("Status effect additional loss: %d chips", additional_loss);
                }
            }

            player->state = PLAYER_STATE_LOST;
            outcome = "LOSE";

            // Track loss in stats
            Stats_RecordTurnLost();
            Stats_RecordChipsLost(bet_amount);

            // ✅ NEW: Fire event
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_LOSS);

        } else {
            ReturnBet(player);
            player->state = PLAYER_STATE_PUSH;
            outcome = "PUSH - Half Damage";

            // Push deals half damage (explicitly calculate half to avoid any issues)
            int full_damage = hand_value * bet_amount;
            damage_dealt = full_damage / 2;

            // Track push in stats
            Stats_RecordTurnPushed();

            // ✅ NEW: Fire event
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_PUSH);
        }

        d_LogInfoF("%s: %s (damage_dealt=%d)",
                   GetPlayerName(player), outcome, damage_dealt);

        // Apply combat damage to enemy if in combat mode
        if (game->is_combat_mode && game->current_enemy) {
            if (damage_dealt > 0) {
                TakeDamage(game->current_enemy, damage_dealt);
                TweenEnemyHP(game->current_enemy);  // Smooth HP bar drain animation
                TriggerEnemyDamageEffect(game->current_enemy, &g_tween_manager);  // Shake + red flash

                // Track damage in global stats (use appropriate source)
                DamageSource_t damage_source = (player->state == PLAYER_STATE_PUSH)
                    ? DAMAGE_SOURCE_TURN_PUSH
                    : DAMAGE_SOURCE_TURN_WIN;
                Stats_RecordDamage(damage_source, damage_dealt);

                // Spawn floating damage number (centered, above HP bar)
                // Position using constants from sceneBlackjack.h
                SpawnDamageNumber(damage_dealt,
                                  SCREEN_WIDTH / 2 + ENEMY_HP_BAR_X_OFFSET,
                                  ENEMY_HP_BAR_Y - DAMAGE_NUMBER_Y_OFFSET,
                                  false);

                d_LogInfoF("Combat: %s deals %d damage to %s (HP: %d/%d)",
                          GetPlayerName(player), damage_dealt,
                          GetEnemyName(game->current_enemy),
                          game->current_enemy->current_hp,
                          game->current_enemy->max_hp);
            }

            // Check for enemy defeat
            if (game->current_enemy->is_defeated) {
                d_LogInfoF("COMBAT VICTORY! %s has been defeated!",
                          GetEnemyName(game->current_enemy));

                // Clear all status effects on victory (no chip drain on victory screen!)
                for (size_t j = 0; j < game->active_players->count; j++) {
                    int* player_id = (int*)d_IndexDataFromArray(game->active_players, j);
                    Player_t* p = Game_GetPlayerByID(*player_id);
                    if (p && !p->is_dealer && p->status_effects) {
                        ClearAllStatusEffects(p->status_effects);
                    }
                }

                // Don't continue to normal round end - go to victory state
                return;
            }
        }
    }

    // ✅ NEW: Tick status effect durations (remove expired effects)
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        if (player->status_effects) {
            TickStatusEffectDurations(player->status_effects);
        }
    }

    // ✅ NEW: Fire round end event (after all outcomes resolved)
    Game_TriggerEvent(game, GAME_EVENT_HAND_END);
}

void Game_StartNewRound(GameContext_t* game) {
    if (!game) return;

    d_LogInfo("Starting new round...");

    // Clear all hands
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

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

void Game_AddPlayerToGame(GameContext_t* game, int player_id) {
    if (!game) {
        d_LogError("AddPlayerToGame: NULL game pointer");
        return;
    }

    d_AppendDataToArray(game->active_players, &player_id);
    d_LogInfoF("Added player %d to game", player_id);
}

Player_t* Game_GetCurrentPlayer(GameContext_t* game) {
    if (!game || game->current_player_index >= (int)game->active_players->count) {
        return NULL;
    }

    int* id = (int*)d_IndexDataFromArray(game->active_players,
                                          game->current_player_index);
    if (!id) return NULL;

    return Game_GetPlayerByID(*id);
}

Player_t* Game_GetPlayerByID(int player_id) {
    if (!g_players) return NULL;

    // Constitutional pattern: Table stores Player_t by value, return direct pointer
    return (Player_t*)d_GetDataFromTable(g_players, &player_id);
}

