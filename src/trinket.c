/*
 * Trinket System Implementation
 * Constitutional pattern: Heap-allocated pointer types in global registry
 */

#include "../include/trinket.h"
#include "../include/player.h"
#include "../include/trinkets/degenerateGambit.h"
// Elite Membership now loaded from DUF (data/trinkets/event_trinkets.duf)
#include "../include/trinkets/stackTrace.h"
#include "../include/scenes/sceneBlackjack.h"  // For GetTweenManager
#include "../include/tween/tween.h"            // For TweenFloat, TWEEN_EASE_*
#include "../include/trinketEffects.h"         // For ExecuteTrinketEffect
#include "../include/loaders/trinketLoader.h"  // For GetTrinketTemplate
#include "../include/game.h"                   // For GameEventToString
#include <stdlib.h>

// ============================================================================
// GLOBAL REGISTRY (OLD system - hardcoded class trinkets)
// ============================================================================

// OLD Trinket_t system (class trinkets like Degenerate's Gambit)
// Separate from DUF TrinketTemplate_t system in trinketLoader.c
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
    // Elite Membership (ID 1) now loaded from DUF (data/trinkets/event_trinkets.duf)
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

int GetEmptyTrinketSlot(const Player_t* player) {
    if (!player) {
        return -1;
    }

    for (int i = 0; i < 6; i++) {
        if (!player->trinket_slot_occupied[i]) {
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

    // Note: trinket_slots is a fixed-size array embedded in Player_t, not a pointer

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

    // Check DUF trinket instances (regular trinkets are passive-only)
    for (int i = 0; i < 6; i++) {
        // Skip empty slots
        if (!player->trinket_slot_occupied[i]) {
            continue;
        }

        TrinketInstance_t* instance = &player->trinket_slots[i];
        const TrinketTemplate_t* template = GetTrinketTemplate(d_StringPeek(instance->base_trinket_key));

        if (!template) {
            d_LogWarningF("Trinket instance in slot %d has invalid template key: %s",
                          i, d_StringPeek(instance->base_trinket_key));
            continue;
        }

        // Check primary passive trigger
        if (template->passive_trigger == event) {
            // Check optional bet condition
            if (template->passive_condition_bet_gte > 0 && player->current_bet < template->passive_condition_bet_gte) {
                d_LogDebugF("Trinket %s condition not met: bet %d < %d",
                            d_StringPeek(template->name),
                            player->current_bet,
                            template->passive_condition_bet_gte);
                // NOTE: template is borrowed pointer from cache - do NOT free!
                // Skip to next slot (condition not met)
                continue;
            }

            d_LogInfoF("🔔 Triggering DUF trinket passive: %s (event: %s, effect: %d, value: %d, bet: %d)",
                       d_StringPeek(template->name),
                       GameEventToString(event),
                       template->passive_effect_type,
                       template->passive_effect_value,
                       player->current_bet);

            ExecuteTrinketEffect(template, instance, player, game, i, false);
        }

        // Check secondary passive trigger (if exists)
        if (template->passive_trigger_2 != 0 && template->passive_trigger_2 == event) {
            // Note: Secondary passives don't have separate condition fields
            // If needed in future, add passive_condition_bet_gte_2 field

            d_LogInfoF("🔔 Triggering DUF trinket SECONDARY passive: %s (event: %s, effect: %d, value: %d, bet: %d)",
                       d_StringPeek(template->name),
                       GameEventToString(event),
                       template->passive_effect_type_2,
                       template->passive_effect_value_2,
                       player->current_bet);

            ExecuteTrinketEffect(template, instance, player, game, i, true);
        }

        // NOTE: template is borrowed pointer from cache - do NOT free!
    }
}

void TickTrinketCooldowns(Player_t* player) {
    if (!player) {
        return;
    }

    // Only class trinkets have active abilities with cooldowns
    // Regular trinkets (TrinketInstance_t) are passive-only
    Trinket_t* class_trinket = GetClassTrinket(player);
    if (class_trinket && class_trinket->active_cooldown_current > 0) {
        class_trinket->active_cooldown_current--;
        d_LogInfoF("CLASS trinket %s cooldown: %d",
                   GetTrinketName(class_trinket), class_trinket->active_cooldown_current);
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

    // Only class trinkets have active abilities (slot_index = -1)
    // Regular trinkets (TrinketInstance_t) are passive-only
    if (slot_index != -1) {
        d_LogErrorF("ActivateTrinket: Regular trinkets don't have active abilities (slot %d)", slot_index);
        return;
    }

    Trinket_t* trinket = GetClassTrinket(player);
    if (!trinket) {
        d_LogError("ActivateTrinket: No class trinket equipped");
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
    trinket->active_effect(player, game, target, trinket, 0);

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

    // Using color palette colors
    switch (rarity) {
        case TRINKET_RARITY_COMMON:
            *r = 231; *g = 213; *b = 179;  // Light beige (#e7d5b3)
            break;
        case TRINKET_RARITY_UNCOMMON:
            *r = 168; *g = 202; *b = 88;   // Green (#a8ca58)
            break;
        case TRINKET_RARITY_RARE:
            *r = 79; *g = 143; *b = 186;   // Blue (#4f8fba)
            break;
        case TRINKET_RARITY_LEGENDARY:
            *r = 222; *g = 158; *b = 65;   // Gold/Orange (#de9e41)
            break;
        case TRINKET_RARITY_CLASS:
            *r = 162; *g = 62; *b = 140;   // Purple (#a23e8c)
            break;
        case TRINKET_RARITY_EVENT:
            *r = 115; *g = 190; *b = 211;  // Teal (#73bed3)
            break;
        default:
            *r = 235; *g = 237; *b = 233;  // White fallback (#ebede9)
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
// MODIFIER SYSTEM (Win/Loss Modifiers - REMOVED)
// ============================================================================
//
// ModifyWinningsWithTrinkets() and ModifyLossesWithTrinkets() have been removed.
// Elite Membership now uses the standard DUF event trigger system:
// - PLAYER_WIN → ExecuteAddChipsPercent() (adds bonus chips + tracks stats)
// - PLAYER_LOSS → ExecuteRefundChipsPercent() (refunds chips + tracks stats)
//
// This makes Elite Membership consistent with other DUF trinkets (Lucky Chip,
// Insurance Policy, etc.) and fixes stats tracking bugs.
// ============================================================================
