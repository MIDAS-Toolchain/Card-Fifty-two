/*
 * Trinket Loader - Parse trinkets from DUF files
 */

#include "../../include/loaders/trinketLoader.h"
#include "../../include/ability.h"  // For GameEventFromString
#include <string.h>

// ============================================================================
// GLOBAL REGISTRY (Enemy pattern: DUF tree + key cache, no table)
// ============================================================================

// Global trinket database registry (extensible for modular trinket packs)
static dArray_t* g_trinket_databases = NULL;  // Array of dDUFValue_t* (all loaded trinket DUFs)
dDUFValue_t* g_trinkets_db = NULL;             // Legacy: points to first DUF (for compatibility)
dArray_t* g_trinket_key_cache = NULL;           // Cache of all trinket keys (const char*) for iteration

// ============================================================================
// ENUM CONVERTERS
// ============================================================================

TrinketEffectType_t TrinketEffectTypeFromString(const char* str) {
    if (!str) return TRINKET_EFFECT_NONE;

    if (strcmp(str, "add_chips") == 0) return TRINKET_EFFECT_ADD_CHIPS;
    if (strcmp(str, "add_chips_percent") == 0) return TRINKET_EFFECT_ADD_CHIPS_PERCENT;
    if (strcmp(str, "lose_chips") == 0) return TRINKET_EFFECT_LOSE_CHIPS;
    if (strcmp(str, "apply_status") == 0) return TRINKET_EFFECT_APPLY_STATUS;
    if (strcmp(str, "clear_status") == 0) return TRINKET_EFFECT_CLEAR_STATUS;
    if (strcmp(str, "trinket_stack") == 0) return TRINKET_EFFECT_TRINKET_STACK;
    if (strcmp(str, "trinket_stack_reset") == 0) return TRINKET_EFFECT_TRINKET_STACK_RESET;
    if (strcmp(str, "refund_chips_percent") == 0) return TRINKET_EFFECT_REFUND_CHIPS_PERCENT;
    if (strcmp(str, "deal_damage") == 0) return TRINKET_EFFECT_ADD_DAMAGE_FLAT;  // Renamed from add_damage_flat
    if (strcmp(str, "add_damage_flat") == 0) return TRINKET_EFFECT_ADD_DAMAGE_FLAT;  // Keep old name for backwards compat
    if (strcmp(str, "damage_multiplier") == 0) return TRINKET_EFFECT_DAMAGE_MULTIPLIER;
    if (strcmp(str, "add_tag_to_cards") == 0) return TRINKET_EFFECT_ADD_TAG_TO_CARDS;
    if (strcmp(str, "buff_tag_damage") == 0) return TRINKET_EFFECT_BUFF_TAG_DAMAGE;
    if (strcmp(str, "push_damage_percent") == 0) return TRINKET_EFFECT_PUSH_DAMAGE_PERCENT;

    d_LogWarningF("Unknown trinket effect type: %s", str);
    return TRINKET_EFFECT_NONE;
}

TrinketRarity_t TrinketRarityFromString(const char* str) {
    if (!str) return TRINKET_RARITY_COMMON;

    if (strcmp(str, "common") == 0) return TRINKET_RARITY_COMMON;
    if (strcmp(str, "uncommon") == 0) return TRINKET_RARITY_UNCOMMON;
    if (strcmp(str, "rare") == 0) return TRINKET_RARITY_RARE;
    if (strcmp(str, "legendary") == 0) return TRINKET_RARITY_LEGENDARY;
    if (strcmp(str, "event") == 0) return TRINKET_RARITY_EVENT;
    if (strcmp(str, "class") == 0) return TRINKET_RARITY_CLASS;

    d_LogWarningF("Unknown trinket rarity: %s", str);
    return TRINKET_RARITY_COMMON;
}

// ============================================================================
// DUF PARSING
// ============================================================================

/**
 * ParseTrinketTemplate - Parse single trinket from DUF node
 *
 * @param trinket_node - DUF table node (@lucky_chip, etc)
 * @param trinket_key - Key from DUF (@key becomes trinket_key)
 * @param out_trinket - Output trinket template
 * @return bool - true on success, false on parse error
 */
static bool ParseTrinketTemplate(dDUFValue_t* trinket_node, const char* trinket_key, TrinketTemplate_t* out_trinket) {
    if (!trinket_node || !trinket_key || !out_trinket) {
        return false;
    }

    // Initialize to zero
    memset(out_trinket, 0, sizeof(TrinketTemplate_t));

    // Parse trinket_key
    out_trinket->trinket_key = d_StringInit();
    if (!out_trinket->trinket_key) {
        d_LogError("ParseTrinketTemplate: Failed to allocate trinket_key");
        return false;
    }
    d_StringSet(out_trinket->trinket_key, trinket_key, 0);

    // Parse name (required)
    dDUFValue_t* name_node = d_DUFGetObjectItem(trinket_node, "name");
    if (!name_node || !name_node->value_string) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' missing 'name' field", trinket_key);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    out_trinket->name = d_StringInit();
    if (!out_trinket->name) {
        d_LogError("ParseTrinketTemplate: Failed to allocate name");
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    d_StringSet(out_trinket->name, name_node->value_string, 0);

    // Parse flavor (required)
    dDUFValue_t* flavor_node = d_DUFGetObjectItem(trinket_node, "flavor");
    if (!flavor_node || !flavor_node->value_string) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' missing 'flavor' field", trinket_key);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    out_trinket->flavor = d_StringInit();
    if (!out_trinket->flavor) {
        d_LogError("ParseTrinketTemplate: Failed to allocate flavor");
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    d_StringSet(out_trinket->flavor, flavor_node->value_string, 0);

    // Parse rarity (required)
    dDUFValue_t* rarity_node = d_DUFGetObjectItem(trinket_node, "rarity");
    if (!rarity_node || !rarity_node->value_string) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' missing 'rarity' field", trinket_key);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    out_trinket->rarity = TrinketRarityFromString(rarity_node->value_string);

    // Parse base_value (required)
    dDUFValue_t* value_node = d_DUFGetObjectItem(trinket_node, "base_value");
    if (!value_node) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' missing 'base_value' field", trinket_key);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    out_trinket->base_value = (int)value_node->value_int;

    // Parse passive_trigger (required)
    dDUFValue_t* trigger_node = d_DUFGetObjectItem(trinket_node, "passive_trigger");
    if (!trigger_node || !trigger_node->value_string) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' missing 'passive_trigger' field", trinket_key);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    // Handle special ON_EQUIP trigger (not in GameEvent_t enum)
    if (strcmp(trigger_node->value_string, "ON_EQUIP") == 0) {
        out_trinket->passive_trigger = 999;  // Special value for on-equip effects
    } else {
        // Parse game event (GAME_EVENT_COMBAT_START == 0 is valid, so we can't validate by == 0)
        // GameEventFromString logs a warning for truly invalid strings
        out_trinket->passive_trigger = GameEventFromString(trigger_node->value_string);
        // We can't validate == 0 as error because COMBAT_START is enum value 0
    }

    // Parse passive_effect_type (required)
    dDUFValue_t* effect_type_node = d_DUFGetObjectItem(trinket_node, "passive_effect_type");
    if (!effect_type_node || !effect_type_node->value_string) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' missing 'passive_effect_type' field", trinket_key);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }
    out_trinket->passive_effect_type = TrinketEffectTypeFromString(effect_type_node->value_string);
    if (out_trinket->passive_effect_type == TRINKET_EFFECT_NONE) {
        d_LogErrorF("ParseTrinketTemplate: Trinket '%s' has invalid effect type '%s'",
                   trinket_key, effect_type_node->value_string);
        CleanupTrinketTemplate(out_trinket);
        return false;
    }

    // Parse passive_effect_value (optional)
    dDUFValue_t* effect_value_node = d_DUFGetObjectItem(trinket_node, "passive_effect_value");
    if (effect_value_node) {
        out_trinket->passive_effect_value = (int)effect_value_node->value_int;
    }

    // Parse passive_status_key (optional, for APPLY_STATUS/CLEAR_STATUS)
    dDUFValue_t* status_key_node = d_DUFGetObjectItem(trinket_node, "passive_status_key");
    if (status_key_node && status_key_node->value_string) {
        out_trinket->passive_status_key = d_StringInit();
        if (!out_trinket->passive_status_key) {
            d_LogError("ParseTrinketTemplate: Failed to allocate passive_status_key");
            CleanupTrinketTemplate(out_trinket);
            return false;
        }
        d_StringSet(out_trinket->passive_status_key, status_key_node->value_string, 0);
    }

    // Parse passive_status_stacks (optional)
    dDUFValue_t* status_stacks_node = d_DUFGetObjectItem(trinket_node, "passive_status_stacks");
    if (status_stacks_node) {
        out_trinket->passive_status_stacks = (int)status_stacks_node->value_int;
    }

    // Parse trinket stack fields (optional, for TRINKET_STACK effect)
    dDUFValue_t* stack_stat_node = d_DUFGetObjectItem(trinket_node, "passive_stack_stat");
    if (stack_stat_node && stack_stat_node->value_string) {
        out_trinket->passive_stack_stat = d_StringInit();
        if (!out_trinket->passive_stack_stat) {
            d_LogError("ParseTrinketTemplate: Failed to allocate passive_stack_stat");
            CleanupTrinketTemplate(out_trinket);
            return false;
        }
        d_StringSet(out_trinket->passive_stack_stat, stack_stat_node->value_string, 0);
    }

    dDUFValue_t* stack_value_node = d_DUFGetObjectItem(trinket_node, "passive_stack_value");
    if (stack_value_node) {
        out_trinket->passive_stack_value = (int)stack_value_node->value_int;
    }

    dDUFValue_t* stack_max_node = d_DUFGetObjectItem(trinket_node, "passive_stack_max");
    if (stack_max_node) {
        out_trinket->passive_stack_max = (int)stack_max_node->value_int;
    }

    dDUFValue_t* stack_on_max_node = d_DUFGetObjectItem(trinket_node, "passive_stack_on_max");
    if (stack_on_max_node && stack_on_max_node->value_string) {
        out_trinket->passive_stack_on_max = d_StringInit();
        if (!out_trinket->passive_stack_on_max) {
            d_LogError("ParseTrinketTemplate: Failed to allocate passive_stack_on_max");
            CleanupTrinketTemplate(out_trinket);
            return false;
        }
        d_StringSet(out_trinket->passive_stack_on_max, stack_on_max_node->value_string, 0);
    }

    // Parse tag fields (optional, for ADD_TAG_TO_CARDS/BUFF_TAG_DAMAGE)
    dDUFValue_t* tag_node = d_DUFGetObjectItem(trinket_node, "passive_tag");
    if (tag_node && tag_node->value_string) {
        out_trinket->passive_tag = d_StringInit();
        if (!out_trinket->passive_tag) {
            d_LogError("ParseTrinketTemplate: Failed to allocate passive_tag");
            CleanupTrinketTemplate(out_trinket);
            return false;
        }
        d_StringSet(out_trinket->passive_tag, tag_node->value_string, 0);
    }

    dDUFValue_t* tag_count_node = d_DUFGetObjectItem(trinket_node, "passive_tag_count");
    if (tag_count_node) {
        out_trinket->passive_tag_count = (int)tag_count_node->value_int;
    }

    dDUFValue_t* tag_buff_node = d_DUFGetObjectItem(trinket_node, "passive_tag_buff_value");
    if (tag_buff_node) {
        out_trinket->passive_tag_buff_value = (int)tag_buff_node->value_int;
    }

    // Parse secondary passive (optional)
    dDUFValue_t* trigger_2_node = d_DUFGetObjectItem(trinket_node, "passive_trigger_2");
    if (trigger_2_node && trigger_2_node->value_string) {
        // Handle ON_EQUIP for secondary trigger (check BEFORE calling GameEventFromString)
        if (strcmp(trigger_2_node->value_string, "ON_EQUIP") == 0) {
            out_trinket->passive_trigger_2 = 999;
        } else {
            out_trinket->passive_trigger_2 = GameEventFromString(trigger_2_node->value_string);
        }
    }

    dDUFValue_t* effect_type_2_node = d_DUFGetObjectItem(trinket_node, "passive_effect_type_2");
    if (effect_type_2_node && effect_type_2_node->value_string) {
        out_trinket->passive_effect_type_2 = TrinketEffectTypeFromString(effect_type_2_node->value_string);
    }

    dDUFValue_t* effect_value_2_node = d_DUFGetObjectItem(trinket_node, "passive_effect_value_2");
    if (effect_value_2_node) {
        out_trinket->passive_effect_value_2 = (int)effect_value_2_node->value_int;
    }

    dDUFValue_t* status_key_2_node = d_DUFGetObjectItem(trinket_node, "passive_status_key_2");
    if (status_key_2_node && status_key_2_node->value_string) {
        out_trinket->passive_status_key_2 = d_StringInit();
        if (!out_trinket->passive_status_key_2) {
            d_LogError("ParseTrinketTemplate: Failed to allocate passive_status_key_2");
            CleanupTrinketTemplate(out_trinket);
            return false;
        }
        d_StringSet(out_trinket->passive_status_key_2, status_key_2_node->value_string, 0);
    }

    dDUFValue_t* status_stacks_2_node = d_DUFGetObjectItem(trinket_node, "passive_status_stacks_2");
    if (status_stacks_2_node) {
        out_trinket->passive_status_stacks_2 = (int)status_stacks_2_node->value_int;
    }

    dDUFValue_t* tag_2_node = d_DUFGetObjectItem(trinket_node, "passive_tag_2");
    if (tag_2_node && tag_2_node->value_string) {
        out_trinket->passive_tag_2 = d_StringInit();
        if (!out_trinket->passive_tag_2) {
            d_LogError("ParseTrinketTemplate: Failed to allocate passive_tag_2");
            CleanupTrinketTemplate(out_trinket);
            return false;
        }
        d_StringSet(out_trinket->passive_tag_2, tag_2_node->value_string, 0);
    }

    // Parse optional condition
    dDUFValue_t* bet_gte_node = d_DUFGetObjectItem(trinket_node, "passive_condition_bet_gte");
    if (bet_gte_node) {
        out_trinket->passive_condition_bet_gte = (int)bet_gte_node->value_int;
    }

    // Validation successful (removed verbose logging to avoid crashes)
    // d_LogInfoF("Parsed trinket: %s (%s, %s)",
    //            trinket_key, d_StringPeek(out_trinket->name),
    //            d_StringPeek(out_trinket->flavor));

    return true;
}

// ============================================================================
// PUBLIC API
// ============================================================================

dDUFError_t* LoadTrinketDatabase(const char* filepath, dDUFValue_t** out_db) {
    if (!filepath || !out_db) {
        d_LogError("LoadTrinketDatabase: NULL parameters");
        return NULL;
    }

    dDUFError_t* err = d_DUFParseFile(filepath, out_db);
    if (err) {
        return err;  // Caller handles error
    }

    d_LogInfoF("Loaded trinket database from %s", filepath);
    return NULL;
}

bool PopulateTrinketTemplates(dDUFValue_t* trinkets_db) {
    if (!trinkets_db) {
        d_LogError("PopulateTrinketTemplates: NULL database");
        return false;
    }

    // Create key cache (if not already created) - stores all trinket keys for iteration
    if (!g_trinket_key_cache) {
        g_trinket_key_cache = d_ArrayInit(32, sizeof(const char*));
        if (!g_trinket_key_cache) {
            d_LogFatal("PopulateTrinketTemplates: Failed to create key cache");
            return false;
        }
    }

    // Iterate through all trinket entries and cache keys (enemy pattern: no table storage)
    dDUFValue_t* trinket_entry = trinkets_db->child;
    int loaded_count = 0;

    while (trinket_entry) {
        const char* trinket_key = trinket_entry->string;  // @lucky_chip → "lucky_chip"
        if (!trinket_key) {
            trinket_entry = trinket_entry->next;
            continue;
        }

        // Cache the key for later lookup
        d_ArrayAppend(g_trinket_key_cache, &trinket_key);
        loaded_count++;

        trinket_entry = trinket_entry->next;
    }

    d_LogInfoF("Cached %d trinket keys from DUF", loaded_count);
    return loaded_count > 0;
}

/**
 * PopulateAllTrinketTemplates - Load all trinket DUFs into global registry
 *
 * @param databases - Array of dDUFValue_t* (trinket DUF files)
 * @return bool - Success/failure
 *
 * Replaces old MergeTrinketDatabases approach with extensible multi-DUF system.
 */
bool PopulateAllTrinketTemplates(dArray_t* databases) {
    if (!databases) {
        d_LogError("PopulateAllTrinketTemplates: NULL databases array");
        return false;
    }

    // Store global reference for on-demand loading
    g_trinket_databases = databases;

    // Set legacy g_trinkets_db to first database (for backward compatibility)
    if (databases->count > 0) {
        dDUFValue_t** first_db = (dDUFValue_t**)d_ArrayGet(databases, 0);
        if (first_db && *first_db) {
            g_trinkets_db = *first_db;
        }
    }

    // Populate trinket templates from each DUF file
    bool success = true;
    for (size_t i = 0; i < databases->count; i++) {
        dDUFValue_t** db_ptr = (dDUFValue_t**)d_ArrayGet(databases, i);
        if (!db_ptr || !*db_ptr) continue;

        if (!PopulateTrinketTemplates(*db_ptr)) {
            d_LogErrorF("Failed to populate trinket templates from DUF #%zu", i);
            success = false;
        }
    }

    return success;
}

// DEPRECATED: Use PopulateAllTrinketTemplates instead
bool MergeTrinketDatabases(dDUFValue_t* combat_db, dDUFValue_t* other_db) {
    bool success = true;

    if (combat_db) {
        success = PopulateTrinketTemplates(combat_db) && success;
    }

    if (other_db) {
        success = PopulateTrinketTemplates(other_db) && success;
    }

    return success;
}

bool ValidateTrinketDatabase(dDUFValue_t* trinkets_db, char* out_error_msg, size_t error_msg_size) {
    if (!trinkets_db || !out_error_msg) {
        return false;
    }

    // Iterate through all trinket entries and validate
    dDUFValue_t* trinket_entry = trinkets_db->child;
    int validated_count = 0;

    while (trinket_entry) {
        const char* trinket_key = trinket_entry->string;
        if (!trinket_key) {
            trinket_entry = trinket_entry->next;
            continue;
        }

        // Try to parse trinket (validation test) - use enemy pattern (heap alloc)
        TrinketTemplate_t* test_trinket = malloc(sizeof(TrinketTemplate_t));
        if (!test_trinket) {
            d_LogFatal("ValidateTrinketDUF: Failed to allocate test trinket");
            return false;
        }
        memset(test_trinket, 0, sizeof(TrinketTemplate_t));

        if (!ParseTrinketTemplate(trinket_entry, trinket_key, test_trinket)) {
            // Validation failed - write error message
            snprintf(out_error_msg, error_msg_size,
                    "Trinket DUF Validation Failed\n\n"
                    "Trinket: %s\n\n"
                    "Check console logs for details.\n\n"
                    "Common issues:\n"
                    "- Invalid effect type\n"
                    "- Invalid rarity\n"
                    "- Missing required fields",
                    trinket_key);
            free(test_trinket);
            return false;
        }

        // Clean up test trinket (enemy pattern: cleanup then free)
        CleanupTrinketTemplate(test_trinket);
        free(test_trinket);
        validated_count++;
        trinket_entry = trinket_entry->next;
    }

    d_LogInfoF("✓ Trinket Validation: All %d trinkets valid", validated_count);
    return true;
}

/**
 * LoadTrinketTemplateFromDUF - Load trinket template from DUF (enemy pattern)
 *
 * @param trinket_key - Key name of trinket to load (e.g., "lucky_chip", "loaded_dice")
 * @return TrinketTemplate_t* - Heap-allocated template (caller must free), NULL on failure
 *
 * IMPORTANT: Returned template is HEAP-ALLOCATED. Caller must:
 * 1. Call CleanupTrinketTemplate() to free internal dStrings
 * 2. Call free() to free the struct itself
 */
TrinketTemplate_t* LoadTrinketTemplateFromDUF(const char* trinket_key) {
    if (!trinket_key) {
        d_LogError("LoadTrinketTemplateFromDUF: NULL trinket_key");
        return NULL;
    }

    if (!g_trinkets_db) {
        d_LogError("LoadTrinketTemplateFromDUF: g_trinkets_db is NULL");
        return NULL;
    }

    // Search through all loaded trinket DUFs (extensible for modular packs)
    dDUFValue_t* trinket_node = NULL;

    if (g_trinket_databases) {
        // New system: iterate through all loaded DUF files
        for (size_t i = 0; i < g_trinket_databases->count; i++) {
            dDUFValue_t** db_ptr = (dDUFValue_t**)d_ArrayGet(g_trinket_databases, i);
            if (!db_ptr || !*db_ptr) continue;

            trinket_node = d_DUFGetObjectItem(*db_ptr, trinket_key);
            if (trinket_node) break;  // Found it!
        }
    } else if (g_trinkets_db) {
        // Fallback: legacy single-DB system
        trinket_node = d_DUFGetObjectItem(g_trinkets_db, trinket_key);
    }

    if (!trinket_node) {
        d_LogErrorF("Trinket '%s' not found in any DUF database", trinket_key);
        return NULL;
    }

    // Allocate template on heap (caller owns this memory)
    TrinketTemplate_t* template = malloc(sizeof(TrinketTemplate_t));
    if (!template) {
        d_LogFatal("LoadTrinketTemplateFromDUF: Failed to allocate template");
        return NULL;
    }
    memset(template, 0, sizeof(TrinketTemplate_t));

    // Parse template from DUF
    if (!ParseTrinketTemplate(trinket_node, trinket_key, template)) {
        d_LogErrorF("Failed to parse trinket '%s'", trinket_key);
        free(template);
        return NULL;
    }

    // Validate critical fields (enemy pattern: catch parse bugs early)
    if (!template->name || !template->flavor) {
        d_LogErrorF("Trinket '%s' has NULL name or flavor after parse (parse bug!)", trinket_key);
        CleanupTrinketTemplate(template);
        free(template);
        return NULL;
    }

    return template;
}

/**
 * GetTrinketTemplate - Wrapper for LoadTrinketTemplateFromDUF (for backward compatibility)
 *
 * NOTE: This allocates memory! Caller must cleanup the returned template.
 */
TrinketTemplate_t* GetTrinketTemplate(const char* trinket_key) {
    return LoadTrinketTemplateFromDUF(trinket_key);
}

void CleanupTrinketTemplate(TrinketTemplate_t* trinket) {
    if (!trinket) {
        d_LogWarning("CleanupTrinketTemplate: NULL trinket pointer");
        return;
    }

    // Follow DestroyEnemy pattern: check NULL before destroying
    if (trinket->trinket_key) {
        d_StringDestroy(trinket->trinket_key);
        trinket->trinket_key = NULL;
    }
    if (trinket->name) {
        d_StringDestroy(trinket->name);
        trinket->name = NULL;
    }
    if (trinket->flavor) {
        d_StringDestroy(trinket->flavor);
        trinket->flavor = NULL;
    }
    if (trinket->passive_status_key) {
        d_StringDestroy(trinket->passive_status_key);
        trinket->passive_status_key = NULL;
    }
    if (trinket->passive_stack_stat) {
        d_StringDestroy(trinket->passive_stack_stat);
        trinket->passive_stack_stat = NULL;
    }
    if (trinket->passive_stack_on_max) {
        d_StringDestroy(trinket->passive_stack_on_max);
        trinket->passive_stack_on_max = NULL;
    }
    if (trinket->passive_tag) {
        d_StringDestroy(trinket->passive_tag);
        trinket->passive_tag = NULL;
    }
    if (trinket->passive_status_key_2) {
        d_StringDestroy(trinket->passive_status_key_2);
        trinket->passive_status_key_2 = NULL;
    }
    if (trinket->passive_tag_2) {
        d_StringDestroy(trinket->passive_tag_2);
        trinket->passive_tag_2 = NULL;
    }
}

void CleanupTrinketLoaderSystem(void) {
    // Enemy pattern: No table to cleanup, just key cache and DUF tree
    if (g_trinket_key_cache) {
        d_ArrayDestroy(g_trinket_key_cache);
        g_trinket_key_cache = NULL;
    }

    if (g_trinkets_db) {
        d_DUFFree(g_trinkets_db);
        g_trinkets_db = NULL;
    }

    d_LogInfo("Trinket loader system cleaned up");
}
