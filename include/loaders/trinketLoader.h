#ifndef TRINKET_LOADER_H
#define TRINKET_LOADER_H

#include "../structs.h"
#include <Daedalus.h>

// Global trinket DUF registry (Enemy pattern: DUF tree only, no table)
extern dDUFValue_t* g_trinkets_db;  // Raw DUF data (source of truth)
extern dArray_t* g_trinket_key_cache;  // Cached trinket keys for iteration

/**
 * TrinketEffectTypeFromString - Convert string → TrinketEffectType_t enum
 *
 * @param str - Effect type string ("add_chips", "apply_status", etc)
 * @return TrinketEffectType_t - Enum value, or TRINKET_EFFECT_NONE if unknown
 *
 * Logs warning if unknown string encountered.
 */
TrinketEffectType_t TrinketEffectTypeFromString(const char* str);

/**
 * TrinketRarityFromString - Convert string → TrinketRarity_t enum
 *
 * @param str - Rarity string ("common", "uncommon", "rare", "legendary", "event")
 * @return TrinketRarity_t - Enum value, or TRINKET_RARITY_COMMON if unknown
 *
 * Logs warning if unknown string encountered.
 */
TrinketRarity_t TrinketRarityFromString(const char* str);

/**
 * LoadTrinketDatabase - Parse trinket DUF file
 *
 * @param filepath - Path to DUF file (e.g., "data/trinkets/combat_trinkets.duf")
 * @param out_db - Output pointer to parsed DUF database
 * @return dDUFError_t* - Error info on failure, NULL on success
 *
 * Parses DUF file but does NOT populate g_trinket_templates.
 * Call PopulateTrinketTemplates() after successful parse.
 */
dDUFError_t* LoadTrinketDatabase(const char* filepath, dDUFValue_t** out_db);

/**
 * PopulateTrinketTemplates - Load all trinkets into g_trinket_templates table
 *
 * @param trinkets_db - Parsed DUF database
 * @return bool - true on success, false on failure
 *
 * Iterates through all @trinket entries in DUF and parses them into g_trinket_templates.
 */
bool PopulateTrinketTemplates(dDUFValue_t* trinkets_db);

/**
 * MergeTrinketDatabases - Combine multiple DUF databases (combat + event)
 *
 * @param combat_db - Combat trinkets database
 * @param event_db - Event trinkets database
 *
 * Populates g_trinket_templates from both databases.
 * Call after loading both DUF files.
 */
bool MergeTrinketDatabases(dDUFValue_t* combat_db, dDUFValue_t* event_db);

/**
 * ValidateTrinketDatabase - Validate all trinkets parse correctly
 *
 * @param trinkets_db - Parsed DUF database
 * @param out_error_msg - Buffer for error message
 * @param error_msg_size - Size of error buffer
 * @return bool - true if valid, false if any trinket fails validation
 *
 * Attempts to parse all trinkets. If any fail, writes detailed error message.
 */
bool ValidateTrinketDatabase(dDUFValue_t* trinkets_db, char* out_error_msg, size_t error_msg_size);

/**
 * LoadTrinketTemplateFromDUF - Load trinket template from DUF (enemy pattern)
 *
 * Allocates a new TrinketTemplate_t on the heap and parses it from DUF.
 * Caller must cleanup with CleanupTrinketTemplate() and free().
 *
 * @param trinket_key - Trinket key string ("lucky_chip", "loaded_dice", etc)
 * @return TrinketTemplate_t* - Heap-allocated template, or NULL if not found
 */
TrinketTemplate_t* LoadTrinketTemplateFromDUF(const char* trinket_key);

/**
 * GetTrinketTemplate - Wrapper for LoadTrinketTemplateFromDUF (backward compatibility)
 *
 * @param trinket_key - Trinket key string ("lucky_chip", "loaded_dice", etc)
 * @return TrinketTemplate_t* - Heap-allocated template, or NULL if not found
 */
TrinketTemplate_t* GetTrinketTemplate(const char* trinket_key);

/**
 * CleanupTrinketTemplate - Free dStrings inside TrinketTemplate_t
 *
 * @param trinket - Trinket template to clean up
 *
 * Frees internal dString_t but NOT the struct itself (stored by value in table).
 */
void CleanupTrinketTemplate(TrinketTemplate_t* trinket);

/**
 * CleanupTrinketSystem - Free all trinket templates and DUF database
 *
 * Called on shutdown to free g_trinket_templates and g_trinkets_db.
 */
void CleanupTrinketLoaderSystem(void);

#endif // TRINKET_LOADER_H
