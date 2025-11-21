/*
 * Event System Implementation
 * Constitutional pattern: EventChoice_t as value types in dArray_t (per ADR-006)
 */

#include "event.h"
#include "player.h"
#include "deck.h"
#include "random.h"
#include "trinket.h"  // For trinket reward system

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
    if (choice->tag_target_strategies) {
        d_DestroyArray(choice->tag_target_strategies);
        choice->tag_target_strategies = NULL;
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
    choice.tag_target_strategies = d_InitArray(2, sizeof(TagTargetStrategy_t));
    choice.removed_tags = d_InitArray(2, sizeof(CardTag_t));

    // Initialize requirement to NONE (no requirement by default)
    choice.requirement.type = REQUIREMENT_NONE;

    // Initialize enemy modifiers to defaults
    choice.enemy_hp_multiplier = 1.0f;  // Normal HP (no modification)

    // Initialize trinket reward to none
    choice.trinket_reward_id = -1;  // -1 = no trinket reward

    if (!choice.text || !choice.result_text || !choice.granted_tags ||
        !choice.tag_target_strategies || !choice.removed_tags) {
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
    // Backward compatible: uses TAG_TARGET_RANDOM_CARD strategy
    AddCardTagToChoiceWithStrategy(event, choice_index, tag, TAG_TARGET_RANDOM_CARD);
}

void AddCardTagToChoiceWithStrategy(EventEncounter_t* event, int choice_index,
                                     CardTag_t tag, TagTargetStrategy_t strategy) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("AddCardTagToChoiceWithStrategy: Invalid event or choice_index");
        return;
    }

    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
    d_AppendDataToArray(choice->granted_tags, &tag);
    d_AppendDataToArray(choice->tag_target_strategies, &strategy);

    d_LogInfoF("Choice %d: Added tag grant %s (strategy: %d)", choice_index, GetCardTagName(tag), strategy);
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

void SetChoiceRequirement(EventEncounter_t* event, int choice_index, ChoiceRequirement_t requirement) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("SetChoiceRequirement: Invalid event or choice_index");
        return;
    }

    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
    choice->requirement = requirement;

    d_LogInfoF("Choice %d: Set requirement (type: %d)", choice_index, requirement.type);
}

void SetChoiceTrinketReward(EventEncounter_t* event, int choice_index, int trinket_id) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("SetChoiceTrinketReward: Invalid event or choice_index");
        return;
    }

    if (trinket_id < 0) {
        d_LogWarning("SetChoiceTrinketReward: Negative trinket_id (use -1 for no reward)");
    }

    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
    choice->trinket_reward_id = trinket_id;

    d_LogInfoF("Choice %d: Set trinket reward (ID: %d)", choice_index, trinket_id);
}

void SetChoiceEnemyHPMultiplier(EventEncounter_t* event, int choice_index, float multiplier) {
    if (!event || choice_index < 0 || (size_t)choice_index >= event->choices->count) {
        d_LogError("SetChoiceEnemyHPMultiplier: Invalid event or choice_index");
        return;
    }

    if (multiplier < 0.0f) {
        d_LogWarning("SetChoiceEnemyHPMultiplier: Negative multiplier clamped to 0");
        multiplier = 0.0f;
    }

    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(event->choices, choice_index);
    choice->enemy_hp_multiplier = multiplier;

    d_LogInfoF("Choice %d: Set enemy HP multiplier to %.2f", choice_index, multiplier);
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

    // Apply granted tags (using targeting strategies)
    for (size_t i = 0; i < choice->granted_tags->count; i++) {
        CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray(choice->granted_tags, i);
        TagTargetStrategy_t* strategy = (TagTargetStrategy_t*)d_IndexDataFromArray(choice->tag_target_strategies, i);

        if (!tag || !strategy) {
            d_LogErrorF("ApplyEventConsequences: Missing tag or strategy at index %zu", i);
            continue;
        }

        switch (*strategy) {
            case TAG_TARGET_RANDOM_CARD: {
                // Random card from full 52-card deck
                int random_card_id = GetRandomInt(0, 51);
                AddCardTag(random_card_id, *tag);
                d_LogInfoF("Granted tag %s to card %d (RANDOM)", GetCardTagName(*tag), random_card_id);
                break;
            }

            case TAG_TARGET_HIGHEST_UNTAGGED: {
                // Highest rank card that doesn't already have this tag
                int highest_card_id = -1;
                CardRank_t highest_rank = RANK_ACE;  // Start at lowest
                for (int card_id = 0; card_id < 52; card_id++) {
                    if (!HasCardTag(card_id, *tag)) {  // Skip cards already tagged
                        CardRank_t rank = (card_id % 13) + 1;  // Calculate rank from card_id
                        if (highest_card_id < 0 || rank > highest_rank) {
                            highest_rank = rank;
                            highest_card_id = card_id;
                        }
                    }
                }
                if (highest_card_id >= 0) {
                    AddCardTag(highest_card_id, *tag);
                    d_LogInfoF("Granted tag %s to card %d (HIGHEST_UNTAGGED)", GetCardTagName(*tag), highest_card_id);
                } else {
                    d_LogInfo("HIGHEST_UNTAGGED: All cards already have this tag");
                }
                break;
            }

            case TAG_TARGET_LOWEST_UNTAGGED: {
                // Lowest rank card that doesn't already have this tag
                int lowest_card_id = -1;
                CardRank_t lowest_rank = RANK_KING;  // Start at highest
                for (int card_id = 0; card_id < 52; card_id++) {
                    if (!HasCardTag(card_id, *tag)) {  // Skip cards already tagged
                        CardRank_t rank = (card_id % 13) + 1;  // Calculate rank from card_id
                        if (lowest_card_id < 0 || rank < lowest_rank) {
                            lowest_rank = rank;
                            lowest_card_id = card_id;
                        }
                    }
                }
                if (lowest_card_id >= 0) {
                    AddCardTag(lowest_card_id, *tag);
                    d_LogInfoF("Granted tag %s to card %d (LOWEST_UNTAGGED)", GetCardTagName(*tag), lowest_card_id);
                } else {
                    d_LogInfo("LOWEST_UNTAGGED: All cards already have this tag");
                }
                break;
            }

            case TAG_TARGET_SUIT_HEARTS: {
                // All 13 hearts (card_id 0-12)
                int count = 0;
                for (int card_id = 0; card_id < 13; card_id++) {
                    AddCardTag(card_id, *tag);
                    count++;
                }
                d_LogInfoF("Granted tag %s to %d hearts (SUIT_HEARTS)", GetCardTagName(*tag), count);
                break;
            }

            case TAG_TARGET_SUIT_DIAMONDS: {
                // All 13 diamonds (card_id 13-25)
                int count = 0;
                for (int card_id = 13; card_id < 26; card_id++) {
                    AddCardTag(card_id, *tag);
                    count++;
                }
                d_LogInfoF("Granted tag %s to %d diamonds (SUIT_DIAMONDS)", GetCardTagName(*tag), count);
                break;
            }

            case TAG_TARGET_SUIT_CLUBS: {
                // All 13 clubs (card_id 26-38)
                int count = 0;
                for (int card_id = 26; card_id < 39; card_id++) {
                    AddCardTag(card_id, *tag);
                    count++;
                }
                d_LogInfoF("Granted tag %s to %d clubs (SUIT_CLUBS)", GetCardTagName(*tag), count);
                break;
            }

            case TAG_TARGET_SUIT_SPADES: {
                // All 13 spades (card_id 39-51)
                int count = 0;
                for (int card_id = 39; card_id < 52; card_id++) {
                    AddCardTag(card_id, *tag);
                    count++;
                }
                d_LogInfoF("Granted tag %s to %d spades (SUIT_SPADES)", GetCardTagName(*tag), count);
                break;
            }

            case TAG_TARGET_RANK_ACES: {
                // All 4 aces (one per suit)
                int count = 0;
                for (int card_id = 0; card_id < 52; card_id++) {
                    CardRank_t rank = (card_id % 13) + 1;  // Calculate rank from card_id
                    if (rank == RANK_ACE) {
                        AddCardTag(card_id, *tag);
                        count++;
                    }
                }
                d_LogInfoF("Granted tag %s to %d aces (RANK_ACES)", GetCardTagName(*tag), count);
                break;
            }

            case TAG_TARGET_RANK_FACE_CARDS: {
                // All face cards (J, Q, K = 12 cards total)
                int count = 0;
                for (int card_id = 0; card_id < 52; card_id++) {
                    CardRank_t rank = (card_id % 13) + 1;  // Calculate rank from card_id
                    if (rank == RANK_JACK || rank == RANK_QUEEN || rank == RANK_KING) {
                        AddCardTag(card_id, *tag);
                        count++;
                    }
                }
                d_LogInfoF("Granted tag %s to %d face cards (RANK_FACE_CARDS)", GetCardTagName(*tag), count);
                break;
            }

            case TAG_TARGET_ALL_CARDS: {
                // All 52 cards
                for (int card_id = 0; card_id < 52; card_id++) {
                    AddCardTag(card_id, *tag);
                }
                d_LogInfoF("Granted tag %s to all 52 cards (ALL_CARDS)", GetCardTagName(*tag));
                break;
            }

            default:
                d_LogErrorF("ApplyEventConsequences: Unknown strategy %d", *strategy);
                break;
        }
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

    // Apply trinket reward
    if (choice->trinket_reward_id >= 0) {
        Trinket_t* trinket = GetTrinketByID(choice->trinket_reward_id);
        if (trinket) {
            int empty_slot = GetEmptyTrinketSlot(player);
            if (empty_slot >= 0) {
                EquipTrinket(player, empty_slot, trinket);
                d_LogInfoF("Granted trinket: %s (slot %d)", GetTrinketName(trinket), empty_slot);
            } else {
                d_LogWarning("No empty trinket slot - trinket reward lost!");
                // TODO: Show modal to player: "Inventory full - trinket lost"
            }
        } else {
            d_LogErrorF("Trinket reward ID %d not found in registry", choice->trinket_reward_id);
        }
    }

    d_LogInfo("Event consequences applied");
}

// ============================================================================
// REQUIREMENT SYSTEM
// ============================================================================

int CountCardsWithTag(CardTag_t tag) {
    int count = 0;

    // Iterate through all 52 cards (0-51)
    for (int card_id = 0; card_id < 52; card_id++) {
        if (HasCardTag(card_id, tag)) {
            count++;
        }
    }

    return count;
}

bool IsChoiceRequirementMet(const ChoiceRequirement_t* req, const Player_t* player) {
    if (!req) {
        d_LogError("IsChoiceRequirementMet: NULL requirement");
        return true;  // Fail-safe: allow choice if requirement is NULL
    }

    switch (req->type) {
        case REQUIREMENT_NONE:
            // No requirement, always met
            return true;

        case REQUIREMENT_TAG_COUNT: {
            // Count cards with specific tag in full 52-card deck
            int count = CountCardsWithTag(req->data.tag_req.required_tag);
            bool met = count >= req->data.tag_req.min_count;
            d_LogDebugF("TAG_COUNT requirement: %d/%d cards with %s (met: %d)",
                       count, req->data.tag_req.min_count,
                       GetCardTagName(req->data.tag_req.required_tag), met);
            return met;
        }

        case REQUIREMENT_TRINKET:
            // Trinket system not yet implemented
            d_LogWarning("TRINKET requirement check: Trinket system not implemented yet");
            return false;  // Locked until trinket system exists

        case REQUIREMENT_HP_THRESHOLD: {
            if (!player) {
                d_LogError("HP_THRESHOLD requirement: NULL player");
                return false;
            }
            // Note: Player_t doesn't have HP field yet, using sanity as placeholder
            // TODO: Update when HP system is implemented
            d_LogWarning("HP_THRESHOLD requirement: Player HP system not implemented yet");
            return false;
        }

        case REQUIREMENT_SANITY_THRESHOLD: {
            if (!player) {
                d_LogError("SANITY_THRESHOLD requirement: NULL player");
                return false;
            }
            bool met = player->sanity >= req->data.sanity_req.threshold;
            d_LogDebugF("SANITY_THRESHOLD requirement: %d >= %d (met: %d)",
                       player->sanity, req->data.sanity_req.threshold, met);
            return met;
        }

        case REQUIREMENT_CHIPS_THRESHOLD: {
            if (!player) {
                d_LogError("CHIPS_THRESHOLD requirement: NULL player");
                return false;
            }
            bool met = player->chips >= req->data.chips_req.threshold;
            d_LogDebugF("CHIPS_THRESHOLD requirement: %d >= %d (met: %d)",
                       player->chips, req->data.chips_req.threshold, met);
            return met;
        }

        default:
            d_LogErrorF("IsChoiceRequirementMet: Unknown requirement type %d", req->type);
            return false;
    }
}

void GetRequirementTooltip(const ChoiceRequirement_t* req, dString_t* out) {
    if (!req || !out) {
        d_LogError("GetRequirementTooltip: NULL parameter");
        return;
    }

    switch (req->type) {
        case REQUIREMENT_NONE:
            // No requirement, no tooltip needed
            d_StringSet(out, "", 0);
            break;

        case REQUIREMENT_TAG_COUNT: {
            int current_count = CountCardsWithTag(req->data.tag_req.required_tag);
            d_StringFormat(out, "Requires at least %d %s cards (%d/%d)",
                          req->data.tag_req.min_count,
                          GetCardTagName(req->data.tag_req.required_tag),
                          current_count,
                          req->data.tag_req.min_count);
            break;
        }

        case REQUIREMENT_TRINKET:
            d_StringFormat(out, "Requires specific trinket (ID: %d)",
                          req->data.trinket_req.required_trinket_id);
            break;

        case REQUIREMENT_HP_THRESHOLD:
            d_StringFormat(out, "Requires HP >= %d",
                          req->data.hp_req.threshold);
            break;

        case REQUIREMENT_SANITY_THRESHOLD:
            d_StringFormat(out, "Requires sanity >= %d",
                          req->data.sanity_req.threshold);
            break;

        case REQUIREMENT_CHIPS_THRESHOLD:
            d_StringFormat(out, "Requires at least %d chips",
                          req->data.chips_req.threshold);
            break;

        default:
            d_StringFormat(out, "Unknown requirement type %d", req->type);
            break;
    }
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
// TUTORIAL EVENTS
// ============================================================================

/**
 * CreateSystemMaintenanceEvent - "System Maintenance" event
 *
 * Theme: System corruption with choice to accelerate/sabotage
 * Demonstrates locked choice requiring CURSED cards + enemy HP modification
 */
EventEncounter_t* CreateSystemMaintenanceEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "System Maintenance",
        "The Didact's body sits motionless at the table.\n\n"
        "A maintenance panel opens in the wall. You hear mechanical sounds—grinding, clicking, rebuilding.",
        EVENT_TYPE_CHOICE
    );

    if (!event) {
        d_LogError("Failed to create SystemMaintenanceEvent");
        return NULL;
    }

    // Choice A: Investigate panel (-10 sanity, Stack Trace trinket)
    AddEventChoice(event,
                   "Investigate the panel",
                   "> DIAGNOSTIC_MODE_ACTIVE\n\n"
                   "You dig through cascading error logs. Corruption spreads through "
                   "every subroutine—the machine is dying, and it knows it.\n\n"
                   "A diagnostic tool ejects from the wreckage. Every crash leaves "
                   "a trace. Now you can read them.",
                   0,    // No chips
                   -10); // -10 sanity
    SetChoiceTrinketReward(event, 0, 2);  // Grant Stack Trace (trinket_id = 2)

    // Choice B: Walk away (+20 chips, 3 random cards CURSED)
    AddEventChoice(event,
                   "Walk away",
                   "> REBUILD_CANCELLED\n\n"
                   "You pocket loose chips scattered near the panel. The maintenance "
                   "cycle aborts. Something in the static reaches out—three cards in "
                   "your deck pulse with wrongness.\n\n"
                   "The corruption has spread to you.",
                   20,  // +20 chips
                   0);  // No sanity change
    // Tag 3 random cards as CURSED
    AddCardTagToChoiceWithStrategy(event, 1, CARD_TAG_CURSED, TAG_TARGET_RANDOM_CARD);
    AddCardTagToChoiceWithStrategy(event, 1, CARD_TAG_CURSED, TAG_TARGET_RANDOM_CARD);
    AddCardTagToChoiceWithStrategy(event, 1, CARD_TAG_CURSED, TAG_TARGET_RANDOM_CARD);

    // Choice C: Sabotage maintenance [REQUIRES 1 CURSED CARD]
    AddEventChoice(event,
                   "Sabotage the maintenance",
                   "> MANUAL_OVERRIDE_ACCEPTED\n\n"
                   "Your cursed cards interface directly with the corrupted system. "
                   "Feedback screeches through your skull as you accelerate the decay.\n\n"
                   "The Daemon will boot incomplete. Weakened. Your mind pays the price.",
                   0,    // No chips
                   -20); // -20 sanity
    // Set requirement: needs at least 1 CURSED card
    ChoiceRequirement_t req = {.type = REQUIREMENT_TAG_COUNT};
    req.data.tag_req.required_tag = CARD_TAG_CURSED;
    req.data.tag_req.min_count = 1;
    SetChoiceRequirement(event, 2, req);
    // Set enemy HP multiplier: Daemon starts at 75% HP
    SetChoiceEnemyHPMultiplier(event, 2, 0.75f);

    d_LogInfo("Created SystemMaintenanceEvent");
    return event;
}

/**
 * CreateHouseOddsEvent - "House Odds" event
 *
 * Theme: Casino elite membership with tag-based conditional unlock
 * Demonstrates multiple strategies and enemy HP scaling
 */
EventEncounter_t* CreateHouseOddsEvent(void) {
    EventEncounter_t* event = CreateEvent(
        "House Odds",
        "A screen flickers to life above the table.\n\n"
        "CONGRATULATIONS - YOU'VE BEEN UPGRADED TO ELITE STATUS.\n\n"
        "Your next opponent has been selected for maximum engagement.",
        EVENT_TYPE_CHOICE
    );

    if (!event) {
        d_LogError("Failed to create HouseOddsEvent");
        return NULL;
    }

    // Choice A: Accept upgrade (Daemon +50% HP, Elite Membership trinket)
    AddEventChoice(event,
                   "Accept the upgrade",
                   "\"Excellent choice.\"\n\n"
                   "The dealer slides a black card across the table. Your fingers tingle "
                   "as you accept it. The card hums with probability manipulation.\n\n"
                   "Elite Status granted. The casino's algorithms now favor you... "
                   "but the Daemon has been enhanced to match.",
                   0,  // No chips (rewards come later via trinket)
                   0); // No sanity change
    // Grant Elite Membership trinket (ID 1)
    SetChoiceTrinketReward(event, 0, 1);
    // Set enemy HP multiplier: Daemon starts at 130% HP
    SetChoiceEnemyHPMultiplier(event, 0, 1.3f);

    // Choice B: Refuse (-15 sanity, all Aces → LUCKY)
    AddEventChoice(event,
                   "Refuse the upgrade",
                   "\"Disappointing.\"\n\n"
                   "The dealer's smile freezes. Reality hiccups. When the world stabilizes, "
                   "your Aces shimmer with stolen luck. The House's punishment backfired.\n\n"
                   "You feel the sanity drain, but your deck pulses with new power.",
                   0,    // No chips
                   -15); // -15 sanity
    // Tag all 4 Aces as LUCKY
    AddCardTagToChoiceWithStrategy(event, 1, CARD_TAG_LUCKY, TAG_TARGET_RANK_ACES);

    // Choice C: Negotiate terms [REQUIRES 1 LUCKY CARD]
    AddEventChoice(event,
                   "Negotiate terms",
                   "\"Ah, you have luck on your side.\" The dealer inclines their head.\n\n"
                   "Your lucky card catches the light as leverage. The terms shift in your favor. "
                   "Face cards darken with brutal intent.\n\n"
                   "The deal is struck. The Daemon awaits at standard strength.",
                   30,  // +30 chips
                   0);  // No sanity change
    // Set requirement: needs at least 1 LUCKY card
    ChoiceRequirement_t req2 = {.type = REQUIREMENT_TAG_COUNT};
    req2.data.tag_req.required_tag = CARD_TAG_LUCKY;
    req2.data.tag_req.min_count = 1;
    SetChoiceRequirement(event, 2, req2);
    // Tag all face cards (J, Q, K = 12 cards) as BRUTAL
    AddCardTagToChoiceWithStrategy(event, 2, CARD_TAG_BRUTAL, TAG_TARGET_RANK_FACE_CARDS);
    // No HP multiplier (normal Daemon HP)

    d_LogInfo("Created HouseOddsEvent");
    return event;
}
