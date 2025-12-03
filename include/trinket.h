#ifndef TRINKET_H
#define TRINKET_H

#include "common.h"

// NOTE: Trinket_t, TrinketRarity_t, TrinketTargetType_t are defined in structs.h
// (moved there to resolve circular dependency: trinket.h → common.h → structs.h → Trinket_t)

// Global trinket template registry (Constitutional: value types, NOT pointers!)
// Templates stored BY VALUE in table - players copy template → their slots
// Pattern matches g_players (ADR-003): Store by value, return pointer to value in table
extern dTable_t* g_trinket_templates;  // trinket_id → Trinket_t (stored BY VALUE!)

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * InitTrinketSystem - Initialize global trinket registry
 *
 * Creates g_trinkets table and registers all trinkets
 * Called once in main.c Initialize()
 */
void InitTrinketSystem(void);

/**
 * CleanupTrinketSystem - Free all trinkets and destroy registry
 *
 * Called once in main.c Cleanup()
 */
void CleanupTrinketSystem(void);

/**
 * CreateTrinketTemplate - Create and register trinket template
 *
 * @param trinket_id - Unique ID
 * @param name - Trinket name (copied to dString_t)
 * @param description - Full description (copied to dString_t)
 * @param rarity - Rarity tier (common/uncommon/rare/legendary)
 * @return Trinket_t* - Pointer to VALUE in g_trinket_templates table, or NULL on failure
 *
 * Constitutional pattern: Returns pointer to VALUE stored in table (like g_players)
 * Template is stored BY VALUE in g_trinket_templates, players copy it to their slots
 * NO malloc() - builds on stack, stores by value via d_TableSet()
 */
Trinket_t* CreateTrinketTemplate(int trinket_id, const char* name, const char* description, TrinketRarity_t rarity);

/**
 * CleanupTrinketValue - Free dString_t resources inside trinket value
 *
 * @param trinket - Pointer to trinket value
 *
 * Frees internal dString_t but NOT the trinket struct itself
 * (struct is stored by value in table/array, Daedalus owns it)
 * Pattern matches status effect cleanup
 */
void CleanupTrinketValue(Trinket_t* trinket);

/**
 * GetTrinketByID - Lookup trinket by ID from registry
 *
 * @param trinket_id - Trinket ID to find
 * @return Trinket_t* - Pointer to trinket, or NULL if not found
 */
Trinket_t* GetTrinketByID(int trinket_id);

// ============================================================================
// PLAYER TRINKET MANAGEMENT
// ============================================================================

/**
 * GetEmptyTrinketSlot - Find first empty slot in player's trinkets
 *
 * @param player - Player to search
 * @return int - First empty slot index (0-5), or -1 if all slots full
 */
int GetEmptyTrinketSlot(const Player_t* player);

// ============================================================================
// TRIGGER SYSTEM (Integrates with GameEvent_t)
// ============================================================================

/**
 * CheckTrinketPassiveTriggers - Check if player's trinkets should trigger on event
 *
 * @param player - Player whose trinkets to check
 * @param event - Game event that occurred
 * @param game - Game context (for effect execution)
 *
 * Iterates player's equipped trinkets, fires matching passive effects
 * Called by Game_TriggerEvent() when gameplay events occur
 */
void CheckTrinketPassiveTriggers(Player_t* player, GameEvent_t event, GameContext_t* game);

/**
 * TickTrinketCooldowns - Decrement all active cooldowns by 1
 *
 * @param player - Player whose trinkets to tick
 *
 * Called at start of each player turn
 * Cooldowns clamped to 0 (never negative)
 */
void TickTrinketCooldowns(Player_t* player);

// ============================================================================
// MODIFIER SYSTEM (Win/Loss Modifiers - REMOVED)
// ============================================================================
//
// ModifyWinningsWithTrinkets() and ModifyLossesWithTrinkets() have been removed.
// Elite Membership and similar trinkets now use the standard DUF event trigger system:
// - PLAYER_WIN → ExecuteAddChipsPercent() (adds bonus chips + tracks stats)
// - PLAYER_LOSS → ExecuteRefundChipsPercent() (refunds chips + tracks stats)
//
// This makes all trinkets consistent (Lucky Chip, Insurance Policy, etc.) and
// fixes stats tracking bugs. See src/trinket.c for implementation details.
// ============================================================================

// ============================================================================
// ACTIVE TARGETING
// ============================================================================

/**
 * CanActivateTrinket - Check if trinket can be activated (cooldown ready)
 *
 * @param trinket - Trinket to check
 * @return bool - true if active_cooldown_current == 0, false otherwise
 */
bool CanActivateTrinket(const Trinket_t* trinket);

/**
 * ActivateTrinket - Activate trinket's active effect on target
 *
 * @param player - Player activating trinket
 * @param game - Game context
 * @param slot_index - Slot index (0-5) of trinket to activate
 * @param target - Target for active effect (type depends on trinket->active_target_type)
 *
 * Fires active_effect callback, sets cooldown to max
 * Does nothing if trinket not ready or slot empty
 */
void ActivateTrinket(Player_t* player, GameContext_t* game, int slot_index, void* target);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * GetTrinketName - Get trinket name as const char*
 *
 * @param trinket - Trinket to query
 * @return const char* - Trinket name, or "Unknown Trinket" if NULL
 */
const char* GetTrinketName(const Trinket_t* trinket);

/**
 * GetTrinketDescription - Get trinket description
 *
 * @param trinket - Trinket to query
 * @return const char* - Full description, or empty string if NULL
 */
const char* GetTrinketDescription(const Trinket_t* trinket);

/**
 * GetTrinketCooldown - Get current cooldown value
 *
 * @param trinket - Trinket to query
 * @return int - Current cooldown (0 = ready, >0 = not ready)
 */
int GetTrinketCooldown(const Trinket_t* trinket);

/**
 * IsTrinketReady - Check if trinket is ready to activate
 *
 * @param trinket - Trinket to check
 * @return bool - true if cooldown is 0, false otherwise
 */
bool IsTrinketReady(const Trinket_t* trinket);

/**
 * GetTrinketRarityName - Get human-readable rarity name
 *
 * @param rarity - Rarity tier
 * @return const char* - "Common", "Uncommon", "Rare", or "Legendary"
 */
const char* GetTrinketRarityName(TrinketRarity_t rarity);

/**
 * GetTrinketRarityColor - Get RGB color for rarity display
 *
 * @param rarity - Rarity tier
 * @param r, g, b - Output color values (0-255)
 */
void GetTrinketRarityColor(TrinketRarity_t rarity, int* r, int* g, int* b);

// ============================================================================
// CLASS TRINKET SYSTEM
// ============================================================================

/**
 * GetClassTrinket - Get player's class trinket
 *
 * @param player - Player to query
 * @return Trinket_t* - Pointer to class trinket, or NULL if none equipped
 */
Trinket_t* GetClassTrinket(const Player_t* player);

/**
 * EquipClassTrinket - Equip trinket to player's class trinket slot
 *
 * @param player - Player to equip trinket to
 * @param trinket - Trinket to equip (pointer stored, not copied)
 * @return bool - true on success, false if NULL player or trinket
 */
bool EquipClassTrinket(Player_t* player, Trinket_t* trinket);

#endif // TRINKET_H
