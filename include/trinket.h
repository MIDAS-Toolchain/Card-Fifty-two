#ifndef TRINKET_H
#define TRINKET_H

#include "common.h"
#include "game.h"  // For GameEvent_t

// Forward declarations
typedef struct Player Player_t;
typedef struct GameContext GameContext_t;
typedef struct Trinket Trinket_t;

// ============================================================================
// TRINKET ENUMS
// ============================================================================

/**
 * TrinketTargetType_t - What can trinket actives target?
 */
typedef enum {
    TRINKET_TARGET_NONE,        // No targeting (self-buff)
    TRINKET_TARGET_CARD,        // Target a card (player or dealer)
    TRINKET_TARGET_ENEMY,       // Target the enemy
    TRINKET_TARGET_HAND,        // Target entire hand (yours or dealer's)
} TrinketTargetType_t;

// ============================================================================
// TRINKET STRUCTURE
// ============================================================================

/**
 * Trinket_t - Equipment with passive and active effects
 *
 * Constitutional Amendment #1 (2025-11-14): VALUE TYPE (like Card_t, StatusEffectInstance_t)
 * - Global registry (g_trinket_templates) stores templates as VALUES
 * - Players store copies in trinket_slots (dArray_t of Trinket_t)
 * - Equipping COPIES template → player slot (each player has own instance)
 * - Passives triggered by GameEvent_t (reuse existing system)
 * - Actives manually triggered with targeting
 * - Tweening uses TweenFloatInArray() with slot_index (NO dangling pointers!)
 */
typedef struct Trinket {
    int trinket_id;                    // Unique ID (0-N)
    dString_t* name;                   // "Degenerate's Gambit"
    dString_t* description;            // Full description

    // Passive (event-triggered)
    GameEvent_t passive_trigger;       // Which event triggers passive?
    void (*passive_effect)(Player_t* player, GameContext_t* game, Trinket_t* self, size_t slot_index);
    dString_t* passive_description;    // "When you HIT on 15+: Deal X damage"

    // Active (player-activated)
    TrinketTargetType_t active_target_type;
    void (*active_effect)(Player_t* player, GameContext_t* game, void* target, Trinket_t* self, size_t slot_index);
    int active_cooldown_max;           // Turns until reusable
    int active_cooldown_current;       // Current cooldown (0 = ready)
    dString_t* active_description;     // "Double a card ≤5"

    // State tracking (per-trinket scaling)
    int passive_damage_bonus;          // For Degenerate: +5 per active use
    int total_damage_dealt;            // Total damage dealt this combat (for stats)
    // Future: Add more state fields as needed

    // Animation state (for shake/flash on proc - matches status effects/abilities)
    float shake_offset_x;              // X shake offset (tweened)
    float shake_offset_y;              // Y shake offset (tweened)
    float flash_alpha;                 // Red flash alpha (tweened, 0-255)

} Trinket_t;

// Global trinket template registry (Constitutional: value types)
// Templates copied into player slots - each player has own instance with own state
extern dTable_t* g_trinket_templates;  // trinket_id → Trinket_t (value type!)

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
 * CreateTrinket - Allocate and initialize a trinket
 *
 * @param trinket_id - Unique ID
 * @param name - Trinket name (copied to dString_t)
 * @param description - Full description (copied to dString_t)
 * @return Trinket_t* - Heap-allocated trinket, or NULL on failure
 *
 * Constitutional pattern: Heap-allocated pointer type
 */
Trinket_t* CreateTrinket(int trinket_id, const char* name, const char* description);

/**
 * DestroyTrinket - Free trinket resources
 *
 * @param trinket - Pointer to trinket pointer (double pointer for nulling)
 *
 * Destroys internal dString_t and frees trinket struct
 */
void DestroyTrinket(Trinket_t** trinket);

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
 * EquipTrinket - Equip trinket to player slot
 *
 * @param player - Player to equip trinket to
 * @param slot_index - Slot index (0-5)
 * @param trinket - Trinket to equip (pointer stored, not copied)
 * @return bool - true on success, false if invalid slot or NULL trinket
 */
bool EquipTrinket(Player_t* player, int slot_index, Trinket_t* trinket);

/**
 * UnequipTrinket - Remove trinket from player slot
 *
 * @param player - Player to unequip from
 * @param slot_index - Slot index (0-5)
 *
 * Sets slot to NULL (trinket still exists in registry)
 */
void UnequipTrinket(Player_t* player, int slot_index);

/**
 * GetEquippedTrinket - Get trinket in player slot
 *
 * @param player - Player to query
 * @param slot_index - Slot index (0-5)
 * @return Trinket_t* - Pointer to equipped trinket, or NULL if empty/invalid
 */
Trinket_t* GetEquippedTrinket(const Player_t* player, int slot_index);

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
