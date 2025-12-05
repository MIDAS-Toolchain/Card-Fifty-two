/*
 * Enemy Combat System Implementation
 * Constitutional pattern: dString_t for names, dArray_t for abilities
 * Data-driven: Abilities loaded from DUF files
 */

#include "../include/enemy.h"
#include "../include/ability.h"
#include "../include/tween/tween.h"
#include "../include/statusEffects.h"
#include "../include/cardTags.h"  // For ClearCardTags()
#include "../include/stats.h"
#include "../include/player.h"
#include "../include/deck.h"
#include "../include/loaders/trinketLoader.h"  // For GetTrinketTemplate
#include "../include/scenes/components/visualEffects.h"  // For VFX_SpawnDamageNumber
#include "../include/scenes/sceneBlackjack.h"  // For GetVisualEffects, ENEMY_HP_BAR constants
#include <stdlib.h>

// External globals
extern dTable_t* g_players;
extern TweenManager_t g_tween_manager;  // From sceneBlackjack.c

// ============================================================================
// LIFECYCLE
// ============================================================================

Enemy_t* CreateEnemy(const char* name, int max_hp) {
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
    enemy->description = d_StringInit();
    if (!enemy->name || !enemy->description) {
        d_LogError("CreateEnemy: Failed to allocate strings");
        if (enemy->name) d_StringDestroy(enemy->name);
        if (enemy->description) d_StringDestroy(enemy->description);
        free(enemy);
        return NULL;
    }
    d_StringSet(enemy->name, name);
    // Description will be set by loader

    // Initialize HP
    enemy->max_hp = max_hp;
    enemy->current_hp = max_hp;
    enemy->display_hp = (float)max_hp;  // Start at max (will be tweened on damage)
    enemy->total_damage_taken = 0;      // Track cumulative damage for TRIGGER_DAMAGE_ACCUMULATOR
    enemy->is_defeated = false;

    // Initialize abilities array (Constitutional: dArray_t storing Ability_t*)
    // Capacity: 8 (most enemies have 3-5 abilities)
    enemy->abilities = d_ArrayInit(8, sizeof(Ability_t*));

    if (!enemy->abilities) {
        d_LogError("CreateEnemy: Failed to allocate abilities array");
        d_StringDestroy(enemy->name);
        d_StringDestroy(enemy->description);
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
    enemy->green_flash_alpha = 0.0f;

    // Initialize defeat animation (start invisible for spawn fade-in)
    enemy->defeat_fade_alpha = 0.0f;  // Start invisible, will fade in
    enemy->defeat_scale = 1.0f;       // Normal size by default

    d_LogInfoF("Created enemy: %s (HP: %d)",
               d_StringPeek(enemy->name), max_hp);

    return enemy;
}

// NOTE: Enemy factory functions (CreateTheDidact, CreateTheDaemon) removed
// Enemies are now loaded from DUF files via LoadEnemyFromDUF()
// See: data/enemies/tutorial_enemies.duf

void DestroyEnemy(Enemy_t** enemy) {
    if (!enemy || !*enemy) return;

    Enemy_t* e = *enemy;

    // NOTE: Card tags (VICIOUS, VAMPIRIC, LUCKY, JAGGED, SHARP) persist across combats
    // They are permanent upgrades from rewards, not temporary combat effects
    // Only DOUBLED tag is temporary (removed in ClearHand())

    // Destroy dString_t fields
    if (e->name) {
        d_StringDestroy(e->name);
        e->name = NULL;
    }

    if (e->description) {
        d_StringDestroy(e->description);
        e->description = NULL;
    }

    // Destroy abilities (each Ability_t* must be freed individually)
    if (e->abilities) {
        for (size_t i = 0; i < e->abilities->count; i++) {
            Ability_t** ability_ptr = (Ability_t**)d_ArrayGet(e->abilities, i);
            if (ability_ptr && *ability_ptr) {
                DestroyAbility(ability_ptr);
            }
        }
        d_ArrayDestroy(e->abilities);
        e->abilities = NULL;
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
// ABILITY MANAGEMENT (Data-Driven System)
// ============================================================================

// NOTE: Old ability management functions removed (AddPassiveAbility, AddActiveAbility, etc.)
// Abilities are now loaded from DUF files via enemyLoader.c
// See: loaders/enemyLoader.h and ability.h

// ============================================================================
// COMBAT ACTIONS
// ============================================================================

void TakeDamage(Enemy_t* enemy, int damage) {
    if (!enemy) return;

    int old_hp = enemy->current_hp;
    enemy->current_hp -= damage;

    // Accumulate total damage (never decreases, even if enemy heals)
    enemy->total_damage_taken += damage;

    // Clamp to 0
    if (enemy->current_hp < 0) {
        enemy->current_hp = 0;
    }

    d_LogInfoF("%s took %d damage (%d -> %d HP, %d total damage)",
               d_StringPeek(enemy->name), damage, old_hp, enemy->current_hp, enemy->total_damage_taken);

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

void HealEnemy(Enemy_t* enemy, int amount, TweenManager_t* tween_manager) {
    if (!enemy) return;

    int old_hp = enemy->current_hp;
    enemy->current_hp += amount;

    // Clamp to max_hp
    if (enemy->current_hp > enemy->max_hp) {
        enemy->current_hp = enemy->max_hp;
    }

    d_LogInfoF("%s healed %d HP (%d -> %d)",
               d_StringPeek(enemy->name), amount, old_hp, enemy->current_hp);

    // Check for heal punish trinkets (find first available charge)
    // NOTE: Only ONE punish triggers per heal (first trinket with charges wins)
    extern Player_t* g_human_player;  // From sceneBlackjack.c
    if (g_human_player) {
        // Find first trinket with available punish charges
        for (int i = 0; i < 6; i++) {
            if (!g_human_player->trinket_slot_occupied[i]) continue;

            TrinketInstance_t* trinket = &g_human_player->trinket_slots[i];
            if (!trinket->base_trinket_key) continue;
            if (trinket->heal_punishes_remaining <= 0) continue;

            // Consume one charge from ONLY THIS trinket (first match wins)
            trinket->heal_punishes_remaining--;

            // Deal damage equal to heal amount
            int punish_damage = amount;
            TakeDamage(enemy, punish_damage);

            // Track stat on this trinket
            TRINKET_ADD_STAT(trinket, TRINKET_STAT_HEAL_DAMAGE_DEALT, punish_damage);

            d_LogInfoF("💔 Trinket slot %d punished heal! Dealt %d damage (%d punishes remaining)",
                       i, punish_damage, trinket->heal_punishes_remaining);

            // Trigger damage flash (red flash instead of green heal flash)
            if (tween_manager) {
                TriggerEnemyDamageEffect(enemy, tween_manager);
            }

            // Show punished heal damage number (red, normal style)
            VisualEffects_t* vfx = GetVisualEffects();
            if (vfx) {
                // Position below enemy HP bar (numbers rise up, so spawn lower)
                float center_x = GetGameAreaX() + (GetGameAreaWidth() / 2.0f) + ENEMY_PORTRAIT_X_OFFSET;
                float center_y = GetEnemyHealthBarY() + 30;  // Below HP bar so it's visible as it rises

                // Spawn red damage number (NOT crit - red is clear enough)
                VFX_SpawnDamageNumber(vfx, punish_damage, center_x, center_y,
                                     false,  // is_healing = false (red damage)
                                     false,  // is_crit = false (normal size, not gold)
                                     false); // is_rake = false
            }

            return;  // Don't show heal flash if we punished the heal
        }
    }

    // Trigger heal visual effect (green flash)
    if (tween_manager) {
        TriggerEnemyHealEffect(enemy, tween_manager);
    }

    // Trigger ENEMY_HEAL event (for other potential trinkets)
    extern GameContext_t g_game;  // From sceneBlackjack.c
    Game_TriggerEvent(&g_game, GAME_EVENT_ENEMY_HEAL);
}

float GetEnemyHPPercent(const Enemy_t* enemy) {
    if (!enemy || enemy->max_hp <= 0) return 0.0f;
    return (float)enemy->current_hp / (float)enemy->max_hp;
}

// ============================================================================
// ABILITY TRIGGERS (Data-Driven System)
// ============================================================================

void CheckEnemyAbilityTriggers(Enemy_t* enemy, GameEvent_t event, GameContext_t* game) {
    if (!enemy || !game) return;

    float hp_percent = GetEnemyHPPercent(enemy);
    int total_damage = enemy->total_damage_taken;

    // Check each ability in unified array
    for (size_t i = 0; i < enemy->abilities->count; i++) {
        Ability_t** ability_ptr = (Ability_t**)d_ArrayGet(enemy->abilities, i);
        if (!ability_ptr || !*ability_ptr) continue;

        Ability_t* ability = *ability_ptr;

        if (CheckAbilityTrigger(ability, event, hp_percent, total_damage)) {
            ExecuteAbility(ability, enemy, game);
        }
    }
}

void ResetAbilityTriggers(Enemy_t* enemy) {
    if (!enemy || !enemy->abilities) return;
    ResetAbilityStates(enemy->abilities);
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

    // Load image as surface using SDL_image
    SDL_Surface* surface = IMG_Load(filename);
    if (!surface) {
        d_LogError("LoadEnemyPortrait: Failed to load image");
        return false;
    }

    // Free existing surface if any
    if (enemy->portrait_surface) {
        SDL_FreeSurface(enemy->portrait_surface);
    }

    // Store surface
    enemy->portrait_surface = surface;
    enemy->portrait_dirty = true;  // Mark dirty to trigger texture creation

    d_LogInfoF("Loaded portrait for %s: %s", d_StringPeek(enemy->name), filename);
    return true;
}

void RefreshEnemyPortraitTexture(Enemy_t* enemy) {
    if (!enemy || !enemy->portrait_surface) return;

    // Free existing texture if any
    if (enemy->portrait_texture) {
        SDL_DestroyTexture(enemy->portrait_texture);
        enemy->portrait_texture = NULL;
    }

    // Create texture directly from original surface (using SDL directly since a_SurfaceToTexture is not implemented)
    enemy->portrait_texture = SDL_CreateTextureFromSurface(app.renderer, enemy->portrait_surface);
    if (!enemy->portrait_texture) {
        d_LogErrorF("Failed to create portrait texture: %s", SDL_GetError());
        return;
    }

    // Clear dirty flag
    enemy->portrait_dirty = false;

    d_LogInfo("Enemy portrait texture created");
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

void TriggerEnemyHealEffect(Enemy_t* enemy, TweenManager_t* tween_manager) {
    if (!enemy || !tween_manager) return;

    // Stop any existing green flash tweens
    StopTweensForTarget(tween_manager, &enemy->green_flash_alpha);

    // Green flash: Start at 0.6 alpha, fade to 0 over 0.5s (slightly longer than damage for soothing feel)
    enemy->green_flash_alpha = 0.6f;
    TweenFloat(tween_manager, &enemy->green_flash_alpha, 0.0f, 0.5f, TWEEN_EASE_OUT_QUAD);

    d_LogInfo("Enemy heal effect triggered (green flash)");
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

float GetEnemyGreenFlashAlpha(const Enemy_t* enemy) {
    if (!enemy) return 0.0f;
    return enemy->green_flash_alpha;
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
