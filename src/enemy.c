/*
 * Enemy Combat System Implementation
 * Constitutional pattern: dString_t for names, dArray_t for abilities
 */

#include "../include/enemy.h"
#include "../include/tween/tween.h"
#include <stdlib.h>

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
    enemy->passive_abilities = d_InitArray(sizeof(AbilityData_t), 4);
    enemy->active_abilities = d_InitArray(sizeof(AbilityData_t), 4);

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

    d_LogInfoF("Created enemy: %s (HP: %d, Threat: %d)",
               d_StringPeek(enemy->name), max_hp, chip_threat);

    return enemy;
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
        .has_triggered = false
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
        .has_triggered = false
    };

    d_AppendDataToArray(enemy->active_abilities, &ability_data);

    d_LogInfoF("%s gained active ability: %s (trigger: %d, value: %.2f)",
               d_StringPeek(enemy->name), GetAbilityName(ability),
               trigger, trigger_value);
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
        default:                          return "Unknown Ability";
    }
}

// ============================================================================
// COMBAT ACTIONS
// ============================================================================

int EnemyPlaceBet(const Enemy_t* enemy) {
    if (!enemy) return 0;

    // Base bet = chip_threat (chips at risk this round)
    int bet = enemy->chip_threat;

    // TODO: Modify bet based on active abilities or HP thresholds
    // For now, just return base chip threat

    d_LogInfoF("%s bets %d chips (threat)",
               d_StringPeek(enemy->name), bet);

    return bet;
}

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

void CheckAbilityTriggers(Enemy_t* enemy) {
    if (!enemy) return;

    float hp_percent = GetEnemyHPPercent(enemy);

    // Check each active ability
    for (size_t i = 0; i < enemy->active_abilities->count; i++) {
        AbilityData_t* data = (AbilityData_t*)d_IndexDataFromArray(enemy->active_abilities, i);
        if (!data) continue;

        // Skip if already triggered (one-time abilities)
        if (data->trigger == TRIGGER_ONCE_PER_COMBAT && data->has_triggered) {
            continue;
        }

        bool should_trigger = false;

        switch (data->trigger) {
            case TRIGGER_HP_THRESHOLD:
                // Trigger if HP drops below threshold
                if (hp_percent <= data->trigger_value) {
                    should_trigger = true;
                }
                break;

            case TRIGGER_RANDOM:
                // Random chance each round
                // TODO: Implement random roll
                break;

            case TRIGGER_ONCE_PER_COMBAT:
                // Already handled above
                break;

            case TRIGGER_PLAYER_ACTION:
                // TODO: Implement player action hooks
                break;

            default:
                break;
        }

        if (should_trigger) {
            data->has_triggered = true;
            d_LogInfoF("%s triggered ability: %s",
                       d_StringPeek(enemy->name), GetAbilityName(data->ability_id));

            // TODO: Actually execute ability effects
        }
    }
}

void ResetAbilityTriggers(Enemy_t* enemy) {
    if (!enemy) return;

    for (size_t i = 0; i < enemy->active_abilities->count; i++) {
        AbilityData_t* data = (AbilityData_t*)d_IndexDataFromArray(enemy->active_abilities, i);
        if (data) {
            data->has_triggered = false;
        }
    }

    d_LogInfo("Enemy ability triggers reset");
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
