/*
 * State Storage Module
 * Manages typed state variables for game state machine
 * Constitutional pattern: dTable_t for typed storage, no void*
 */

#ifndef STATE_STORAGE_H
#define STATE_STORAGE_H

#include <Daedalus.h>
#include <stdbool.h>
#include "defs.h"

// ============================================================================
// STATE DATA STRUCTURE
// ============================================================================

/**
 * GameStateData_t - Typed state variable storage
 *
 * Constitutional pattern:
 * - Separate typed tables for each data type (no void*)
 * - Static storage for bool/phase values (persistent memory)
 */
typedef struct GameStateData {
    dTable_t* bool_flags;      // Key: char*, Value: bool*
    dTable_t* int_values;      // Key: char*, Value: int*
    dTable_t* dealer_phase;    // Key: char*, Value: DealerPhase_t*

    // Targeting state (for trinket actives) - TEMPORARILY DISABLED FOR TESTING
    // int targeting_trinket_slot; // -1 = none, 0-5 = slot index
    // int targeting_player_id;    // Player who owns the trinket
} GameStateData_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * StateData_Init - Initialize state storage
 *
 * @param data - State data struct to initialize
 * @return true on success, false on allocation failure
 */
bool StateData_Init(GameStateData_t* data);

/**
 * StateData_Cleanup - Destroy state storage
 *
 * @param data - State data struct to cleanup
 */
void StateData_Cleanup(GameStateData_t* data);

// ============================================================================
// BOOL FLAG OPERATIONS
// ============================================================================

/**
 * StateData_SetBool - Set a boolean flag
 *
 * @param data - State data
 * @param key - Flag name (must be string literal or persistent memory)
 * @param value - Boolean value to set
 */
void StateData_SetBool(GameStateData_t* data, const char* key, bool value);

/**
 * StateData_GetBool - Get a boolean flag
 *
 * @param data - State data
 * @param key - Flag name
 * @param default_value - Value to return if key not found
 * @return Flag value or default_value
 */
bool StateData_GetBool(const GameStateData_t* data, const char* key, bool default_value);

/**
 * StateData_ClearBool - Remove a boolean flag
 *
 * @param data - State data
 * @param key - Flag name to remove
 */
void StateData_ClearBool(GameStateData_t* data, const char* key);

// ============================================================================
// INT VALUE OPERATIONS
// ============================================================================

/**
 * StateData_SetInt - Set an integer value
 *
 * @param data - State data
 * @param key - Value name (must be string literal or persistent memory)
 * @param value - Integer value to set
 */
void StateData_SetInt(GameStateData_t* data, const char* key, int value);

/**
 * StateData_GetInt - Get an integer value
 *
 * @param data - State data
 * @param key - Value name
 * @param default_value - Value to return if key not found
 * @return Integer value or default_value
 */
int StateData_GetInt(const GameStateData_t* data, const char* key, int default_value);

// ============================================================================
// DEALER PHASE OPERATIONS
// ============================================================================

/**
 * StateData_SetPhase - Set dealer phase
 *
 * @param data - State data
 * @param phase - Dealer phase value
 */
void StateData_SetPhase(GameStateData_t* data, DealerPhase_t phase);

/**
 * StateData_GetPhase - Get dealer phase
 *
 * @param data - State data
 * @return Current dealer phase (defaults to CHECK_REVEAL if not set)
 */
DealerPhase_t StateData_GetPhase(const GameStateData_t* data);

/**
 * StateData_ClearPhase - Remove dealer phase state
 *
 * @param data - State data
 */
void StateData_ClearPhase(GameStateData_t* data);

#endif // STATE_STORAGE_H
