/*
 * Enemy Loader - Parse enemies from DUF files with nested ability definitions
 */

#include "../include/loaders/enemyLoader.h"
#include "../include/ability.h"
#include <Daedalus.h>
#include <string.h>

// ============================================================================
// DUF PARSING HELPERS
// ============================================================================

/**
 * ParseTrigger - Parse trigger object from DUF
 */
static bool ParseTrigger(dDUFValue_t* trigger_node, AbilityTrigger_t* out_trigger) {
    if (!trigger_node || !out_trigger) return false;

    // Get trigger type (required)
    dDUFValue_t* type_node = d_DUFGetObjectItem(trigger_node, "type");
    if (!type_node || !type_node->value_string) {
        d_LogError("ParseTrigger: Missing 'type' field");
        return false;
    }

    TriggerType_t parsed_type = TriggerTypeFromString(type_node->value_string);
    // Validate: If string conversion returns default fallback, check if it was intentional
    if (parsed_type == TRIGGER_PASSIVE && strcmp(type_node->value_string, "passive") != 0) {
        d_LogErrorF("ParseTrigger: Invalid trigger type '%s'", type_node->value_string);
        return false;
    }
    out_trigger->type = parsed_type;

    // Parse type-specific fields
    switch (out_trigger->type) {
        case TRIGGER_ON_EVENT: {
            dDUFValue_t* event_node = d_DUFGetObjectItem(trigger_node, "event");
            if (!event_node || !event_node->value_string) {
                d_LogError("ParseTrigger: on_event trigger missing 'event' field");
                return false;
            }
            GameEvent_t parsed_event = GameEventFromString(event_node->value_string);
            if (parsed_event == 0) {
                d_LogErrorF("ParseTrigger: Invalid game event '%s'", event_node->value_string);
                return false;
            }
            out_trigger->event = parsed_event;
            break;
        }

        case TRIGGER_COUNTER: {
            dDUFValue_t* event_node = d_DUFGetObjectItem(trigger_node, "event");
            dDUFValue_t* count_node = d_DUFGetObjectItem(trigger_node, "count");
            if (!event_node || !event_node->value_string) {
                d_LogError("ParseTrigger: counter trigger missing 'event' field");
                return false;
            }
            if (!count_node) {
                d_LogError("ParseTrigger: counter trigger missing 'count' field");
                return false;
            }
            GameEvent_t parsed_event = GameEventFromString(event_node->value_string);
            if (parsed_event == 0) {
                d_LogErrorF("ParseTrigger: Invalid game event '%s'", event_node->value_string);
                return false;
            }
            out_trigger->event = parsed_event;
            out_trigger->counter_max = (int)count_node->value_int;
            break;
        }

        case TRIGGER_HP_THRESHOLD: {
            dDUFValue_t* threshold_node = d_DUFGetObjectItem(trigger_node, "threshold");
            if (threshold_node) {
                out_trigger->threshold = (float)threshold_node->value_double;
            }
            break;
        }

        case TRIGGER_RANDOM: {
            dDUFValue_t* chance_node = d_DUFGetObjectItem(trigger_node, "chance");
            dDUFValue_t* event_node = d_DUFGetObjectItem(trigger_node, "event");
            if (!chance_node) {
                d_LogError("ParseTrigger: random trigger missing 'chance' field");
                return false;
            }
            if (!event_node || !event_node->value_string) {
                d_LogError("ParseTrigger: random trigger missing 'event' field");
                return false;
            }
            GameEvent_t parsed_event = GameEventFromString(event_node->value_string);
            if (parsed_event == 0) {
                d_LogErrorF("ParseTrigger: Invalid game event '%s'", event_node->value_string);
                return false;
            }
            out_trigger->chance = (float)chance_node->value_double;
            out_trigger->event = parsed_event;
            break;
        }

        case TRIGGER_ON_ACTION: {
            dDUFValue_t* action_node = d_DUFGetObjectItem(trigger_node, "action");
            if (!action_node || !action_node->value_string) {
                d_LogError("ParseTrigger: on_action trigger missing 'action' field");
                return false;
            }
            PlayerAction_t parsed_action = PlayerActionFromString(action_node->value_string);
            // Validate: ACTION_HIT is the default fallback
            if (parsed_action == ACTION_HIT && strcmp(action_node->value_string, "HIT") != 0) {
                d_LogErrorF("ParseTrigger: Invalid player action '%s'", action_node->value_string);
                return false;
            }
            out_trigger->action = parsed_action;
            break;
        }

        case TRIGGER_HP_SEGMENT: {
            dDUFValue_t* segment_node = d_DUFGetObjectItem(trigger_node, "segment_percent");
            if (segment_node) {
                out_trigger->segment_percent = (int)segment_node->value_int;
                out_trigger->segments_triggered = 0;  // Reset bitmask
            } else {
                d_LogError("hp_segment trigger missing segment_percent field");
                out_trigger->segment_percent = 25;  // Default to 25%
                out_trigger->segments_triggered = 0;
            }
            break;
        }

        case TRIGGER_DAMAGE_ACCUMULATOR: {
            dDUFValue_t* threshold_node = d_DUFGetObjectItem(trigger_node, "damage_threshold");
            if (threshold_node) {
                out_trigger->damage_threshold = (int)threshold_node->value_int;
                out_trigger->damage_accumulated = 0;  // Start at 0
            } else {
                d_LogError("damage_accumulator trigger missing damage_threshold field");
                out_trigger->damage_threshold = 1000;  // Default threshold
                out_trigger->damage_accumulated = 0;
            }
            break;
        }

        case TRIGGER_PASSIVE:
        default:
            break;
    }

    return true;
}

/**
 * ParseEffect - Parse single effect from DUF object
 *
 * OWNERSHIP: If effect.message is allocated (EFFECT_MESSAGE), ownership is TRANSFERRED
 * to the caller. Caller MUST add effect to Ability_t via AddEffect(), which takes ownership.
 * DO NOT manually free effect.message - DestroyAbility() will free it.
 */
static bool ParseEffect(dDUFValue_t* effect_node, AbilityEffect_t* out_effect) {
    if (!effect_node || !out_effect) return false;

    // Initialize to zero
    memset(out_effect, 0, sizeof(AbilityEffect_t));

    // Get effect type (required)
    dDUFValue_t* type_node = d_DUFGetObjectItem(effect_node, "type");
    if (!type_node || !type_node->value_string) {
        d_LogError("ParseEffect: Missing 'type' field");
        return false;
    }

    EffectType_t parsed_type = EffectTypeFromString(type_node->value_string);
    // Validate: If string conversion returns EFFECT_NONE, check if it was intentional
    if (parsed_type == EFFECT_NONE && strcmp(type_node->value_string, "none") != 0) {
        d_LogErrorF("ParseEffect: Invalid effect type '%s'", type_node->value_string);
        return false;
    }
    out_effect->type = parsed_type;

    // Parse type-specific fields
    switch (out_effect->type) {
        case EFFECT_APPLY_STATUS: {
            // Status effects ALWAYS target the player - no 'target' field needed
            out_effect->target = TARGET_PLAYER;
            dDUFValue_t* status_node = d_DUFGetObjectItem(effect_node, "status");
            dDUFValue_t* value_node = d_DUFGetObjectItem(effect_node, "value");
            dDUFValue_t* duration_node = d_DUFGetObjectItem(effect_node, "duration");

            if (!status_node || !status_node->value_string) {
                d_LogError("ParseEffect: apply_status effect missing 'status' field");
                return false;
            }
            StatusEffect_t parsed_status = StatusEffectFromString(status_node->value_string);
            // Validate: STATUS_NONE is the default fallback
            if (parsed_status == STATUS_NONE) {
                d_LogErrorF("ParseEffect: Invalid status effect '%s'", status_node->value_string);
                return false;
            }
            out_effect->status = parsed_status;

            if (value_node) {
                out_effect->value = (int)value_node->value_int;
            }
            if (duration_node) {
                out_effect->duration = (int)duration_node->value_int;
            }
            break;
        }

        case EFFECT_REMOVE_STATUS: {
            // Remove status also ALWAYS targets the player
            out_effect->target = TARGET_PLAYER;
            dDUFValue_t* status_node = d_DUFGetObjectItem(effect_node, "status");
            if (!status_node || !status_node->value_string) {
                d_LogError("ParseEffect: remove_status effect missing 'status' field");
                return false;
            }
            StatusEffect_t parsed_status = StatusEffectFromString(status_node->value_string);
            // Validate: STATUS_NONE is the default fallback
            if (parsed_status == STATUS_NONE) {
                d_LogErrorF("ParseEffect: Invalid status effect '%s'", status_node->value_string);
                return false;
            }
            out_effect->status = parsed_status;
            break;
        }

        case EFFECT_HEAL:
        case EFFECT_DAMAGE: {
            // Heal/Damage effects require explicit 'target' field
            dDUFValue_t* target_node = d_DUFGetObjectItem(effect_node, "target");
            if (!target_node || !target_node->value_string) {
                d_LogError("ParseEffect: heal/damage effect missing 'target' field");
                return false;
            }
            EffectTarget_t parsed_target = TargetFromString(target_node->value_string);
            if (parsed_target == TARGET_PLAYER && strcmp(target_node->value_string, "player") != 0) {
                d_LogErrorF("ParseEffect: Invalid target '%s'", target_node->value_string);
                return false;
            }
            out_effect->target = parsed_target;

            dDUFValue_t* value_node = d_DUFGetObjectItem(effect_node, "value");
            if (value_node) {
                out_effect->value = (int)value_node->value_int;
            }
            break;
        }

        case EFFECT_MESSAGE: {
            // Messages don't have a target concept
            out_effect->target = TARGET_PLAYER;
            dDUFValue_t* message_node = d_DUFGetObjectItem(effect_node, "message");
            if (message_node && message_node->value_string) {
                out_effect->message = d_StringInit();
                d_StringSet(out_effect->message, message_node->value_string, 0);
            }
            break;
        }

        case EFFECT_SHUFFLE_DECK:
        case EFFECT_DISCARD_HAND:
        case EFFECT_FORCE_HIT:
        case EFFECT_REVEAL_HOLE:
        default:
            // These effects implicitly target the player (deck/hand manipulation)
            out_effect->target = TARGET_PLAYER;
            break;
    }

    return true;
}

/**
 * ParseAbility - Parse complete ability from DUF nested object
 */
static Ability_t* ParseAbility(dDUFValue_t* ability_node) {
    if (!ability_node) return NULL;

    // Get name and description (required)
    dDUFValue_t* name_node = d_DUFGetObjectItem(ability_node, "name");
    dDUFValue_t* desc_node = d_DUFGetObjectItem(ability_node, "description");

    if (!name_node || !name_node->value_string) {
        d_LogError("ParseAbility: Missing 'name' field");
        return NULL;
    }

    const char* name = name_node->value_string;
    const char* description = (desc_node && desc_node->value_string) ? desc_node->value_string : "";

    // Create ability
    Ability_t* ability = CreateAbility(name, description);
    if (!ability) {
        d_LogError("ParseAbility: Failed to create ability");
        return NULL;
    }

    // Parse trigger (required)
    dDUFValue_t* trigger_node = d_DUFGetObjectItem(ability_node, "trigger");
    if (!trigger_node) {
        d_LogError("ParseAbility: Missing 'trigger' field");
        DestroyAbility(&ability);
        return NULL;
    }

    if (!ParseTrigger(trigger_node, &ability->trigger)) {
        d_LogError("ParseAbility: Failed to parse trigger");
        DestroyAbility(&ability);
        return NULL;
    }

    // Parse cooldown (optional)
    dDUFValue_t* cooldown_node = d_DUFGetObjectItem(ability_node, "cooldown");
    if (cooldown_node) {
        ability->cooldown_max = (int)cooldown_node->value_int;
    }

    // Parse effects array (required)
    dDUFValue_t* effects_node = d_DUFGetObjectItem(ability_node, "effects");
    if (!effects_node || effects_node->type != D_DUF_ARRAY) {
        d_LogError("ParseAbility: Missing or invalid 'effects' array");
        DestroyAbility(&ability);
        return NULL;
    }

    // Parse each effect
    dDUFValue_t* effect_item = effects_node->child;
    size_t effects_parsed = 0;
    while (effect_item) {
        if (effect_item->type == D_DUF_TABLE) {
            AbilityEffect_t effect = {0};
            if (ParseEffect(effect_item, &effect)) {
                AddEffect(ability, &effect);
                effects_parsed++;
            } else {
                d_LogWarning("ParseAbility: Failed to parse effect, skipping");
            }
        }
        effect_item = effect_item->next;
    }

    // Validate: Ability MUST have at least one effect
    if (effects_parsed == 0) {
        d_LogErrorF("ParseAbility: '%s' has no valid effects (all parse attempts failed)", name);
        DestroyAbility(&ability);
        return NULL;
    }

    d_LogInfoF("Parsed ability: %s (%zu effects)", name, effects_parsed);
    return ability;
}

// ============================================================================
// ENEMY LOADING
// ============================================================================

/**
 * LoadEnemyFromDUF - Load enemy definition from DUF database
 *
 * @param enemies_db - Parsed DUF root containing enemy definitions
 * @param enemy_key - Key name of enemy to load (e.g., "didact", "daemon")
 *
 * @return Enemy_t* on success, NULL on failure
 */
Enemy_t* LoadEnemyFromDUF(dDUFValue_t* enemies_db, const char* enemy_key) {
    if (!enemies_db || !enemy_key) {
        d_LogError("LoadEnemyFromDUF: NULL parameters");
        return NULL;
    }

    // Get enemy table from database
    dDUFValue_t* enemy_data = d_DUFGetObjectItem(enemies_db, enemy_key);
    if (!enemy_data) {
        dString_t* error = d_StringInit();
        d_StringFormat(error, "Enemy '%s' not found in DUF database", enemy_key);
        d_LogError(d_StringPeek(error));
        d_StringDestroy(error);
        return NULL;
    }

    // Parse required fields
    dDUFValue_t* name_node = d_DUFGetObjectItem(enemy_data, "name");
    dDUFValue_t* hp_node = d_DUFGetObjectItem(enemy_data, "hp");

    if (!name_node || !hp_node) {
        dString_t* error = d_StringInit();
        d_StringFormat(error, "Enemy '%s' missing required fields (name, hp)", enemy_key);
        d_LogError(d_StringPeek(error));
        d_StringDestroy(error);
        return NULL;
    }

    // Create base enemy
    const char* name = name_node->value_string;
    int max_hp = (int)hp_node->value_int;

    Enemy_t* enemy = CreateEnemy(name, max_hp);
    if (!enemy) {
        dString_t* error = d_StringInit();
        d_StringFormat(error, "Failed to create enemy '%s'", enemy_key);
        d_LogError(d_StringPeek(error));
        d_StringDestroy(error);
        return NULL;
    }

    // Parse description (optional)
    dDUFValue_t* desc_node = d_DUFGetObjectItem(enemy_data, "description");
    if (desc_node && desc_node->value_string && enemy->description) {
        d_StringSet(enemy->description, desc_node->value_string, 0);
    }

    // Parse and add abilities (required - enemies must have at least one ability)
    dDUFValue_t* abilities_node = d_DUFGetObjectItem(enemy_data, "abilities");
    if (!abilities_node || abilities_node->type != D_DUF_ARRAY) {
        dString_t* error = d_StringInit();
        d_StringFormat(error, "Enemy '%s' missing or invalid 'abilities' array", enemy_key);
        d_LogError(d_StringPeek(error));
        d_StringDestroy(error);
        DestroyEnemy(&enemy);
        return NULL;
    }

    dDUFValue_t* ability_item = abilities_node->child;
    int ability_count = 0;

    while (ability_item) {
        if (ability_item->type == D_DUF_TABLE) {
            Ability_t* ability = ParseAbility(ability_item);
            if (ability) {
                // Add Ability_t* pointer to enemy->abilities array
                d_ArrayAppend(enemy->abilities, &ability);
                ability_count++;
            } else {
                d_LogWarning("LoadEnemyFromDUF: Failed to parse ability, skipping");
            }
        }
        ability_item = ability_item->next;
    }

    // Validate: Enemy MUST have at least one valid ability
    if (ability_count == 0) {
        dString_t* error = d_StringInit();
        d_StringFormat(error, "Enemy '%s' has no valid abilities (all parse attempts failed)", enemy_key);
        d_LogError(d_StringPeek(error));
        d_StringDestroy(error);
        DestroyEnemy(&enemy);
        return NULL;
    }

    d_LogInfoF("Loaded %d abilities for enemy '%s'", ability_count, name);

    // Parse image_path (optional)
    dDUFValue_t* image_node = d_DUFGetObjectItem(enemy_data, "image_path");
    if (image_node && image_node->value_string) {
        if (!LoadEnemyPortrait(enemy, image_node->value_string)) {
            d_LogWarningF("Failed to load portrait for enemy '%s'", name);
        }
    }

    dString_t* info = d_StringInit();
    d_StringFormat(info, "Loaded enemy '%s' from DUF (HP: %d, Abilities: %zu)",
                   name, max_hp, enemy->abilities->count);
    d_LogInfo(d_StringPeek(info));
    d_StringDestroy(info);

    return enemy;
}

/**
 * GetEnemyNameFromDUF - Lightweight name lookup without full enemy parse
 */
const char* GetEnemyNameFromDUF(dDUFValue_t* enemies_db, const char* enemy_key) {
    if (!enemies_db || !enemy_key) {
        return "Unknown";
    }

    // Get enemy table from database
    dDUFValue_t* enemy_data = d_DUFGetObjectItem(enemies_db, enemy_key);
    if (!enemy_data) {
        return "Unknown";
    }

    // Get name field (don't parse abilities, portrait, etc)
    dDUFValue_t* name_node = d_DUFGetObjectItem(enemy_data, "name");
    if (!name_node || !name_node->value_string) {
        return "Unknown";
    }

    // Return pointer to DUF string (valid until DUF freed)
    return name_node->value_string;
}

bool ValidateEnemyDatabase(dDUFValue_t* enemies_db, char* out_error_msg, size_t error_msg_size) {
    if (!enemies_db || !out_error_msg) {
        return false;
    }

    // Iterate through all enemy entries in DUF database
    dDUFValue_t* enemy_entry = enemies_db->child;
    int validated_enemies = 0;

    while (enemy_entry) {
        // Get enemy key from DUF node (the @key syntax becomes node->string)
        const char* enemy_key = enemy_entry->string;
        if (!enemy_key) {
            enemy_entry = enemy_entry->next;
            continue;
        }

        // Try to load this enemy
        Enemy_t* enemy = LoadEnemyFromDUF(enemies_db, enemy_key);
        if (!enemy) {
            // Validation failed - write error message
            const char* enemy_name = GetEnemyNameFromDUF(enemies_db, enemy_key);
            snprintf(out_error_msg, error_msg_size,
                    "DUF Validation Failed\n\n"
                    "Enemy: %s (key: '%s')\n"
                    "File: data/enemies/tutorial_enemies.duf\n\n"
                    "Check console logs for details.\n\n"
                    "Common issues:\n"
                    "- Typo in field name (e.g., 'abilitiesf' instead of 'abilities')\n"
                    "- Missing required fields\n"
                    "- Invalid enum values",
                    enemy_name, enemy_key);
            return false;
        }

        // Successfully loaded - clean up and continue
        DestroyEnemy(&enemy);
        validated_enemies++;
        enemy_entry = enemy_entry->next;
    }

    d_LogInfoF("✓ DUF Validation: All %d enemies valid", validated_enemies);
    return true;
}
