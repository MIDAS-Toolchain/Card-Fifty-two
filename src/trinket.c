/*
 * Trinket System Implementation
 * Constitutional pattern: Heap-allocated pointer types in global registry
 */

#include "../include/trinket.h"
#include "../include/player.h"
#include "../include/trinkets/degenerateGambit.h"
#include "../include/trinkets/eliteMembership.h"
#include "../include/trinkets/stackTrace.h"
#include "../include/scenes/sceneBlackjack.h"  // For GetTweenManager
#include "../include/tween/tween.h"            // For TweenFloat, TWEEN_EASE_*
#include <stdlib.h>

// ============================================================================
// GLOBAL REGISTRY
// ============================================================================

dTable_t* g_trinket_templates = NULL;  // trinket_id → Trinket_t (BY VALUE!)

// ============================================================================
// LIFECYCLE
// ============================================================================

void InitTrinketSystem(void) {
    if (g_trinket_templates) {
        d_LogWarning("InitTrinketSystem: g_trinket_templates already initialized");
        return;
    }

    // Constitutional pattern: Store trinket templates BY VALUE (not pointers!)
    // Matches g_players pattern (ADR-003)
    g_trinket_templates = d_TableInit(sizeof(int), sizeof(Trinket_t),
                                       d_HashInt, d_CompareInt, 16);

    if (!g_trinket_templates) {
        d_LogFatal("InitTrinketSystem: Failed to create g_trinket_templates table");
        return;
    }

    d_LogInfo("Trinket system initialized");

    // Register all trinket templates
    CreateDegenerateGambitTrinket();  // ID 0 - Degenerate starter trinket
    CreateEliteMembershipTrinket();   // ID 1 - Elite Membership (House Odds event)
    CreateStackTraceTrinket();        // ID 2 - Stack Trace (System Maintenance event)
}

void CleanupTrinketSystem(void) {
    if (!g_trinket_templates) {
        return;
    }

    // Get all trinket IDs from table (Constitutional: use Daedalus helper)
    dArray_t* trinket_ids = d_TableGetAllKeys(g_trinket_templates);
    if (trinket_ids) {
        // Iterate through all templates and clean up internal dStrings
        for (size_t i = 0; i < trinket_ids->count; i++) {
            int* trinket_id = (int*)d_ArrayGet(trinket_ids, i);
            if (trinket_id) {
                Trinket_t* trinket = (Trinket_t*)d_TableGet(g_trinket_templates, trinket_id);
                if (trinket) {
                    CleanupTrinketValue(trinket);  // Free dStrings inside
                }
            }
        }
        d_ArrayDestroy(trinket_ids);  // Pass pointer directly, not address
    }

    d_TableDestroy(&g_trinket_templates);
    g_trinket_templates = NULL;

    d_LogInfo("Trinket system cleaned up");
}

Trinket_t* CreateTrinketTemplate(int trinket_id, const char* name, const char* description, TrinketRarity_t rarity) {
    if (!name || !description) {
        d_LogError("CreateTrinketTemplate: NULL name or description");
        return NULL;
    }

    // Constitutional pattern: Build template on STACK, store BY VALUE (NO malloc!)
    Trinket_t template = {0};

    // Initialize ID and rarity
    template.trinket_id = trinket_id;
    template.rarity = rarity;

    // Initialize name (Constitutional: dString_t, not char[])
    template.name = d_StringInit();
    if (!template.name) {
        d_LogError("CreateTrinketTemplate: Failed to allocate name");
        return NULL;
    }
    d_StringSet(template.name, name, 0);

    // Initialize description
    template.description = d_StringInit();
    if (!template.description) {
        d_LogError("CreateTrinketTemplate: Failed to allocate description");
        d_StringDestroy(template.name);
        return NULL;
    }
    d_StringSet(template.description, description, 0);

    // Initialize passive (use COMBAT_START as default "no-op" trigger)
    template.passive_trigger = GAME_EVENT_COMBAT_START;
    template.passive_effect = NULL;  // NULL effect = won't trigger
    template.passive_description = d_StringInit();
    if (!template.passive_description) {
        d_LogError("CreateTrinketTemplate: Failed to allocate passive_description");
        d_StringDestroy(template.name);
        d_StringDestroy(template.description);
        return NULL;
    }

    // Initialize active
    template.active_target_type = TRINKET_TARGET_NONE;
    template.active_effect = NULL;
    template.active_cooldown_max = 0;
    template.active_cooldown_current = 0;
    template.active_description = d_StringInit();
    if (!template.active_description) {
        d_LogError("CreateTrinketTemplate: Failed to allocate active_description");
        d_StringDestroy(template.name);
        d_StringDestroy(template.description);
        d_StringDestroy(template.passive_description);
        return NULL;
    }

    // Initialize state tracking (will be reset per-instance when equipped)
    template.passive_damage_bonus = 0;
    template.total_damage_dealt = 0;

    // Initialize animation state
    template.shake_offset_x = 0.0f;
    template.shake_offset_y = 0.0f;
    template.flash_alpha = 0.0f;

    // Initialize additional stats (at end for binary compatibility)
    template.total_bonus_chips = 0;
    template.total_refunded_chips = 0;

    // Store template BY VALUE in global table (Constitutional pattern!)
    d_TableSet(g_trinket_templates, &template.trinket_id, &template);

    // Return pointer to VALUE stored in table (like g_players)
    Trinket_t* stored_template = (Trinket_t*)d_TableGet(g_trinket_templates, &trinket_id);
    if (!stored_template) {
        d_LogError("CreateTrinketTemplate: Failed to store template in table");
        CleanupTrinketValue(&template);  // Clean up dStrings before returning
        return NULL;
    }

    d_LogInfoF("Created trinket template: %s (ID: %d)", d_StringPeek(stored_template->name), trinket_id);

    return stored_template;
}

void CleanupTrinketValue(Trinket_t* trinket) {
    if (!trinket) {
        return;
    }

    // Free internal dString_t resources (Constitutional: Daedalus owns the struct itself)
    if (trinket->name) {
        d_StringDestroy(trinket->name);
        trinket->name = NULL;
    }
    if (trinket->description) {
        d_StringDestroy(trinket->description);
        trinket->description = NULL;
    }
    if (trinket->passive_description) {
        d_StringDestroy(trinket->passive_description);
        trinket->passive_description = NULL;
    }
    if (trinket->active_description) {
        d_StringDestroy(trinket->active_description);
        trinket->active_description = NULL;
    }

    // DO NOT free trinket struct itself - it's stored by value in table/array
    // Daedalus will free the struct when table/array is destroyed
    d_LogDebug("Cleaned up trinket value (dStrings freed, struct preserved)");
}

Trinket_t* GetTrinketByID(int trinket_id) {
    if (!g_trinket_templates) {
        d_LogError("GetTrinketByID: g_trinket_templates not initialized");
        return NULL;
    }

    // Constitutional pattern: Table stores VALUES, return pointer to value in table
    Trinket_t* trinket = (Trinket_t*)d_TableGet(g_trinket_templates, &trinket_id);
    if (!trinket) {
        return NULL;  // Trinket not found
    }

    return trinket;
}

// ============================================================================
// PLAYER TRINKET MANAGEMENT
// ============================================================================

bool EquipTrinket(Player_t* player, int slot_index, Trinket_t* trinket_template) {
    if (!player) {
        d_LogError("EquipTrinket: NULL player pointer");
        return false;
    }

    if (!trinket_template) {
        d_LogError("EquipTrinket: NULL trinket template pointer");
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

    if ((size_t)slot_index >= player->trinket_slots->count) {
        d_LogError("EquipTrinket: Slot index out of bounds");
        return false;
    }

    // Constitutional pattern: COPY template → slot by VALUE (each player gets own instance!)
    // Get pointer to slot value in array
    Trinket_t* slot = (Trinket_t*)d_ArrayGet(player->trinket_slots, slot_index);
    if (!slot) {
        d_LogError("EquipTrinket: Failed to access slot");
        return false;
    }

    // Cleanup old trinket in slot if it exists
    if (slot->name) {  // name != NULL means slot was previously occupied
        CleanupTrinketValue(slot);
    }

    // Deep copy template → slot (memcpy struct, then deep-copy dStrings)
    memcpy(slot, trinket_template, sizeof(Trinket_t));

    // Deep-copy dStrings (shallow copy above copied pointers, we need new instances)
    slot->name = d_StringInit();
    d_StringSet(slot->name, d_StringPeek(trinket_template->name), 0);

    slot->description = d_StringInit();
    d_StringSet(slot->description, d_StringPeek(trinket_template->description), 0);

    slot->passive_description = d_StringInit();
    d_StringSet(slot->passive_description, d_StringPeek(trinket_template->passive_description), 0);

    slot->active_description = d_StringInit();
    d_StringSet(slot->active_description, d_StringPeek(trinket_template->active_description), 0);

    // Reset per-instance state (each copy starts fresh)
    slot->passive_damage_bonus = 0;
    slot->total_damage_dealt = 0;
    slot->total_bonus_chips = 0;
    slot->total_refunded_chips = 0;
    slot->shake_offset_x = 0.0f;
    slot->shake_offset_y = 0.0f;
    slot->flash_alpha = 0.0f;
    slot->active_cooldown_current = 0;  // Ready on equip

    d_LogInfoF("Equipped trinket %s to slot %d (VALUE copy)", GetTrinketName(slot), slot_index);

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

    // Get pointer to slot value
    Trinket_t* slot = (Trinket_t*)d_ArrayGet(player->trinket_slots, slot_index);
    if (slot) {
        // Cleanup dStrings inside trinket value
        if (slot->name) {  // name != NULL means slot was occupied
            CleanupTrinketValue(slot);
        }
        // Zero out slot (mark as empty)
        memset(slot, 0, sizeof(Trinket_t));
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

    // Constitutional pattern: Return pointer to VALUE in array
    Trinket_t* trinket = (Trinket_t*)d_ArrayGet(player->trinket_slots, slot_index);
    if (!trinket) {
        return NULL;
    }

    // Empty slot check: if name is NULL, slot is empty
    if (!trinket->name) {
        return NULL;
    }

    return trinket;
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

const char* GetTrinketRarityName(TrinketRarity_t rarity) {
    switch (rarity) {
        case TRINKET_RARITY_COMMON:    return "Common";
        case TRINKET_RARITY_UNCOMMON:  return "Uncommon";
        case TRINKET_RARITY_RARE:      return "Rare";
        case TRINKET_RARITY_LEGENDARY: return "Legendary";
        case TRINKET_RARITY_CLASS:     return "Class";
        default:                       return "Unknown";
    }
}

void GetTrinketRarityColor(TrinketRarity_t rarity, int* r, int* g, int* b) {
    if (!r || !g || !b) return;

    switch (rarity) {
        case TRINKET_RARITY_COMMON:
            *r = 200; *g = 200; *b = 200;  // Gray/white
            break;
        case TRINKET_RARITY_UNCOMMON:
            *r = 100; *g = 255; *b = 100;  // Green
            break;
        case TRINKET_RARITY_RARE:
            *r = 100; *g = 150; *b = 255;  // Blue
            break;
        case TRINKET_RARITY_LEGENDARY:
            *r = 255; *g = 215; *b = 0;    // Gold
            break;
        case TRINKET_RARITY_CLASS:
            *r = 180; *g = 100; *b = 255;  // Purple (class-specific)
            break;
        default:
            *r = 255; *g = 255; *b = 255;  // White fallback
            break;
    }
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

    // Return pointer to embedded value if equipped, NULL otherwise
    if (!player->has_class_trinket) {
        return NULL;
    }

    return (Trinket_t*)&player->class_trinket;  // Cast away const for API compatibility
}

bool EquipClassTrinket(Player_t* player, Trinket_t* trinket_template) {
    if (!player || !trinket_template) {
        d_LogError("EquipClassTrinket: NULL player or trinket template");
        return false;
    }

    // Cleanup old class trinket if exists
    if (player->has_class_trinket) {
        CleanupTrinketValue(&player->class_trinket);
    }

    // Constitutional pattern: COPY template → embedded value (player gets own copy!)
    // Deep copy template → class_trinket (memcpy struct, then deep-copy dStrings)
    memcpy(&player->class_trinket, trinket_template, sizeof(Trinket_t));

    // Deep-copy dStrings (shallow copy above copied pointers, we need new instances)
    player->class_trinket.name = d_StringInit();
    d_StringSet(player->class_trinket.name, d_StringPeek(trinket_template->name), 0);

    player->class_trinket.description = d_StringInit();
    d_StringSet(player->class_trinket.description, d_StringPeek(trinket_template->description), 0);

    player->class_trinket.passive_description = d_StringInit();
    d_StringSet(player->class_trinket.passive_description, d_StringPeek(trinket_template->passive_description), 0);

    player->class_trinket.active_description = d_StringInit();
    d_StringSet(player->class_trinket.active_description, d_StringPeek(trinket_template->active_description), 0);

    // Reset per-instance state (each copy starts fresh)
    player->class_trinket.passive_damage_bonus = 0;
    player->class_trinket.total_damage_dealt = 0;
    player->class_trinket.total_bonus_chips = 0;
    player->class_trinket.total_refunded_chips = 0;
    player->class_trinket.shake_offset_x = 0.0f;
    player->class_trinket.shake_offset_y = 0.0f;
    player->class_trinket.flash_alpha = 0.0f;
    player->class_trinket.active_cooldown_current = 0;  // Ready on equip

    player->has_class_trinket = true;
    d_LogInfoF("Equipped class trinket: %s (VALUE copy)", GetTrinketName(&player->class_trinket));
    return true;
}

// ============================================================================
// MODIFIER SYSTEM (Win/Loss Modifiers - Pattern matches statusEffects.c)
// ============================================================================

int ModifyWinningsWithTrinkets(Player_t* player, int base_winnings, int bet_amount) {
    if (!player || !player->trinket_slots) {
        return base_winnings;
    }

    int modified = base_winnings;

    // Check class trinket first
    Trinket_t* class_trinket = GetClassTrinket(player);
    if (class_trinket && class_trinket->trinket_id == 1) {  // Elite Membership
        int bonus = (bet_amount * 30) / 100;  // 30% of bet
        modified += bonus;
        class_trinket->total_bonus_chips += bonus;
        d_LogInfoF("💳 Elite Membership (Class): +%d bonus chips (30%% win boost)", bonus);
    }

    // Check regular trinket slots
    for (int i = 0; i < 6; i++) {
        Trinket_t* trinket = GetEquippedTrinket(player, i);
        if (trinket && trinket->trinket_id == 1) {  // Elite Membership
            int bonus = (bet_amount * 30) / 100;  // 30% of bet
            modified += bonus;
            trinket->total_bonus_chips += bonus;
            d_LogInfoF("💳 Elite Membership (Slot %d): +%d bonus chips (30%% win boost)", i, bonus);
        }
    }

    return modified;
}

int ModifyLossesWithTrinkets(Player_t* player, int base_loss, int bet_amount) {
    (void)base_loss;  // Unused, but kept for API consistency with status effects

    if (!player || !player->trinket_slots) {
        return 0;
    }

    int total_refund = 0;

    // Check class trinket first
    Trinket_t* class_trinket = GetClassTrinket(player);
    if (class_trinket && class_trinket->trinket_id == 1) {  // Elite Membership
        int refund = (bet_amount * 30) / 100;  // 30% of bet
        total_refund += refund;
        class_trinket->total_refunded_chips += refund;
        d_LogInfoF("💳 Elite Membership (Class): +%d refunded chips (30%% loss protection)", refund);
    }

    // Check regular trinket slots
    for (int i = 0; i < 6; i++) {
        Trinket_t* trinket = GetEquippedTrinket(player, i);
        if (trinket && trinket->trinket_id == 1) {  // Elite Membership
            int refund = (bet_amount * 30) / 100;  // 30% of bet
            total_refund += refund;
            trinket->total_refunded_chips += refund;
            d_LogInfoF("💳 Elite Membership (Slot %d): +%d refunded chips (30%% loss protection)", i, refund);
        }
    }

    return total_refund;
}
