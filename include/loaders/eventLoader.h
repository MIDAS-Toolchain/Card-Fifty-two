#ifndef EVENT_LOADER_H
#define EVENT_LOADER_H

#include "../event.h"
#include <Daedalus.h>

// ============================================================================
// GLOBAL REGISTRY (DUF tree + no table, following ADR-019 pattern)
// ============================================================================

/**
 * g_events_db - Raw DUF tree for event definitions
 *
 * Loaded at startup, freed on shutdown.
 * Events are parsed on-demand from this tree.
 */
extern dDUFValue_t* g_events_db;

// ============================================================================
// LOAD DATABASE (Startup - parse file into tree)
// ============================================================================

/**
 * LoadEventDatabase - Parse event DUF file into tree structure
 *
 * @param filepath - Path to DUF file ("data/events/tutorial_events.duf")
 * @param out_db - Output DUF tree (freed on shutdown)
 * @return dDUFError_t* - Parse error (line/column info), or NULL on success
 *
 * Called once at startup. Stores raw DUF tree in global g_events_db.
 * If error is returned, caller must call d_DUFErrorFree().
 */
dDUFError_t* LoadEventDatabase(const char* filepath, dDUFValue_t** out_db);

// ============================================================================
// VALIDATE DATABASE (Startup - fail fast on schema errors)
// ============================================================================

/**
 * ValidateEventDatabase - Validate ALL events in database
 *
 * @param db - Parsed DUF database
 * @param out_error_msg - Buffer for error message (shown in SDL dialog)
 * @param error_msg_size - Size of error buffer
 * @return bool - true if ALL events valid, false if ANY fail
 *
 * Iterates through every event entry and attempts to parse it.
 * If ANY event fails, writes detailed error message (file, key, issue).
 * Called once at startup. Failure shows SDL dialog and exits.
 *
 * Validation rules:
 * - Event must have exactly 3 choices
 * - Choice 3 must have a requirement field (locked choice pattern)
 * - All tag/strategy/requirement enum values must be valid
 * - All required fields must be present (title, description, type)
 */
bool ValidateEventDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size);

// ============================================================================
// LOAD ON-DEMAND (Runtime - heap-allocated event)
// ============================================================================

/**
 * LoadEventFromDUF - Load event from DUF (on-demand)
 *
 * @param key - Event key ("system_maintenance", "house_odds", etc.)
 * @return EventEncounter_t* - Heap-allocated event, or NULL if not found
 *
 * OWNERSHIP: Returned pointer is HEAP-ALLOCATED. Caller must:
 * - Call DestroyEvent() when done (defined in event.c)
 *
 * Called during gameplay when event is needed (from event pool factories).
 * Validation already happened at startup, so this should never fail.
 *
 * Implementation:
 * - Finds @key in g_events_db
 * - Parses event metadata (title, description, type)
 * - Parses 3 choices (enforced via validation)
 * - Parses granted_tags, trinket_reward, enemy_hp_multiplier, requirement
 * - Returns fully-initialized EventEncounter_t*
 */
EventEncounter_t* LoadEventFromDUF(const char* key);

// ============================================================================
// CLEANUP SYSTEM (Shutdown - free DUF database)
// ============================================================================

/**
 * CleanupEventSystem - Free all event resources
 *
 * Frees g_events_db (DUF tree).
 * Called on shutdown to release memory.
 *
 * NOTE: Does NOT have CleanupEventTemplate() - events use DestroyEvent()
 * from event.c (already exists).
 */
void CleanupEventSystem(void);

#endif // EVENT_LOADER_H
