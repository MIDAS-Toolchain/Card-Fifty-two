/*
 * Enemy Combat System Implementation
 * Constitutional pattern: dString_t for names, dArray_t for abilities
 */

#include "../include/enemy.h"
#include "../include/tween/tween.h"
#include "../include/statusEffects.h"
#include "../include/stats.h"
#include "../include/player.h"
#include "../include/deck.h"
#include <stdlib.h>

// External globals
extern dTable_t* g_players;
extern TweenManager_t g_tween_manager;  // From sceneBlackjack.c

// ============================================================================
// LIFECYCLE
// ============================================================================

Enemy_t* CreateEnemy(const char* name, int max_hp, int chip_threat) {
    if (!name) {
        d_LogError("CreateEnemy: NULL name");
        return NULL;
    }

    if (max_hp <= 0) {
        d_LogError("CreateEnemy: Invalid max_hp");
        return NULL;
    }

    Enemy_t* enemy = malloc(sizeof(Enemy_t));
    if (!enemy) {
        d_LogFatal("CreateEnemy: Failed to allocate enemy");
        return NULL;
    }

    // Initialize name (Constitutional: dString_t, not char[])
    enemy->name = d_StringInit();
    if (!enemy->name) {
        d_LogError("CreateEnemy: Failed to allocate name");
        free(enemy);
        return NULL;
    }
    d_StringSet(enemy->name, name, 0);

    // Initialize HP and chip threat
    enemy->max_hp = max_hp;
    enemy->current_hp = max_hp;
    enemy->display_hp = (float)max_hp;  // Start at max (will be tweened on damage)
    enemy->chip_threat = chip_threat;
    enemy->is_defeated = false;

    // Initialize ability arrays (Constitutional: dArray_t, not raw arrays)
    // d_InitArray(capacity, element_size) - capacity FIRST!
    // Capacity: 16 (prevents realloc during combat - avoids dangling pointer bugs in TweenFloat)
    enemy->passive_abilities = d_InitArray(16, sizeof(AbilityData_t));
    enemy->active_abilities = d_InitArray(16, sizeof(AbilityData_t));

    if (!enemy->passive_abilities || !enemy->active_abilities) {
        d_LogError("CreateEnemy: Failed to allocate ability arrays");
        d_StringDestroy(enemy->name);
        if (enemy->passive_abilities) d_DestroyArray(enemy->passive_abilities);
        if (enemy->active_abilities) d_DestroyArray(enemy->active_abilities);
        free(enemy);
        return NULL;
    }

    // Initialize portrait system (will be loaded externally)
    enemy->portrait_surface = NULL;
    enemy->portrait_texture = NULL;
    enemy->portrait_dirty = false;

    // Initialize damage feedback effects
    enemy->shake_offset_x = 0.0f;
    enemy->shake_offset_y = 0.0f;
    enemy->red_flash_alpha = 0.0f;

    // Initialize defeat animation
    enemy->defeat_fade_alpha = 1.0f;  // Fully visible by default
    enemy->defeat_scale = 1.0f;       // Normal size by default

    d_LogInfoF("Created enemy: %s (HP: %d, Threat: %d)",
               d_StringPeek(enemy->name), max_hp, chip_threat);

    return enemy;
}

Enemy_t* CreateTheDidact(void) {
    // Create base enemy (moderate stats for tutorial)
    int didact_maxhp = 500;
    Enemy_t* didact = CreateEnemy("The Didact", didact_maxhp, 10);
    if (!didact) return NULL;

    // Ability 1: The House Remembers (On player blackjack → GREED for 2 rounds)
    // Theme: "The casino is alive, and it never forgets a winner"
    AddEventAbility(didact, ABILITY_THE_HOUSE_REMEMBERS, GAME_EVENT_PLAYER_BLACKJACK);

    // Ability 2: Irregularity Detected (Every 5 cards drawn → CHIP_DRAIN 5 chips/round for 3 rounds)
    // Theme: "You've been noticed. The system is watching."
    AddCounterAbility(didact, ABILITY_IRREGULARITY_DETECTED, GAME_EVENT_CARD_DRAWN, 5);

    // Ability 3: System Override (Below 30% HP once → Heal 50 HP + Force Shuffle)
    // Theme: "The system recalibrates when threatened."
    AddActiveAbility(didact, ABILITY_SYSTEM_OVERRIDE, TRIGGER_HP_THRESHOLD, 0.3f);

    d_LogInfo("The Didact created with 3 teaching abilities");
    return didact;
}

Enemy_t* CreateTheDaemon(void) {
    // Create elite enemy (5000 HP for second encounter)
    int daemon_maxhp = 5000;
    Enemy_t* daemon = CreateEnemy("The Daemon", daemon_maxhp, 10);
    if (!daemon) return NULL;

    // For now, reuse The Didact's abilities (will be customized later)
    // Ability 1: The House Remembers (On player blackjack → GREED for 2 rounds)
    AddEventAbility(daemon, ABILITY_THE_HOUSE_REMEMBERS, GAME_EVENT_PLAYER_BLACKJACK);

    // Ability 2: Irregularity Detected (Every 5 cards drawn → CHIP_DRAIN 5 chips/round for 3 rounds)
    AddCounterAbility(daemon, ABILITY_IRREGULARITY_DETECTED, GAME_EVENT_CARD_DRAWN, 5);

    // Ability 3: System Override (Below 30% HP once → Heal 50 HP + Force Shuffle)
    AddActiveAbility(daemon, ABILITY_SYSTEM_OVERRIDE, TRIGGER_HP_THRESHOLD, 0.3f);

    d_LogInfo("The Daemon created with 5000 HP (reusing Didact abilities)");
    return daemon;
}

void DestroyEnemy(Enemy_t** enemy) {
    if (!enemy || !*enemy) return;

    Enemy_t* e = *enemy;

    // Destroy dString_t name
    if (e->name) {
        d_StringDestroy(e->name);
        e->name = NULL;
    }

    // Destroy ability arrays
    if (e->passive_abilities) {
        d_DestroyArray(e->passive_abilities);
        e->passive_abilities = NULL;
    }

    if (e->active_abilities) {
        d_DestroyArray(e->active_abilities);
        e->active_abilities = NULL;
    }

    // Destroy portrait surface and texture (owned by enemy)
    if (e->portrait_surface) {
        SDL_FreeSurface(e->portrait_surface);
        e->portrait_surface = NULL;
    }

    if (e->portrait_texture) {
        SDL_DestroyTexture(e->portrait_texture);
        e->portrait_texture = NULL;
    }

    // Free enemy struct
    free(e);
    *enemy = NULL;

    d_LogInfo("Enemy destroyed");
}

// ============================================================================
// ABILITY MANAGEMENT
// ============================================================================

void AddPassiveAbility(Enemy_t* enemy, EnemyAbility_t ability) {
    if (!enemy) {
        d_LogError("AddPassiveAbility: NULL enemy");
        return;
    }

    AbilityData_t ability_data = {
        .ability_id = ability,
        .trigger = TRIGGER_NONE,  // Passive = no trigger
        .trigger_value = 0.0f,
        .has_triggered = false,
        .trigger_event = GAME_EVENT_COMBAT_START,
        .counter_max = 0,
        .counter_current = 0,
        .shake_offset_x = 0.0f,
        .shake_offset_y = 0.0f,
        .flash_alpha = 0.0f
    };

    d_AppendDataToArray(enemy->passive_abilities, &ability_data);

    d_LogInfoF("%s gained passive ability: %s",
               d_StringPeek(enemy->name), GetAbilityName(ability));
}

void AddActiveAbility(Enemy_t* enemy, EnemyAbility_t ability,
                      AbilityTrigger_t trigger, float trigger_value) {
    if (!enemy) {
        d_LogError("AddActiveAbility: NULL enemy");
        return;
    }

    AbilityData_t ability_data = {
        .ability_id = ability,
        .trigger = trigger,
        .trigger_value = trigger_value,
        .has_triggered = false,
        .trigger_event = GAME_EVENT_COMBAT_START,  // Default (unused for non-event triggers)
        .counter_max = 0,
        .counter_current = 0,
        .shake_offset_x = 0.0f,
        .shake_offset_y = 0.0f,
        .flash_alpha = 0.0f
    };

    d_AppendDataToArray(enemy->active_abilities, &ability_data);

    d_LogInfoF("%s gained active ability: %s (trigger: %d, value: %.2f)",
               d_StringPeek(enemy->name), GetAbilityName(ability),
               trigger, trigger_value);
}

void AddEventAbility(Enemy_t* enemy, EnemyAbility_t ability, GameEvent_t event) {
    if (!enemy) {
        d_LogError("AddEventAbility: NULL enemy");
        return;
    }

    AbilityData_t ability_data = {
        .ability_id = ability,
        .trigger = TRIGGER_ON_EVENT,
        .trigger_value = 0.0f,
        .has_triggered = false,
        .trigger_event = event,
        .counter_max = 0,
        .counter_current = 0,
        .shake_offset_x = 0.0f,
        .shake_offset_y = 0.0f,
        .flash_alpha = 0.0f
    };

    d_AppendDataToArray(enemy->active_abilities, &ability_data);

    d_LogInfoF("%s gained event ability: %s (event: %s)",
               d_StringPeek(enemy->name), GetAbilityName(ability),
               GameEventToString(event));
}

void AddCounterAbility(Enemy_t* enemy, EnemyAbility_t ability, GameEvent_t event, int counter_max) {
    if (!enemy) {
        d_LogError("AddCounterAbility: NULL enemy");
        return;
    }

    AbilityData_t ability_data = {
        .ability_id = ability,
        .trigger = TRIGGER_COUNTER,
        .trigger_value = 0.0f,
        .has_triggered = false,
        .trigger_event = event,
        .counter_max = counter_max,
        .counter_current = 0,
        .shake_offset_x = 0.0f,
        .shake_offset_y = 0.0f,
        .flash_alpha = 0.0f
    };

    d_AppendDataToArray(enemy->active_abilities, &ability_data);

    d_LogInfoF("%s gained counter ability: %s (event: %s, counter: %d)",
               d_StringPeek(enemy->name), GetAbilityName(ability),
               GameEventToString(event), counter_max);
}

bool HasAbility(const Enemy_t* enemy, EnemyAbility_t ability) {
    if (!enemy) return false;

    // Check passive abilities
    for (size_t i = 0; i < enemy->passive_abilities->count; i++) {
        AbilityData_t* data = (AbilityData_t*)d_IndexDataFromArray(enemy->passive_abilities, i);
        if (data && data->ability_id == ability) {
            return true;
        }
    }

    // Check active abilities
    for (size_t i = 0; i < enemy->active_abilities->count; i++) {
        AbilityData_t* data = (AbilityData_t*)d_IndexDataFromArray(enemy->active_abilities, i);
        if (data && data->ability_id == ability) {
            return true;
        }
    }

    return false;
}

const char* GetAbilityName(EnemyAbility_t ability) {
    switch (ability) {
        case ABILITY_HOUSE_ALWAYS_WINS:   return "House Always Wins";
        case ABILITY_CARD_COUNTER:        return "Card Counter";
        case ABILITY_LOADED_DECK:         return "Loaded Deck";
        case ABILITY_CHIP_DRAIN:          return "Chip Drain";
        case ABILITY_PRESSURE:            return "Pressure";
        case ABILITY_DOUBLE_OR_NOTHING:   return "Double or Nothing";
        case ABILITY_RESHUFFLE_REALITY:   return "Reshuffle Reality";
        case ABILITY_HOUSE_RULES:         return "House Rules";
        case ABILITY_ALL_IN:              return "All In";
        case ABILITY_GLITCH:              return "Glitch";
        case ABILITY_THE_HOUSE_REMEMBERS:    return "The House Remembers";
        case ABILITY_IRREGULARITY_DETECTED:  return "Irregularity Detected";
        case ABILITY_SYSTEM_OVERRIDE:        return "System Override";
        default:                          return "Unknown Ability";
    }
}

// ============================================================================
// COMBAT ACTIONS
// ============================================================================

void TakeDamage(Enemy_t* enemy, int damage) {
    if (!enemy) return;

    int old_hp = enemy->current_hp;
    enemy->current_hp -= damage;

    // Clamp to 0
    if (enemy->current_hp < 0) {
        enemy->current_hp = 0;
    }

    d_LogInfoF("%s took %d damage (%d -> %d HP)",
               d_StringPeek(enemy->name), damage, old_hp, enemy->current_hp);

    // Check for defeat
    if (enemy->current_hp <= 0 && !enemy->is_defeated) {
        enemy->is_defeated = true;
        d_LogInfoF("%s has been defeated!", d_StringPeek(enemy->name));

        // Track combat victory in global stats
        Stats_RecordCombatWon();

        // Trigger defeat animation (fade + zoom out)
        TriggerEnemyDefeatAnimation(enemy, &g_tween_manager);
    }
}

void HealEnemy(Enemy_t* enemy, int amount) {
    if (!enemy) return;

    int old_hp = enemy->current_hp;
    enemy->current_hp += amount;

    // Clamp to max_hp
    if (enemy->current_hp > enemy->max_hp) {
        enemy->current_hp = enemy->max_hp;
    }

    d_LogInfoF("%s healed %d HP (%d -> %d)",
               d_StringPeek(enemy->name), amount, old_hp, enemy->current_hp);
}

float GetEnemyHPPercent(const Enemy_t* enemy) {
    if (!enemy || enemy->max_hp <= 0) return 0.0f;
    return (float)enemy->current_hp / (float)enemy->max_hp;
}

// ============================================================================
// ABILITY TRIGGERS
// ============================================================================

// Helper: Execute ability effects (status effects, rule changes, etc.)
static void ExecuteAbilityEffect(Enemy_t* enemy, EnemyAbility_t ability, GameContext_t* game) {
    if (!enemy || !game) return;

    // Get human player (ID = 1) - Constitutional pattern: use local variable, not compound literal
    // NOTE: g_players stores Player_t BY VALUE, so d_GetDataFromTable returns Player_t*, not Player_t**
    int player_id = 1;
    Player_t* player = (Player_t*)d_GetDataFromTable(g_players, &player_id);
    if (!player) {
        d_LogError("ExecuteAbilityEffect: Player ID 1 not found in g_players");
        return;
    }
    if (!player->status_effects) {
        d_LogError("ExecuteAbilityEffect: Player has no status effects manager");
        return;
    }

    switch (ability) {
        case ABILITY_THE_HOUSE_REMEMBERS:
            // Apply STATUS_GREED for 2 rounds (win only 50% chips)
            ApplyStatusEffect(player->status_effects, STATUS_GREED, 0, 2);
            d_LogInfoF("%s: \"The casino is alive, and it never forgets a winner.\" (GREED applied)",
                      d_StringPeek(enemy->name));
            break;

        case ABILITY_IRREGULARITY_DETECTED:
            // Apply STATUS_CHIP_DRAIN (lose 5 chips/round) for 3 rounds
            ApplyStatusEffect(player->status_effects, STATUS_CHIP_DRAIN, 5, 3);
            d_LogInfoF("%s: \"You've been noticed. The system is watching.\" (CHIP_DRAIN applied)",
                      d_StringPeek(enemy->name));
            break;

        case ABILITY_SYSTEM_OVERRIDE:
            // Heal 50 HP + Force deck shuffle (desperation move)
            HealEnemy(enemy, 50);

            // Force deck shuffle by discarding all cards back to deck
            if (game->deck) {
                // Reshuffle the deck
                ShuffleDeck(game->deck);
                d_LogInfo("The deck has been forcefully reshuffled!");
            }

            d_LogInfoF("%s: \"The casino is taking control. The system recalibrates.\" (Healed 50 HP + Force Shuffle)",
                      d_StringPeek(enemy->name));
            break;

        default:
            d_LogWarningF("ExecuteAbilityEffect: Unimplemented ability %d", ability);
            break;
    }
}

void CheckEnemyAbilityTriggers(Enemy_t* enemy, GameEvent_t event, GameContext_t* game) {
    if (!enemy || !game) return;

    float hp_percent = GetEnemyHPPercent(enemy);

    // Check each active ability
    for (size_t i = 0; i < enemy->active_abilities->count; i++) {
        AbilityData_t* data = (AbilityData_t*)d_IndexDataFromArray(enemy->active_abilities, i);
        if (!data) continue;

        bool should_trigger = false;

        switch (data->trigger) {
            case TRIGGER_HP_THRESHOLD:
                // Trigger if HP drops below threshold (check once per combat)
                if (hp_percent <= data->trigger_value && !data->has_triggered) {
                    should_trigger = true;
                    data->has_triggered = true;  // One-time trigger
                }
                break;

            case TRIGGER_ON_EVENT:
                // Trigger every time specific event occurs
                if (event == data->trigger_event) {
                    should_trigger = true;
                }
                break;

            case TRIGGER_COUNTER:
                // Trigger after N occurrences of event
                if (event == data->trigger_event) {
                    data->counter_current++;
                    d_LogDebugF("%s counter: %d/%d",
                               GetAbilityName(data->ability_id),
                               data->counter_current, data->counter_max);

                    if (data->counter_current >= data->counter_max) {
                        should_trigger = true;
                        data->counter_current = 0;  // Reset counter
                    }
                }
                break;

            case TRIGGER_ONCE_PER_COMBAT:
                // Already handled by has_triggered flag
                break;

            case TRIGGER_RANDOM:
                // TODO: Random chance
                break;

            case TRIGGER_PLAYER_ACTION:
                // TODO: Player action hooks
                break;

            default:
                break;
        }

        if (should_trigger) {
            d_LogInfoF("%s triggered ability: %s (event: %s)",
                       d_StringPeek(enemy->name),
                       GetAbilityName(data->ability_id),
                       GameEventToString(event));

            // Trigger shake/flash animation on ability card (SAFE - uses index-based tweening)
            data->shake_offset_x = 0.0f;
            data->shake_offset_y = 0.0f;
            data->flash_alpha = 255.0f;

            // Use TweenFloatInArray to avoid dangling pointer issues
            // Shake X: 0 → 4px (elastic bounce will overshoot to -4, then settle at 0)
            TweenFloatInArray(&g_tween_manager, &enemy->active_abilities, i,
                              offsetof(AbilityData_t, shake_offset_x),
                              4.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);

            // Shake Y: 0 → -3px (elastic bounce will overshoot to +3, then settle at 0)
            TweenFloatInArray(&g_tween_manager, &enemy->active_abilities, i,
                              offsetof(AbilityData_t, shake_offset_y),
                              -3.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);

            // Flash: Red overlay fade from 255 → 0
            TweenFloatInArray(&g_tween_manager, &enemy->active_abilities, i,
                              offsetof(AbilityData_t, flash_alpha),
                              0.0f, 0.5f, TWEEN_EASE_OUT_QUAD);

            // Execute ability effects
            ExecuteAbilityEffect(enemy, data->ability_id, game);
        }
    }
}

void ResetAbilityTriggers(Enemy_t* enemy) {
    if (!enemy) return;

    for (size_t i = 0; i < enemy->active_abilities->count; i++) {
        AbilityData_t* data = (AbilityData_t*)d_IndexDataFromArray(enemy->active_abilities, i);
        if (data) {
            data->has_triggered = false;
            data->counter_current = 0;  // Reset counters too
        }
    }

    d_LogInfo("Enemy ability triggers and counters reset");
}

// ============================================================================
// QUERIES
// ============================================================================

void EnemyToString(const Enemy_t* enemy, dString_t* out) {
    if (!enemy || !out) return;

    d_StringClear(out);
    d_StringAppend(out, d_StringPeek(enemy->name), d_StringGetLength(enemy->name));
    d_StringAppend(out, " | HP: ", 7);
    d_StringAppendInt(out, enemy->current_hp);
    d_StringAppend(out, "/", 1);
    d_StringAppendInt(out, enemy->max_hp);
    d_StringAppend(out, " | Threat: ", 11);
    d_StringAppendInt(out, enemy->chip_threat);
}

const char* GetEnemyName(const Enemy_t* enemy) {
    if (!enemy || !enemy->name) return "Unknown Enemy";
    return d_StringPeek(enemy->name);
}

// ============================================================================
// PORTRAIT MANAGEMENT
// ============================================================================

bool LoadEnemyPortrait(Enemy_t* enemy, const char* filename) {
    if (!enemy) {
        d_LogError("LoadEnemyPortrait: NULL enemy");
        return false;
    }

    if (!filename) {
        d_LogError("LoadEnemyPortrait: NULL filename");
        return false;
    }

    // Load image as surface using Archimedes
    SDL_Surface* surface = a_Image(filename);
    if (!surface) {
        d_LogError("LoadEnemyPortrait: Failed to load image");
        return false;
    }

    // Free existing surface if any
    if (enemy->portrait_surface) {
        SDL_FreeSurface(enemy->portrait_surface);
    }

    // Free existing texture if any
    if (enemy->portrait_texture) {
        SDL_DestroyTexture(enemy->portrait_texture);
        enemy->portrait_texture = NULL;
    }

    // Store surface and mark as dirty
    enemy->portrait_surface = surface;
    enemy->portrait_dirty = true;

    d_LogInfoF("Loaded portrait for %s: %s", d_StringPeek(enemy->name), filename);
    return true;
}

void RefreshEnemyPortraitTexture(Enemy_t* enemy) {
    if (!enemy || !enemy->portrait_surface) return;

    // Free existing texture
    if (enemy->portrait_texture) {
        SDL_DestroyTexture(enemy->portrait_texture);
        enemy->portrait_texture = NULL;
    }

    // Convert surface to texture using Archimedes (destroy=0, we own the surface)
    enemy->portrait_texture = a_ToTexture(enemy->portrait_surface, 0);
    if (!enemy->portrait_texture) {
        d_LogError("RefreshEnemyPortraitTexture: Failed to create texture");
        return;
    }

    // Clear dirty flag
    enemy->portrait_dirty = false;

    d_LogInfo("Enemy portrait texture refreshed");
}

SDL_Texture* GetEnemyPortraitTexture(Enemy_t* enemy) {
    if (!enemy) return NULL;

    // Auto-refresh if dirty
    if (enemy->portrait_dirty && enemy->portrait_surface) {
        RefreshEnemyPortraitTexture(enemy);
    }

    return enemy->portrait_texture;
}

// ============================================================================
// DAMAGE FEEDBACK EFFECTS
// ============================================================================

void TriggerEnemyDamageEffect(Enemy_t* enemy, TweenManager_t* tween_manager) {
    if (!enemy || !tween_manager) return;

    // Stop any existing shake/flash tweens first
    StopTweensForTarget(tween_manager, &enemy->shake_offset_x);
    StopTweensForTarget(tween_manager, &enemy->red_flash_alpha);

    // Shake: Start at -15px offset (instant), then elastic snap back to 0
    // This creates a visible jolt followed by bouncy recovery
    enemy->shake_offset_x = -15.0f;
    TweenFloat(tween_manager, &enemy->shake_offset_x, 0.0f, 0.5f, TWEEN_EASE_OUT_ELASTIC);

    // Red flash: Start at 0.7 alpha, fade to 0 over 0.4s
    enemy->red_flash_alpha = 0.7f;
    TweenFloat(tween_manager, &enemy->red_flash_alpha, 0.0f, 0.4f, TWEEN_EASE_OUT_QUAD);

    d_LogInfo("Enemy damage effect triggered (shake + red flash)");
}

void GetEnemyShakeOffset(const Enemy_t* enemy, float* out_x, float* out_y) {
    if (!enemy) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }

    if (out_x) *out_x = enemy->shake_offset_x;
    if (out_y) *out_y = enemy->shake_offset_y;
}

float GetEnemyRedFlashAlpha(const Enemy_t* enemy) {
    if (!enemy) return 0.0f;
    return enemy->red_flash_alpha;
}

void TriggerEnemyDefeatAnimation(Enemy_t* enemy, TweenManager_t* tween_manager) {
    if (!enemy || !tween_manager) return;

    // Stop any existing defeat tweens
    StopTweensForTarget(tween_manager, &enemy->defeat_fade_alpha);
    StopTweensForTarget(tween_manager, &enemy->defeat_scale);

    // Fade out: alpha 1.0 → 0.0 over 1.5s (smooth cubic easing)
    enemy->defeat_fade_alpha = 1.0f;
    TweenFloat(tween_manager, &enemy->defeat_fade_alpha, 0.0f, 1.5f, TWEEN_EASE_OUT_CUBIC);

    // Zoom out: scale 1.0 → 0.8 over 1.5s (same easing for sync)
    enemy->defeat_scale = 1.0f;
    TweenFloat(tween_manager, &enemy->defeat_scale, 0.8f, 1.5f, TWEEN_EASE_OUT_CUBIC);

    d_LogInfo("Enemy defeat animation triggered (fade + zoom out)");
}

float GetEnemyDefeatAlpha(const Enemy_t* enemy) {
    if (!enemy) return 1.0f;
    return enemy->defeat_fade_alpha;
}

float GetEnemyDefeatScale(const Enemy_t* enemy) {
    if (!enemy) return 1.0f;
    return enemy->defeat_scale;
}
