#ifndef AFFIX_LOADER_H
#define AFFIX_LOADER_H

#include "../structs.h"
#include <Daedalus.h>

// Global affix DUF registry (Enemy pattern: DUF tree only, no table)
extern dDUFValue_t* g_affixes_db;  // Raw DUF data (freed on shutdown)

/**
 * LoadAffixDatabase - Parse affix DUF file
 *
 * @param filepath - Path to DUF file (e.g., "data/affixes/combat_affixes.duf")
 * @param out_db - Output pointer to parsed DUF database
 * @return dDUFError_t* - Error info on failure, NULL on success
 *
 * Enemy pattern: Parses DUF file only, does NOT create table.
 * Affixes parsed on-demand via LoadAffixTemplateFromDUF().
 */
dDUFError_t* LoadAffixDatabase(const char* filepath, dDUFValue_t** out_db);

/**
 * LoadAffixTemplateFromDUF - Load affix template from DUF (enemy pattern)
 *
 * @param stat_key - Affix key string ("damage_bonus_percent", etc)
 * @return AffixTemplate_t* - Heap-allocated template, or NULL if not found
 *
 * IMPORTANT: Returned template is HEAP-ALLOCATED. Caller must:
 * 1. Call CleanupAffixTemplate() to free internal dStrings
 * 2. Call free() to free the struct itself
 *
 * Enemy pattern: On-demand parsing, validates all fields before returning.
 * Returns NULL on any error (missing key, parse failure, invalid values).
 */
AffixTemplate_t* LoadAffixTemplateFromDUF(const char* stat_key);

/**
 * GetAllAffixKeys - Get array of all affix keys for iteration
 *
 * @return dArray_t* - Array of const char* (affix keys), or NULL on failure
 *
 * Returns array of string pointers to DUF keys (valid until DUF freed).
 * Used for weighted random selection during trinket affix rolling.
 * Caller must destroy array with d_ArrayDestroy() after use.
 */
dArray_t* GetAllAffixKeys(void);

/**
 * ValidateAffixDatabase - Validate all affixes parse correctly
 *
 * @param affixes_db - Parsed DUF database
 * @param out_error_msg - Buffer for error message
 * @param error_msg_size - Size of error buffer
 * @return bool - true if valid, false if any affix fails validation
 *
 * Attempts to parse all affixes. If any fail, writes detailed error message.
 */
bool ValidateAffixDatabase(dDUFValue_t* affixes_db, char* out_error_msg, size_t error_msg_size);

/**
 * CleanupAffixTemplate - Free dStrings inside AffixTemplate_t
 *
 * @param affix - Affix template to clean up
 *
 * Frees internal dString_t but NOT the struct itself (stored by value in table).
 */
void CleanupAffixTemplate(AffixTemplate_t* affix);

/**
 * CleanupAffixSystem - Free all affix templates and DUF database
 *
 * Called on shutdown to free g_affix_templates and g_affixes_db.
 */
void CleanupAffixSystem(void);

#endif // AFFIX_LOADER_H
