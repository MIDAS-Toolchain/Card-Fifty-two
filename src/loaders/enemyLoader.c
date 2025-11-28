/*
 * Enemy Loader - Parse enemies from DUF files with nested ability definitions
 */

#include "loaders/enemyLoader.h"
#include "ability.h"
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

    out_trigger->type = TriggerTypeFromString(type_node->value_string);

    // Parse type-specific fields
    switch (out_trigger->type) {
        case TRIGGER_ON_EVENT: {
            dDUFValue_t* event_node = d_DUFGetObjectItem(trigger_node, "event");
            if (event_node && event_node->value_string) {
                out_trigger->event = GameEventFromString(event_node->value_string);
            }
            break;
        }

        case TRIGGER_COUNTER: {
            dDUFValue_t* event_node = d_DUFGetObjectItem(trigger_node, "event");
            dDUFValue_t* count_node = d_DUFGetObjectItem(trigger_node, "count");
            if (event_node && event_node->value_string) {
                out_trigger->event = GameEventFromString(event_node->value_string);
            }
            if (count_node) {
                out_trigger->counter_max = (int)count_node->value_int;
            }
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
            if (chance_node) {
                out_trigger->chance = (float)chance_node->value_double;
            }
            if (event_node && event_node->value_string) {
                out_trigger->event = GameEventFromString(event_node->value_string);
            }
            break;
        }

        case TRIGGER_ON_ACTION: {
            dDUFValue_t* action_node = d_DUFGetObjectItem(trigger_node, "action");
            if (action_node && action_node->value_string) {
                out_trigger->action = PlayerActionFromString(action_node->value_string);
            }
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

        case TRIGGER_PASSIVE:
        default:
            break;
    }

    return true;
}

/**
 * ParseEffect - Parse single effect from DUF object
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

    out_effect->type = EffectTypeFromString(type_node->value_string);

    // Parse target (default to TARGET_PLAYER)
    dDUFValue_t* target_node = d_DUFGetObjectItem(effect_node, "target");
    if (target_node && target_node->value_string) {
        out_effect->target = TargetFromString(target_node->value_string);
    } else {
        out_effect->target = TARGET_PLAYER;
    }

    // Parse type-specific fields
    switch (out_effect->type) {
        case EFFECT_APPLY_STATUS: {
            dDUFValue_t* status_node = d_DUFGetObjectItem(effect_node, "status");
            dDUFValue_t* value_node = d_DUFGetObjectItem(effect_node, "value");
            dDUFValue_t* duration_node = d_DUFGetObjectItem(effect_node, "duration");

            if (status_node && status_node->value_string) {
                out_effect->status = StatusEffectFromString(status_node->value_string);
            }
            if (value_node) {
                out_effect->value = (int)value_node->value_int;
            }
            if (duration_node) {
                out_effect->duration = (int)duration_node->value_int;
            }
            break;
        }

        case EFFECT_REMOVE_STATUS: {
            dDUFValue_t* status_node = d_DUFGetObjectItem(effect_node, "status");
            if (status_node && status_node->value_string) {
                out_effect->status = StatusEffectFromString(status_node->value_string);
            }
            break;
        }

        case EFFECT_HEAL:
        case EFFECT_DAMAGE: {
            dDUFValue_t* value_node = d_DUFGetObjectItem(effect_node, "value");
            if (value_node) {
                out_effect->value = (int)value_node->value_int;
            }
            break;
        }

        case EFFECT_MESSAGE: {
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
            // No additional fields needed
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
    while (effect_item) {
        if (effect_item->type == D_DUF_TABLE) {
            AbilityEffect_t effect = {0};
            if (ParseEffect(effect_item, &effect)) {
                AddEffect(ability, &effect);
            } else {
                d_LogWarning("ParseAbility: Failed to parse effect, skipping");
            }
        }
        effect_item = effect_item->next;
    }

    d_LogInfoF("Parsed ability: %s", name);
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

    // Parse and add abilities (nested objects)
    dDUFValue_t* abilities_node = d_DUFGetObjectItem(enemy_data, "abilities");
    if (abilities_node && abilities_node->type == D_DUF_ARRAY) {
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

        d_LogInfoF("Loaded %d abilities for enemy '%s'", ability_count, name);
    }

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
