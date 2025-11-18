/*
 * Trinket System Implementation
 * Constitutional pattern: Heap-allocated pointer types in global registry
 */

#include "../include/trinket.h"
#include "../include/player.h"
#include "../include/trinkets/degenerateGambit.h"
#include "../include/scenes/sceneBlackjack.h"  // For GetTweenManager
#include "../include/tween/tween.h"            // For TweenFloat, TWEEN_EASE_*
#include <stdlib.h>

// ============================================================================
// GLOBAL REGISTRY
// ============================================================================

dTable_t* g_trinkets = NULL;  // trinket_id → Trinket_t*

// ============================================================================
// LIFECYCLE
// ============================================================================

void InitTrinketSystem(void) {
    if (g_trinkets) {
        d_LogWarning("InitTrinketSystem: g_trinkets already initialized");
        return;
    }

    // Constitutional pattern: dTable_t for O(1) lookup
    g_trinkets = d_InitTable(sizeof(int), sizeof(Trinket_t*),
                              d_HashInt, d_CompareInt, 16);

    if (!g_trinkets) {
        d_LogFatal("InitTrinketSystem: Failed to create g_trinkets table");
        return;
    }

    d_LogInfo("Trinket system initialized");

    // Register all trinkets
    CreateDegenerateGambitTrinket();  // ID 0 - Degenerate starter trinket
}

void CleanupTrinketSystem(void) {
    if (!g_trinkets) {
        return;
    }

    // Get all trinket IDs from table (Constitutional: use Daedalus helper)
    dArray_t* trinket_ids = d_GetAllKeysFromTable(g_trinkets);
    if (trinket_ids) {
        // Iterate through all actual trinket IDs (handles non-sequential IDs)
        for (size_t i = 0; i < trinket_ids->count; i++) {
            int* trinket_id = (int*)d_IndexDataFromArray(trinket_ids, i);
            if (trinket_id) {
                Trinket_t** trinket_ptr = (Trinket_t**)d_GetDataFromTable(g_trinkets, trinket_id);
                if (trinket_ptr && *trinket_ptr) {
                    DestroyTrinket(trinket_ptr);
                }
            }
        }
        d_DestroyArray(trinket_ids);
    }

    d_DestroyTable(&g_trinkets);
    g_trinkets = NULL;

    d_LogInfo("Trinket system cleaned up");
}

Trinket_t* CreateTrinket(int trinket_id, const char* name, const char* description) {
    if (!name || !description) {
        d_LogError("CreateTrinket: NULL name or description");
        return NULL;
    }

    Trinket_t* trinket = malloc(sizeof(Trinket_t));
    if (!trinket) {
        d_LogFatal("CreateTrinket: Failed to allocate Trinket_t");
        return NULL;
    }

    // Initialize ID
    trinket->trinket_id = trinket_id;

    // Initialize name (Constitutional: dString_t, not char[])
    trinket->name = d_StringInit();
    if (!trinket->name) {
        d_LogError("CreateTrinket: Failed to allocate name");
        free(trinket);
        return NULL;
    }
    d_StringSet(trinket->name, name, 0);

    // Initialize description
    trinket->description = d_StringInit();
    if (!trinket->description) {
        d_LogError("CreateTrinket: Failed to allocate description");
        d_StringDestroy(trinket->name);
        free(trinket);
        return NULL;
    }
    d_StringSet(trinket->description, description, 0);

    // Initialize passive (use ROUND_START as default "no-op" trigger)
    trinket->passive_trigger = GAME_EVENT_COMBAT_START;
    trinket->passive_effect = NULL;  // NULL effect = won't trigger
    trinket->passive_description = d_StringInit();
    if (!trinket->passive_description) {
        d_LogError("CreateTrinket: Failed to allocate passive_description");
        d_StringDestroy(trinket->name);
        d_StringDestroy(trinket->description);
        free(trinket);
        return NULL;
    }

    // Initialize active
    trinket->active_target_type = TRINKET_TARGET_NONE;
    trinket->active_effect = NULL;
    trinket->active_cooldown_max = 0;
    trinket->active_cooldown_current = 0;
    trinket->active_description = d_StringInit();
    if (!trinket->active_description) {
        d_LogError("CreateTrinket: Failed to allocate active_description");
        d_StringDestroy(trinket->name);
        d_StringDestroy(trinket->description);
        d_StringDestroy(trinket->passive_description);
        free(trinket);
        return NULL;
    }

    // Initialize state tracking
    trinket->passive_damage_bonus = 0;
    trinket->total_damage_dealt = 0;

    // Initialize animation state
    trinket->shake_offset_x = 0.0f;
    trinket->shake_offset_y = 0.0f;
    trinket->flash_alpha = 0.0f;

    // Register in global table
    d_SetDataInTable(g_trinkets, &trinket->trinket_id, &trinket);

    d_LogInfoF("Created trinket: %s (ID: %d)", d_StringPeek(trinket->name), trinket_id);

    return trinket;
}

void DestroyTrinket(Trinket_t** trinket) {
    if (!trinket || !*trinket) {
        return;
    }

    Trinket_t* t = *trinket;

    // Destroy dString_t resources
    if (t->name) d_StringDestroy(t->name);
    if (t->description) d_StringDestroy(t->description);
    if (t->passive_description) d_StringDestroy(t->passive_description);
    if (t->active_description) d_StringDestroy(t->active_description);

    // Free trinket struct
    free(t);
    *trinket = NULL;

    d_LogInfo("Trinket destroyed");
}

Trinket_t* GetTrinketByID(int trinket_id) {
    if (!g_trinkets) {
        d_LogError("GetTrinketByID: g_trinkets not initialized");
        return NULL;
    }

    Trinket_t** trinket_ptr = (Trinket_t**)d_GetDataFromTable(g_trinkets, &trinket_id);
    if (!trinket_ptr) {
        return NULL;  // Trinket not found
    }

    return *trinket_ptr;
}

// ============================================================================
// PLAYER TRINKET MANAGEMENT
// ============================================================================

bool EquipTrinket(Player_t* player, int slot_index, Trinket_t* trinket) {
    if (!player) {
        d_LogError("EquipTrinket: NULL player pointer");
        return false;
    }

    if (!trinket) {
        d_LogError("EquipTrinket: NULL trinket pointer");
        return false;
    }

    if (!player->trinket_slots) {
        d_LogError("EquipTrinket: player trinket_slots not initialized");
        return false;
    }

    if (slot_index < 0 || slot_index >= 6) {
        d_LogError("EquipTrinket: Invalid slot_index (must be 0-5)");
        return false;
    }

    // Store trinket pointer in slot (Constitutional: store pointer, not copy)
    if ((size_t)slot_index >= player->trinket_slots->count) {
        d_LogError("EquipTrinket: Slot index out of bounds");
        return false;
    }

    // Manually replace pointer at index (Daedalus doesn't have d_SetDataInArray)
    Trinket_t** slot_ptr = (Trinket_t**)d_IndexDataFromArray(player->trinket_slots, slot_index);
    if (!slot_ptr) {
        d_LogError("EquipTrinket: Failed to access slot");
        return false;
    }
    *slot_ptr = trinket;

    d_LogInfoF("Equipped trinket %s to slot %d", GetTrinketName(trinket), slot_index);

    return true;
}

void UnequipTrinket(Player_t* player, int slot_index) {
    if (!player || !player->trinket_slots) {
        return;
    }

    if (slot_index < 0 || slot_index >= 6) {
        d_LogError("UnequipTrinket: Invalid slot_index (must be 0-5)");
        return;
    }

    if ((size_t)slot_index >= player->trinket_slots->count) {
        return;
    }

    // Set slot to NULL (manually replace pointer)
    Trinket_t** slot_ptr = (Trinket_t**)d_IndexDataFromArray(player->trinket_slots, slot_index);
    if (slot_ptr) {
        *slot_ptr = NULL;
    }

    d_LogInfoF("Unequipped trinket from slot %d", slot_index);
}

Trinket_t* GetEquippedTrinket(const Player_t* player, int slot_index) {
    if (!player || !player->trinket_slots) {
        return NULL;
    }

    if (slot_index < 0 || slot_index >= 6) {
        return NULL;
    }

    if ((size_t)slot_index >= player->trinket_slots->count) {
        return NULL;
    }

    // Get pointer from array
    Trinket_t** trinket_ptr = (Trinket_t**)d_IndexDataFromArray(player->trinket_slots, slot_index);
    if (!trinket_ptr) {
        return NULL;
    }

    return *trinket_ptr;
}

int GetEmptyTrinketSlot(const Player_t* player) {
    if (!player || !player->trinket_slots) {
        return -1;
    }

    for (int i = 0; i < 6; i++) {
        if ((size_t)i >= player->trinket_slots->count) {
            break;
        }

        Trinket_t* trinket = GetEquippedTrinket(player, i);
        if (!trinket) {
            return i;  // Found empty slot
        }
    }

    return -1;  // All slots full
}

// ============================================================================
// TRIGGER SYSTEM
// ============================================================================

void CheckTrinketPassiveTriggers(Player_t* player, GameEvent_t event, GameContext_t* game) {
    if (!player) {
        d_LogDebug("CheckTrinketPassiveTriggers: NULL player");
        return;
    }

    if (!player->trinket_slots) {
        d_LogDebug("CheckTrinketPassiveTriggers: Player has no trinket_slots");
        return;
    }

    if (!game) {
        d_LogDebug("CheckTrinketPassiveTriggers: NULL game");
        return;
    }

    // Check class trinket first
    Trinket_t* class_trinket = GetClassTrinket(player);
    if (class_trinket && class_trinket->passive_trigger == event && class_trinket->passive_effect) {
        d_LogInfoF("Triggering CLASS trinket passive: %s (event: %d)",
                   GetTrinketName(class_trinket), event);

        // Fire passive effect callback (use slot_index 0 for class trinket - doesn't matter for tweening)
        class_trinket->passive_effect(player, game, class_trinket, 0);
    }

    // Iterate all equipped regular trinkets
    for (int i = 0; i < 6; i++) {
        Trinket_t* trinket = GetEquippedTrinket(player, i);
        if (!trinket) {
            continue;  // Empty slot
        }

        // Check if passive triggers on this event
        if (trinket->passive_trigger == event && trinket->passive_effect) {
            d_LogInfoF("Triggering trinket passive: %s (event: %d)",
                       GetTrinketName(trinket), event);

            // Fire passive effect callback with slot_index for safe tweening
            trinket->passive_effect(player, game, trinket, (size_t)i);
        }
    }
}

void TickTrinketCooldowns(Player_t* player) {
    if (!player || !player->trinket_slots) {
        return;
    }

    // Tick class trinket cooldown first
    Trinket_t* class_trinket = GetClassTrinket(player);
    if (class_trinket && class_trinket->active_cooldown_current > 0) {
        class_trinket->active_cooldown_current--;
        d_LogInfoF("CLASS trinket %s cooldown: %d",
                   GetTrinketName(class_trinket), class_trinket->active_cooldown_current);
    }

    // Decrement all regular trinket cooldowns
    for (int i = 0; i < 6; i++) {
        Trinket_t* trinket = GetEquippedTrinket(player, i);
        if (!trinket) {
            continue;  // Empty slot
        }

        if (trinket->active_cooldown_current > 0) {
            trinket->active_cooldown_current--;
            d_LogInfoF("Trinket %s cooldown: %d",
                       GetTrinketName(trinket), trinket->active_cooldown_current);
        }
    }
}

// ============================================================================
// ACTIVE TARGETING
// ============================================================================

bool CanActivateTrinket(const Trinket_t* trinket) {
    if (!trinket) {
        return false;
    }

    return trinket->active_cooldown_current == 0;
}

void ActivateTrinket(Player_t* player, GameContext_t* game, int slot_index, void* target) {
    if (!player || !game) {
        d_LogError("ActivateTrinket: NULL player or game");
        return;
    }

    Trinket_t* trinket = GetEquippedTrinket(player, slot_index);
    if (!trinket) {
        d_LogError("ActivateTrinket: No trinket in slot");
        return;
    }

    if (!CanActivateTrinket(trinket)) {
        d_LogWarning("ActivateTrinket: Trinket on cooldown");
        return;
    }

    if (!trinket->active_effect) {
        d_LogWarning("ActivateTrinket: Trinket has no active effect");
        return;
    }

    d_LogInfoF("Activating trinket: %s", GetTrinketName(trinket));

    // Fire active effect callback
    trinket->active_effect(player, game, target, trinket, (size_t)slot_index);

    // Set cooldown
    trinket->active_cooldown_current = trinket->active_cooldown_max;
}

// ============================================================================
// QUERIES
// ============================================================================

const char* GetTrinketName(const Trinket_t* trinket) {
    if (!trinket || !trinket->name) {
        return "Unknown Trinket";
    }
    return d_StringPeek(trinket->name);
}

const char* GetTrinketDescription(const Trinket_t* trinket) {
    if (!trinket || !trinket->description) {
        return "";
    }
    return d_StringPeek(trinket->description);
}

int GetTrinketCooldown(const Trinket_t* trinket) {
    if (!trinket) {
        return 0;
    }
    return trinket->active_cooldown_current;
}

bool IsTrinketReady(const Trinket_t* trinket) {
    if (!trinket) {
        return false;
    }
    return trinket->active_cooldown_current == 0;
}

// ============================================================================
// CLASS TRINKET SYSTEM
// ============================================================================

Trinket_t* GetClassTrinket(const Player_t* player) {
    if (!player) {
        return NULL;
    }
    return player->class_trinket;
}

bool EquipClassTrinket(Player_t* player, Trinket_t* trinket) {
    if (!player || !trinket) {
        d_LogError("EquipClassTrinket: NULL player or trinket");
        return false;
    }

    player->class_trinket = trinket;
    d_LogInfoF("Equipped class trinket: %s", GetTrinketName(trinket));
    return true;
}
