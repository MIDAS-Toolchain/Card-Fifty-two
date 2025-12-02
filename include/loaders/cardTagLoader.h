#ifndef CARD_TAG_LOADER_H
#define CARD_TAG_LOADER_H

#include "../common.h"
#include "../cardTags.h"

/**
 * Card Tag Loader (ADR-019 Compliant)
 *
 * Loads card tag definitions from data/card_tags/tags.duf at startup.
 * Tags define trigger/effect patterns similar to enemy abilities.
 *
 * Pattern: On-demand DUF loader (parse at startup, load on demand)
 *
 * Tag Structure (DUF):
 *   @cursed {
 *       display_name: "Cursed"
 *       description: "10 damage to enemy when drawn"
 *       color_r: 165
 *       color_g: 48
 *       color_b: 48
 *       trigger: { type: "on_draw" }
 *       effects: [ { type: "damage_enemy", value: 10 } ]
 *   }
 *
 * Trigger Types:
 *   - "on_draw": Fires when THIS card is drawn (CURSED, VAMPIRIC)
 *   - "passive": Always active while in hand (LUCKY, BRUTAL, DOUBLED)
 *
 * Trigger Scopes:
 *   - "global": Effect stacks for ALL cards with tag (LUCKY, BRUTAL)
 *   - "single_card": Effect only applies to THIS card (DOUBLED)
 *
 * Effect Types:
 *   - "damage_enemy": Deal damage to current enemy
 *   - "add_chips": Grant chips to player
 *   - "add_crit_percent": Increase crit chance
 *   - "add_damage_percent": Increase damage
 *   - "multiply_card_value": Multiply card value for hand calculation
 */

// ============================================================================
// GLOBAL DATABASE
// ============================================================================

/**
 * g_card_tags_db - Global DUF database for tag definitions
 *
 * Loaded at startup via LoadCardTagDatabase().
 * Contains 5 tags: cursed, vampiric, lucky, brutal, doubled
 */
extern dDUFValue_t* g_card_tags_db;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * LoadCardTagDatabase - Parse tags.duf file
 *
 * @param filepath - Path to tags.duf (e.g., "data/card_tags/tags.duf")
 * @param out_db - Output: parsed DUF database
 * @return dDUFError_t* on failure, NULL on success
 *
 * Call once during game initialization (after DUF system init).
 * Sets g_card_tags_db global.
 *
 * Example:
 *   dDUFError_t* err = LoadCardTagDatabase("data/card_tags/tags.duf", &g_card_tags_db);
 *   if (err) { ShowErrorDialog(err->message); }
 */
dDUFError_t* LoadCardTagDatabase(const char* filepath, dDUFValue_t** out_db);

/**
 * ValidateCardTagDatabase - Verify all 5 tags present with valid structure
 *
 * @param db - Parsed DUF database
 * @param out_error_msg - Output: error message buffer (if validation fails)
 * @param error_msg_size - Size of error message buffer
 * @return true if valid, false if validation failed
 *
 * Checks:
 *   - All 5 tags present (@cursed, @vampiric, @lucky, @brutal, @doubled)
 *   - Each tag has: display_name, description, color_r/g/b, trigger, effects
 *   - Trigger has valid type ("on_draw" or "passive")
 *   - Effects array not empty
 *   - Color values in range 0-255
 *
 * Call after LoadCardTagDatabase() before game starts.
 *
 * Example:
 *   char err_msg[256];
 *   if (!ValidateCardTagDatabase(g_card_tags_db, err_msg, sizeof(err_msg))) {
 *       ShowErrorDialog(err_msg);
 *   }
 */
bool ValidateCardTagDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size);

/**
 * CleanupCardTagSystem - Free tag database
 *
 * Call once during game shutdown.
 * Frees g_card_tags_db and sets to NULL.
 */
void CleanupCardTagSystem(void);

/**
 * LoadCardTagFromDUF - Load tag metadata from database (validation helper)
 *
 * @param tag - Tag enum
 * @return true if tag exists and valid, false otherwise
 *
 * Note: This is primarily used by ValidateCardTagDatabase() for testing.
 * Most code should use GetTagDisplayName/Color/Description() for queries.
 *
 * Example: LoadCardTagFromDUF(CARD_TAG_CURSED) → true (if valid in database)
 */
bool LoadCardTagFromDUF(CardTag_t tag);

// ============================================================================
// TAG METADATA QUERIES
// ============================================================================

/**
 * GetTagDisplayName - Get tag display name from DUF
 *
 * @param tag - Tag enum
 * @return const char* - Display name ("Cursed", "Lucky", etc.)
 *
 * Replaces hardcoded GetCardTagName() switch statement.
 * Returns from DUF database (cached for performance).
 *
 * Example: GetTagDisplayName(CARD_TAG_CURSED) → "Cursed"
 */
const char* GetTagDisplayName(CardTag_t tag);

/**
 * GetTagDescription - Get tag effect description from DUF
 *
 * @param tag - Tag enum
 * @return const char* - Description ("10 damage to enemy when drawn")
 *
 * Replaces hardcoded GetCardTagDescription() switch statement.
 * Returns from DUF database.
 *
 * Example: GetTagDescription(CARD_TAG_CURSED) → "10 damage to enemy when drawn"
 */
const char* GetTagDescription(CardTag_t tag);

/**
 * GetTagColor - Get tag UI color from DUF
 *
 * @param tag - Tag enum
 * @param out_r - Output: red channel (0-255)
 * @param out_g - Output: green channel (0-255)
 * @param out_b - Output: blue channel (0-255)
 *
 * Replaces hardcoded GetCardTagColor() switch statement.
 * Returns RGB values from DUF database.
 *
 * Example: GetTagColor(CARD_TAG_CURSED, &r, &g, &b) → r=165, g=48, b=48
 */
void GetTagColor(CardTag_t tag, int* out_r, int* out_g, int* out_b);

// ============================================================================
// TAG TRIGGER/EFFECT QUERIES
// ============================================================================

/**
 * GetTagTriggerType - Get tag trigger type from DUF
 *
 * @param tag - Tag enum
 * @return const char* - Trigger type ("on_draw" or "passive")
 *
 * Used by ProcessCardTagEffects() to determine execution pattern.
 *
 * Example: GetTagTriggerType(CARD_TAG_CURSED) → "on_draw"
 *          GetTagTriggerType(CARD_TAG_LUCKY) → "passive"
 */
const char* GetTagTriggerType(CardTag_t tag);

/**
 * GetTagTriggerScope - Get tag trigger scope from DUF
 *
 * @param tag - Tag enum
 * @return const char* - Trigger scope ("global", "single_card", or NULL if not specified)
 *
 * Used for passive tags to determine aggregation behavior.
 *
 * Example: GetTagTriggerScope(CARD_TAG_LUCKY) → "global"
 *          GetTagTriggerScope(CARD_TAG_DOUBLED) → "single_card"
 *          GetTagTriggerScope(CARD_TAG_CURSED) → NULL (on_draw has no scope)
 */
const char* GetTagTriggerScope(CardTag_t tag);

/**
 * GetTagDuration - Get tag duration from DUF
 *
 * @param tag - Tag enum
 * @return const char* - Duration ("one_turn" or NULL if permanent)
 *
 * Used to auto-remove tags after hand resolution.
 *
 * Example: GetTagDuration(CARD_TAG_DOUBLED) → "one_turn"
 *          GetTagDuration(CARD_TAG_CURSED) → NULL (permanent)
 */
const char* GetTagDuration(CardTag_t tag);

/**
 * GetTagEffects - Get tag effects array from DUF
 *
 * @param tag - Tag enum
 * @return dDUFValue_t* - Effects array (DUF_ARRAY type), or NULL if not found
 *
 * Used by ProcessCardTagEffects() to execute tag logic.
 *
 * Each effect has:
 *   - type: "damage_enemy", "add_chips", "add_crit_percent", etc.
 *   - value/multiplier: numeric parameter
 *   - stacking (optional): "multiply" for additive stacking
 *
 * Example:
 *   dDUFValue_t* effects = GetTagEffects(CARD_TAG_CURSED);
 *   for (size_t i = 0; i < effects->as.array->count; i++) {
 *       dDUFValue_t* effect = d_ArrayGet(effects->as.array, i);
 *       const char* type = d_DUFGet(effect, "type")->as.string;
 *       // Execute effect...
 *   }
 */
dDUFValue_t* GetTagEffects(CardTag_t tag);

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * CardTagToString - Convert CardTag_t to DUF key
 *
 * @param tag - Tag enum
 * @return const char* - DUF key ("cursed", "vampiric", etc.)
 *
 * Internal helper for DUF lookups.
 * Maps enum to lowercase DUF key names.
 *
 * Example: CardTagToString(CARD_TAG_CURSED) → "cursed"
 */
const char* CardTagToString(CardTag_t tag);

#endif // CARD_TAG_LOADER_H
