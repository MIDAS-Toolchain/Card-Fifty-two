/*
 * Card Fifty-Two - Game State Machine Implementation
 * Constitutional pattern: dTable_t for state data, dArray_t for player list
 */

#include "../include/common.h"
#include "../include/game.h"
#include "../include/state.h"
#include "../include/player.h"
#include "../include/deck.h"
#include "../include/hand.h"
#include "../include/enemy.h"
#include "../include/stats.h"
#include "../include/event.h"
#include "../include/scenes/sceneBlackjack.h"
#include "../include/scenes/components/visualEffects.h"
#include "../include/cardAnimation.h"
#include "../include/tween/tween.h"
#include "../include/cardTags.h"
#include "../include/stateStorage.h"
#include "../include/statusEffects.h"
#include "../include/trinket.h"
#include "../include/loaders/trinketLoader.h"
#include "../include/audioHelper.h"

// External globals
extern dTable_t* g_players;
extern TweenManager_t g_tween_manager;  // From sceneBlackjack.c
extern int g_cleared_status_effects;  // From sceneBlackjack.c (for result screen message)

// ============================================================================
// GAME EVENTS
// ============================================================================

const char* GameEventToString(GameEvent_t event) {
    switch (event) {
        case GAME_EVENT_COMBAT_START:        return "COMBAT_START";
        case GAME_EVENT_HAND_START:          return "HAND_START";
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

                // Set dirty flag for combat stat recalculation (ADR-09)
                player->combat_stats_dirty = true;

                // Play card slide sound
                PlayCardSlideSound();

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

                // Set dirty flag for combat stat recalculation (ADR-09)
                player->combat_stats_dirty = true;

                // Play card slide sound
                PlayCardSlideSound();

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

    // NOTE: Betting modifiers REMOVED from status effects (ADR-002)
    // Status effects are outcome modifiers only - betting is controlled by sanity system
    // Bet amount validation happens below (CanAffordBet, minimum 1 chip)

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
        Card_t* hidden = (Card_t*)d_ArrayGet(dealer->hand.cards, 0);
        if (hidden) {
            hidden->face_up = true;

            // Set dirty flag for combat stat recalculation (dealer card revealed - ADR-09)
            Player_t* human_player = Game_GetPlayerByID(1);
            if (human_player) {
                human_player->combat_stats_dirty = true;
            }

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

        // Play card slide sound
        PlayCardSlideSound();

        // Set dirty flag for combat stat recalculation (dealer drew card - ADR-09)
        Player_t* human_player = Game_GetPlayerByID(1);
        if (human_player) {
            human_player->combat_stats_dirty = true;
        }

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
    return (const Card_t*)d_ArrayGet(dealer->hand.cards, 1);
}

// ============================================================================
// ROUND MANAGEMENT
// ============================================================================

// Deck position (center-right of screen, approximate)
// Deck position is dynamic - cards deal from center of table
#define CARD_DEAL_DURATION 0.4f

// Calculate deck position at runtime (center of screen, at table height)
static inline float GetDeckPositionX(void) {
    return GetWindowWidth() / 2.0f;
}

static inline float GetDeckPositionY(void) {
    // Table is at bottom of screen, deck should be slightly above table center
    return GetWindowHeight() - 300.0f;  // Roughly where table/dealer area is
}  // Seconds for card to slide from deck to hand

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

    // Get actual runtime Y position from FlexBox layout (matches render positions!)
    int base_y;
    if (player->is_dealer) {
        base_y = GetDealerCardY();  // Runtime Y from FlexBox
    } else {
        base_y = GetPlayerCardY();  // Runtime Y from FlexBox
    }

    // Calculate final position using shared fan layout
    int target_x_int, target_y_int;
    CalculateCardFanPosition(card_index, hand_size, base_y, &target_x_int, &target_y_int);

    float target_x = (float)target_x_int;
    float target_y = (float)target_y_int;

    // Get managers (from sceneBlackjack.c)
    CardTransitionManager_t* anim_mgr = GetCardTransitionManager();
    TweenManager_t* tween_mgr = GetTweenManager();

    // Start animation from deck to hand (using runtime deck position)
    StartCardDealAnimation(anim_mgr, tween_mgr,
                           hand, card_index,
                           GetDeckPositionX(), GetDeckPositionY(),
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
            int* id = (int*)d_ArrayGet(game->active_players, i);
            Player_t* player = Game_GetPlayerByID(*id);

            if (!player) continue;

            // Dealer's first card is face down
            bool face_up = !(player->is_dealer && round == 0);

            // Deal card with animation
            if (!Game_DealCardWithAnimation(game->deck, &player->hand, player, face_up)) {
                d_LogError("Failed to deal card during initial hands!");
                return;
            }

            // Play card slide sound
            PlayCardSlideSound();

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

    // Set dirty flag after initial deal (ADR-09: hands changed)
    Player_t* human_player = Game_GetPlayerByID(1);
    if (human_player) {
        human_player->combat_stats_dirty = true;
        d_LogInfo("Initial hands dealt - combat stats marked dirty");
    }

    // Check for blackjacks
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_ArrayGet(game->active_players, i);
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
        int* id = (int*)d_ArrayGet(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        if (player->status_effects) {
            ProcessStatusEffectsRoundStart(player->status_effects, player);

            // Check for game over after status effects (CHIP_DRAIN can reduce chips to 0!)
            if (player->chips <= 0) {
                d_LogWarning("Player reached 0 chips from CHIP_DRAIN - GAME OVER!");
                game->player_defeated = true;
                // Transition to game over screen
                State_Transition(game, STATE_GAME_OVER);
                return;  // Early exit - don't continue round resolution
            }
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
        int* id = (int*)d_ArrayGet(game->active_players, i);
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
            // Fire bust event BEFORE LoseBet (trinkets need current_bet value)
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_BUST);

            // Fire loss event BEFORE LoseBet (trinkets need current_bet value for refunds)
            d_LogInfoF("📢 Firing PLAYER_LOSS event (current_bet=%d, chips=%d)",
                       player->current_bet, player->chips);
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_LOSS);

            // Apply loss refund from aggregated stats (Elite Membership passive + affixes)
            // Do this BEFORE LoseBet() so we can refund part of the bet
            if (player->loss_refund_percent > 0) {
                int refund = (bet_amount * player->loss_refund_percent) / 100;
                player->chips += refund;
                d_LogInfoF("💰 Loss refund: +%d%% of %d bet = +%d chips (total: %d chips)",
                           player->loss_refund_percent, bet_amount, refund, player->chips);
            }

            LoseBet(player);

            // Check for game over (0 chips = defeat)
            if (player->chips <= 0) {
                d_LogWarning("Player reached 0 chips - GAME OVER!");
                game->player_defeated = true;
                player->state = PLAYER_STATE_LOST;
                State_Transition(game, STATE_GAME_OVER);
                return;  // Skip remaining loss processing and damage calculation
            }

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

        } else if (player->hand.is_blackjack) {
            if (dealer->hand.is_blackjack) {
                player->state = PLAYER_STATE_PUSH;
                outcome = "PUSH";

                // Push damage based on player stat (default 0%, Pusher's Pebble = 50%)
                int full_damage = hand_value * bet_amount;
                damage_dealt = (full_damage * player->push_damage_percent) / 100;

                // Track push in stats
                Stats_RecordTurnPushed();

                // Fire event BEFORE returning bet (trinkets need current_bet value)
                Game_TriggerEvent(game, GAME_EVENT_PLAYER_PUSH);

                // Return bet after event (clears current_bet)
                ReturnBet(player);
            } else {
                // Calculate base winnings (net profit only)
                int base_winnings = (int)(bet_amount * 1.5f);

                // Apply status effect win modifiers (GREED caps at 50% of bet)
                if (player->status_effects) {
                    base_winnings = ModifyWinnings(player->status_effects, base_winnings, bet_amount);
                }

                // Trinket win modifiers now handled via event system (GAME_EVENT_PLAYER_WIN)
                // Elite Membership and other trinkets trigger ExecuteAddChipsPercent() automatically

                // Award net winnings only (bet was never deducted)
                player->chips += base_winnings;
                Stats_UpdateChipsPeak(player->chips);

                player->state = PLAYER_STATE_WON;
                outcome = "BLACKJACK - WIN 3:2";
                // Damage = hand_value × bet_amount
                damage_dealt = hand_value * bet_amount;

                // Track win in stats
                Stats_RecordTurnWon();
                Stats_RecordChipsWon(base_winnings);

                // Fire event BEFORE clearing bet (trinkets need current_bet value)
                Game_TriggerEvent(game, GAME_EVENT_PLAYER_BLACKJACK);

                // Clear bet after event (so trinkets can access bet amount)
                player->current_bet = 0;
            }
        } else if (dealer_bust) {
            // Calculate base winnings (net profit only)
            int base_winnings = bet_amount;

            // Apply status effect win modifiers (GREED caps at 50% of bet)
            if (player->status_effects) {
                base_winnings = ModifyWinnings(player->status_effects, base_winnings, bet_amount);
            }

            // Trinket win modifiers now handled via event system (GAME_EVENT_PLAYER_WIN)
            // Elite Membership and other trinkets trigger ExecuteAddChipsPercent() automatically

            // Award net winnings only (bet was never deducted)
            player->chips += base_winnings;

            player->state = PLAYER_STATE_WON;
            outcome = "DEALER BUST - WIN";
            damage_dealt = hand_value * bet_amount;

            // Track win in stats
            Stats_RecordTurnWon();
            Stats_RecordChipsWon(base_winnings);

            // Fire event BEFORE clearing bet (trinkets need current_bet value)
            d_LogInfoF("📢 Firing PLAYER_WIN event (current_bet=%d, chips=%d)",
                       player->current_bet, player->chips);
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_WIN);

            // Apply win bonus from aggregated stats (Elite Membership passive + affixes)
            if (player->win_bonus_percent > 0) {
                int bonus = (bet_amount * player->win_bonus_percent) / 100;
                player->chips += bonus;
                d_LogInfoF("💰 Win bonus: +%d%% of %d bet = +%d chips (total: %d chips)",
                           player->win_bonus_percent, bet_amount, bonus, player->chips);
            }

            // Apply flat chip bonus on win (Prosperous affix)
            if (player->flat_chips_on_win > 0) {
                player->chips += player->flat_chips_on_win;
                d_LogInfoF("💰 Flat win bonus: +%d chips (total: %d chips)",
                           player->flat_chips_on_win, player->chips);
            }

            // Clear bet after event (so trinkets can access bet amount)
            player->current_bet = 0;

        } else if (player->hand.total_value > dealer_value) {
            // Calculate base winnings (net profit only)
            int base_winnings = bet_amount;

            // ✅ NEW: Apply status effect win modifiers (GREED caps at 50% of bet)
            if (player->status_effects) {
                base_winnings = ModifyWinnings(player->status_effects, base_winnings, bet_amount);
            }

            // Trinket win modifiers now handled via event system (GAME_EVENT_PLAYER_WIN)
            // Elite Membership and other trinkets trigger ExecuteAddChipsPercent() automatically

            // Award net winnings only (bet was never deducted)
            player->chips += base_winnings;

            player->state = PLAYER_STATE_WON;
            outcome = "WIN";
            damage_dealt = hand_value * bet_amount;

            // Track win in stats
            Stats_RecordTurnWon();
            Stats_RecordChipsWon(base_winnings);

            // Fire event BEFORE clearing bet (trinkets need current_bet value)
            d_LogInfoF("📢 Firing PLAYER_WIN event (current_bet=%d, chips=%d)",
                       player->current_bet, player->chips);
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_WIN);

            // Apply win bonus from aggregated stats (Elite Membership passive + affixes)
            if (player->win_bonus_percent > 0) {
                int bonus = (bet_amount * player->win_bonus_percent) / 100;
                player->chips += bonus;
                d_LogInfoF("💰 Win bonus: +%d%% of %d bet = +%d chips (total: %d chips)",
                           player->win_bonus_percent, bet_amount, bonus, player->chips);
            }

            // Apply flat chip bonus on win (Prosperous affix)
            if (player->flat_chips_on_win > 0) {
                player->chips += player->flat_chips_on_win;
                d_LogInfoF("💰 Flat win bonus: +%d chips (total: %d chips)",
                           player->flat_chips_on_win, player->chips);
            }

            // Clear bet after event (so trinkets can access bet amount)
            player->current_bet = 0;

        } else if (player->hand.total_value < dealer_value) {
            // Fire loss event BEFORE LoseBet (trinkets need current_bet value)
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_LOSS);

            // Apply loss refund from aggregated stats (Elite Membership passive + affixes)
            // Do this BEFORE LoseBet() so we can refund part of the bet
            if (player->loss_refund_percent > 0) {
                int refund = (bet_amount * player->loss_refund_percent) / 100;
                player->chips += refund;
                d_LogInfoF("💰 Loss refund: +%d%% of %d bet = +%d chips (total: %d chips)",
                           player->loss_refund_percent, bet_amount, refund, player->chips);
            }

            LoseBet(player);

            // Check for game over (0 chips = defeat)
            if (player->chips <= 0) {
                d_LogWarning("Player reached 0 chips - GAME OVER!");
                game->player_defeated = true;
                player->state = PLAYER_STATE_LOST;
                State_Transition(game, STATE_GAME_OVER);
                return;  // Skip remaining loss processing
            }

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

            // Trinket loss modifiers now handled via event system (GAME_EVENT_PLAYER_LOSS)
            // Elite Membership and other trinkets trigger ExecuteRefundChipsPercent() automatically

            player->state = PLAYER_STATE_LOST;
            outcome = "LOSE";

            // Track loss in stats
            Stats_RecordTurnLost();
            Stats_RecordChipsLost(bet_amount);

        } else {
            player->state = PLAYER_STATE_PUSH;
            outcome = "PUSH";

            // Push damage based on player stat (default 0%, Pusher's Pebble = 50%)
            int full_damage = hand_value * bet_amount;
            damage_dealt = (full_damage * player->push_damage_percent) / 100;

            // Track push in stats
            Stats_RecordTurnPushed();

            // Fire event BEFORE returning bet (trinkets need current_bet value)
            Game_TriggerEvent(game, GAME_EVENT_PLAYER_PUSH);

            // Return bet after event (clears current_bet)
            ReturnBet(player);
        }

        d_LogInfoF("%s: %s (base_damage=%d)",
                   GetPlayerName(player), outcome, damage_dealt);

        // Apply combat damage to enemy if in combat mode
        if (game->is_combat_mode && game->current_enemy) {
            if (damage_dealt > 0) {
                // Apply combat stat modifiers (ADR-013: Universal damage modifier)
                bool is_crit = false;
                damage_dealt = ApplyPlayerDamageModifiers(player, damage_dealt, &is_crit);
                TakeDamage(game->current_enemy, damage_dealt);

                // Apply RAKE status effect outcome modifier AFTER damage (ADR-002 compliant)
                // RAKE: Lose chips when dealing damage (consumes stacks)
                if (player->status_effects) {
                    int rake_penalty = ApplyRakeEffect(player->status_effects, player, damage_dealt);
                    if (rake_penalty > 0) {
                        d_LogInfoF("RAKE penalty applied: -%d chips", rake_penalty);
                    }
                }

                // Check for game over after rake (rake can reduce chips to 0!)
                if (player->chips <= 0) {
                    d_LogWarning("Player reached 0 chips from RAKE penalty - GAME OVER!");
                    game->player_defeated = true;
                    // Transition to game over screen
                    State_Transition(game, STATE_GAME_OVER);
                    return;  // Early exit - don't continue turn processing
                }

                TweenEnemyHP(game->current_enemy);  // Smooth HP bar drain animation
                TriggerEnemyDamageEffect(game->current_enemy, &g_tween_manager);  // Shake + red flash

                // Track damage in global stats (use appropriate source)
                DamageSource_t damage_source = (player->state == PLAYER_STATE_PUSH)
                    ? DAMAGE_SOURCE_TURN_PUSH
                    : DAMAGE_SOURCE_TURN_WIN;
                Stats_RecordDamage(damage_source, damage_dealt);

                // Track push damage for Pusher's Pebble trinket (after modifiers applied)
                if (player->state == PLAYER_STATE_PUSH && player->push_damage_percent > 0) {
                    // Find Pusher's Pebble in player's trinket slots
                    for (int slot = 0; slot < 6; slot++) {
                        if (!player->trinket_slot_occupied[slot]) continue;

                        TrinketInstance_t* instance = &player->trinket_slots[slot];
                        const TrinketTemplate_t* template = GetTrinketTemplate(
                            d_StringPeek(instance->base_trinket_key)
                        );

                        if (template && template->passive_effect_type == TRINKET_EFFECT_PUSH_DAMAGE_PERCENT) {
                            // Calculate this trinket's proportional contribution
                            // If multiple trinkets have push_damage_percent, split proportionally
                            int trinket_contribution = (damage_dealt * template->passive_effect_value) / player->push_damage_percent;
                            TRINKET_ADD_STAT(instance, TRINKET_STAT_DAMAGE_DEALT, trinket_contribution);
                            d_LogDebugF("Pusher's Pebble tracking: +%d modified push damage", trinket_contribution);

                            CleanupTrinketTemplate((TrinketTemplate_t*)template);
                            free((void*)template);
                            break;  // Only one Pusher's Pebble can exist
                        }

                        if (template) {
                            CleanupTrinketTemplate((TrinketTemplate_t*)template);
                            free((void*)template);
                        }
                    }
                }

                // Spawn floating damage number (centered, above HP bar)
                // Position using constants from sceneBlackjack.h
                VisualEffects_t* vfx = GetVisualEffects();
                if (vfx) {
                    VFX_SpawnDamageNumber(vfx, damage_dealt,
                                          SCREEN_WIDTH / 2 + ENEMY_HP_BAR_X_OFFSET,
                                          ENEMY_HP_BAR_Y - DAMAGE_NUMBER_Y_OFFSET,
                                          false, is_crit, false);  // Pass crit flag, not rake
                }

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
                g_cleared_status_effects = 0;  // Reset before accumulating
                for (size_t j = 0; j < game->active_players->count; j++) {
                    int* player_id = (int*)d_ArrayGet(game->active_players, j);
                    Player_t* p = Game_GetPlayerByID(*player_id);
                    if (p && !p->is_dealer && p->status_effects) {
                        size_t cleared = ClearAllStatusEffects(p->status_effects);
                        g_cleared_status_effects += cleared;  // Track for result screen
                    }
                }
                d_LogInfoF("Combat victory: Cleared %d total status effects", g_cleared_status_effects);

                // Don't continue to normal round end - go to victory state
                return;
            }
        }
    }

    // ✅ NEW: Tick status effect durations (remove expired effects)
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_ArrayGet(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (!player || player->is_dealer) continue;

        if (player->status_effects) {
            TickStatusEffectDurations(player->status_effects);
        }

        // Tick trinket cooldowns at END of round (after win/loss/push resolved)
        TickTrinketCooldowns(player);
    }

    // ✅ NEW: Fire round end event (after all outcomes resolved)
    Game_TriggerEvent(game, GAME_EVENT_HAND_END);
}

void Game_StartNewRound(GameContext_t* game) {
    if (!game) return;

    d_LogInfo("Starting new round...");

    // Clear all hands
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_ArrayGet(game->active_players, i);
        Player_t* player = Game_GetPlayerByID(*id);

        if (player) {
            ClearHand(&player->hand, game->deck);
            player->current_bet = 0;
            player->state = PLAYER_STATE_WAITING;

            // Set dirty flag for combat stat recalculation (hand was cleared - ADR-09)
            player->combat_stats_dirty = true;
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
// EVENT SYSTEM
// ============================================================================

void Game_ApplyEventConsequences(GameContext_t* game, EventEncounter_t* event, Player_t* player) {
    if (!game || !event || !player) {
        d_LogError("Game_ApplyEventConsequences: NULL parameter");
        return;
    }

    if (event->selected_choice < 0) {
        d_LogWarning("Game_ApplyEventConsequences: No choice selected");
        return;
    }

    const EventChoice_t* choice = GetEventChoice(event, event->selected_choice);
    if (!choice) {
        d_LogError("Game_ApplyEventConsequences: Invalid selected_choice");
        return;
    }

    // Apply consequences using event.c helper
    ApplyEventConsequences(event, player, game->deck);

    // Apply enemy HP multiplier for next combat only (if not default 1.0) - ONE-TIME
    if (choice->next_enemy_hp_multi != 1.0f) {
        game->next_enemy_hp_multiplier = choice->next_enemy_hp_multi;
        d_LogInfoF("Event consequence: Next enemy HP multiplier set to %.2f (one-time)", choice->next_enemy_hp_multi);
    }

    // Trigger game events for any abilities that might react
    // (Future: enemy abilities could react to chip/sanity changes)
    if (choice->chips_delta != 0) {
        d_LogDebugF("Event consequence: chips changed by %+d", choice->chips_delta);
        // TODO: Add GAME_EVENT_CHIPS_CHANGED to GameEvent_t enum
        // Game_TriggerEvent(game, GAME_EVENT_CHIPS_CHANGED);
    }

    if (choice->sanity_delta != 0) {
        d_LogDebugF("Event consequence: sanity changed by %+d", choice->sanity_delta);
        // TODO: Add GAME_EVENT_SANITY_CHANGED to GameEvent_t enum
        // Game_TriggerEvent(game, GAME_EVENT_SANITY_CHANGED);
    }

    d_LogInfoF("Applied event consequences: chips=%+d, sanity=%+d, tags=%zu/%zu, hp_mult=%.2f",
               choice->chips_delta, choice->sanity_delta,
               choice->granted_tags->count, choice->removed_tags->count,
               choice->next_enemy_hp_multi);
}

int Game_GetEventRerollCost(const GameContext_t* game) {
    if (!game) return 50;  // Fallback to default
    return game->event_reroll_cost;
}

void Game_IncrementRerollCost(GameContext_t* game) {
    if (!game) return;
    game->event_reroll_cost *= 2;
    d_LogDebugF("Event reroll cost doubled: %d", game->event_reroll_cost);
}

void Game_ResetRerollCost(GameContext_t* game) {
    if (!game) return;
    game->event_reroll_cost = game->event_reroll_base_cost;
    d_LogDebugF("Event reroll cost reset to base: %d", game->event_reroll_cost);
}

// ============================================================================
// PLAYER MANAGEMENT
// ============================================================================

void Game_AddPlayerToGame(GameContext_t* game, int player_id) {
    if (!game) {
        d_LogError("AddPlayerToGame: NULL game pointer");
        return;
    }

    d_ArrayAppend(game->active_players, &player_id);
    d_LogInfoF("Added player %d to game", player_id);
}

Player_t* Game_GetCurrentPlayer(GameContext_t* game) {
    if (!game || game->current_player_index >= (int)game->active_players->count) {
        return NULL;
    }

    int* id = (int*)d_ArrayGet(game->active_players,
                                          game->current_player_index);
    if (!id) return NULL;

    return Game_GetPlayerByID(*id);
}

Player_t* Game_GetPlayerByID(int player_id) {
    if (!g_players) return NULL;

    // Constitutional pattern: Table stores Player_t by value, return direct pointer
    return (Player_t*)d_TableGet(g_players, &player_id);
}

