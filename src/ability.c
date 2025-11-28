/*
 * Card Fifty-Two - Data-Driven Ability System
 * Abilities are defined in DUF files with trigger conditions and effect chains
 */

#include "../include/ability.h"
#include "../include/enemy.h"
#include "../include/game.h"
#include "../include/player.h"
#include "../include/deck.h"
#include <string.h>
#include <stdlib.h>

// External globals
extern dTable_t* g_players;

// ============================================================================
// LIFECYCLE
// ============================================================================

Ability_t* CreateAbility(const char* name, const char* description) {
    if (!name || !description) {
        d_LogError("CreateAbility: NULL name or description");
        return NULL;
    }

    // EXCEPTION: Ability_t contains heap-owned dString_t*/dArray_t* - stored as pointers not values
    Ability_t* ability = (Ability_t*)calloc(1, sizeof(Ability_t));
    if (!ability) {
        d_LogError("CreateAbility: Failed to allocate ability");
        return NULL;
    }

    ability->name = d_StringInit();
    ability->description = d_StringInit();

    if (!ability->name || !ability->description) {
        d_LogError("CreateAbility: Failed to allocate strings");
        if (ability->name) d_StringDestroy(ability->name);
        if (ability->description) d_StringDestroy(ability->description);
        free(ability);
        return NULL;
    }

    d_StringSet(ability->name, name, 0);
    d_StringSet(ability->description, description, 0);

    // Initialize effects array (capacity 4, most abilities have 1-3 effects)
    ability->effects = d_ArrayInit(4, sizeof(AbilityEffect_t));
    if (!ability->effects) {
        d_LogError("CreateAbility: Failed to allocate effects array");
        d_StringDestroy(ability->name);
        d_StringDestroy(ability->description);
        free(ability);
        return NULL;
    }

    // Initialize runtime state
    ability->cooldown_max = 0;
    ability->cooldown_current = 0;
    ability->has_triggered = false;
    ability->counter_current = 0;

    // Initialize animation
    ability->shake_offset_x = 0.0f;
    ability->shake_offset_y = 0.0f;
    ability->flash_alpha = 0.0f;

    return ability;
}

void DestroyAbility(Ability_t** ability) {
    if (!ability || !*ability) return;

    Ability_t* a = *ability;

    // Destroy strings
    if (a->name) d_StringDestroy(a->name);
    if (a->description) d_StringDestroy(a->description);

    // Destroy effect messages
    if (a->effects) {
        for (size_t i = 0; i < a->effects->count; i++) {
            AbilityEffect_t* effect = (AbilityEffect_t*)d_ArrayGet(a->effects, i);
            if (effect && effect->message) {
                d_StringDestroy(effect->message);
            }
        }
        d_ArrayDestroy(a->effects);
    }

    free(a);
    *ability = NULL;
}

void AddEffect(Ability_t* ability, const AbilityEffect_t* effect) {
    if (!ability || !effect) {
        d_LogError("AddEffect: NULL parameters");
        return;
    }

    d_ArrayAppend(ability->effects, effect);
}

// ============================================================================
// EXECUTION
// ============================================================================

bool CheckAbilityTrigger(Ability_t* ability, GameEvent_t event, float enemy_hp_percent) {
    if (!ability) return false;

    // Check cooldown
    if (ability->cooldown_current > 0) {
        return false;
    }

    bool should_trigger = false;

    switch (ability->trigger.type) {
        case TRIGGER_PASSIVE:
            // Passive abilities don't "trigger" on events
            return false;

        case TRIGGER_ON_EVENT:
            if (event == ability->trigger.event) {
                should_trigger = true;
            }
            break;

        case TRIGGER_COUNTER:
            if (event == ability->trigger.event) {
                ability->counter_current++;
                d_LogDebugF("%s counter: %d/%d",
                           d_StringPeek(ability->name),
                           ability->counter_current,
                           ability->trigger.counter_max);

                if (ability->counter_current >= ability->trigger.counter_max) {
                    should_trigger = true;
                    ability->counter_current = 0;  // Reset counter
                }
            }
            break;

        case TRIGGER_HP_THRESHOLD:
            if (enemy_hp_percent <= ability->trigger.threshold) {
                if (ability->trigger.once) {
                    if (!ability->has_triggered) {
                        should_trigger = true;
                        ability->has_triggered = true;
                    }
                } else {
                    should_trigger = true;
                }
            }
            break;

        case TRIGGER_RANDOM:
            if (event == ability->trigger.event) {
                float roll = (float)rand() / (float)RAND_MAX;
                if (roll <= ability->trigger.chance) {
                    should_trigger = true;
                }
            }
            break;

        case TRIGGER_ON_ACTION:
            // TODO: Need to extend GameEvent_t to include player action info
            // For now, this trigger type is unsupported
            break;

        case TRIGGER_HP_SEGMENT: {
            // Fires at regular HP intervals (e.g., every 25% HP lost)
            // Uses bitmask to track which thresholds have been crossed
            int segment_size = ability->trigger.segment_percent;

            // Validate segment_size divides 100 evenly
            if (segment_size <= 0 || 100 % segment_size != 0) {
                d_LogErrorF("Invalid segment_percent %d (must divide 100 evenly)", segment_size);
                break;
            }

            int num_segments = (100 / segment_size) - 1;  // 25% = 3 segments (75, 50, 25)

            // Check each threshold from high to low
            for (int i = 0; i < num_segments; i++) {
                int threshold_percent = 100 - ((i + 1) * segment_size);  // 75, 50, 25...
                float threshold = threshold_percent / 100.0f;
                uint8_t bit = (1 << i);

                // Check if crossed this threshold and not already triggered
                if (enemy_hp_percent <= threshold && !(ability->trigger.segments_triggered & bit)) {
                    ability->trigger.segments_triggered |= bit;
                    should_trigger = true;
                    d_LogInfoF("HP Segment triggered at %d%% (enemy at %.1f%%)",
                               threshold_percent, enemy_hp_percent * 100);
                    break;  // Only trigger once per damage instance
                }
            }
            break;
        }
    }

    return should_trigger;
}

void ExecuteAbility(Ability_t* ability, Enemy_t* enemy, GameContext_t* game) {
    if (!ability || !enemy || !game) {
        d_LogError("ExecuteAbility: NULL parameters");
        return;
    }

    d_LogInfoF("Ability triggered: %s", d_StringPeek(ability->name));

    // Execute all effects in sequence
    for (size_t i = 0; i < ability->effects->count; i++) {
        AbilityEffect_t* effect = (AbilityEffect_t*)d_ArrayGet(ability->effects, i);
        if (effect) {
            ExecuteEffect(effect, enemy, game);
        }
    }

    // Start cooldown if applicable
    if (ability->cooldown_max > 0) {
        ability->cooldown_current = ability->cooldown_max;
    }
}

void ExecuteEffect(const AbilityEffect_t* effect, Enemy_t* enemy, GameContext_t* game) {
    if (!effect || !enemy || !game) return;

    // Get human player (ID = 1)
    int player_id = 1;
    Player_t* player = (Player_t*)d_TableGet(g_players, &player_id);

    switch (effect->type) {
        case EFFECT_APPLY_STATUS:
            if (!player || !player->status_effects) {
                d_LogError("ExecuteEffect: Player or status effects manager not found");
                break;
            }
            ApplyStatusEffect(player->status_effects, effect->status, effect->value, effect->duration);
            d_LogInfoF("Applied %s for %d rounds (value: %d)",
                      GetStatusEffectName(effect->status), effect->duration, effect->value);
            break;

        case EFFECT_REMOVE_STATUS:
            if (!player || !player->status_effects) {
                d_LogError("ExecuteEffect: Player or status effects manager not found");
                break;
            }
            RemoveStatusEffect(player->status_effects, effect->status);
            d_LogInfoF("Removed %s", GetStatusEffectName(effect->status));
            break;

        case EFFECT_HEAL:
            if (effect->target == TARGET_SELF) {
                HealEnemy(enemy, effect->value);
                d_LogInfoF("Enemy healed for %d HP", effect->value);
            } else {
                // TARGET_PLAYER - heal chips
                if (player) {
                    player->chips += effect->value;
                    if (player->chips < 0) player->chips = 0;  // Clamp to 0
                    d_LogInfoF("Player healed for %d chips", effect->value);
                }
            }
            break;

        case EFFECT_DAMAGE:
            if (effect->target == TARGET_SELF) {
                // EXCEPTION: Enemy self-damage (no player modifiers apply)
                TakeDamage(enemy, effect->value);
                d_LogInfoF("Enemy took %d damage", effect->value);
            } else {
                // TARGET_PLAYER - lose chips
                if (player) {
                    player->chips -= effect->value;
                    if (player->chips < 0) player->chips = 0;  // Clamp to 0
                    d_LogInfoF("Player lost %d chips", effect->value);
                }
            }
            break;

        case EFFECT_SHUFFLE_DECK:
            if (game->deck) {
                ShuffleDeck(game->deck);
                d_LogInfo("Deck forcefully reshuffled!");
            }
            break;

        case EFFECT_DISCARD_HAND:
            if (player && game->deck) {
                ClearHand(&player->hand, game->deck);
                d_LogInfo("Player's hand discarded!");
            }
            break;

        case EFFECT_FORCE_HIT:
            // TODO: Need to implement forced draw logic
            d_LogWarning("EFFECT_FORCE_HIT not yet implemented");
            break;

        case EFFECT_REVEAL_HOLE:
            // TODO: Need to set dealer hole card face_up flag
            d_LogWarning("EFFECT_REVEAL_HOLE not yet implemented");
            break;

        case EFFECT_MESSAGE:
            if (effect->message) {
                d_LogInfo(d_StringPeek(effect->message));
            }
            break;

        default:
            d_LogWarningF("Unknown effect type: %d", effect->type);
            break;
    }
}

void TickAbilityCooldowns(dArray_t* abilities) {
    if (!abilities) return;

    for (size_t i = 0; i < abilities->count; i++) {
        Ability_t** ability_ptr = (Ability_t**)d_ArrayGet(abilities, i);
        if (ability_ptr && *ability_ptr) {
            Ability_t* ability = *ability_ptr;
            if (ability->cooldown_current > 0) {
                ability->cooldown_current--;
            }
        }
    }
}

void ResetAbilityStates(dArray_t* abilities) {
    if (!abilities) return;

    for (size_t i = 0; i < abilities->count; i++) {
        Ability_t** ability_ptr = (Ability_t**)d_ArrayGet(abilities, i);
        if (ability_ptr && *ability_ptr) {
            Ability_t* ability = *ability_ptr;
            ability->has_triggered = false;
            ability->counter_current = 0;
            ability->cooldown_current = 0;
        }
    }
}

// ============================================================================
// STRING CONVERSION (DUF parsing helpers)
// ============================================================================

EffectType_t EffectTypeFromString(const char* str) {
    if (!str) return EFFECT_NONE;

    if (strcmp(str, "apply_status") == 0) return EFFECT_APPLY_STATUS;
    if (strcmp(str, "remove_status") == 0) return EFFECT_REMOVE_STATUS;
    if (strcmp(str, "heal") == 0) return EFFECT_HEAL;
    if (strcmp(str, "damage") == 0) return EFFECT_DAMAGE;
    if (strcmp(str, "shuffle_deck") == 0) return EFFECT_SHUFFLE_DECK;
    if (strcmp(str, "discard_hand") == 0) return EFFECT_DISCARD_HAND;
    if (strcmp(str, "force_hit") == 0) return EFFECT_FORCE_HIT;
    if (strcmp(str, "reveal_hole") == 0) return EFFECT_REVEAL_HOLE;
    if (strcmp(str, "message") == 0) return EFFECT_MESSAGE;

    d_LogWarningF("Unknown effect type: %s", str);
    return EFFECT_NONE;
}

TriggerType_t TriggerTypeFromString(const char* str) {
    if (!str) return TRIGGER_PASSIVE;

    if (strcmp(str, "passive") == 0) return TRIGGER_PASSIVE;
    if (strcmp(str, "on_event") == 0) return TRIGGER_ON_EVENT;
    if (strcmp(str, "counter") == 0) return TRIGGER_COUNTER;
    if (strcmp(str, "hp_threshold") == 0) return TRIGGER_HP_THRESHOLD;
    if (strcmp(str, "random") == 0) return TRIGGER_RANDOM;
    if (strcmp(str, "on_action") == 0) return TRIGGER_ON_ACTION;
    if (strcmp(str, "hp_segment") == 0) return TRIGGER_HP_SEGMENT;

    d_LogWarningF("Unknown trigger type: %s", str);
    return TRIGGER_PASSIVE;
}

EffectTarget_t TargetFromString(const char* str) {
    if (!str) return TARGET_PLAYER;

    if (strcmp(str, "player") == 0) return TARGET_PLAYER;
    if (strcmp(str, "self") == 0) return TARGET_SELF;

    d_LogWarningF("Unknown target: %s", str);
    return TARGET_PLAYER;
}

GameEvent_t GameEventFromString(const char* str) {
    if (!str) return 0;

    if (strcmp(str, "COMBAT_START") == 0) return GAME_EVENT_COMBAT_START;
    if (strcmp(str, "HAND_END") == 0) return GAME_EVENT_HAND_END;
    if (strcmp(str, "PLAYER_WIN") == 0) return GAME_EVENT_PLAYER_WIN;
    if (strcmp(str, "PLAYER_LOSS") == 0) return GAME_EVENT_PLAYER_LOSS;
    if (strcmp(str, "PLAYER_PUSH") == 0) return GAME_EVENT_PLAYER_PUSH;
    if (strcmp(str, "PLAYER_BUST") == 0) return GAME_EVENT_PLAYER_BUST;
    if (strcmp(str, "PLAYER_BLACKJACK") == 0) return GAME_EVENT_PLAYER_BLACKJACK;
    if (strcmp(str, "DEALER_BUST") == 0) return GAME_EVENT_DEALER_BUST;
    if (strcmp(str, "CARD_DRAWN") == 0) return GAME_EVENT_CARD_DRAWN;
    if (strcmp(str, "PLAYER_ACTION_END") == 0) return GAME_EVENT_PLAYER_ACTION_END;
    if (strcmp(str, "CARD_TAG_CURSED") == 0) return GAME_EVENT_CARD_TAG_CURSED;
    if (strcmp(str, "CARD_TAG_VAMPIRIC") == 0) return GAME_EVENT_CARD_TAG_VAMPIRIC;

    d_LogWarningF("Unknown game event: %s", str);
    return 0;
}

StatusEffect_t StatusEffectFromString(const char* str) {
    if (!str) return STATUS_NONE;

    // ADR-002: Status effects are outcome modifiers only (no betting modifiers)
    if (strcmp(str, "CHIP_DRAIN") == 0) return STATUS_CHIP_DRAIN;
    if (strcmp(str, "TILT") == 0) return STATUS_TILT;
    if (strcmp(str, "GREED") == 0) return STATUS_GREED;
    if (strcmp(str, "RAKE") == 0) return STATUS_RAKE;

    d_LogWarningF("Unknown status effect: %s", str);
    return STATUS_NONE;
}

PlayerAction_t PlayerActionFromString(const char* str) {
    if (!str) return ACTION_HIT;

    if (strcmp(str, "HIT") == 0) return ACTION_HIT;
    if (strcmp(str, "STAND") == 0) return ACTION_STAND;
    if (strcmp(str, "DOUBLE") == 0) return ACTION_DOUBLE;

    d_LogWarningF("Unknown player action: %s", str);
    return ACTION_HIT;
}

const char* PlayerActionToString(PlayerAction_t action) {
    switch (action) {
        case ACTION_HIT: return "HIT";
        case ACTION_STAND: return "STAND";
        case ACTION_DOUBLE: return "DOUBLE";
        default: return "UNKNOWN_ACTION";
    }
}
