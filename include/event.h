#ifndef EVENT_H
#define EVENT_H

#include "common.h"
#include "cardTags.h"

// ============================================================================
// EVENT SYSTEM (Slay the Spire-style encounters)
// ============================================================================

/**
 * EventType_t - Type of event encounter
 */
typedef enum {
    EVENT_TYPE_DIALOGUE,    // Pure story/lore (1+ choices)
    EVENT_TYPE_CHOICE,      // Branching encounter with consequences
    EVENT_TYPE_SHOP,        // Buy/sell cards (FUTURE)
    EVENT_TYPE_REST,        // Heal/upgrade (FUTURE)
    EVENT_TYPE_BLESSING,    // Pure positive outcome
    EVENT_TYPE_CURSE        // Pure negative outcome
} EventType_t;

/**
 * EventChoice_t - A single choice option in an event
 *
 * Constitutional pattern (per ADR-006):
 * - Value type stored in dArray_t (not pointers)
 * - dString_t for text (not char[])
 * - dArray_t for tag arrays (not raw arrays)
 * - EventEncounter_t owns the choices array
 */
typedef struct EventChoice {
    dString_t* text;            // Choice button text ("Accept the deal", "Refuse")
    dString_t* result_text;     // Outcome description after choice
    int chips_delta;            // Chip reward/cost (positive = gain, negative = lose)
    int sanity_delta;           // Sanity reward/cost (positive = gain, negative = lose)
    dArray_t* granted_tags;     // CardTag_t array to add to random cards
    dArray_t* removed_tags;     // CardTag_t array to remove from cards
} EventChoice_t;

/**
 * EventEncounter_t - A single event encounter
 *
 * Constitutional pattern:
 * - Heap-allocated (pointer type)
 * - Uses dString_t for text (not char[])
 * - Uses dArray_t for choices (value types)
 * - Simple cleanup with DestroyEvent()
 */
typedef struct EventEncounter {
    dString_t* title;           // Event name ("The Broken Dealer")
    dString_t* description;     // Main narrative text
    EventType_t type;           // Event type
    dArray_t* choices;          // Array of EventChoice_t (value types, per ADR-006)
    int selected_choice;        // Player's selection (-1 = none yet)
    bool is_complete;           // true = ready to exit event
} EventEncounter_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateEvent - Initialize a new event
 *
 * @param title - Event title
 * @param description - Main narrative text
 * @param type - Event type
 * @return EventEncounter_t* - Heap-allocated event, or NULL on failure
 *
 * Constitutional pattern: Heap-allocated entity (like Enemy_t)
 * Caller must call DestroyEvent() when done
 */
EventEncounter_t* CreateEvent(const char* title, const char* description, EventType_t type);

/**
 * DestroyEvent - Free event resources
 *
 * @param event - Pointer to event pointer (double pointer for nulling)
 *
 * Frees all internal dString_t and dArray_t, then frees event struct.
 * Per ADR-006: Single call handles nested cleanup (choices array owns memory).
 */
void DestroyEvent(EventEncounter_t** event);

// ============================================================================
// CHOICE MANAGEMENT
// ============================================================================

/**
 * AddEventChoice - Add a choice option to event
 *
 * @param event - Event to modify
 * @param text - Choice button text
 * @param result_text - Outcome description
 * @param chips_delta - Chip reward/cost
 * @param sanity_delta - Sanity reward/cost
 *
 * Per ADR-006: Copies EventChoice_t into event->choices array (value type).
 * Internal dString_t and dArray_t are initialized empty.
 */
void AddEventChoice(EventEncounter_t* event, const char* text, const char* result_text,
                    int chips_delta, int sanity_delta);

/**
 * AddCardTagToChoice - Add a card tag grant to a choice
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param tag - Tag to grant on selection
 *
 * When player selects this choice, a random card will receive this tag.
 */
void AddCardTagToChoice(EventEncounter_t* event, int choice_index, CardTag_t tag);

/**
 * RemoveCardTagFromChoice - Add a tag removal to a choice
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param tag - Tag to remove on selection
 *
 * When player selects this choice, this tag will be removed from all cards.
 */
void RemoveCardTagFromChoice(EventEncounter_t* event, int choice_index, CardTag_t tag);

/**
 * SelectEventChoice - Mark a choice as selected
 *
 * @param event - Event to update
 * @param choice_index - Index of selected choice
 *
 * Sets event->selected_choice and event->is_complete = true.
 * Does not apply consequences (caller handles that).
 */
void SelectEventChoice(EventEncounter_t* event, int choice_index);

/**
 * GetEventChoice - Get a choice by index
 *
 * @param event - Event to query
 * @param choice_index - Index of choice
 * @return const EventChoice_t* - Pointer to choice, or NULL if invalid index
 *
 * Constitutional pattern: Returns pointer to internal value (don't modify!)
 */
const EventChoice_t* GetEventChoice(const EventEncounter_t* event, int choice_index);

/**
 * GetChoiceCount - Get number of choices in event
 *
 * @param event - Event to query
 * @return int - Number of choices
 */
int GetChoiceCount(const EventEncounter_t* event);

// ============================================================================
// CONSEQUENCE APPLICATION
// ============================================================================

/**
 * ApplyEventConsequences - Execute selected choice consequences
 *
 * @param event - Event with selected choice
 * @param player - Player to apply consequences to
 * @param deck - Deck to modify (for tag grants/removals)
 *
 * Applies chips_delta, sanity_delta, and card tag modifications.
 * Should be called once after player confirms selection.
 */
void ApplyEventConsequences(EventEncounter_t* event, struct Player* player, struct Deck* deck);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * EventToString - Convert event to human-readable string
 *
 * @param event - Event to convert
 * @param out - Output dString_t (caller must create and destroy)
 *
 * Format: "Event: <title> | Type: <type> | Choices: <count>"
 */
void EventToString(const EventEncounter_t* event, dString_t* out);

/**
 * GetEventTypeName - Convert event type enum to string
 *
 * @param type - Event type
 * @return const char* - Type name ("Dialogue", "Choice", etc.)
 */
const char* GetEventTypeName(EventType_t type);

// ============================================================================
// PRESET EVENTS (for testing/tutorial)
// ============================================================================

/**
 * CreateGamblersBargainEvent - Create "The Gambler's Bargain" event
 *
 * @return EventEncounter_t* - Preconfigured tutorial event
 *
 * Theme: Risk vs reward choice
 * - Option 1: Gain 20 chips, add CURSED tag to random card
 * - Option 2: Lose 10 chips, add BLESSED tag to random card
 * - Option 3: Decline, no consequences
 */
EventEncounter_t* CreateGamblersBargainEvent(void);

/**
 * CreateRiggedDeckEvent - Create "The Rigged Deck" event
 *
 * @return EventEncounter_t* - Preconfigured event
 *
 * Theme: Card tag manipulation
 * - Option 1: Remove CURSED from all cards, lose 15 sanity
 * - Option 2: Add LUCKY to 3 random cards, lose 30 chips
 * - Option 3: Walk away, no change
 */
EventEncounter_t* CreateRiggedDeckEvent(void);

/**
 * CreateBrokenSlotMachineEvent - Create "The Broken Slot Machine" event
 *
 * @return EventEncounter_t* - Preconfigured event
 *
 * Theme: Volatile random outcome
 * - Option 1: Pull lever - gain/lose random chips (-50 to +50)
 * - Option 2: Fix machine - costs 20 chips, add WILD tag to 1 card
 * - Option 3: Ignore, no change
 */
EventEncounter_t* CreateBrokenSlotMachineEvent(void);

/**
 * CreateDataBrokerEvent - Create "The Data Broker's Offer" event
 *
 * @return EventEncounter_t* - Tutorial event introducing sanity mechanic
 *
 * Theme: Trade sanity for immediate resources (escalating risk/reward)
 * - Option 1: Decline (safe - no change)
 * - Option 2: Minor exchange (+100 chips, -50 sanity)
 * - Option 3: Major exchange (trinket placeholder, -75 sanity)
 * - Option 4: Total exchange (+200 chips + trinket, -100 sanity)
 *
 * Design intent: Teach sanity → betting restrictions → combat difficulty spiral
 * Placement: After defeating The Didact, before The Daemon (unbeatable if sanity depleted)
 */
EventEncounter_t* CreateDataBrokerEvent(void);

#endif // EVENT_H
