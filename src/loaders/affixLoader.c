/*
 * Affix Loader - Parse affixes from DUF files
 */

#include "../../include/loaders/affixLoader.h"
#include <string.h>

// ============================================================================
// GLOBAL REGISTRY (Enemy Pattern: DUF tree only, no table)
// ============================================================================

dDUFValue_t* g_affixes_db = NULL;    // Raw DUF data

// ============================================================================
// DUF PARSING
// ============================================================================

/**
 * ParseAffixTemplate - Parse single affix from DUF node
 *
 * @param affix_node - DUF table node (@damage_bonus_percent, etc)
 * @param stat_key - Key from DUF (@key becomes stat_key)
 * @param out_affix - Output affix template
 * @return bool - true on success, false on parse error
 */
static bool ParseAffixTemplate(dDUFValue_t* affix_node, const char* stat_key, AffixTemplate_t* out_affix) {
    if (!affix_node || !stat_key || !out_affix) {
        return false;
    }

    // Initialize to zero
    memset(out_affix, 0, sizeof(AffixTemplate_t));

    // Parse stat_key (the @key from DUF)
    out_affix->stat_key = d_StringInit();
    if (!out_affix->stat_key) {
        d_LogError("ParseAffixTemplate: Failed to allocate stat_key");
        return false;
    }
    d_StringSet(out_affix->stat_key, stat_key, 0);

    // Parse name (required)
    dDUFValue_t* name_node = d_DUFGetObjectItem(affix_node, "name");
    if (!name_node || !name_node->value_string) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' missing 'name' field", stat_key);
        d_StringDestroy(out_affix->stat_key);
        return false;
    }
    out_affix->name = d_StringInit();
    if (!out_affix->name) {
        d_LogError("ParseAffixTemplate: Failed to allocate name");
        d_StringDestroy(out_affix->stat_key);
        return false;
    }
    d_StringSet(out_affix->name, name_node->value_string, 0);

    // Parse description (required)
    dDUFValue_t* desc_node = d_DUFGetObjectItem(affix_node, "description");
    if (!desc_node || !desc_node->value_string) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' missing 'description' field", stat_key);
        d_StringDestroy(out_affix->stat_key);
        d_StringDestroy(out_affix->name);
        return false;
    }
    out_affix->description = d_StringInit();
    if (!out_affix->description) {
        d_LogError("ParseAffixTemplate: Failed to allocate description");
        d_StringDestroy(out_affix->stat_key);
        d_StringDestroy(out_affix->name);
        return false;
    }
    d_StringSet(out_affix->description, desc_node->value_string, 0);

    // Parse min_value (required)
    dDUFValue_t* min_node = d_DUFGetObjectItem(affix_node, "min_value");
    if (!min_node) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' missing 'min_value' field", stat_key);
        CleanupAffixTemplate(out_affix);
        return false;
    }
    out_affix->min_value = (int)min_node->value_int;

    // Parse max_value (required)
    dDUFValue_t* max_node = d_DUFGetObjectItem(affix_node, "max_value");
    if (!max_node) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' missing 'max_value' field", stat_key);
        CleanupAffixTemplate(out_affix);
        return false;
    }
    out_affix->max_value = (int)max_node->value_int;

    // Parse weight (required)
    dDUFValue_t* weight_node = d_DUFGetObjectItem(affix_node, "weight");
    if (!weight_node) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' missing 'weight' field", stat_key);
        CleanupAffixTemplate(out_affix);
        return false;
    }
    out_affix->weight = (int)weight_node->value_int;

    // Validate: min < max
    if (out_affix->min_value >= out_affix->max_value) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' has min_value (%d) >= max_value (%d)",
                   stat_key, out_affix->min_value, out_affix->max_value);
        CleanupAffixTemplate(out_affix);
        return false;
    }

    // Validate: weight > 0
    if (out_affix->weight <= 0) {
        d_LogErrorF("ParseAffixTemplate: Affix '%s' has invalid weight (%d)", stat_key, out_affix->weight);
        CleanupAffixTemplate(out_affix);
        return false;
    }

    d_LogDebugF("Parsed affix: %s (%s, %d-%d, weight %d)",
                stat_key, d_StringPeek(out_affix->name),
                out_affix->min_value, out_affix->max_value, out_affix->weight);

    return true;
}

// ============================================================================
// PUBLIC API
// ============================================================================

dDUFError_t* LoadAffixDatabase(const char* filepath, dDUFValue_t** out_db) {
    if (!filepath || !out_db) {
        d_LogError("LoadAffixDatabase: NULL parameters");
        return NULL;
    }

    dDUFError_t* err = d_DUFParseFile(filepath, out_db);
    if (err) {
        return err;  // Caller handles error
    }

    d_LogInfoF("Loaded affix database from %s", filepath);
    return NULL;
}

/**
 * LoadAffixTemplateFromDUF - Load affix template from DUF (enemy pattern)
 */
AffixTemplate_t* LoadAffixTemplateFromDUF(const char* stat_key) {
    // 1. Validate parameters
    if (!stat_key) {
        d_LogError("LoadAffixTemplateFromDUF: NULL stat_key");
        return NULL;
    }

    if (!g_affixes_db) {
        d_LogError("LoadAffixTemplateFromDUF: g_affixes_db is NULL (not loaded yet?)");
        return NULL;
    }

    // 2. Validate DUF lookup
    dDUFValue_t* affix_node = d_DUFGetObjectItem(g_affixes_db, stat_key);
    if (!affix_node) {
        d_LogErrorF("Affix '%s' not found in DUF database", stat_key);
        return NULL;
    }

    // 3. Validate allocation
    AffixTemplate_t* template = malloc(sizeof(AffixTemplate_t));
    if (!template) {
        d_LogFatal("LoadAffixTemplateFromDUF: Failed to allocate template");
        return NULL;
    }
    memset(template, 0, sizeof(AffixTemplate_t));

    // 4. Validate parsing
    if (!ParseAffixTemplate(affix_node, stat_key, template)) {
        d_LogErrorF("Failed to parse affix '%s'", stat_key);
        free(template);
        return NULL;
    }

    // 5. Validate critical fields (enemy pattern: catch parse bugs early)
    if (!template->name || !template->description) {
        d_LogErrorF("Affix '%s' has NULL name or description after parse (parse bug!)", stat_key);
        CleanupAffixTemplate(template);
        free(template);
        return NULL;
    }

    // 6. Validate value ranges
    if (template->min_value >= template->max_value) {
        d_LogErrorF("Affix '%s' has invalid range [%d, %d]",
                   stat_key, template->min_value, template->max_value);
        CleanupAffixTemplate(template);
        free(template);
        return NULL;
    }

    // 7. Validate weight
    if (template->weight <= 0) {
        d_LogErrorF("Affix '%s' has invalid weight (%d)", stat_key, template->weight);
        CleanupAffixTemplate(template);
        free(template);
        return NULL;
    }

    return template;
}

/**
 * GetAllAffixKeys - Get array of all affix keys for iteration
 */
dArray_t* GetAllAffixKeys(void) {
    if (!g_affixes_db) {
        d_LogError("GetAllAffixKeys: g_affixes_db is NULL");
        return NULL;
    }

    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    dArray_t* keys = d_ArrayInit(16, sizeof(const char*));
    if (!keys) {
        d_LogError("GetAllAffixKeys: Failed to create array");
        return NULL;
    }

    // Iterate DUF children and collect keys
    dDUFValue_t* child = g_affixes_db->child;
    int key_count = 0;
    while (child) {
        if (child->string) {  // @damage_bonus_percent → "damage_bonus_percent"
            const char* key = child->string;
            d_ArrayAppend(keys, &key);
            key_count++;
        }
        child = child->next;
    }

    if (key_count == 0) {
        d_LogWarning("GetAllAffixKeys: No affixes found in database");
    }

    return keys;
}

bool ValidateAffixDatabase(dDUFValue_t* affixes_db, char* out_error_msg, size_t error_msg_size) {
    if (!affixes_db || !out_error_msg) {
        return false;
    }

    // Iterate through all affix entries and validate
    dDUFValue_t* affix_entry = affixes_db->child;
    int validated_count = 0;

    while (affix_entry) {
        const char* stat_key = affix_entry->string;
        if (!stat_key) {
            affix_entry = affix_entry->next;
            continue;
        }

        // Try to parse affix (validation test)
        AffixTemplate_t test_affix = {0};
        if (!ParseAffixTemplate(affix_entry, stat_key, &test_affix)) {
            // Validation failed - write error message
            snprintf(out_error_msg, error_msg_size,
                    "Affix DUF Validation Failed\n\n"
                    "Affix: %s\n"
                    "File: data/affixes/combat_affixes.duf\n\n"
                    "Check console logs for details.\n\n"
                    "Common issues:\n"
                    "- Missing required fields (name, description, min/max, weight)\n"
                    "- min_value >= max_value\n"
                    "- weight <= 0",
                    stat_key);
            return false;
        }

        // Clean up test affix (we're just validating, not storing)
        CleanupAffixTemplate(&test_affix);
        validated_count++;
        affix_entry = affix_entry->next;
    }

    d_LogInfoF("✓ Affix Validation: All %d affixes valid", validated_count);
    return true;
}

void CleanupAffixTemplate(AffixTemplate_t* affix) {
    if (!affix) return;

    if (affix->stat_key) {
        d_StringDestroy(affix->stat_key);
        affix->stat_key = NULL;
    }
    if (affix->name) {
        d_StringDestroy(affix->name);
        affix->name = NULL;
    }
    if (affix->description) {
        d_StringDestroy(affix->description);
        affix->description = NULL;
    }
}

void CleanupAffixSystem(void) {
    // Enemy pattern: No table to cleanup, just DUF tree
    if (g_affixes_db) {
        d_DUFFree(g_affixes_db);
        g_affixes_db = NULL;
    }

    d_LogInfo("Affix system cleaned up");
}
