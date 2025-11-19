#ifndef ACT_H
#define ACT_H

#include "common.h"
#include "enemy.h"
#include "eventPool.h"

// ============================================================================
// ACT SYSTEM - Roguelike Progression Structure
// ============================================================================

/**
 * EncounterType_t - Types of encounters in an act
 *
 * ENCOUNTER_NORMAL: Regular combat encounter
 * ENCOUNTER_ELITE: Harder combat, better rewards
 * ENCOUNTER_BOSS: Act-ending boss fight
 * ENCOUNTER_EVENT: Post-combat event (uses act's event pool)
 *
 * Note: Shop/Rest are NOT encounter types - they're event choices!
 */
typedef enum EncounterType {
    ENCOUNTER_NORMAL,
    ENCOUNTER_ELITE,
    ENCOUNTER_BOSS,
    ENCOUNTER_EVENT
} EncounterType_t;

/**
 * Encounter_t - Single encounter in an act sequence
 *
 * Constitutional pattern:
 * - Stores factory function pointer (not enemy instance!)
 * - Portrait path as C string (will be loaded on-demand)
 * - For EVENT encounters: enemy_factory and portrait_path are NULL
 */
typedef struct Encounter {
    EncounterType_t type;              // Type of encounter
    Enemy_t* (*enemy_factory)(void);   // Factory for combat encounters (NULL for events)
    char portrait_path[128];           // Path to enemy PNG (empty for events)
} Encounter_t;

/**
 * Act_t - Complete act with ordered encounter sequence
 *
 * Constitutional pattern:
 * - dArray_t of Encounter_t (stored by value, not pointer!)
 * - EventPool_t for post-combat events
 * - current_encounter_index tracks progress
 *
 * Design:
 * Tutorial Act: [NORMAL, EVENT, ELITE, EVENT]
 * Later Acts: [NORMAL, EVENT, NORMAL, EVENT, ELITE, EVENT, BOSS]
 */
typedef struct Act {
    dArray_t* encounters;              // Array of Encounter_t (ordered sequence)
    EventPool_t* event_pool;           // Events that can appear after combats
    int current_encounter_index;       // Which encounter we're on (0-indexed)
} Act_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateAct - Initialize empty act
 *
 * @return Act_t* - Heap-allocated act, or NULL on failure
 *
 * Act starts with no encounters. Use AddEncounter() to build sequence.
 */
Act_t* CreateAct(void);

/**
 * DestroyAct - Free act resources
 *
 * @param act - Pointer to act pointer (double pointer for nulling)
 *
 * Destroys encounters array and event pool.
 * Does NOT destroy enemy instances (caller manages those).
 */
void DestroyAct(Act_t** act);

// ============================================================================
// ACT MANAGEMENT
// ============================================================================

/**
 * AddEncounter - Add encounter to act sequence
 *
 * @param act - Act to modify
 * @param type - Encounter type
 * @param enemy_factory - Factory function (NULL for EVENT type)
 * @param portrait_path - Path to enemy PNG (NULL for EVENT type)
 *
 * Encounters are executed in the order they're added.
 *
 * Example:
 *   AddEncounter(act, ENCOUNTER_NORMAL, CreateGoblin, "goblin.png");
 *   AddEncounter(act, ENCOUNTER_EVENT, NULL, NULL);
 */
void AddEncounter(Act_t* act, EncounterType_t type,
                  Enemy_t* (*enemy_factory)(void),
                  const char* portrait_path);

/**
 * GetCurrentEncounter - Get current encounter in sequence
 *
 * @param act - Act to query
 * @return Encounter_t* - Pointer to current encounter, or NULL if act complete
 *
 * Returns pointer to encounter INSIDE array (don't free it!).
 */
Encounter_t* GetCurrentEncounter(const Act_t* act);

/**
 * AdvanceEncounter - Move to next encounter
 *
 * @param act - Act to modify
 *
 * Increments current_encounter_index.
 * Call after completing an encounter.
 */
void AdvanceEncounter(Act_t* act);

/**
 * IsActComplete - Check if all encounters finished
 *
 * @param act - Act to check
 * @return bool - true if current_index >= total encounters
 */
bool IsActComplete(const Act_t* act);

/**
 * GetEncounterTypeName - Get string name of encounter type
 *
 * @param type - Encounter type
 * @return const char* - Human-readable name
 */
const char* GetEncounterTypeName(EncounterType_t type);

// ============================================================================
// PRESET ACTS
// ============================================================================

/**
 * CreateTutorialAct - Create tutorial act (2 combats, 2 events)
 *
 * @return Act_t* - Tutorial act with preset sequence
 *
 * Encounter sequence:
 * 1. NORMAL: The Didact (500 HP, didact.png)
 * 2. EVENT: Tutorial event pool
 * 3. ELITE: The Daemon (5000 HP, daemon.png)
 * 4. EVENT: Tutorial event pool
 *
 * Event pool: CreateTutorialEventPool() (SanityTrade, ChipGamble)
 */
Act_t* CreateTutorialAct(void);

#endif // ACT_H
