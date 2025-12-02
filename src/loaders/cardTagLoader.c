/*
 * Card Tag Loader Implementation (ADR-019 Compliant)
 * Loads card tag definitions from DUF at startup
 */

#include "../../include/loaders/cardTagLoader.h"
#include <string.h>

// ============================================================================
// GLOBAL DATABASE
// ============================================================================

dDUFValue_t* g_card_tags_db = NULL;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

const char* CardTagToString(CardTag_t tag) {
    switch (tag) {
        case CARD_TAG_CURSED:   return "cursed";
        case CARD_TAG_VAMPIRIC: return "vampiric";
        case CARD_TAG_LUCKY:    return "lucky";
        case CARD_TAG_JAGGED:   return "jagged";
        case CARD_TAG_DOUBLED:  return "doubled";
        default:
            d_LogErrorF("CardTagToString: Unknown tag %d", tag);
            return "unknown";
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

dDUFError_t* LoadCardTagDatabase(const char* filepath, dDUFValue_t** out_db) {
    if (!filepath || !out_db) {
        d_LogError("LoadCardTagDatabase: NULL parameter");
        return NULL;  // Invalid call
    }

    d_LogInfoF("Loading card tag database from %s", filepath);

    // Parse DUF file (ADR-019: simple wrapper around d_DUFParseFile)
    dDUFError_t* err = d_DUFParseFile(filepath, out_db);
    if (err) {
        d_LogErrorF("Failed to parse %s: %s", filepath, d_StringPeek(err->message));
        return err;
    }

    d_LogInfoF("Card tag database loaded successfully (%s)", filepath);
    return NULL;  // Success
}

bool ValidateCardTagDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size) {
    if (!db) {
        snprintf(out_error_msg, error_msg_size,
                "Card Tag DUF Validation Failed\n\n"
                "Card tag database is NULL\n\n"
                "Common issues:\n"
                "- All 5 tags required: cursed, vampiric, lucky, jagged, doubled\n"
                "- Each tag needs display_name, description, color_r/g/b\n"
                "- Trigger type must be 'on_draw' or 'passive'\n"
                "- Effects array cannot be empty");
        d_LogError(out_error_msg);
        return false;
    }

    if (db->type != D_DUF_TABLE) {
        snprintf(out_error_msg, error_msg_size,
                "Card Tag DUF Validation Failed\n\n"
                "Card tag database is not a table (type=%d)\n\n"
                "Common issues:\n"
                "- All 5 tags required: cursed, vampiric, lucky, jagged, doubled\n"
                "- Each tag needs display_name, description, color_r/g/b\n"
                "- Trigger type must be 'on_draw' or 'passive'\n"
                "- Effects array cannot be empty", db->type);
        d_LogError(out_error_msg);
        return false;
    }

    d_LogInfo("Validating card tag database...");

    // Required tags (all 5 must be present)
    const char* required_tags[] = {"cursed", "vampiric", "lucky", "jagged", "doubled"};
    const int num_required = 5;

    for (int i = 0; i < num_required; i++) {
        const char* tag_key = required_tags[i];
        dDUFValue_t* tag_entry = d_DUFGetObjectItem(db, tag_key);

        if (!tag_entry) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Missing tag: %s (expected all 5 tags: cursed, vampiric, lucky, jagged, doubled)",
                    tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        // Validate tag structure
        dDUFValue_t* display_name = d_DUFGetObjectItem(tag_entry, "display_name");
        dDUFValue_t* description = d_DUFGetObjectItem(tag_entry, "description");
        dDUFValue_t* color_r = d_DUFGetObjectItem(tag_entry, "color_r");
        dDUFValue_t* color_g = d_DUFGetObjectItem(tag_entry, "color_g");
        dDUFValue_t* color_b = d_DUFGetObjectItem(tag_entry, "color_b");
        dDUFValue_t* trigger = d_DUFGetObjectItem(tag_entry, "trigger");
        dDUFValue_t* effects = d_DUFGetObjectItem(tag_entry, "effects");

        // Check required fields
        if (!display_name || display_name->type != D_DUF_STRING || !display_name->value_string) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' missing or invalid 'display_name' field\n\n"
                    "Common issues:\n"
                    "- All 5 tags required: cursed, vampiric, lucky, jagged, doubled\n"
                    "- Each tag needs display_name, description, color_r/g/b\n"
                    "- Trigger type must be 'on_draw' or 'passive'\n"
                    "- Effects array cannot be empty", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        if (!description || description->type != D_DUF_STRING || !description->value_string) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' missing or invalid 'description' field", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        // Validate color values (must be integers 0-255)
        if (!color_r || color_r->type != D_DUF_INT || color_r->value_int < 0 || color_r->value_int > 255) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' has invalid 'color_r' (must be 0-255)", tag_key);
            d_LogError(out_error_msg);
            return false;
        }
        if (!color_g || color_g->type != D_DUF_INT || color_g->value_int < 0 || color_g->value_int > 255) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' has invalid 'color_g' (must be 0-255)", tag_key);
            d_LogError(out_error_msg);
            return false;
        }
        if (!color_b || color_b->type != D_DUF_INT || color_b->value_int < 0 || color_b->value_int > 255) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' has invalid 'color_b' (must be 0-255)", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        // Validate trigger block
        if (!trigger || trigger->type != D_DUF_TABLE) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' missing or invalid 'trigger' block", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        dDUFValue_t* trigger_type = d_DUFGetObjectItem(trigger, "type");
        if (!trigger_type || trigger_type->type != D_DUF_STRING || !trigger_type->value_string) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' trigger missing or invalid 'type' field", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        // Validate trigger type is "on_draw" or "passive"
        const char* trigger_type_str = trigger_type->value_string;
        if (strcmp(trigger_type_str, "on_draw") != 0 && strcmp(trigger_type_str, "passive") != 0) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' has invalid trigger type '%s' (must be 'on_draw' or 'passive')",
                    tag_key, trigger_type_str);
            d_LogError(out_error_msg);
            return false;
        }

        // Validate effects array
        if (!effects) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' missing 'effects' array", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        // Count effect children
        int effect_count = 0;
        dDUFValue_t* effect_child = effects->child;
        while (effect_child) {
            effect_count++;
            effect_child = effect_child->next;
        }

        if (effect_count == 0) {
            snprintf(out_error_msg, error_msg_size,
                    "Card Tag DUF Validation Failed\n\n"
                    "Tag '%s' has empty 'effects' array (must have at least 1 effect)", tag_key);
            d_LogError(out_error_msg);
            return false;
        }

        // Validate each effect has 'type' field
        effect_child = effects->child;
        int effect_idx = 0;
        while (effect_child) {
            dDUFValue_t* effect_type = d_DUFGetObjectItem(effect_child, "type");
            if (!effect_type || effect_type->type != D_DUF_STRING || !effect_type->value_string) {
                snprintf(out_error_msg, error_msg_size,
                        "Card Tag DUF Validation Failed\n\n"
                        "Tag '%s' effect[%d] missing or invalid 'type' field", tag_key, effect_idx);
                d_LogError(out_error_msg);
                return false;
            }
            effect_child = effect_child->next;
            effect_idx++;
        }

        d_LogInfoF("✓ Tag '%s' validated successfully", tag_key);
    }

    d_LogInfo("Card tag database validation complete: All 5 tags valid");
    return true;
}

void CleanupCardTagSystem(void) {
    if (g_card_tags_db) {
        d_LogInfo("Cleaning up card tag system");
        d_DUFFree(g_card_tags_db);
        g_card_tags_db = NULL;
    }
}

// ============================================================================
// ON-DEMAND LOADER (for validation testing)
// ============================================================================

/**
 * LoadCardTagFromDUF - Load tag metadata from database (validation helper)
 *
 * Note: This is primarily used by ValidateCardTagDatabase() for testing.
 * Most code should use GetTagDisplayName/Color/Description() for queries.
 *
 * @param tag - Tag enum
 * @return true if tag exists and valid, false otherwise
 */
bool LoadCardTagFromDUF(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("LoadCardTagFromDUF: g_card_tags_db not initialized");
        return false;
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        d_LogErrorF("LoadCardTagFromDUF: Tag '%s' not found in database", tag_key);
        return false;
    }

    // Validate required fields exist
    dDUFValue_t* display_name = d_DUFGetObjectItem(tag_entry, "display_name");
    dDUFValue_t* description = d_DUFGetObjectItem(tag_entry, "description");
    dDUFValue_t* color_r = d_DUFGetObjectItem(tag_entry, "color_r");
    dDUFValue_t* color_g = d_DUFGetObjectItem(tag_entry, "color_g");
    dDUFValue_t* color_b = d_DUFGetObjectItem(tag_entry, "color_b");
    dDUFValue_t* trigger = d_DUFGetObjectItem(tag_entry, "trigger");
    dDUFValue_t* effects = d_DUFGetObjectItem(tag_entry, "effects");

    if (!display_name || !description || !color_r || !color_g || !color_b ||
        !trigger || !effects) {
        d_LogErrorF("LoadCardTagFromDUF: Tag '%s' missing required fields", tag_key);
        return false;
    }

    return true;
}

// ============================================================================
// TAG METADATA QUERIES
// ============================================================================

const char* GetTagDisplayName(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("GetTagDisplayName: g_card_tags_db not initialized");
        return "Unknown";
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        d_LogErrorF("GetTagDisplayName: Tag '%s' not found in database", tag_key);
        return "Unknown";
    }

    dDUFValue_t* display_name = d_DUFGetObjectItem(tag_entry, "display_name");
    if (!display_name || !display_name->value_string) {
        d_LogErrorF("GetTagDisplayName: Tag '%s' missing display_name", tag_key);
        return "Unknown";
    }

    return display_name->value_string;
}

const char* GetTagDescription(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("GetTagDescription: g_card_tags_db not initialized");
        return "Unknown effect";
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        d_LogErrorF("GetTagDescription: Tag '%s' not found in database", tag_key);
        return "Unknown effect";
    }

    dDUFValue_t* description = d_DUFGetObjectItem(tag_entry, "description");
    if (!description || !description->value_string) {
        d_LogErrorF("GetTagDescription: Tag '%s' missing description", tag_key);
        return "Unknown effect";
    }

    return description->value_string;
}

void GetTagColor(CardTag_t tag, int* out_r, int* out_g, int* out_b) {
    // Default fallback (gray)
    int default_r = 87, default_g = 114, default_b = 119;

    if (!out_r || !out_g || !out_b) {
        d_LogError("GetTagColor: NULL output parameter");
        return;
    }

    if (!g_card_tags_db) {
        d_LogError("GetTagColor: g_card_tags_db not initialized");
        *out_r = default_r; *out_g = default_g; *out_b = default_b;
        return;
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        d_LogErrorF("GetTagColor: Tag '%s' not found in database", tag_key);
        *out_r = default_r; *out_g = default_g; *out_b = default_b;
        return;
    }

    dDUFValue_t* color_r = d_DUFGetObjectItem(tag_entry, "color_r");
    dDUFValue_t* color_g = d_DUFGetObjectItem(tag_entry, "color_g");
    dDUFValue_t* color_b = d_DUFGetObjectItem(tag_entry, "color_b");

    if (!color_r || color_r->type != D_DUF_INT ||
        !color_g || color_g->type != D_DUF_INT ||
        !color_b || color_b->type != D_DUF_INT) {
        d_LogErrorF("GetTagColor: Tag '%s' missing or invalid color values", tag_key);
        *out_r = default_r; *out_g = default_g; *out_b = default_b;
        return;
    }

    *out_r = (int)color_r->value_int;
    *out_g = (int)color_g->value_int;
    *out_b = (int)color_b->value_int;
}

// ============================================================================
// TAG TRIGGER/EFFECT QUERIES
// ============================================================================

const char* GetTagTriggerType(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("GetTagTriggerType: g_card_tags_db not initialized");
        return NULL;
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        d_LogErrorF("GetTagTriggerType: Tag '%s' not found in database", tag_key);
        return NULL;
    }

    dDUFValue_t* trigger = d_DUFGetObjectItem(tag_entry, "trigger");
    if (!trigger || trigger->type != D_DUF_TABLE) {
        d_LogErrorF("GetTagTriggerType: Tag '%s' missing trigger block", tag_key);
        return NULL;
    }

    dDUFValue_t* trigger_type = d_DUFGetObjectItem(trigger, "type");
    if (!trigger_type || !trigger_type->value_string) {
        d_LogErrorF("GetTagTriggerType: Tag '%s' missing trigger.type", tag_key);
        return NULL;
    }

    return trigger_type->value_string;
}

const char* GetTagTriggerScope(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("GetTagTriggerScope: g_card_tags_db not initialized");
        return NULL;
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        return NULL;  // Not an error, just no scope
    }

    dDUFValue_t* trigger = d_DUFGetObjectItem(tag_entry, "trigger");
    if (!trigger || trigger->type != D_DUF_TABLE) {
        return NULL;
    }

    dDUFValue_t* scope = d_DUFGetObjectItem(trigger, "scope");
    if (!scope || !scope->value_string) {
        return NULL;  // Scope is optional
    }

    return scope->value_string;
}

const char* GetTagDuration(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("GetTagDuration: g_card_tags_db not initialized");
        return NULL;
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        return NULL;
    }

    dDUFValue_t* trigger = d_DUFGetObjectItem(tag_entry, "trigger");
    if (!trigger || trigger->type != D_DUF_TABLE) {
        return NULL;
    }

    dDUFValue_t* duration = d_DUFGetObjectItem(trigger, "duration");
    if (!duration || !duration->value_string) {
        return NULL;  // Duration is optional (NULL = permanent)
    }

    return duration->value_string;
}

dDUFValue_t* GetTagEffects(CardTag_t tag) {
    if (!g_card_tags_db) {
        d_LogError("GetTagEffects: g_card_tags_db not initialized");
        return NULL;
    }

    const char* tag_key = CardTagToString(tag);
    dDUFValue_t* tag_entry = d_DUFGetObjectItem(g_card_tags_db, tag_key);
    if (!tag_entry) {
        d_LogErrorF("GetTagEffects: Tag '%s' not found in database", tag_key);
        return NULL;
    }

    dDUFValue_t* effects = d_DUFGetObjectItem(tag_entry, "effects");
    if (!effects) {
        d_LogErrorF("GetTagEffects: Tag '%s' missing effects array", tag_key);
        return NULL;
    }

    return effects;
}
