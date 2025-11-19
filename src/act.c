/*
 * Act System Implementation
 * Roguelike progression with ordered encounter sequences
 */

#include "../include/act.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

Act_t* CreateAct(void) {
    Act_t* act = malloc(sizeof(Act_t));
    if (!act) {
        d_LogError("Failed to allocate Act");
        return NULL;
    }

    // Initialize encounter array (capacity FIRST, element_size second - ADR-006)
    act->encounters = d_InitArray(16, sizeof(Encounter_t));
    if (!act->encounters) {
        d_LogError("Failed to initialize encounters array");
        free(act);
        return NULL;
    }

    act->event_pool = NULL;  // Set by CreateTutorialAct or manually
    act->current_encounter_index = 0;

    d_LogInfo("Act created");
    return act;
}

void DestroyAct(Act_t** act) {
    if (!act || !*act) return;

    Act_t* a = *act;

    // Destroy encounters array
    if (a->encounters) {
        d_DestroyArray(a->encounters);
        a->encounters = NULL;
    }

    // Destroy event pool
    if (a->event_pool) {
        DestroyEventPool(&a->event_pool);
    }

    free(a);
    *act = NULL;

    d_LogInfo("Act destroyed");
}

// ============================================================================
// ACT MANAGEMENT
// ============================================================================

void AddEncounter(Act_t* act, EncounterType_t type,
                  Enemy_t* (*enemy_factory)(void),
                  const char* portrait_path) {
    if (!act) {
        d_LogError("AddEncounter: NULL act");
        return;
    }

    Encounter_t encounter = {0};
    encounter.type = type;
    encounter.enemy_factory = enemy_factory;

    // Copy portrait path (or leave empty for events)
    if (portrait_path) {
        strncpy(encounter.portrait_path, portrait_path, sizeof(encounter.portrait_path) - 1);
        encounter.portrait_path[sizeof(encounter.portrait_path) - 1] = '\0';
    } else {
        encounter.portrait_path[0] = '\0';
    }

    d_AppendDataToArray(act->encounters, &encounter);

    d_LogInfoF("Added %s encounter to act (total: %zu)",
              GetEncounterTypeName(type), act->encounters->count);
}

Encounter_t* GetCurrentEncounter(const Act_t* act) {
    if (!act || !act->encounters) {
        return NULL;
    }

    if (act->current_encounter_index >= (int)act->encounters->count) {
        return NULL;  // Act complete
    }

    return (Encounter_t*)d_IndexDataFromArray(act->encounters, act->current_encounter_index);
}

void AdvanceEncounter(Act_t* act) {
    if (!act) {
        d_LogError("AdvanceEncounter: NULL act");
        return;
    }

    act->current_encounter_index++;
    d_LogInfoF("Advanced to encounter %d/%zu",
              act->current_encounter_index, act->encounters->count);
}

bool IsActComplete(const Act_t* act) {
    if (!act || !act->encounters) {
        return true;
    }

    return act->current_encounter_index >= (int)act->encounters->count;
}

const char* GetEncounterTypeName(EncounterType_t type) {
    switch (type) {
        case ENCOUNTER_NORMAL: return "NORMAL";
        case ENCOUNTER_ELITE:  return "ELITE";
        case ENCOUNTER_BOSS:   return "BOSS";
        case ENCOUNTER_EVENT:  return "EVENT";
        default:               return "UNKNOWN";
    }
}

// ============================================================================
// PRESET ACTS
// ============================================================================

Act_t* CreateTutorialAct(void) {
    Act_t* act = CreateAct();
    if (!act) return NULL;

    // Set event pool (tutorial events: SanityTrade, ChipGamble)
    act->event_pool = CreateTutorialEventPool();
    if (!act->event_pool) {
        d_LogError("Failed to create tutorial event pool");
        DestroyAct(&act);
        return NULL;
    }

    // Encounter sequence:
    // 1. NORMAL: The Didact
    AddEncounter(act, ENCOUNTER_NORMAL, CreateTheDidact, "resources/enemies/didact.png");

    // 2. EVENT: Tutorial event pool
    AddEncounter(act, ENCOUNTER_EVENT, NULL, NULL);

    // 3. ELITE: The Daemon
    AddEncounter(act, ENCOUNTER_ELITE, CreateTheDaemon, "resources/enemies/daemon.png");

    // 4. EVENT: Tutorial event pool
    AddEncounter(act, ENCOUNTER_EVENT, NULL, NULL);

    d_LogInfo("Tutorial act created: [NORMAL, EVENT, ELITE, EVENT]");
    return act;
}
