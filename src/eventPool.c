/*
 * Event Pool System Implementation
 * Weighted random selection of event factories
 */

#include "../include/eventPool.h"
#include "../include/random.h"
#include <stdlib.h>
#include <string.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

EventPool_t* CreateEventPool(void) {
    EventPool_t* pool = malloc(sizeof(EventPool_t));
    if (!pool) {
        d_LogFatal("Failed to allocate EventPool");
        return NULL;
    }

    // d_InitArray(capacity, element_size) - capacity FIRST!
    pool->factories = d_InitArray(8, sizeof(EventFactory));
    pool->weights = d_InitArray(8, sizeof(int));
    pool->total_weight = 0;

    if (!pool->factories || !pool->weights) {
        d_LogFatal("Failed to initialize EventPool arrays");
        if (pool->factories) d_DestroyArray(pool->factories);
        if (pool->weights) d_DestroyArray(pool->weights);
        free(pool);
        return NULL;
    }

    d_LogInfo("EventPool created");
    return pool;
}

void DestroyEventPool(EventPool_t** pool) {
    if (!pool || !*pool) return;

    if ((*pool)->factories) {
        d_DestroyArray((*pool)->factories);
    }
    if ((*pool)->weights) {
        d_DestroyArray((*pool)->weights);
    }

    free(*pool);
    *pool = NULL;
    d_LogInfo("EventPool destroyed");
}

// ============================================================================
// POOL MANAGEMENT
// ============================================================================

void AddEventToPool(EventPool_t* pool, EventFactory factory, int weight) {
    if (!pool) {
        d_LogError("AddEventToPool: NULL pool");
        return;
    }

    if (!factory) {
        d_LogError("AddEventToPool: NULL factory function");
        return;
    }

    if (weight <= 0) {
        d_LogWarning("AddEventToPool: Non-positive weight, clamping to 1");
        weight = 1;
    }

    // Append factory and weight (parallel arrays)
    d_AppendDataToArray(pool->factories, &factory);
    d_AppendDataToArray(pool->weights, &weight);
    pool->total_weight += weight;

    d_LogInfoF("Added event factory to pool (weight: %d, total: %d)", weight, pool->total_weight);
}

EventEncounter_t* GetRandomEventFromPool(EventPool_t* pool) {
    if (!pool) {
        d_LogError("GetRandomEventFromPool: NULL pool");
        return NULL;
    }

    if (pool->factories->count == 0) {
        d_LogError("GetRandomEventFromPool: Pool is empty");
        return NULL;
    }

    if (pool->total_weight <= 0) {
        d_LogError("GetRandomEventFromPool: Total weight is 0");
        return NULL;
    }

    // Weighted random selection
    int random_value = GetRandomInt(0, pool->total_weight - 1);
    int accumulated_weight = 0;

    for (size_t i = 0; i < pool->factories->count; i++) {
        int* weight = (int*)d_IndexDataFromArray(pool->weights, i);
        if (!weight) continue;

        accumulated_weight += *weight;

        if (random_value < accumulated_weight) {
            // Found the selected factory!
            EventFactory* factory = (EventFactory*)d_IndexDataFromArray(pool->factories, i);
            if (!factory || !*factory) {
                d_LogErrorF("GetRandomEventFromPool: NULL factory at index %zu", i);
                return NULL;
            }

            // Call factory to create event
            EventEncounter_t* event = (*factory)();
            d_LogInfoF("Selected event from pool (index: %zu, weight: %d)", i, *weight);
            return event;
        }
    }

    // Fallback: should never reach here, but return first event if we do
    d_LogWarning("GetRandomEventFromPool: Fell through selection loop (using fallback)");
    EventFactory* fallback = (EventFactory*)d_IndexDataFromArray(pool->factories, 0);
    if (fallback && *fallback) {
        return (*fallback)();
    }

    return NULL;
}

EventEncounter_t* GetDifferentEventFromPool(EventPool_t* pool, const char* previous_title) {
    if (!pool) {
        d_LogError("GetDifferentEventFromPool: NULL pool");
        return NULL;
    }

    // If pool only has 1 event, return it (can't avoid)
    if (GetEventPoolSize(pool) <= 1) {
        d_LogInfo("GetDifferentEventFromPool: Pool has <=1 events, returning any");
        return GetRandomEventFromPool(pool);
    }

    // If no previous title, just get any event
    if (!previous_title) {
        return GetRandomEventFromPool(pool);
    }

    // Try up to 10 times to get a different event
    for (int attempt = 0; attempt < 10; attempt++) {
        EventEncounter_t* event = GetRandomEventFromPool(pool);
        if (!event) {
            d_LogError("GetDifferentEventFromPool: Failed to create event");
            return NULL;
        }

        // Check if title is different
        const char* new_title = d_StringPeek(event->title);
        if (strcmp(new_title, previous_title) != 0) {
            // Success! Got a different event
            d_LogInfoF("GetDifferentEventFromPool: Got different event '%s' (was '%s')",
                      new_title, previous_title);
            return event;
        }

        // Same event, destroy and retry
        d_LogInfoF("GetDifferentEventFromPool: Got same event '%s', retrying (attempt %d/10)",
                  new_title, attempt + 1);
        DestroyEvent(&event);
    }

    // Fallback: after 10 tries, just return whatever we get
    d_LogWarning("GetDifferentEventFromPool: Failed to get different event after 10 tries, returning duplicate");
    return GetRandomEventFromPool(pool);
}

// ============================================================================
// QUERIES
// ============================================================================

int GetEventPoolSize(const EventPool_t* pool) {
    if (!pool || !pool->factories) return 0;
    return (int)pool->factories->count;
}

int GetEventPoolTotalWeight(const EventPool_t* pool) {
    if (!pool) return 0;
    return pool->total_weight;
}

// ============================================================================
// PRESET POOLS
// ============================================================================

// Forward declare event factories (will be defined in event.c)
extern EventEncounter_t* CreateSystemMaintenanceEvent(void);
extern EventEncounter_t* CreateHouseOddsEvent(void);

EventPool_t* CreateTutorialEventPool(void) {
    EventPool_t* pool = CreateEventPool();
    if (!pool) return NULL;

    // Tutorial has 2 events demonstrating locked choices + enemy HP modification
    AddEventToPool(pool, CreateSystemMaintenanceEvent, 50);  // Teaches CURSED tag requirement + 75% HP
    AddEventToPool(pool, CreateHouseOddsEvent, 50);          // Teaches LUCKY tag requirement + 150% HP

    d_LogInfo("Tutorial event pool created (2 events: System Maintenance + House Odds)");
    return pool;
}
