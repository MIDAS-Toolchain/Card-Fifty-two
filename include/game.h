#ifndef GAME_H
#define GAME_H

#include "common.h"

// Forward declarations
struct EventEncounter;

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

#define PLAYER_ACTION_DELAY  0.5f   // Delay after player action before dealer turn
#define DEALER_ACTION_DELAY  0.5f   // Delay between dealer actions
#define CARD_REVEAL_DELAY    0.3f   // Card flip animation time

// ============================================================================
// GAME EVENTS (for ability system)
// ============================================================================

/**
 * GameEvent_t - Events that occur during gameplay
 * Used to trigger enemy abilities and status effects
 *
 * Hierarchy: COMBAT (boss fight) → HAND (blackjack round) → PLAYER_ACTION
 */
typedef enum {
    GAME_EVENT_COMBAT_START,         // Combat begins (enemy appears) - fires ONCE per combat
    GAME_EVENT_HAND_START,           // Blackjack hand starts (before dealing cards) - fires MULTIPLE times per combat
    GAME_EVENT_HAND_END,             // Blackjack hand ends (after payouts) - fires MULTIPLE times per combat
    GAME_EVENT_PLAYER_WIN,           // Player wins hand (beat dealer)
    GAME_EVENT_PLAYER_LOSS,          // Player loses hand
    GAME_EVENT_PLAYER_PUSH,          // Tie with dealer
    GAME_EVENT_PLAYER_BUST,          // Player busts (over 21)
    GAME_EVENT_PLAYER_BLACKJACK,     // Player gets blackjack
    GAME_EVENT_DEALER_BUST,          // Dealer busts
    GAME_EVENT_CARD_DRAWN,           // Player draws card (hit)
    GAME_EVENT_PLAYER_ACTION_END,    // Player's action ends (stand/bust/blackjack/double)
    GAME_EVENT_CARD_TAG_CURSED,      // CURSED tag activated (10 damage to enemy)
    GAME_EVENT_CARD_TAG_VAMPIRIC,    // VAMPIRIC tag activated (5 damage + 5 chips)
} GameEvent_t;

/**
 * Game_TriggerEvent - Fire game event (triggers enemy abilities, status effects)
 *
 * @param game - Game context
 * @param event - Event type
 *
 * Notifies ability system of gameplay outcomes for event-driven triggers
 */
void Game_TriggerEvent(GameContext_t* game, GameEvent_t event);

/**
 * GameEventToString - Convert event enum to string (for logging)
 *
 * @param event - Event type
 * @return const char* - Event name
 */
const char* GameEventToString(GameEvent_t event);

// ============================================================================
// PLAYER ACTIONS
// ============================================================================

/**
 * Game_ExecutePlayerAction - Apply a player action to the game state
 *
 * @param game - Game context
 * @param player - Player performing action
 * @param action - Action to execute
 */
void Game_ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action);

/**
 * Game_GetAIAction - Determine AI player's action (basic strategy)
 *
 * @param player - AI player
 * @param dealer_upcard - Dealer's visible card
 * @return PlayerAction_t - Recommended action
 */
PlayerAction_t Game_GetAIAction(const Player_t* player, const Card_t* dealer_upcard);

// ============================================================================
// INPUT PROCESSING (UI → Game Logic Bridge)
// ============================================================================

/**
 * Game_ProcessBettingInput - Handle bet placement from UI layer
 *
 * @param game - Game context
 * @param player - Player placing bet
 * @param bet_amount - Amount to bet
 * @return true if bet accepted and state transitioned, false if invalid
 *
 * Validates bet amount, player chips, and game state before executing.
 * Automatically transitions to STATE_DEALING on success.
 */
bool Game_ProcessBettingInput(GameContext_t* game, Player_t* player, int bet_amount);

/**
 * Game_ProcessPlayerTurnInput - Handle player action from UI layer
 *
 * @param game - Game context
 * @param player - Player taking action
 * @param action - Action to execute (HIT/STAND/DOUBLE)
 * @return true if action valid and executed, false if rejected
 *
 * Validates action legality (e.g., DOUBLE only on 2 cards) before executing.
 * Ensures player is current player before allowing action.
 */
bool Game_ProcessPlayerTurnInput(GameContext_t* game, Player_t* player, PlayerAction_t action);

// ============================================================================
// DEALER LOGIC
// ============================================================================

/**
 * Game_DealerTurn - Execute dealer's turn (hits on 16, stands on 17)
 *
 * @param game - Game context
 */
void Game_DealerTurn(GameContext_t* game);

/**
 * Game_GetDealerUpcard - Get dealer's visible card
 *
 * @param game - Game context
 * @return const Card_t* - Dealer's upcard (NULL if none)
 */
const Card_t* Game_GetDealerUpcard(const GameContext_t* game);

// ============================================================================
// ROUND MANAGEMENT
// ============================================================================

/**
 * Game_DealInitialHands - Deal 2 cards to each player with animations
 *
 * @param game - Game context
 */
void Game_DealInitialHands(GameContext_t* game);

/**
 * Game_DealCardWithAnimation - Deal card to player and spawn animation
 *
 * Helper function that deals a card, adds it to hand, and starts animation
 * from deck position to calculated hand position.
 *
 * @param deck - Deck to deal from
 * @param hand - Hand to add card to
 * @param player - Player receiving the card (used to calculate correct Y position)
 * @param face_up - Should card be face-up?
 * @return true if card dealt successfully, false if deck empty
 */
bool Game_DealCardWithAnimation(Deck_t* deck, Hand_t* hand, Player_t* player, bool face_up);

/**
 * Game_ResolveRound - Calculate winners and apply payouts
 *
 * @param game - Game context
 */
void Game_ResolveRound(GameContext_t* game);

/**
 * Game_StartNewRound - Reset for next round
 *
 * @param game - Game context
 */
void Game_StartNewRound(GameContext_t* game);

// ============================================================================
// EVENT SYSTEM
// ============================================================================

/**
 * Game_ApplyEventConsequences - Apply selected event choice consequences
 *
 * @param game - Game context
 * @param event - Event encounter with selected choice
 * @param player - Player to apply consequences to
 *
 * Applies chips/sanity deltas and card tag modifications.
 * Triggers game events (GAME_EVENT_CHIPS_CHANGED, GAME_EVENT_SANITY_CHANGED).
 * Should be called once after player confirms event selection.
 *
 * Requirements:
 * - event->selected_choice must be valid (>= 0)
 * - game->deck must exist for tag application
 */
void Game_ApplyEventConsequences(GameContext_t* game, struct EventEncounter* event, Player_t* player);

/**
 * Game_GetEventRerollCost - Get current event reroll cost
 *
 * @param game - Game context
 * @return int - Current reroll cost in chips
 */
int Game_GetEventRerollCost(const GameContext_t* game);

/**
 * Game_IncrementRerollCost - Double the reroll cost (exponential scaling)
 *
 * @param game - Game context
 *
 * Doubles game->event_reroll_cost (50 → 100 → 200 → 400...).
 * Call after successful reroll.
 */
void Game_IncrementRerollCost(GameContext_t* game);

/**
 * Game_ResetRerollCost - Reset reroll cost to base value
 *
 * @param game - Game context
 *
 * Sets game->event_reroll_cost = game->event_reroll_base_cost (50).
 * Call at start of new event encounter.
 */
void Game_ResetRerollCost(GameContext_t* game);

// ============================================================================
// PLAYER MANAGEMENT
// ============================================================================

/**
 * Game_AddPlayerToGame - Add player to active game
 *
 * @param game - Game context
 * @param player_id - Player ID to add
 */
void Game_AddPlayerToGame(GameContext_t* game, int player_id);

/**
 * Game_GetCurrentPlayer - Get player whose turn it is
 *
 * @param game - Game context
 * @return Player_t* - Current player (NULL if invalid)
 */
Player_t* Game_GetCurrentPlayer(GameContext_t* game);

/**
 * Game_GetPlayerByID - Lookup player in global table
 *
 * @param player_id - Player ID
 * @return Player_t* - Player pointer (NULL if not found)
 */
Player_t* Game_GetPlayerByID(int player_id);

#endif // GAME_H
