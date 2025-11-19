#ifndef EVENT_POOL_H
#define EVENT_POOL_H

#include "event.h"
#include <stdbool.h>

// ============================================================================
// EVENT POOL SYSTEM
// ============================================================================

/**
 * EventFactory - Function pointer type for event creation
 *
 * @return EventEncounter_t* - Newly created event (caller must destroy)
 *
 * Event factories are pure functions that create events on-demand.
 * They're stored in the pool as function pointers, not event instances.
 */
typedef EventEncounter_t* (*EventFactory)(void);

/**
 * EventPool_t - Weighted random pool of event factories
 *
 * Constitutional pattern:
 * - dArray_t for dynamic factory storage
 * - dArray_t for parallel weight array
 * - Factories stored as function pointers (not event instances!)
 * - Weighted random selection for variety
 *
 * Usage:
 * 1. Create pool: EventPool_t* pool = CreateEventPool();
 * 2. Add factories: AddEventToPool(pool, CreateMyEvent, 50);
 * 3. Get random event: EventEncounter_t* event = GetRandomEventFromPool(pool);
 * 4. Use event, then destroy it: DestroyEvent(&event);
 * 5. Cleanup pool: DestroyEventPool(&pool);
 */
typedef struct EventPool {
    dArray_t* factories;    // Array of EventFactory function pointers
    dArray_t* weights;      // Parallel array of int (selection weights)
    int total_weight;       // Sum of all weights (cached for performance)
} EventPool_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateEventPool - Initialize an empty event pool
 *
 * @return EventPool_t* - Heap-allocated pool, or NULL on failure
 *
 * Pool starts empty with 0 total weight.
 * Add factories with AddEventToPool().
 */
EventPool_t* CreateEventPool(void);

/**
 * DestroyEventPool - Free event pool
 *
 * @param pool - Pointer to pool pointer (double pointer for nulling)
 *
 * IMPORTANT: Does NOT destroy events created from pool!
 * Only destroys the pool structure itself.
 * Caller must manually DestroyEvent() for any events they created.
 */
void DestroyEventPool(EventPool_t** pool);

// ============================================================================
// POOL MANAGEMENT
// ============================================================================

/**
 * AddEventToPool - Add an event factory to the pool
 *
 * @param pool - Pool to modify
 * @param factory - Event creation function
 * @param weight - Selection weight (higher = more likely)
 *
 * Constitutional pattern: Stores function pointer, NOT event instance.
 * Weight determines probability: weight / total_weight.
 *
 * Example:
 *   AddEventToPool(pool, CreateCombatEvent, 60);  // 60% likely
 *   AddEventToPool(pool, CreateRestEvent, 40);    // 40% likely
 */
void AddEventToPool(EventPool_t* pool, EventFactory factory, int weight);

/**
 * GetRandomEventFromPool - Select and create a random event
 *
 * @param pool - Pool to select from
 * @return EventEncounter_t* - Newly created event, or NULL if pool empty
 *
 * Uses weighted random selection based on factory weights.
 * Creates a NEW event instance - caller MUST call DestroyEvent() when done.
 *
 * Algorithm:
 * 1. Generate random number 0 to total_weight
 * 2. Iterate through factories, accumulating weights
 * 3. When accumulated weight >= random number, call that factory
 * 4. Return the created event
 */
EventEncounter_t* GetRandomEventFromPool(EventPool_t* pool);

/**
 * GetDifferentEventFromPool - Get random event, avoiding previous selection
 *
 * @param pool - Pool to select from
 * @param previous_title - Title of previous event to avoid (NULL = any event)
 * @return EventEncounter_t* - Newly created event (different from previous), or NULL if impossible
 *
 * Tries up to 10 times to get a different event.
 * If pool only has 1 event, returns that event anyway (can't avoid).
 * Useful for reroll mechanics to prevent immediate repeats.
 *
 * Example:
 *   EventEncounter_t* first = GetRandomEventFromPool(pool);
 *   // User rerolls...
 *   EventEncounter_t* second = GetDifferentEventFromPool(pool, d_StringPeek(first->title));
 *   // second guaranteed different (if pool size > 1)
 */
EventEncounter_t* GetDifferentEventFromPool(EventPool_t* pool, const char* previous_title);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * GetEventPoolSize - Get number of factories in pool
 *
 * @param pool - Pool to query
 * @return int - Number of event factories, or 0 if NULL
 */
int GetEventPoolSize(const EventPool_t* pool);

/**
 * GetEventPoolTotalWeight - Get sum of all weights
 *
 * @param pool - Pool to query
 * @return int - Total weight, or 0 if NULL/empty
 */
int GetEventPoolTotalWeight(const EventPool_t* pool);

// ============================================================================
// PRESET POOLS (for tutorial/testing)
// ============================================================================

/**
 * CreateTutorialEventPool - Create tutorial-specific event pool
 *
 * @return EventPool_t* - Pool with 2 tutorial events (50/50 split)
 *
 * Contains:
 * - CreateSanityTradeEvent (50% weight)
 * - CreateChipGambleEvent (50% weight)
 *
 * Designed for tutorial combat encounters after defeating The Didact.
 */
EventPool_t* CreateTutorialEventPool(void);

#endif // EVENT_POOL_H
