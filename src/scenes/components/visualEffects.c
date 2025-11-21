/*
 * Visual Effects Component - Damage Numbers and Screen Shake
 * Extracted from sceneBlackjack.c (Phase 3 refactoring)
 */

#include "../../../include/scenes/components/visualEffects.h"
#include "../../../include/common.h"
#include "../../../include/defs.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

VisualEffects_t* CreateVisualEffects(TweenManager_t* tween_mgr) {
    if (!tween_mgr) {
        d_LogError("CreateVisualEffects: tween_mgr is NULL");
        return NULL;
    }

    VisualEffects_t* vfx = malloc(sizeof(VisualEffects_t));
    if (!vfx) {
        d_LogError("CreateVisualEffects: malloc failed");
        return NULL;
    }

    // Initialize damage numbers pool
    memset(vfx->damage_numbers, 0, sizeof(vfx->damage_numbers));
    memset(vfx->callback_data, 0, sizeof(vfx->callback_data));

    // Initialize screen shake
    vfx->shake_offset_x = 0.0f;
    vfx->shake_offset_y = 0.0f;
    vfx->shake_cooldown = 0.0f;

    // Store dependencies
    vfx->tween_manager = tween_mgr;

    d_LogInfo("Visual effects component created");
    return vfx;
}

void DestroyVisualEffects(VisualEffects_t** vfx) {
    if (!vfx || !*vfx) return;

    // Note: We don't own tween_manager, so we don't destroy it
    // Active tweens will be cleaned up by tween manager itself

    free(*vfx);
    *vfx = NULL;

    d_LogInfo("Visual effects component destroyed");
}

// ============================================================================
// UPDATE / RENDER
// ============================================================================

void UpdateVisualEffects(VisualEffects_t* vfx, float dt) {
    if (!vfx) return;

    // Decay shake cooldown
    if (vfx->shake_cooldown > 0.0f) {
        vfx->shake_cooldown -= dt;
        if (vfx->shake_cooldown < 0.0f) {
            vfx->shake_cooldown = 0.0f;
        }
    }
}

void RenderDamageNumbers(VisualEffects_t* vfx) {
    if (!vfx) return;

    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        DamageNumber_t* dmg = &vfx->damage_numbers[i];
        if (!dmg->active) continue;

        // Debug: log rendering of healing numbers
        if (dmg->is_healing) {
            d_LogRateLimitedF(D_LOG_RATE_LIMIT_FLAG_HASH_FORMAT_STRING, D_LOG_LEVEL_INFO,
                             1, 0.5,  // 1 log per 0.5 seconds
                             "Rendering healing number: slot=%d, damage=%d, alpha=%.2f, active=%d",
                             i, dmg->damage, dmg->alpha, dmg->active);
        }

        if (dmg->alpha < 0.01f) continue;  // Skip if fully faded

        // Choose color and scale based on type
        aColor_t color;
        float scale;

        if (dmg->is_crit) {
            // CRIT: Gold, larger scale
            color = (aColor_t){255, 204, 0, (int)(dmg->alpha * 255)};  // Gold (#ffcc00)
            scale = 1.5f;
        } else if (dmg->is_healing) {
            // Healing: Green, normal scale
            color = (aColor_t){117, 167, 67, (int)(dmg->alpha * 255)};  // Green
            scale = 1.0f;
        } else {
            // Normal damage: Red, normal scale
            color = (aColor_t){165, 48, 48, (int)(dmg->alpha * 255)};  // Red
            scale = 1.0f;
        }

        // Format damage text
        dString_t* dmg_text = d_StringInit();
        if (dmg->is_crit) {
            d_StringFormat(dmg_text, "CRIT! -%d", dmg->damage);
        } else if (dmg->is_healing) {
            d_StringFormat(dmg_text, "+%d", dmg->damage);
        } else {
            d_StringFormat(dmg_text, "-%d", dmg->damage);
        }

        // Render at current position with alpha and scale
        aFontConfig_t dmg_config = {
            .type = FONT_ENTER_COMMAND,
            .color = color,
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = scale
        };
        a_DrawTextStyled((char*)d_StringPeek(dmg_text), (int)dmg->x, (int)dmg->y, &dmg_config);

        d_StringDestroy(dmg_text);
    }
}

void ApplyScreenShakeViewport(VisualEffects_t* vfx) {
    if (!vfx) return;

    // Only apply if shake is active
    if (vfx->shake_offset_x != 0.0f || vfx->shake_offset_y != 0.0f) {
        SDL_Rect viewport = {
            (int)vfx->shake_offset_x,
            (int)vfx->shake_offset_y,
            SCREEN_WIDTH,
            SCREEN_HEIGHT
        };
        SDL_RenderSetViewport(app.renderer, &viewport);
    }
}

void RestoreViewport(void) {
    // Reset viewport to default (no offset)
    SDL_RenderSetViewport(app.renderer, NULL);
}

// ============================================================================
// DAMAGE NUMBER CALLBACKS
// ============================================================================

static void OnDamageNumberComplete(void* user_data) {
    // Mark damage number slot as free (only if generation matches)
    DamageNumberCallbackData_t* callback_data = (DamageNumberCallbackData_t*)user_data;
    if (callback_data && callback_data->dmg) {
        DamageNumber_t* dmg = callback_data->dmg;

        // Check if this callback is stale (slot was reused with new generation)
        if (dmg->generation != callback_data->generation) {
            d_LogInfoF("Damage number callback IGNORED (stale generation): callback_gen=%u, current_gen=%u",
                      callback_data->generation, dmg->generation);
            return;
        }

        d_LogInfoF("Damage number complete callback fired, marking inactive (damage=%d, is_healing=%d, alpha=%.2f, gen=%u)",
                  dmg->damage, dmg->is_healing, dmg->alpha, dmg->generation);
        dmg->active = false;
        dmg->alpha = 0.0f;  // Force alpha to 0 just in case
    }
}

// ============================================================================
// PUBLIC EFFECTS API
// ============================================================================

void VFX_SpawnDamageNumber(VisualEffects_t* vfx, int damage, float world_x, float world_y, bool is_healing, bool is_crit) {
    if (!vfx) return;

    // Count how many active damage numbers are near this spawn position
    // (to offset Y so they don't perfectly overlap)
    int nearby_count = 0;
    const float NEARBY_THRESHOLD = 50.0f;  // Within 50px horizontally
    const float Y_OFFSET_PER_NUMBER = 30.0f;  // Stack vertically by 30px each

    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (vfx->damage_numbers[i].active) {
            float dx = vfx->damage_numbers[i].x - world_x;
            if (fabs(dx) < NEARBY_THRESHOLD) {
                nearby_count++;
            }
        }
    }

    // Find free slot
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (!vfx->damage_numbers[i].active) {
            DamageNumber_t* dmg = &vfx->damage_numbers[i];

            // Increment generation to invalidate any stale callbacks
            dmg->generation++;

            // Mark inactive first (in case StopTweens prevents callback from firing)
            dmg->active = false;
            dmg->alpha = 0.0f;

            // Stop any existing tweens targeting this slot (in case of premature reuse)
            StopTweensForTarget(vfx->tween_manager, &dmg->y);
            StopTweensForTarget(vfx->tween_manager, &dmg->alpha);

            // Initialize damage number with Y offset based on nearby count
            dmg->active = true;
            dmg->x = world_x;
            dmg->y = world_y - (nearby_count * Y_OFFSET_PER_NUMBER);  // Offset upward
            dmg->alpha = 1.0f;
            dmg->damage = damage;
            dmg->is_healing = is_healing;
            dmg->is_crit = is_crit;

            // Setup callback data with current generation
            vfx->callback_data[i].dmg = dmg;
            vfx->callback_data[i].generation = dmg->generation;

            // Tween Y position (rise 50px over 1.0s from offset position)
            TweenFloat(vfx->tween_manager, &dmg->y, dmg->y - 50.0f,
                       1.0f, TWEEN_EASE_OUT_CUBIC);

            // Tween alpha (fade out over 1.0s)
            TweenFloatWithCallback(vfx->tween_manager, &dmg->alpha, 0.0f,
                                   1.0f, TWEEN_LINEAR,
                                   OnDamageNumberComplete, &vfx->callback_data[i]);

            d_LogInfoF("Spawned damage number: slot=%d, damage=%d, x=%.1f, y=%.1f, is_healing=%d, gen=%u",
                      i, damage, dmg->x, dmg->y, is_healing, dmg->generation);
            return;  // Spawned successfully
        }
    }

    // Pool full - oldest damage number will be overwritten next frame
}

void VFX_TriggerScreenShake(VisualEffects_t* vfx, float intensity, float duration) {
    if (!vfx) return;

    // Cooldown check: prevent shake spam (max once per 0.2s)
    if (vfx->shake_cooldown > 0.0f) {
        return;  // Still on cooldown, skip this shake
    }

    // Stop any existing shake tweens
    StopTweensForTarget(vfx->tween_manager, &vfx->shake_offset_x);
    StopTweensForTarget(vfx->tween_manager, &vfx->shake_offset_y);

    // Random shake direction for natural feel
    float dir_x = (rand() % 2 == 0) ? 1.0f : -1.0f;
    float dir_y = (rand() % 2 == 0) ? 1.0f : -1.0f;

    // Set initial shake offset
    vfx->shake_offset_x = intensity * dir_x;
    vfx->shake_offset_y = intensity * dir_y * 0.5f;  // Less vertical shake

    // Tween back to 0 with elastic easing for bounce effect
    TweenFloat(vfx->tween_manager, &vfx->shake_offset_x, 0.0f, duration, TWEEN_EASE_OUT_ELASTIC);
    TweenFloat(vfx->tween_manager, &vfx->shake_offset_y, 0.0f, duration, TWEEN_EASE_OUT_ELASTIC);

    // Set cooldown (0.2 seconds)
    vfx->shake_cooldown = 0.2f;

    d_LogDebugF("Screen shake triggered: intensity=%.1f, duration=%.2f", intensity, duration);
}
