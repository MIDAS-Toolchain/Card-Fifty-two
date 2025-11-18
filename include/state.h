/*
 * State Machine Module
 * Manages game state transitions and state-specific update logic
 * Constitutional pattern: Clean state machine with explicit transitions
 */

#ifndef STATE_H
#define STATE_H

#include "structs.h"
#include "defs.h"
#include <stdbool.h>

// ============================================================================
// STATE MACHINE API
// ============================================================================

/**
 * Initialize game context (allocate tables, set initial state)
 */
void State_InitContext(GameContext_t* game, Deck_t* deck);

/**
 * Cleanup game context (destroy tables, free resources)
 */
void State_CleanupContext(GameContext_t* game);

/**
 * Convert GameState_t enum to human-readable string
 */
const char* State_ToString(GameState_t state);

/**
 * Transition to new state with entry actions
 * Logs transition, updates timers, executes state entry logic
 */
void State_Transition(GameContext_t* game, GameState_t new_state);

/**
 * Main state machine update - delegates to state-specific update functions
 */
void State_UpdateLogic(GameContext_t* game, float dt);

// ============================================================================
// STATE-SPECIFIC UPDATE FUNCTIONS (called by State_UpdateLogic)
// ============================================================================

void State_UpdateBetting(GameContext_t* game);
void State_UpdateDealing(GameContext_t* game);
void State_UpdatePlayerTurn(GameContext_t* game, float dt);
void State_UpdateDealerTurn(GameContext_t* game, float dt);
void State_UpdateShowdown(GameContext_t* game);
void State_UpdateRoundEnd(GameContext_t* game);
void State_UpdateCombatVictory(GameContext_t* game);
void State_UpdateRewardScreen(GameContext_t* game);
void State_UpdateEvent(GameContext_t* game);

#endif // STATE_H
