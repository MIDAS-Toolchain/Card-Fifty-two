/*
 * Event System Implementation
 * Constitutional pattern: EventChoice_t as value types in dArray_t (per ADR-006)
 */

#include "event.h"
#include "player.h"
#include "deck.h"
#include "random.h"

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * DestroyEventChoice - Free internal resources of a choice
 *
 * @param choice - Choice to clean up (value type, not pointer)
 *
 * Called before removing choice from array or destroying array.
 */
static void DestroyEventChoice(EventChoice_t* choice) {
    if (!choice) return;

    if (choice->text) {
        d_StringDestroy(choice->text);
        choice->text = NULL;
    }
    if (choice->result_text) {
        d_StringDestroy(choice->result_text);
        choice->result_text = NULL;
    }
    if (choice->granted_tags) {
        d_DestroyArray(choice->granted_tags);
        choice->granted_tags = NULL;
    }
    if (choice->removed_tags) {
        d_DestroyArray(choice->removed_tags);
        choice->removed_tags = NULL;
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

EventEncounter_t* CreateEvent(const char* title, const char* description, EventType_t type) {
    if (!title || !description) {
        d_LogError("CreateEvent: NULL title or description");
        return NULL;
    }

    EventEncounter_t* event = malloc(sizeof(EventEncounter_t));
    if (!event) {
        d_LogFatal("CreateEvent: Failed to allocate EventEncounter_t");
        return NULL;
    }

    event->title = d_StringInit();
    event->description = d_StringInit();
    event->type = type;
    // d_InitArray(capacity, element_size) - capacity FIRST!
    event->choices = d_InitArray(4, sizeof(EventChoice_t));  // Typical event has 2-4 choices
    event->selected_choice = -1;
    event->is_complete = false;

    if (!event->title || !event->description || !event->choices) {
        d_LogFatal("CreateEvent: Failed to initialize internal resources");
        if (event->title) d_StringDestroy(event->title);
        if (event->description) d_StringDestroy(event->description);
        if (event->choices) d_DestroyArray(event->choices);
        free(event);
        return NULL;
    }

    d_StringSet(event->title, title, 0);
    d_StringSet(event->description, description, 0);

    d_LogInfoF("Created event: %s", title);
    return event;
}

void DestroyEvent(EventEncounter_t** event) {
    if (!event || !*event) {
        d_LogError("DestroyEvent: NULL event pointer");
        return;
    }

    EventEncounter_t* evt = *event;

    // Free all choices (value types in array, per ADR-006)
    if (evt->choices) {
        for (size_t i = 0; i < evt->choices->count; i++) {
            EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(evt->choices, i);
            DestroyEventChoice(choice);
        }
        d_DestroyArray(evt->choices);
    }

    // Free event strings
    if (evt->title) {
        d_StringDestroy(evt->title);
    }
    if (evt->description) {
        d_StringDestroy(evt->description);
    }

    free(evt);
    *event = NULL;

    d_LogInfo("Event destroyed");
}

// ============================================================================
// CHOICE MANAGEMENT
// ============================================================================

void AddEventChoice(EventEncounter_t* event, const char* text, const char* result_text,
                    int chips_delta, int sanity_delta) {
    if (!event) {
        d_LogError("AddEventChoice: NULL event");
        return;
    }
    if (!text || !result_text) {
        d_LogError("AddEventChoice: NULL text or result_text");
        return;
    }

    // Initialize choice (value type, per ADR-006)
    EventChoice_t choice = {0};
    choice.text = d_StringInit();
    choice.result_text = d_StringInit();
    choice.chips_delta = chips_delta;
    choice.sanity_delta = sanity_delta;
    // d_InitArray(capacity, element_size) - capacity FIRST!
    choice.granted_tags = d_InitArray(2, sizeof(CardTag_t));
    choice.removed_tags = d_InitArray(2, sizeof(CardTag_t));

    if (!choice.text || !choice.result_text || !choice.granted_tags || !choice.removed_tags) {
        d_LogFatal("AddEventChoice: Failed to initialize choice resources");
        DestroyEventChoice(&choice);
        return;
    }

    d_StringSet(choice.text, text, 0);
    d_StringSet(choice.result_text, result_text, 0);

    // Copy choice into array (value type, per ADR-006)
    d_AppendDataToArray(event->choices, &choice);

    d_LogInfoF("Added choice to event: %s", text);
}

void AddCardTagToChoice(EventEncounter_t* event, int choice_index, CardTag_t tag) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("AddCardTagToChoice: Invalid event or choice_index");
        return;
    }

    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
    d_AppendDataToArray(choice->granted_tags, &tag);

    d_LogInfoF("Choice %d: Added tag grant %s", choice_index, GetCardTagName(tag));
}

void RemoveCardTagFromChoice(EventEncounter_t* event, int choice_index, CardTag_t tag) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("RemoveCardTagFromChoice: Invalid event or choice_index");
        return;
    }

    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
    d_AppendDataToArray(choice->removed_tags, &tag);

    d_LogInfoF("Choice %d: Added tag removal %s", choice_index, GetCardTagName(tag));
}

void SelectEventChoice(EventEncounter_t* event, int choice_index) {
    if (!event) {
        d_LogError("SelectEventChoice: NULL event");
        return;
    }
    if (choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("SelectEventChoice: Invalid choice_index");
        return;
    }

    event->selected_choice = choice_index;
    event->is_complete = true;

    const EventChoice_t* choice = GetEventChoice(event, choice_index);
    d_LogInfoF("Player selected choice: %s", d_StringPeek(choice->text));
}

const EventChoice_t* GetEventChoice(const EventEncounter_t* event, int choice_index) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        return NULL;
    }

    return (const EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
}

int GetChoiceCount(const EventEncounter_t* event) {
    if (!event || !event->choices) {
        return 0;
    }

    return (int)event->choices->count;
}

// ============================================================================
// CONSEQUENCE APPLICATION
// ============================================================================

void ApplyEventConsequences(EventEncounter_t* event, Player_t* player, Deck_t* deck) {
    if (!event || !player || !deck) {
        d_LogError("ApplyEventConsequences: NULL parameter");
        return;
    }

    if (event->selected_choice < 0) {
        d_LogWarning("ApplyEventConsequences: No choice selected");
        return;
    }

    const EventChoice_t* choice = GetEventChoice(event, event->selected_choice);
    if (!choice) {
        d_LogError("ApplyEventConsequences: Invalid selected_choice");
        return;
    }

    // Apply chips delta
    if (choice->chips_delta != 0) {
        player->chips += choice->chips_delta;
        if (player->chips < 0) player->chips = 0;  // Clamp to 0
        d_LogInfoF("Applied chips delta: %+d (new total: %d)",
                   choice->chips_delta, player->chips);
    }

    // Apply sanity delta
    if (choice->sanity_delta != 0) {
        ModifyPlayerSanity(player, choice->sanity_delta);
    }

    // Apply granted tags (to random cards)
    for (size_t i = 0; i < choice->granted_tags->count; i++) {
        CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray(choice->granted_tags, i);

        // Pick random card from deck
        int random_card_id = GetRandomInt(0, 51);
        AddCardTag(random_card_id, *tag);

        d_LogInfoF("Granted tag %s to card %d", GetCardTagName(*tag), random_card_id);
    }

    // Apply removed tags (from all cards)
    for (size_t i = 0; i < choice->removed_tags->count; i++) {
        CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray(choice->removed_tags, i);

        // Remove from all 52 cards
        for (int card_id = 0; card_id < 52; card_id++) {
            if (HasCardTag(card_id, *tag)) {
                RemoveCardTag(card_id, *tag);
            }
        }

        d_LogInfoF("Removed tag %s from all cards", GetCardTagName(*tag));
    }

    d_LogInfo("Event consequences applied");
}

// ============================================================================
// QUERIES
// ============================================================================

void EventToString(const EventEncounter_t* event, dString_t* out) {
    if (!event || !out) {
        d_LogError("EventToString: NULL parameter");
        return;
    }

    d_StringFormat(out, "Event: %s | Type: %s | Choices: %d",
                   d_StringPeek(event->title),
                   GetEventTypeName(event->type),
                   (int)event->choices->count);
}

const char* GetEventTypeName(EventType_t type) {
    switch (type) {
        case EVENT_TYPE_DIALOGUE: return "Dialogue";
        case EVENT_TYPE_CHOICE:   return "Choice";
        case EVENT_TYPE_SHOP:     return "Shop";
        case EVENT_TYPE_REST:     return "Rest";
        case EVENT_TYPE_BLESSING: return "Blessing";
        case EVENT_TYPE_CURSE:    return "Curse";
        default:                  return "Unknown";
    }
}

// ============================================================================
// PRESET EVENTS
// ============================================================================

EventEncounter_t* CreateGamblersBargainEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "The Gambler's Bargain",
        "A shadowy figure approaches with a deck of cards.\n"
        "\"I offer you a choice, friend. Fortune favors the bold...\"",
        EVENT_TYPE_CHOICE
    );

    if (!event) return NULL;

    // Option 1: High risk, high reward
    AddEventChoice(event,
                   "Accept the cursed chips (+20 chips, CURSED tag)",
                   "You take the chips. They feel cold in your hand.\n"
                   "One of your cards begins to darken...",
                   20,   // +20 chips
                   -5);  // -5 sanity
    AddCardTagToChoice(event, 0, CARD_TAG_CURSED);

    // Option 2: Safe blessing
    AddEventChoice(event,
                   "Pay for vampiric power (-10 chips, VAMPIRIC tag)",
                   "You hand over the chips. Dark energy seeps into your deck.\n"
                   "One card pulses with crimson light - it hungers.",
                   -10,  // -10 chips
                   5);   // +5 sanity
    AddCardTagToChoice(event, 1, CARD_TAG_VAMPIRIC);

    // Option 3: Decline
    AddEventChoice(event,
                   "Walk away (no change)",
                   "The figure vanishes into the shadows.\n"
                   "You continue on your path, unchanged.",
                   0,    // No chips
                   0);   // No sanity

    return event;
}

EventEncounter_t* CreateRiggedDeckEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "The Rigged Deck",
        "You discover a dealer's hidden stash of marked cards.\n"
        "The deck pulses with unnatural energy...",
        EVENT_TYPE_CHOICE
    );

    if (!event) return NULL;

    // Option 1: Cleanse curses
    AddEventChoice(event,
                   "Cleanse the corruption (-15 sanity, remove CURSED)",
                   "You focus your will, burning away the dark energy.\n"
                   "The effort leaves you drained, but your deck is pure.",
                   0,     // No chips
                   -15);  // -15 sanity
    RemoveCardTagFromChoice(event, 0, CARD_TAG_CURSED);

    // Option 2: Steal lucky cards
    AddEventChoice(event,
                   "Steal the best cards (-30 chips, add LUCKY to 3 cards)",
                   "You pocket the dealer's favorites. They'll notice eventually...\n"
                   "Three of your cards now carry their luck.",
                   -30,   // -30 chips (bribe cost)
                   0);    // No sanity change
    // Add 3 lucky tags (same tag added 3 times = 3 random cards)
    AddCardTagToChoice(event, 1, CARD_TAG_LUCKY);
    AddCardTagToChoice(event, 1, CARD_TAG_LUCKY);
    AddCardTagToChoice(event, 1, CARD_TAG_LUCKY);

    // Option 3: Leave it
    AddEventChoice(event,
                   "Leave it untouched (no change)",
                   "You back away from the corrupted deck.\n"
                   "Some mysteries are best left unsolved.",
                   0,     // No chips
                   0);    // No sanity

    return event;
}

EventEncounter_t* CreateBrokenSlotMachineEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "The Broken Slot Machine",
        "An ancient slot machine sits in the corner, sparking erratically.\n"
        "The display reads: \"MALFUNCTION - VOLATILE PAYOUT\"",
        EVENT_TYPE_CHOICE
    );

    if (!event) return NULL;

    // Option 1: Pull lever (volatile outcome)
    int random_chips = GetRandomInt(-50, 50);
    dString_t* pull_text = d_StringInit();
    d_StringFormat(pull_text, "Pull the lever (volatile: %+d chips this time)", random_chips);

    dString_t* pull_result = d_StringInit();
    if (random_chips > 0) {
        d_StringFormat(pull_result,
                       "The machine whirrs and dings! Chips pour out!\n"
                       "You gain %d chips from the jackpot.", random_chips);
    } else {
        d_StringFormat(pull_result,
                       "The machine sparks and smokes! It devours your chips!\n"
                       "You lose %d chips to the broken machine.", -random_chips);
    }

    AddEventChoice(event,
                   d_StringPeek(pull_text),
                   d_StringPeek(pull_result),
                   random_chips,
                   0);
    d_StringDestroy(pull_text);
    d_StringDestroy(pull_result);

    // Option 2: Fix machine
    AddEventChoice(event,
                   "Try to fix it (-20 chips, add BRUTAL tag)",
                   "You tinker with the machine's internals.\n"
                   "One of your cards begins to glow with violent orange energy!",
                   -20,   // Repair cost
                   0);
    AddCardTagToChoice(event, 1, CARD_TAG_BRUTAL);

    // Option 3: Ignore
    AddEventChoice(event,
                   "Walk past it (no change)",
                   "You give the machine a wide berth.\n"
                   "It continues sparking ominously as you leave.",
                   0,
                   0);

    return event;
}

EventEncounter_t* CreateDataBrokerEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "The Data Broker's Offer",
        "A terminal flickers to life in the corner.\n\n"
        "TEXT OUTPUT: \"TRANSACTION AVAILABLE\"\n"
        "TEXT OUTPUT: \"RESOURCES DETECTED: MENTAL_STABILITY\"\n"
        "TEXT OUTPUT: \"EXCHANGE_RATE: FAVORABLE\"\n\n"
        "The screen displays four options.",
        EVENT_TYPE_CHOICE
    );

    if (!event) return NULL;

    // Option 1: Decline (safe path - useless trinket placeholder)
    AddEventChoice(event,
                   "Decline transaction (no change)",
                   "TEXT OUTPUT: \"TRANSACTION_DECLINED\"\n"
                   "TEXT OUTPUT: \"EFFICIENCY_SUB-OPTIMAL\"\n\n"
                   "The terminal goes dark.\n\n"
                   "You walk away unchanged.\n"
                   "The safe choice. The boring choice.",
                   0,    // +0 chips
                   0);   // +0 sanity
    // TODO: Add useless trinket when trinket system implemented

    // Option 2: Minor exchange (moderate risk)
    AddEventChoice(event,
                   "Accept minor exchange (+100 chips, -50 sanity)",
                   "TEXT OUTPUT: \"PROCESSING_TRANSFER\"\n"
                   "TEXT OUTPUT: \"MENTAL_STABILITY: 50% REDUCTION\"\n"
                   "TEXT OUTPUT: \"CURRENCY_ACQUIRED: 100\"\n\n"
                   "You feel your grip on reality loosening.\n"
                   "The chips feel heavier than they should.\n\n"
                   "[SANITY: 50%]\n"
                   "[WARNING: BETTING_RESTRICTIONS_ACTIVE]",
                   100,  // +100 chips
                   -50); // -50 sanity (triggers 75% and 50% thresholds)

    // Option 3: Major exchange (high risk - useful trinket placeholder)
    AddEventChoice(event,
                   "Accept major exchange (useful trinket, -75 sanity)",
                   "TEXT OUTPUT: \"PROCESSING_TRANSFER\"\n"
                   "TEXT OUTPUT: \"MENTAL_STABILITY: 75% REDUCTION\"\n"
                   "TEXT OUTPUT: \"ASSET_ACQUIRED: [TRINKET_DATA]\"\n\n"
                   "Your vision blurs at the edges.\n"
                   "The world feels less... solid.\n"
                   "The trinket pulses in your hand - is it real?\n\n"
                   "[SANITY: 25%]\n"
                   "[WARNING: CRITICAL_BETTING_RESTRICTIONS]",
                   0,    // +0 chips
                   -75); // -75 sanity (triggers all thresholds except 0%)
    // TODO: Add useful trinket when trinket system implemented

    // Option 4: Total exchange (maximum risk)
    AddEventChoice(event,
                   "Accept total exchange (+200 chips + trinket, -100 sanity)",
                   "TEXT OUTPUT: \"PROCESSING_TRANSFER\"\n"
                   "TEXT OUTPUT: \"MENTAL_STABILITY: CRITICAL_FAILURE\"\n"
                   "TEXT OUTPUT: \"CURRENCY_ACQUIRED: 200\"\n"
                   "TEXT OUTPUT: \"ASSET_ACQUIRED: [TRINKET_DATA]\"\n\n"
                   "Everything feels distant. Unreal.\n"
                   "The numbers on the chips seem to shift.\n"
                   "Are they even real anymore?\n\n"
                   "[SANITY: 0%]\n"
                   "[WARNING: MAXIMUM_BETTING_RESTRICTIONS_ACTIVE]",
                   200,  // +200 chips
                   -100); // -100 sanity (complete depletion → forced max bets)
    // TODO: Add useful trinket when trinket system implemented

    return event;
}

// ============================================================================
// TUTORIAL EVENTS (for EventPool system)
// ============================================================================

/**
 * CreateSanityTradeEvent - Tutorial event teaching sanity mechanic
 *
 * @return EventEncounter_t* - Event with 3 choices (safe/gain/trade)
 *
 * Theme: Risk/reward with sanity as resource
 * Design: Simple 3-choice structure for tutorial clarity
 */
EventEncounter_t* CreateSanityTradeEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "The Gambler's Dilemma",
        "A mysterious figure at the table offers you a deal. "
        "\"Gain resources now... but at what cost to your mind?\"\n\n"
        "They slide chips and a strange device across the felt.",
        EVENT_TYPE_CHOICE
    );

    if (!event) {
        d_LogError("Failed to create SanityTradeEvent");
        return NULL;
    }

    // Choice 1: Walk away (safe - no risk, no reward)
    AddEventChoice(event,
                   "Walk away (no change)",
                   "You shake your head and decline the offer.\n"
                   "The figure shrugs and vanishes into the shadows.\n\n"
                   "Sometimes the safest bet is no bet at all.",
                   0,    // +0 chips
                   0);   // +0 sanity

    // Choice 2: Accept the deal (+100 chips, -100 sanity)
    AddEventChoice(event,
                   "Accept the deal (+100 chips, -100 sanity)",
                   "The device hums. Your vision blurs momentarily.\n"
                   "The chips feel heavy in your pocket.\n\n"
                   "\"A fair exchange,\" the figure whispers before fading away.\n\n"
                   "[WARNING: Sanity depleted - betting restrictions active!]",
                   100,  // +100 chips
                   -100); // -100 sanity (complete depletion)

    // Choice 3: Counter-offer (-50 chips, +50 sanity)
    AddEventChoice(event,
                   "Counter-offer (-50 chips, +50 sanity)",
                   "You slide chips across the table.\n"
                   "The figure's eyes widen, then they laugh.\n\n"
                   "\"Buying clarity with coin? How... pragmatic.\"\n\n"
                   "The world feels sharper. More real.",
                   -50,  // -50 chips (you pay them)
                   50);  // +50 sanity (you restore mental state)

    d_LogInfo("Created SanityTradeEvent (tutorial)");
    return event;
}

/**
 * CreateChipGambleEvent - Tutorial event with simple chip outcomes
 *
 * @return EventEncounter_t* - Event with 3 choices (safe/risky/balanced)
 *
 * Theme: Classic risk/reward without sanity complexity
 * Design: Pure chips for simplicity (alternate tutorial path)
 */
EventEncounter_t* CreateChipGambleEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "Victory Spoils",
        "You search through the defeated enemy's belongings.\n\n"
        "Among scattered cards and chips, you find a locked box.\n"
        "Three keys lie beside it, each with a different symbol.",
        EVENT_TYPE_CHOICE
    );

    if (!event) {
        d_LogError("Failed to create ChipGambleEvent");
        return NULL;
    }

    // Choice 1: Conservative choice (safe +15 chips)
    AddEventChoice(event,
                   "Take the marked chips (+15 chips)",
                   "You pocket the chips without touching the box.\n"
                   "Sometimes the safest loot is the visible loot.\n\n"
                   "The box remains locked.",
                   15,  // +15 chips
                   0);  // No sanity change

    // Choice 2: Risky choice (+25 chips, no sanity)
    AddEventChoice(event,
                   "Try the golden key (+25 chips, risky)",
                   "*CLICK* The box opens.\n\n"
                   "Inside: a pile of high-value chips.\n"
                   "But something feels... off about them.\n\n"
                   "You take them anyway. What could go wrong?",
                   25,  // +25 chips
                   0);  // No sanity change

    // Choice 3: Leave empty-handed (no gain, no loss)
    AddEventChoice(event,
                   "Leave everything (no change)",
                   "You walk away from the box and the chips.\n\n"
                   "Sometimes greed is a bigger gamble than you can afford.",
                   0,  // No chips
                   0); // No sanity change

    d_LogInfo("Created ChipGambleEvent (tutorial)");
    return event;
}
