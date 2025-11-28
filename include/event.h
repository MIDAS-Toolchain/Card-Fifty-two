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
 * TagTargetStrategy_t - How to select cards for tag application
 *
 * NOTE: All 52 cards always exist in blackjack deck (not a deckbuilder!)
 * Tags UPGRADE cards, they don't add/remove cards from deck.
 */
typedef enum TagTargetStrategy {
    TAG_TARGET_RANDOM_CARD,           // Random card_id 0-51
    TAG_TARGET_HIGHEST_UNTAGGED,      // Highest rank card WITHOUT this tag
    TAG_TARGET_LOWEST_UNTAGGED,       // Lowest rank card WITHOUT this tag
    TAG_TARGET_SUIT_HEARTS,           // All hearts (13 cards)
    TAG_TARGET_SUIT_DIAMONDS,         // All diamonds (13 cards)
    TAG_TARGET_SUIT_CLUBS,            // All clubs (13 cards)
    TAG_TARGET_SUIT_SPADES,           // All spades (13 cards)
    TAG_TARGET_RANK_ACES,             // All 4 aces
    TAG_TARGET_RANK_FACE_CARDS,       // All face cards (J, Q, K = 12 cards)
    TAG_TARGET_ALL_CARDS              // All 52 cards
} TagTargetStrategy_t;

/**
 * RequirementType_t - Type of requirement for unlocking event choice
 *
 * Polymorphic requirement system supports multiple requirement types.
 */
typedef enum RequirementType {
    REQUIREMENT_NONE,              // No requirement (always unlocked)
    REQUIREMENT_TAG_COUNT,         // Requires N cards with specific tag
    REQUIREMENT_TRINKET,           // Requires specific trinket/relic (future)
    REQUIREMENT_HP_THRESHOLD,      // Requires HP >= threshold
    REQUIREMENT_SANITY_THRESHOLD,  // Requires sanity >= threshold
    REQUIREMENT_CHIPS_THRESHOLD    // Requires chips >= threshold
} RequirementType_t;

/**
 * TagChoiceRequirement_t - Tag count requirement
 *
 * Example: "Requires at least 3 CURSED cards in your deck"
 */
typedef struct TagChoiceRequirement {
    CardTag_t required_tag;    // Tag that cards must have
    int min_count;             // Minimum number of cards with this tag
} TagChoiceRequirement_t;

/**
 * TrinketChoiceRequirement_t - Trinket possession requirement
 *
 * Example: "Requires The Lucky Coin"
 */
typedef struct TrinketChoiceRequirement {
    int required_trinket_id;   // Specific trinket ID (future-proof for trinket system)
} TrinketChoiceRequirement_t;

/**
 * HPChoiceRequirement_t - HP threshold requirement
 *
 * Example: "Requires HP >= 50"
 */
typedef struct HPChoiceRequirement {
    int threshold;             // Minimum HP required
} HPChoiceRequirement_t;

/**
 * SanityChoiceRequirement_t - Sanity threshold requirement
 *
 * Example: "Requires sanity >= 25"
 */
typedef struct SanityChoiceRequirement {
    int threshold;             // Minimum sanity required
} SanityChoiceRequirement_t;

/**
 * ChipsChoiceRequirement_t - Chips threshold requirement
 *
 * Example: "Requires at least 100 chips"
 */
typedef struct ChipsChoiceRequirement {
    int threshold;             // Minimum chips required
} ChipsChoiceRequirement_t;

/**
 * ChoiceRequirement_t - Polymorphic requirement for unlocking event choice
 *
 * UI behavior:
 * - If requirement NOT met: choice is grayed out, hotkey shows lock icon, hover shows requirement
 * - If requirement met: choice is normal, hotkey works
 *
 * Constitutional pattern: Type-tagged union (discriminated union)
 * - type field determines which union member is valid
 * - Defaults to REQUIREMENT_NONE (no requirement)
 */
typedef struct ChoiceRequirement {
    RequirementType_t type;
    union {
        TagChoiceRequirement_t tag_req;
        TrinketChoiceRequirement_t trinket_req;
        HPChoiceRequirement_t hp_req;
        SanityChoiceRequirement_t sanity_req;
        ChipsChoiceRequirement_t chips_req;
    } data;
} ChoiceRequirement_t;

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
    dString_t* text;                     // Choice button text ("Accept the deal", "Refuse")
    dString_t* result_text;              // Outcome description after choice
    int chips_delta;                     // Chip reward/cost (positive = gain, negative = lose)
    int sanity_delta;                    // Sanity reward/cost (positive = gain, negative = lose)
    dArray_t* granted_tags;              // CardTag_t array to add to cards
    dArray_t* tag_target_strategies;     // TagTargetStrategy_t array (parallel with granted_tags)
    dArray_t* removed_tags;              // CardTag_t array to remove from cards

    // Requirements system
    ChoiceRequirement_t requirement;     // Requirement to unlock this choice (defaults to REQUIREMENT_NONE)

    // Enemy combat modifiers (applied when transitioning to combat after event)
    float enemy_hp_multiplier;           // HP multiplier for next enemy (1.0 = normal, 0.75 = 75% HP, 1.5 = 150% HP)

    // Trinket reward system
    int trinket_reward_id;               // Trinket ID to grant to player (-1 = none)
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
 * AddCardTagToChoice - Add a card tag grant to a choice (uses TAG_TARGET_RANDOM_CARD)
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param tag - Tag to grant on selection
 *
 * When player selects this choice, a random card (0-51) will receive this tag.
 * Uses legacy TAG_TARGET_RANDOM_CARD strategy (backward compatible).
 */
void AddCardTagToChoice(EventEncounter_t* event, int choice_index, CardTag_t tag);

/**
 * AddCardTagToChoiceWithStrategy - Add a card tag grant with targeting strategy
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param tag - Tag to grant on selection
 * @param strategy - How to select target card(s)
 *
 * Examples:
 *   TAG_TARGET_RANDOM_CARD - Tag 1 random card (0-51)
 *   TAG_TARGET_HIGHEST_UNTAGGED - Tag highest rank card that doesn't have this tag yet
 *   TAG_TARGET_LOWEST_UNTAGGED - Tag lowest rank card that doesn't have this tag yet
 *   TAG_TARGET_SUIT_HEARTS - Tag all 13 hearts
 *   TAG_TARGET_RANK_ACES - Tag all 4 aces
 *   TAG_TARGET_RANK_FACE_CARDS - Tag all face cards (J/Q/K = 12 cards)
 *   TAG_TARGET_ALL_CARDS - Tag all 52 cards
 */
void AddCardTagToChoiceWithStrategy(EventEncounter_t* event, int choice_index,
                                     CardTag_t tag, TagTargetStrategy_t strategy);

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
 * SetChoiceRequirement - Set requirement for a choice
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param requirement - Requirement to set (e.g., TAG_COUNT, SANITY_THRESHOLD, etc.)
 *
 * Example usage:
 *   ChoiceRequirement_t req = {.type = REQUIREMENT_TAG_COUNT};
 *   req.data.tag_req.required_tag = CARD_TAG_CURSED;
 *   req.data.tag_req.min_count = 3;
 *   SetChoiceRequirement(event, 2, req);
 */
void SetChoiceRequirement(EventEncounter_t* event, int choice_index, ChoiceRequirement_t requirement);

/**
 * SetChoiceTrinketReward - Set trinket reward for a choice
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param trinket_id - Trinket ID to grant (from trinket registry)
 *
 * When player selects this choice, the specified trinket will be equipped
 * to the first empty trinket slot. If all slots full, reward is lost.
 *
 * Example: SetChoiceTrinketReward(event, 0, 1);  // Grant Elite Membership (ID 1)
 */
void SetChoiceTrinketReward(EventEncounter_t* event, int choice_index, int trinket_id);

/**
 * SetChoiceEnemyHPMultiplier - Set enemy HP multiplier for a choice
 *
 * @param event - Event containing choice
 * @param choice_index - Index of choice to modify
 * @param multiplier - HP multiplier (1.0 = normal, 0.75 = 75% HP, 1.5 = 150% HP)
 *
 * When player selects this choice, the next spawned enemy will have modified HP.
 * Example: multiplier = 0.75 means enemy starts at 75% of normal max HP.
 */
void SetChoiceEnemyHPMultiplier(EventEncounter_t* event, int choice_index, float multiplier);

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
// REQUIREMENT SYSTEM
// ============================================================================

/**
 * IsChoiceRequirementMet - Check if a choice requirement is satisfied
 *
 * @param req - Requirement to check
 * @param player - Player to check against
 * @return bool - true if requirement is met (or REQUIREMENT_NONE), false otherwise
 *
 * Checks based on requirement type:
 * - REQUIREMENT_NONE: Always true
 * - REQUIREMENT_TAG_COUNT: Counts cards with tag in full 52-card deck
 * - REQUIREMENT_TRINKET: Not yet implemented (returns false)
 * - REQUIREMENT_HP_THRESHOLD: Checks player HP >= threshold
 * - REQUIREMENT_SANITY_THRESHOLD: Checks player sanity >= threshold
 * - REQUIREMENT_CHIPS_THRESHOLD: Checks player chips >= threshold
 */
bool IsChoiceRequirementMet(const ChoiceRequirement_t* req, const struct Player* player);

/**
 * GetRequirementTooltip - Generate tooltip text for requirement
 *
 * @param req - Requirement to convert to text
 * @param out - Output dString_t (caller must create, this function appends)
 *
 * Examples:
 * - "Requires at least 3 CURSED cards"
 * - "Requires at least 100 chips"
 * - "Requires sanity >= 50"
 */
void GetRequirementTooltip(const ChoiceRequirement_t* req, dString_t* out);

/**
 * CountCardsWithTag - Count how many cards (0-51) have a specific tag
 *
 * @param tag - Tag to search for
 * @return int - Number of cards with this tag (0-52)
 *
 * Iterates through all 52 cards in g_card_metadata, counting cards with tag.
 * Used by REQUIREMENT_TAG_COUNT checking.
 */
int CountCardsWithTag(CardTag_t tag);

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
// TUTORIAL EVENTS
// ============================================================================

/**
 * CreateSystemMaintenanceEvent - Create "System Maintenance" tutorial event
 *
 * @return EventEncounter_t* - Tutorial event demonstrating locked choices + enemy HP modification
 *
 * Theme: System corruption with choice to accelerate it
 * - Choice A: Investigate panel (-10 sanity, trinket TODO)
 * - Choice B: Walk away (+20 chips, 3 random cards CURSED)
 * - Choice C [Requires 1 CURSED card]: Sabotage (Daemon starts 75% HP, -20 sanity)
 *
 * Tutorial Purpose:
 * - Teaches locked choice mechanic (need CURSED cards)
 * - Demonstrates enemy HP modification
 * - Shows consequence of ignoring corruption (Choice B curses deck, unlocking Choice C)
 */
EventEncounter_t* CreateSystemMaintenanceEvent(void);

/**
 * CreateHouseOddsEvent - Create "House Odds" tutorial event
 *
 * @return EventEncounter_t* - Tutorial event demonstrating tag synergies + conditional unlock
 *
 * Theme: Casino elite membership with risky upgrade path
 * - Choice A: Accept upgrade (Daemon +50% HP, trinket TODO, rewards doubled TODO)
 * - Choice B: Refuse (-15 sanity, all Aces → LUCKY)
 * - Choice C [Requires 1 LUCKY card]: Negotiate (Normal HP, +30 chips, face cards → BRUTAL)
 *
 * Tutorial Purpose:
 * - Teaches tag targeting strategies (RANK_ACES, RANK_FACE_CARDS)
 * - Demonstrates enemy HP scaling (Choice A makes Daemon harder)
 * - Shows tag synergy (Choice B grants LUCKY tags, unlocking Choice C)
 */
EventEncounter_t* CreateHouseOddsEvent(void);

// ============================================================================
// EVENT REGISTRY (Single Source of Truth)
// ============================================================================

/**
 * EventFactory_t - Function pointer for event factory functions
 *
 * @return EventEncounter_t* - Newly created event
 */
typedef EventEncounter_t* (*EventFactory_t)(void);

/**
 * EventRegistryEntry_t - Registry entry for an event
 *
 * Maps terminal command names to event factory functions.
 * Provides single source of truth for event name -> factory mapping.
 */
typedef struct EventRegistryEntry {
    const char* command_name;   // Terminal command argument (e.g., "maintenance")
    const char* display_name;   // Human-readable name (e.g., "System Maintenance")
    EventFactory_t factory;     // Factory function to create event
} EventRegistryEntry_t;

/**
 * GetEventRegistry - Get global event registry
 *
 * @param out_count - Output: Number of entries in registry
 * @return const EventRegistryEntry_t* - Array of registry entries (static, don't free)
 *
 * Single source of truth for event name -> factory mapping.
 * Used by terminal autocomplete and command execution.
 */
const EventRegistryEntry_t* GetEventRegistry(int* out_count);

#endif // EVENT_H
