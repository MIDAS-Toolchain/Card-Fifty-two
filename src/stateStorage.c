/*
 * State Storage Module Implementation
 * Constitutional pattern: Typed tables, static storage for persistent values
 */

#include "../include/stateStorage.h"

// ============================================================================
// STATIC STORAGE (Persistent Memory)
// ============================================================================

// Static storage for bool values (only 2 possible values, persistent memory)
static const bool BOOL_TRUE = true;
static const bool BOOL_FALSE = false;

// Static storage for dealer phases (4 possible values, persistent memory)
static const DealerPhase_t PHASE_CHECK_REVEAL = DEALER_PHASE_CHECK_REVEAL;
static const DealerPhase_t PHASE_DECIDE = DEALER_PHASE_DECIDE;
static const DealerPhase_t PHASE_ACTION = DEALER_PHASE_ACTION;
static const DealerPhase_t PHASE_WAIT = DEALER_PHASE_WAIT;

// ============================================================================
// LIFECYCLE
// ============================================================================

bool StateData_Init(GameStateData_t* data) {
    if (!data) {
        d_LogError("StateData_Init: NULL data pointer");
        return false;
    }

    // Initialize typed state tables (Constitutional: explicit types, no void*)
    data->bool_flags = d_TableInit(sizeof(char*), sizeof(bool),
                                    d_HashString, d_CompareString, 16);
    data->int_values = d_TableInit(sizeof(char*), sizeof(int),
                                    d_HashString, d_CompareString, 8);
    data->dealer_phase = d_TableInit(sizeof(char*), sizeof(DealerPhase_t),
                                      d_HashString, d_CompareString, 2);

    if (!data->bool_flags || !data->int_values || !data->dealer_phase) {
        d_LogError("StateData_Init: Failed to initialize state tables");
        if (data->bool_flags) d_TableDestroy(&data->bool_flags);
        if (data->int_values) d_TableDestroy(&data->int_values);
        if (data->dealer_phase) d_TableDestroy(&data->dealer_phase);
        return false;
    }

    // Initialize targeting state (for trinket actives) - TEMPORARILY DISABLED
    // data->targeting_trinket_slot = -1;  // -1 = no trinket active
    // data->targeting_player_id = -1;     // -1 = no player

    d_LogInfoF("StateData initialized (bool_flags=%p, int_values=%p, dealer_phase=%p)",
               (void*)data->bool_flags,
               (void*)data->int_values,
               (void*)data->dealer_phase);

    return true;
}

void StateData_Cleanup(GameStateData_t* data) {
    if (!data) {
        d_LogError("StateData_Cleanup: NULL data pointer");
        return;
    }

    // Destroy typed state tables
    if (data->bool_flags) {
        d_TableDestroy(&data->bool_flags);
        data->bool_flags = NULL;
    }

    if (data->int_values) {
        d_TableDestroy(&data->int_values);
        data->int_values = NULL;
    }

    if (data->dealer_phase) {
        d_TableDestroy(&data->dealer_phase);
        data->dealer_phase = NULL;
    }

    d_LogInfo("StateData cleaned up");
}

// ============================================================================
// BOOL FLAG OPERATIONS
// ============================================================================

void StateData_SetBool(GameStateData_t* data, const char* key, bool value) {
    if (!data) {
        d_LogErrorF("StateData_SetBool: NULL data pointer (key='%s')", key ? key : "NULL");
        return;
    }
    if (!key) {
        d_LogError("StateData_SetBool: NULL key");
        return;
    }
    if (!data->bool_flags) {
        d_LogFatalF("StateData_SetBool: bool_flags is NULL! Memory corruption? (key='%s')", key);
        return;
    }

    // Store pointer to static bool (persistent memory, not stack!)
    d_TableSet(data->bool_flags, &key, value ? &BOOL_TRUE : &BOOL_FALSE);

    // Verify storage (SUSSY checks)
    bool* verify = (bool*)d_TableGet(data->bool_flags, &key);
    if (!verify) {
        d_LogWarningF("StateData_SetBool: SUSSY - Failed to store key='%s', value=%d", key, value);
    } else if (*verify != value) {
        d_LogErrorF("StateData_SetBool: SUSSY - Stored value mismatch! Expected %d, got %d (key='%s')",
                    value, *verify, key);
    }
}

bool StateData_GetBool(const GameStateData_t* data, const char* key, bool default_value) {
    if (!data || !key) {
        return default_value;
    }

    bool* value_ptr = (bool*)d_TableGet(data->bool_flags, &key);
    return value_ptr ? *value_ptr : default_value;
}

void StateData_ClearBool(GameStateData_t* data, const char* key) {
    if (!data || !key) {
        d_LogWarningF("StateData_ClearBool: NULL pointer (data=%p, key=%p)",
                      (void*)data, (void*)key);
        return;
    }

    if (!data->bool_flags) {
        d_LogErrorF("StateData_ClearBool: bool_flags is NULL! (key='%s')", key);
        return;
    }

    // Check if key exists before removing (not an error if it doesn't)
    bool* exists = (bool*)d_TableGet(data->bool_flags, &key);
    if (exists) {
        bool old_value = *exists;  // Copy BEFORE freeing
        d_TableRemove(data->bool_flags, &key);
        d_LogDebugF("StateData_ClearBool: Removed key='%s' (was %d)", key, old_value);
    } else {
        d_LogDebugF("StateData_ClearBool: Key='%s' not found (already clear)", key);
    }
}

// ============================================================================
// INT VALUE OPERATIONS
// ============================================================================

void StateData_SetInt(GameStateData_t* data, const char* key, int value) {
    if (!data || !key) {
        d_LogError("StateData_SetInt: NULL parameter");
        return;
    }

    d_TableSet(data->int_values, &key, &value);
}

int StateData_GetInt(const GameStateData_t* data, const char* key, int default_value) {
    if (!data || !key) {
        d_LogError("StateData_GetInt: NULL parameter");
        return default_value;
    }

    int* value_ptr = (int*)d_TableGet(data->int_values, &key);
    return value_ptr ? *value_ptr : default_value;
}

// ============================================================================
// DEALER PHASE OPERATIONS
// ============================================================================

void StateData_SetPhase(GameStateData_t* data, DealerPhase_t phase) {
    if (!data) {
        d_LogError("StateData_SetPhase: NULL data pointer");
        return;
    }
    if (!data->dealer_phase) {
        d_LogFatal("StateData_SetPhase: dealer_phase is NULL! Memory corruption?");
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
        d_TableSet(data->dealer_phase, &key, phase_ptr);

        // Verify storage (SUSSY checks)
        DealerPhase_t* verify = (DealerPhase_t*)d_TableGet(data->dealer_phase, &key);
        if (!verify) {
            d_LogWarningF("StateData_SetPhase: SUSSY - Failed to store phase=%d", phase);
        } else if (*verify != phase) {
            d_LogErrorF("StateData_SetPhase: SUSSY - Stored phase mismatch! Expected %d, got %d",
                        phase, *verify);
        }
    } else {
        d_LogErrorF("StateData_SetPhase: Invalid phase value %d", phase);
    }
}

DealerPhase_t StateData_GetPhase(const GameStateData_t* data) {
    if (!data) {
        return DEALER_PHASE_CHECK_REVEAL;
    }

    const char* key = "phase";
    DealerPhase_t* phase_ptr = (DealerPhase_t*)d_TableGet(data->dealer_phase, &key);
    return phase_ptr ? *phase_ptr : DEALER_PHASE_CHECK_REVEAL;
}

void StateData_ClearPhase(GameStateData_t* data) {
    if (!data) {
        d_LogWarning("StateData_ClearPhase: NULL data pointer!");
        return;
    }

    if (!data->dealer_phase) {
        d_LogError("StateData_ClearPhase: dealer_phase is NULL!");
        return;
    }

    // SUSSY: Log table state before accessing
    d_LogInfoF("StateData_ClearPhase: data=%p, dealer_phase table=%p",
               (void*)data, (void*)data->dealer_phase);

    // Check if phase exists before removing (not an error if it doesn't)
    const char* key_str = "phase";
    d_LogInfoF("StateData_ClearPhase: About to call d_TableGet with key='%s' at %p",
               key_str, (void*)key_str);

    DealerPhase_t* exists = (DealerPhase_t*)d_TableGet(data->dealer_phase, &key_str);

    d_LogInfo("StateData_ClearPhase: d_TableGet returned");

    if (exists) {
        DealerPhase_t old_phase = *exists;  // Copy BEFORE freeing
        d_TableRemove(data->dealer_phase, &key_str);
        d_LogDebugF("StateData_ClearPhase: Removed phase (was %d)", old_phase);
    } else {
        d_LogDebug("StateData_ClearPhase: Phase not found (already clear)");
    }
}
