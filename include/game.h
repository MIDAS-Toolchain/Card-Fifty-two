#ifndef GAME_H
#define GAME_H

#include "common.h"

// ============================================================================
// GAME CONTEXT LIFECYCLE
// ============================================================================

/**
 * InitGameContext - Initialize a game context
 *
 * Constitutional pattern: Caller provides stack-allocated GameContext_t
 * Uses dTable_t and dArray_t for dynamic data (initialized here)
 *
 * @param game - Pointer to stack-allocated GameContext_t
 * @param deck - Pointer to initialized deck
 */
void InitGameContext(GameContext_t* game, Deck_t* deck);

/**
 * CleanupGameContext - Clean up game context resources
 *
 * Destroys internal dTable_t and dArray_t, but does not free game pointer
 * (caller owns the GameContext_t memory)
 *
 * @param game - Pointer to game context
 */
void CleanupGameContext(GameContext_t* game);

// ============================================================================
// STATE MACHINE
// ============================================================================

/**
 * TransitionState - Move to a new game state
 *
 * Logs transition, resets state timer, executes entry actions
 *
 * @param game - Game context
 * @param new_state - Target state
 */
void TransitionState(GameContext_t* game, GameState_t new_state);

/**
 * UpdateGameLogic - Main game loop update (called every frame)
 *
 * @param game - Game context
 * @param dt - Delta time in seconds
 */
void UpdateGameLogic(GameContext_t* game, float dt);

/**
 * StateToString - Convert state enum to string
 *
 * @param state - Game state
 * @return const char* - State name
 */
const char* StateToString(GameState_t state);

// ============================================================================
// TIMING CONSTANTS
// ============================================================================

#define PLAYER_ACTION_DELAY  0.5f   // Delay after player action before dealer turn
#define DEALER_ACTION_DELAY  0.5f   // Delay between dealer actions
#define CARD_REVEAL_DELAY    0.3f   // Card flip animation time

// ============================================================================
// STATE UPDATE FUNCTIONS
// ============================================================================

/**
 * UpdateBettingState - Handle betting phase logic
 */
void UpdateBettingState(GameContext_t* game);

/**
 * UpdateDealingState - Handle initial card deal
 */
void UpdateDealingState(GameContext_t* game);

/**
 * UpdatePlayerTurnState - Handle player turn logic
 */
void UpdatePlayerTurnState(GameContext_t* game, float dt);

/**
 * UpdateDealerTurnState - Handle dealer turn logic
 */
void UpdateDealerTurnState(GameContext_t* game, float dt);

/**
 * UpdateShowdownState - Handle round resolution
 */
void UpdateShowdownState(GameContext_t* game);

/**
 * UpdateRoundEndState - Handle round cleanup and restart
 */
void UpdateRoundEndState(GameContext_t* game);

/**
 * UpdateCombatVictoryState - Handle combat victory screen display
 */
void UpdateCombatVictoryState(GameContext_t* game);

// ============================================================================
// PLAYER ACTIONS
// ============================================================================

/**
 * ExecutePlayerAction - Apply a player action to the game state
 *
 * @param game - Game context
 * @param player - Player performing action
 * @param action - Action to execute
 */
void ExecutePlayerAction(GameContext_t* game, Player_t* player, PlayerAction_t action);

/**
 * GetAIAction - Determine AI player's action (basic strategy)
 *
 * @param player - AI player
 * @param dealer_upcard - Dealer's visible card
 * @return PlayerAction_t - Recommended action
 */
PlayerAction_t GetAIAction(const Player_t* player, const Card_t* dealer_upcard);

// ============================================================================
// INPUT PROCESSING (UI → Game Logic Bridge)
// ============================================================================

/**
 * ProcessBettingInput - Handle bet placement from UI layer
 *
 * @param game - Game context
 * @param player - Player placing bet
 * @param bet_amount - Amount to bet
 * @return true if bet accepted and state transitioned, false if invalid
 *
 * Validates bet amount, player chips, and game state before executing.
 * Automatically transitions to STATE_DEALING on success.
 */
bool ProcessBettingInput(GameContext_t* game, Player_t* player, int bet_amount);

/**
 * ProcessPlayerTurnInput - Handle player action from UI layer
 *
 * @param game - Game context
 * @param player - Player taking action
 * @param action - Action to execute (HIT/STAND/DOUBLE)
 * @return true if action valid and executed, false if rejected
 *
 * Validates action legality (e.g., DOUBLE only on 2 cards) before executing.
 * Ensures player is current player before allowing action.
 */
bool ProcessPlayerTurnInput(GameContext_t* game, Player_t* player, PlayerAction_t action);

// ============================================================================
// DEALER LOGIC
// ============================================================================

/**
 * DealerTurn - Execute dealer's turn (hits on 16, stands on 17)
 *
 * @param game - Game context
 */
void DealerTurn(GameContext_t* game);

/**
 * GetDealerUpcard - Get dealer's visible card
 *
 * @param game - Game context
 * @return const Card_t* - Dealer's upcard (NULL if none)
 */
const Card_t* GetDealerUpcard(const GameContext_t* game);

// ============================================================================
// ROUND MANAGEMENT
// ============================================================================

/**
 * DealInitialHands - Deal 2 cards to each player with animations
 *
 * @param game - Game context
 */
void DealInitialHands(GameContext_t* game);

/**
 * DealCardWithAnimation - Deal card to player and spawn animation
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
bool DealCardWithAnimation(Deck_t* deck, Hand_t* hand, Player_t* player, bool face_up);

/**
 * ResolveRound - Calculate winners and apply payouts
 *
 * @param game - Game context
 */
void ResolveRound(GameContext_t* game);

/**
 * StartNewRound - Reset for next round
 *
 * @param game - Game context
 */
void StartNewRound(GameContext_t* game);

// ============================================================================
// PLAYER MANAGEMENT
// ============================================================================

/**
 * AddPlayerToGame - Add player to active game
 *
 * @param game - Game context
 * @param player_id - Player ID to add
 */
void AddPlayerToGame(GameContext_t* game, int player_id);

/**
 * GetCurrentPlayer - Get player whose turn it is
 *
 * @param game - Game context
 * @return Player_t* - Current player (NULL if invalid)
 */
Player_t* GetCurrentPlayer(GameContext_t* game);

/**
 * GetPlayerByID - Lookup player in global table
 *
 * @param player_id - Player ID
 * @return Player_t* - Player pointer (NULL if not found)
 */
Player_t* GetPlayerByID(int player_id);

// ============================================================================
// STATE VARIABLE HELPERS (Constitutional: Typed tables)
// ============================================================================

/**
 * SetBoolFlag - Store boolean flag in game state
 *
 * @param game - Game context
 * @param key - String key
 * @param value - Boolean value
 */
void SetBoolFlag(GameContext_t* game, const char* key, bool value);

/**
 * GetBoolFlag - Retrieve boolean flag
 *
 * @param game - Game context
 * @param key - String key
 * @param default_value - Value to return if key not found
 * @return bool - Flag value
 */
bool GetBoolFlag(const GameContext_t* game, const char* key, bool default_value);

/**
 * ClearBoolFlag - Remove boolean flag from state
 *
 * @param game - Game context
 * @param key - String key
 */
void ClearBoolFlag(GameContext_t* game, const char* key);

/**
 * SetDealerPhase - Set current dealer turn phase
 *
 * @param game - Game context
 * @param phase - Dealer phase value
 */
void SetDealerPhase(GameContext_t* game, DealerPhase_t phase);

/**
 * GetDealerPhase - Get current dealer turn phase
 *
 * @param game - Game context
 * @return DealerPhase_t - Current phase (defaults to CHECK_REVEAL)
 */
DealerPhase_t GetDealerPhase(const GameContext_t* game);

/**
 * ClearDealerPhase - Remove dealer phase from state
 *
 * @param game - Game context
 */
void ClearDealerPhase(GameContext_t* game);

#endif // GAME_H
