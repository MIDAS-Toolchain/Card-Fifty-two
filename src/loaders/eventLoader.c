/*
 * Event Loader - Parse events from DUF files
 * Constitutional pattern: On-demand loading with startup validation (ADR-019)
 */

#include "../../include/loaders/eventLoader.h"
#include "../../include/cardTags.h"  // For CardTagFromString
#include <string.h>

// ============================================================================
// GLOBAL REGISTRY
// ============================================================================

dDUFValue_t* g_events_db = NULL;

// ============================================================================
// FORWARD DECLARATIONS (internal helpers)
// ============================================================================

static bool ParseEventChoice(EventEncounter_t* event, dDUFValue_t* choice_node,
                            int choice_idx, const char* event_key);
static bool ParseRequirement(dDUFValue_t* req_node, EventEncounter_t* event,
                            int choice_idx, const char* event_key);

// ============================================================================
// LOAD DATABASE
// ============================================================================

dDUFError_t* LoadEventDatabase(const char* filepath, dDUFValue_t** out_db) {
    if (!filepath || !out_db) {
        d_LogError("LoadEventDatabase: NULL parameters");
        return NULL;
    }

    dDUFError_t* err = d_DUFParseFile(filepath, out_db);
    if (err) {
        return err;  // Caller handles error
    }

    d_LogInfoF("Loaded event database from %s", filepath);
    return NULL;
}

// ============================================================================
// VALIDATE DATABASE
// ============================================================================

bool ValidateEventDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size) {
    if (!db || !out_error_msg) {
        return false;
    }

    // Iterate through all event entries
    dDUFValue_t* entry = db->child;
    int validated_count = 0;

    while (entry) {
        const char* key = entry->string;
        if (!key) {
            entry = entry->next;
            continue;
        }

        // Try to parse this event (validation test)
        EventEncounter_t* test = LoadEventFromDUF(key);
        if (!test) {
            // Validation failed - write error message
            snprintf(out_error_msg, error_msg_size,
                    "Event DUF Validation Failed\n\n"
                    "Event: %s\n"
                    "File: data/events/tutorial_events.duf\n\n"
                    "Check console logs for details.\n\n"
                    "Common issues:\n"
                    "- Event must have exactly 3 choices\n"
                    "- Choice 3 must have a requirement field\n"
                    "- Invalid tag/strategy/requirement enum values\n"
                    "- Missing required fields (title, description, type)",
                    key);
            return false;
        }

        // Clean up test event
        DestroyEvent(&test);
        validated_count++;
        entry = entry->next;
    }

    d_LogInfoF("✓ Event Validation: All %d events valid", validated_count);
    return true;
}

// ============================================================================
// LOAD ON-DEMAND
// ============================================================================

EventEncounter_t* LoadEventFromDUF(const char* key) {
    // STEP 1: Validate parameters
    if (!key) {
        d_LogError("LoadEventFromDUF: NULL key");
        return NULL;
    }

    if (!g_events_db) {
        d_LogError("LoadEventFromDUF: g_events_db is NULL (not initialized)");
        return NULL;
    }

    // STEP 2: Find DUF node
    dDUFValue_t* event_node = d_DUFGetObjectItem(g_events_db, key);
    if (!event_node) {
        d_LogErrorF("Event '%s' not found in DUF database", key);
        return NULL;
    }

    // STEP 3: Parse metadata (title, description, type)
    dDUFValue_t* title_node = d_DUFGetObjectItem(event_node, "title");
    if (!title_node || !title_node->value_string) {
        d_LogErrorF("Event '%s' missing 'title' field", key);
        return NULL;
    }

    dDUFValue_t* desc_node = d_DUFGetObjectItem(event_node, "description");
    if (!desc_node || !desc_node->value_string) {
        d_LogErrorF("Event '%s' missing 'description' field", key);
        return NULL;
    }

    dDUFValue_t* type_node = d_DUFGetObjectItem(event_node, "type");
    if (!type_node || !type_node->value_string) {
        d_LogErrorF("Event '%s' missing 'type' field", key);
        return NULL;
    }

    EventType_t type = EventTypeFromString(type_node->value_string);

    // Create event (uses EXISTING CreateEvent from event.c)
    EventEncounter_t* event = CreateEvent(title_node->value_string,
                                          desc_node->value_string,
                                          type);
    if (!event) {
        d_LogErrorF("Failed to create event '%s'", key);
        return NULL;
    }

    // STEP 4: Parse choices array (MUST BE 3)
    dDUFValue_t* choices_node = d_DUFGetObjectItem(event_node, "choices");
    if (!choices_node || !choices_node->child) {
        d_LogErrorF("Event '%s' missing 'choices' array", key);
        DestroyEvent(&event);
        return NULL;
    }

    // Count choices
    int choice_count = 0;
    dDUFValue_t* choice_node = choices_node->child;
    while (choice_node) {
        choice_count++;
        choice_node = choice_node->next;
    }

    // ENFORCE 3-CHOICE RULE (CRITICAL!)
    if (choice_count != 3) {
        d_LogErrorF("Event '%s' must have exactly 3 choices (found %d) - see CLAUDE.md",
                   key, choice_count);
        DestroyEvent(&event);
        return NULL;
    }

    // STEP 5: Parse each choice
    choice_node = choices_node->child;
    int choice_idx = 0;
    while (choice_node) {
        if (!ParseEventChoice(event, choice_node, choice_idx, key)) {
            d_LogErrorF("Failed to parse choice %d for event '%s'", choice_idx, key);
            DestroyEvent(&event);
            return NULL;
        }
        choice_idx++;
        choice_node = choice_node->next;
    }

    // STEP 6: VALIDATE CHOICE 3 HAS REQUIREMENT (CRITICAL!)
    EventChoice_t* choice_3 = (EventChoice_t*)d_ArrayGet(event->choices, 2);
    if (!choice_3) {
        d_LogErrorF("Event '%s': Failed to get choice 3 (internal error)", key);
        DestroyEvent(&event);
        return NULL;
    }

    if (choice_3->requirement.type == REQUIREMENT_NONE) {
        d_LogErrorF("Event '%s': Choice 3 MUST have a requirement (locked choice pattern) - see CLAUDE.md", key);
        DestroyEvent(&event);
        return NULL;
    }

    d_LogInfoF("Loaded event '%s' from DUF (%d choices)", key, choice_count);
    return event;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

static bool ParseEventChoice(EventEncounter_t* event, dDUFValue_t* choice_node,
                            int choice_idx, const char* event_key) {
    // STEP 1: Parse required fields
    dDUFValue_t* text_node = d_DUFGetObjectItem(choice_node, "text");
    if (!text_node || !text_node->value_string) {
        d_LogErrorF("Event '%s' choice %d missing 'text' field", event_key, choice_idx);
        return false;
    }

    dDUFValue_t* result_node = d_DUFGetObjectItem(choice_node, "result_text");
    if (!result_node || !result_node->value_string) {
        d_LogErrorF("Event '%s' choice %d missing 'result_text' field", event_key, choice_idx);
        return false;
    }

    dDUFValue_t* chips_node = d_DUFGetObjectItem(choice_node, "chips_delta");
    if (!chips_node) {
        d_LogErrorF("Event '%s' choice %d missing 'chips_delta' field", event_key, choice_idx);
        return false;
    }

    dDUFValue_t* sanity_node = d_DUFGetObjectItem(choice_node, "sanity_delta");
    if (!sanity_node) {
        d_LogErrorF("Event '%s' choice %d missing 'sanity_delta' field", event_key, choice_idx);
        return false;
    }

    // Add base choice (uses EXISTING AddEventChoice from event.c)
    AddEventChoice(event,
                   text_node->value_string,
                   result_node->value_string,
                   (int)chips_node->value_int,
                   (int)sanity_node->value_int);

    // STEP 2: Parse granted_tags array (optional)
    dDUFValue_t* tags_node = d_DUFGetObjectItem(choice_node, "granted_tags");
    if (tags_node && tags_node->child) {
        dDUFValue_t* tag_entry = tags_node->child;

        while (tag_entry) {
            // Each tag entry: { tag: "CURSED", strategy: "RANDOM_CARD" }
            dDUFValue_t* tag_name_node = d_DUFGetObjectItem(tag_entry, "tag");
            dDUFValue_t* strategy_node = d_DUFGetObjectItem(tag_entry, "strategy");

            if (!tag_name_node || !tag_name_node->value_string) {
                d_LogWarningF("Event '%s' choice %d: Tag entry missing 'tag' field (skipping)",
                             event_key, choice_idx);
                tag_entry = tag_entry->next;
                continue;
            }

            if (!strategy_node || !strategy_node->value_string) {
                d_LogWarningF("Event '%s' choice %d: Tag entry missing 'strategy' field (skipping)",
                             event_key, choice_idx);
                tag_entry = tag_entry->next;
                continue;
            }

            // Convert strings to enums
            CardTag_t tag = CardTagFromString(tag_name_node->value_string);
            TagTargetStrategy_t strategy = TagTargetStrategyFromString(strategy_node->value_string);

            // Add tag with strategy (uses EXISTING AddCardTagToChoiceWithStrategy from event.c)
            AddCardTagToChoiceWithStrategy(event, choice_idx, tag, strategy);

            tag_entry = tag_entry->next;
        }
    }

    // STEP 3: Parse trinket_reward (optional)
    dDUFValue_t* trinket_node = d_DUFGetObjectItem(choice_node, "trinket_reward");
    if (trinket_node && trinket_node->value_string) {
        // Uses EXISTING SetChoiceTrinketReward from event.c
        SetChoiceTrinketReward(event, choice_idx, trinket_node->value_string);
    }

    // STEP 4: Parse next_enemy_hp_multi (optional, defaults to 1.0)
    // Renamed from "enemy_hp_multiplier" to clarify it only affects the NEXT enemy, not permanent
    dDUFValue_t* hp_mult_node = d_DUFGetObjectItem(choice_node, "next_enemy_hp_multi");
    if (hp_mult_node) {
        float multiplier = (float)hp_mult_node->value_double;
        // Uses EXISTING SetChoiceEnemyHPMultiplier from event.c
        SetChoiceEnemyHPMultiplier(event, choice_idx, multiplier);
    }

    // STEP 5: Parse requirement (REQUIRED for choice 3, optional for 1-2)
    dDUFValue_t* req_node = d_DUFGetObjectItem(choice_node, "requirement");
    if (req_node) {
        if (!ParseRequirement(req_node, event, choice_idx, event_key)) {
            return false;
        }
    }

    return true;
}

static bool ParseRequirement(dDUFValue_t* req_node, EventEncounter_t* event,
                            int choice_idx, const char* event_key) {
    // Parse type (required)
    dDUFValue_t* type_node = d_DUFGetObjectItem(req_node, "type");
    if (!type_node || !type_node->value_string) {
        d_LogErrorF("Event '%s' choice %d requirement missing 'type' field",
                   event_key, choice_idx);
        return false;
    }

    RequirementType_t type = RequirementTypeFromString(type_node->value_string);

    ChoiceRequirement_t requirement = {0};
    requirement.type = type;

    switch (type) {
        case REQUIREMENT_TAG_COUNT: {
            // Parse tag (required)
            dDUFValue_t* tag_node = d_DUFGetObjectItem(req_node, "tag");
            if (!tag_node || !tag_node->value_string) {
                d_LogErrorF("Event '%s' TAG_COUNT requirement missing 'tag' field", event_key);
                return false;
            }

            // Parse min_count (required)
            dDUFValue_t* count_node = d_DUFGetObjectItem(req_node, "min_count");
            if (!count_node) {
                d_LogErrorF("Event '%s' TAG_COUNT requirement missing 'min_count' field", event_key);
                return false;
            }

            requirement.data.tag_req.required_tag = CardTagFromString(tag_node->value_string);
            requirement.data.tag_req.min_count = (int)count_node->value_int;
            break;
        }

        case REQUIREMENT_SANITY_THRESHOLD: {
            dDUFValue_t* threshold_node = d_DUFGetObjectItem(req_node, "threshold");
            if (!threshold_node) {
                d_LogErrorF("Event '%s' SANITY_THRESHOLD requirement missing 'threshold' field", event_key);
                return false;
            }
            requirement.data.sanity_req.threshold = (int)threshold_node->value_int;
            break;
        }

        case REQUIREMENT_CHIPS_THRESHOLD: {
            dDUFValue_t* threshold_node = d_DUFGetObjectItem(req_node, "threshold");
            if (!threshold_node) {
                d_LogErrorF("Event '%s' CHIPS_THRESHOLD requirement missing 'threshold' field", event_key);
                return false;
            }
            requirement.data.chips_req.threshold = (int)threshold_node->value_int;
            break;
        }

        case REQUIREMENT_HP_THRESHOLD: {
            dDUFValue_t* threshold_node = d_DUFGetObjectItem(req_node, "threshold");
            if (!threshold_node) {
                d_LogErrorF("Event '%s' HP_THRESHOLD requirement missing 'threshold' field", event_key);
                return false;
            }
            requirement.data.hp_req.threshold = (int)threshold_node->value_int;
            break;
        }

        case REQUIREMENT_TRINKET: {
            // TODO: Implement trinket requirement parsing (future)
            d_LogWarningF("Event '%s': TRINKET requirement not yet implemented (defaulting to NONE)", event_key);
            requirement.type = REQUIREMENT_NONE;  // Fallback
            break;
        }

        case REQUIREMENT_NONE:
        default:
            // No additional parsing needed
            break;
    }

    // Uses EXISTING SetChoiceRequirement from event.c
    SetChoiceRequirement(event, choice_idx, requirement);
    return true;
}

// ============================================================================
// CLEANUP
// ============================================================================

void CleanupEventSystem(void) {
    if (g_events_db) {
        d_DUFFree(g_events_db);
        g_events_db = NULL;
    }
    d_LogInfo("Event system cleaned up");
}
