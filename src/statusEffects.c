/*
 * Status Effects System - Token Warfare
 * Persistent debuffs that drain chips, restrict betting, and modify outcomes
 */

#include "../include/statusEffects.h"
#include "../include/scenes/sceneBlackjack.h"
#include "../include/tween/tween.h"
#include "../include/random.h"
#include <stdlib.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

StatusEffectManager_t* CreateStatusEffectManager(void) {
    StatusEffectManager_t* manager = malloc(sizeof(StatusEffectManager_t));
    if (!manager) {
        d_LogFatal("CreateStatusEffectManager: Failed to allocate manager");
        return NULL;
    }

    // Initialize effects array (Constitutional: dArray_t, not raw array)
    // d_InitArray(capacity, element_size) - capacity FIRST!
    // Capacity: 32 (prevents realloc during combat - avoids dangling pointer bugs in TweenFloat)
    manager->active_effects = d_InitArray(32, sizeof(StatusEffectInstance_t));
    if (!manager->active_effects) {
        d_LogError("CreateStatusEffectManager: Failed to allocate effects array");
        free(manager);
        return NULL;
    }

    d_LogInfo("Status effect manager created");
    return manager;
}

void DestroyStatusEffectManager(StatusEffectManager_t** manager) {
    if (!manager || !*manager) return;

    StatusEffectManager_t* mgr = *manager;

    // Destroy effects array
    if (mgr->active_effects) {
        d_DestroyArray(mgr->active_effects);
        mgr->active_effects = NULL;
    }

    free(mgr);
    *manager = NULL;

    d_LogInfo("Status effect manager destroyed");
}

// ============================================================================
// EFFECT MANAGEMENT
// ============================================================================

void ApplyStatusEffect(StatusEffectManager_t* manager,
                      StatusEffect_t type,
                      int value,
                      int duration) {
    if (!manager || type == STATUS_NONE) return;

    // Defensive: Verify active_effects array exists
    if (!manager->active_effects) {
        d_LogError("ApplyStatusEffect: manager->active_effects is NULL!");
        return;
    }

    // Check if effect already exists (refresh duration)
    for (size_t i = 0; i < manager->active_effects->count; i++) {
        StatusEffectInstance_t* effect = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, i);

        if (effect && effect->type == type) {
            // Refresh existing effect
            effect->duration = duration;
            effect->value = value;
            effect->intensity = 1.0f;

            // Trigger shake animation on refresh (SAFE - uses index-based tweening)
            TweenManager_t* tween_mgr = GetTweenManager();
            if (tween_mgr) {
                d_LogInfoF("🔴 SHAKE ANIMATION TRIGGERED (refresh) for %s", GetStatusEffectName(type));
                // Reset shake offsets and retrigger
                effect->shake_offset_x = 0.0f;
                effect->shake_offset_y = 0.0f;

                // Use TweenFloatInArray to avoid dangling pointer issues
                TweenFloatInArray(tween_mgr, &manager->active_effects, i,
                                  offsetof(StatusEffectInstance_t, shake_offset_x),
                                  5.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);
                TweenFloatInArray(tween_mgr, &manager->active_effects, i,
                                  offsetof(StatusEffectInstance_t, shake_offset_y),
                                  -3.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);

                // Red flash
                effect->flash_alpha = 255.0f;
                TweenFloatInArray(tween_mgr, &manager->active_effects, i,
                                  offsetof(StatusEffectInstance_t, flash_alpha),
                                  0.0f, 0.5f, TWEEN_EASE_OUT_QUAD);
            }

            d_LogInfoF("Status effect refreshed: %s (value=%d, duration=%d)",
                      GetStatusEffectName(type), value, duration);
            return;
        }
    }

    // Add new effect
    StatusEffectInstance_t new_effect = {
        .type = type,
        .value = value,
        .duration = duration,
        .intensity = 1.0f,
        .shake_offset_x = 0.0f,
        .shake_offset_y = 0.0f,
        .flash_alpha = 0.0f
    };

    d_AppendDataToArray(manager->active_effects, &new_effect);

    // Verify append succeeded before using count-1 as index
    if (manager->active_effects->count == 0) {
        d_LogError("ApplyStatusEffect: d_AppendDataToArray failed (count still 0)");
        return;
    }

    // Trigger shake animation using tween system (SAFE - uses index-based tweening)
    size_t new_effect_index = manager->active_effects->count - 1;
    TweenManager_t* tween_mgr = GetTweenManager();

    if (tween_mgr) {
        d_LogInfoF("🔴 SHAKE ANIMATION TRIGGERED for %s", GetStatusEffectName(type));

        // Use TweenFloatInArray to avoid dangling pointer issues
        // Shake X: 0 → 5 → -5 → 0 (elastic bounce)
        TweenFloatInArray(tween_mgr, &manager->active_effects, new_effect_index,
                          offsetof(StatusEffectInstance_t, shake_offset_x),
                          5.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);
        TweenFloatInArray(tween_mgr, &manager->active_effects, new_effect_index,
                          offsetof(StatusEffectInstance_t, shake_offset_y),
                          -3.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);

        // Red flash: 255 → 0 (fade out)
        StatusEffectInstance_t* added_effect = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, new_effect_index);
        if (added_effect) {
            added_effect->flash_alpha = 255.0f;
        }
        TweenFloatInArray(tween_mgr, &manager->active_effects, new_effect_index,
                          offsetof(StatusEffectInstance_t, flash_alpha),
                          0.0f, 0.5f, TWEEN_EASE_OUT_QUAD);
    } else {
        d_LogError("❌ GetTweenManager() returned NULL - shake animation FAILED");
    }

    d_LogInfoF("Status effect applied: %s (value=%d, duration=%d)",
              GetStatusEffectName(type), value, duration);
}

void RemoveStatusEffect(StatusEffectManager_t* manager, StatusEffect_t type) {
    if (!manager) return;

    for (size_t i = 0; i < manager->active_effects->count; i++) {
        StatusEffectInstance_t* effect = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, i);

        if (effect && effect->type == type) {
            // Remove by swapping with last and decrementing count
            if (i < manager->active_effects->count - 1) {
                StatusEffectInstance_t* last = (StatusEffectInstance_t*)
                    d_IndexDataFromArray(manager->active_effects,
                                        manager->active_effects->count - 1);
                if (last) {
                    *effect = *last;
                }
            }
            manager->active_effects->count--;

            d_LogInfoF("Status effect removed: %s", GetStatusEffectName(type));
            return;
        }
    }
}

bool HasStatusEffect(const StatusEffectManager_t* manager, StatusEffect_t type) {
    if (!manager) return false;

    for (size_t i = 0; i < manager->active_effects->count; i++) {
        const StatusEffectInstance_t* effect = (const StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, i);

        if (effect && effect->type == type && effect->duration > 0) {
            return true;
        }
    }

    return false;
}

StatusEffectInstance_t* GetStatusEffect(StatusEffectManager_t* manager, StatusEffect_t type) {
    if (!manager) return NULL;

    for (size_t i = 0; i < manager->active_effects->count; i++) {
        StatusEffectInstance_t* effect = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, i);

        if (effect && effect->type == type && effect->duration > 0) {
            return effect;
        }
    }

    return NULL;
}

ssize_t GetStatusEffectIndex(StatusEffectManager_t* manager, StatusEffect_t type) {
    if (!manager) return -1;

    for (size_t i = 0; i < manager->active_effects->count; i++) {
        StatusEffectInstance_t* effect = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, i);

        if (effect && effect->type == type && effect->duration > 0) {
            return (ssize_t)i;
        }
    }

    return -1;  // Not found
}

// ============================================================================
// ROUND PROCESSING
// ============================================================================

void ProcessStatusEffectsRoundStart(StatusEffectManager_t* manager, Player_t* player) {
    if (!manager || !player) return;

    // Process CHIP_DRAIN - lose X chips per round
    // Use GetStatusEffectIndex() for safe tweening (avoids dangling pointer!)
    ssize_t drain_idx = GetStatusEffectIndex(manager, STATUS_CHIP_DRAIN);
    if (drain_idx >= 0) {
        StatusEffectInstance_t* drain = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, drain_idx);

        if (!drain) return;  // Safety check

        int drain_amount = drain->value;
        if (drain_amount > player->chips) {
            drain_amount = player->chips;  // Can't lose more than you have
        }

        player->chips -= drain_amount;

        d_LogInfoF("CHIP_DRAIN: Lost %d chips (remaining: %d)",
                  drain_amount, player->chips);

        // Track drain amount for result screen animation
        SetStatusEffectDrainAmount(drain_amount);

        // Trigger flash animation (icon flashes red in sidebar!)
        // SAFE: Use TweenFloatInArray to avoid dangling pointer from array reallocation
        TweenManager_t* tween_mgr = GetTweenManager();
        if (tween_mgr) {
            drain->flash_alpha = 255.0f;
            TweenFloatInArray(tween_mgr, &manager->active_effects, (size_t)drain_idx,
                              offsetof(StatusEffectInstance_t, flash_alpha),
                              0.0f, 0.8f, TWEEN_EASE_OUT_QUAD);
        }

        // TODO: Spawn damage number, play sound effect
    }
}

void ProcessStatusEffectsRoundEnd(StatusEffectManager_t* manager, Player_t* player) {
    (void)manager;  // Reserved for future round-end effects
    (void)player;
}

void ClearAllStatusEffects(StatusEffectManager_t* manager) {
    if (!manager || !manager->active_effects) return;

    size_t count = manager->active_effects->count;
    if (count > 0) {
        d_LogInfoF("Clearing all status effects (%zu effects removed)", count);
        manager->active_effects->count = 0;  // Clear all effects instantly
    }
}

void TickStatusEffectDurations(StatusEffectManager_t* manager) {
    if (!manager) return;

    d_LogDebugF("TickStatusEffectDurations: Starting with %zu active effects",
               manager->active_effects->count);

    // Iterate backwards so we can remove expired effects safely
    for (int i = (int)manager->active_effects->count - 1; i >= 0; i--) {
        StatusEffectInstance_t* effect = (StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, (size_t)i);

        if (!effect) continue;

        d_LogDebugF("Ticking %s: duration %d -> %d",
                   GetStatusEffectName(effect->type), effect->duration, effect->duration - 1);

        // Decrement duration
        effect->duration--;

        // Update intensity for fading visual effect (last round)
        if (effect->duration <= 0) {
            effect->intensity = 0.0f;
        } else if (effect->duration == 1) {
            effect->intensity = 0.5f;  // Fade warning
        }

        // Remove if expired
        if (effect->duration <= 0) {
            d_LogInfoF("Status effect expired: %s (was at duration 0)", GetStatusEffectName(effect->type));

            // Swap with last and decrement count
            if (i < (int)manager->active_effects->count - 1) {
                StatusEffectInstance_t* last = (StatusEffectInstance_t*)
                    d_IndexDataFromArray(manager->active_effects,
                                        manager->active_effects->count - 1);
                if (last) {
                    *effect = *last;
                }
            }
            manager->active_effects->count--;
        }
    }
}

// ============================================================================
// BETTING MODIFIERS
// ============================================================================

int GetMinimumBetWithEffects(const StatusEffectManager_t* manager, int base_min) {
    if (!manager) return base_min;

    int modified_min = base_min;

    // Check MINIMUM_BET effect
    StatusEffectInstance_t* min_bet = GetStatusEffect((StatusEffectManager_t*)manager,
                                                       STATUS_MINIMUM_BET);
    if (min_bet && min_bet->value > modified_min) {
        modified_min = min_bet->value;
    }

    return modified_min;
}

bool CanAdjustBet(const StatusEffectManager_t* manager) {
    if (!manager) return true;

    // Can't adjust if any of these effects are active
    if (HasStatusEffect(manager, STATUS_NO_ADJUST)) return false;
    if (HasStatusEffect(manager, STATUS_FORCED_ALL_IN)) return false;
    if (HasStatusEffect(manager, STATUS_MADNESS)) return false;

    return true;
}

int ModifyBetWithEffects(StatusEffectManager_t* manager, int bet, Player_t* player) {
    if (!manager || !player) return bet;

    // FORCED_ALL_IN - must bet all chips
    if (HasStatusEffect(manager, STATUS_FORCED_ALL_IN)) {
        d_LogInfo("FORCED_ALL_IN: Betting all chips!");
        return player->chips;
    }

    // MADNESS - random bet amount (high-quality RNG)
    if (HasStatusEffect(manager, STATUS_MADNESS)) {
        int random_bet = GetRandomInt(10, 100);
        if (random_bet > player->chips) {
            random_bet = player->chips;
        }
        d_LogInfoF("MADNESS: Random bet %d chips!", random_bet);
        return random_bet;
    }

    // TODO: ESCALATION - must be higher than last bet
    // Would need to store last bet amount in manager or player

    return bet;
}

// ============================================================================
// OUTCOME MODIFIERS
// ============================================================================

int ModifyWinnings(const StatusEffectManager_t* manager, int base_winnings, int bet_amount) {
    if (!manager) return base_winnings;

    int modified = base_winnings;

    // GREED - win only 50% of bet (not 50% of winnings!)
    // This ensures predictable payouts regardless of win type (1:1 vs 3:2)
    if (HasStatusEffect(manager, STATUS_GREED)) {
        modified = bet_amount / 2;
        d_LogInfoF("GREED: Winnings capped at 50%% of bet (%d → %d)", base_winnings, modified);
    }

    return modified;
}

int ModifyLosses(const StatusEffectManager_t* manager, int base_loss) {
    if (!manager) return 0;  // No additional loss

    int additional_loss = 0;

    // TILT - lose 2x on losses (100% extra penalty)
    if (HasStatusEffect(manager, STATUS_TILT)) {
        additional_loss = base_loss;  // Same as base = 2x total
        d_LogInfoF("TILT: Additional loss penalty %d chips", additional_loss);
    }

    return additional_loss;
}

// ============================================================================
// UI/QUERY FUNCTIONS
// ============================================================================

const char* GetStatusEffectName(StatusEffect_t type) {
    switch (type) {
        case STATUS_CHIP_DRAIN:    return "Chip Drain";
        case STATUS_TILT:          return "Tilt";
        case STATUS_GREED:         return "Greed";
        case STATUS_MADNESS:       return "Madness";
        case STATUS_FORCED_ALL_IN: return "All-In";
        case STATUS_ESCALATION:    return "Escalation";
        case STATUS_NO_ADJUST:     return "No Adjust";
        case STATUS_MINIMUM_BET:   return "Min Bet";
        default:                   return "Unknown";
    }
}

const char* GetStatusEffectAbbreviation(StatusEffect_t type) {
    switch (type) {
        case STATUS_CHIP_DRAIN:    return "Cd";  // Chip drain
        case STATUS_TILT:          return "Ti";
        case STATUS_GREED:         return "Gr";
        case STATUS_MADNESS:       return "Ma";
        case STATUS_FORCED_ALL_IN: return "Al";
        case STATUS_ESCALATION:    return "Es";
        case STATUS_NO_ADJUST:     return "Na";
        case STATUS_MINIMUM_BET:   return "Mb";
        default:                   return "??";
    }
}

const char* GetStatusEffectDescription(StatusEffect_t type) {
    switch (type) {
        case STATUS_CHIP_DRAIN:    return "Lose chips each round";
        case STATUS_TILT:          return "Lose 2x chips on loss";
        case STATUS_GREED:         return "Win only 50% chips";
        case STATUS_MADNESS:       return "Random bet amounts";
        case STATUS_FORCED_ALL_IN: return "Must bet all chips";
        case STATUS_ESCALATION:    return "Must increase bet";
        case STATUS_NO_ADJUST:     return "Can't change bet";
        case STATUS_MINIMUM_BET:   return "Increased minimum bet";
        default:                   return "Unknown effect";
    }
}

aColor_t GetStatusEffectColor(StatusEffect_t type) {
    switch (type) {
        case STATUS_CHIP_DRAIN:    return (aColor_t){200, 50, 50, 255};   // Red
        case STATUS_TILT:          return (aColor_t){255, 50, 50, 255};   // Bright red
        case STATUS_GREED:         return (aColor_t){200, 200, 50, 255};  // Yellow
        case STATUS_MADNESS:       return (aColor_t){200, 50, 200, 255};  // Purple
        case STATUS_FORCED_ALL_IN: return (aColor_t){255, 100, 0, 255};   // Orange
        case STATUS_ESCALATION:    return (aColor_t){255, 150, 0, 255};   // Light orange
        case STATUS_NO_ADJUST:     return (aColor_t){100, 100, 100, 255}; // Gray
        case STATUS_MINIMUM_BET:   return (aColor_t){150, 150, 50, 255};  // Dark yellow
        default:                   return (aColor_t){100, 100, 100, 255}; // Gray
    }
}

void RenderStatusEffects(const StatusEffectManager_t* manager, int x, int y) {
    if (!manager || !manager->active_effects) return;

    const int ICON_SIZE = 48;
    const int ICON_SPACING = 8;
    const int MAX_PER_ROW = 6;

    int current_x = x;
    int current_y = y;
    int count = 0;

    for (size_t i = 0; i < manager->active_effects->count; i++) {
        const StatusEffectInstance_t* effect = (const StatusEffectInstance_t*)
            d_IndexDataFromArray(manager->active_effects, i);

        if (!effect || effect->duration <= 0) continue;

        // Wrap to next row after MAX_PER_ROW
        if (count > 0 && count % MAX_PER_ROW == 0) {
            current_x = x;
            current_y += ICON_SIZE + ICON_SPACING;
        }

        // Apply shake offsets (tweened during animation)
        int shake_x = current_x + (int)effect->shake_offset_x;
        int shake_y = current_y + (int)effect->shake_offset_y;

        // Draw icon background (color-coded by effect type)
        aColor_t bg_color = GetStatusEffectColor(effect->type);
        bg_color.a = 200;
        a_DrawFilledRect((aRectf_t){shake_x, shake_y, ICON_SIZE, ICON_SIZE}, bg_color);

        // Draw border (pulsing based on intensity)
        Uint8 border_alpha = (Uint8)(150 + 100 * effect->intensity);
        a_DrawRect((aRectf_t){shake_x, shake_y, ICON_SIZE, ICON_SIZE},
                  (aColor_t){255, 255, 255, border_alpha});

        // Draw red flash overlay (triggered when effect is applied)
        if (effect->flash_alpha > 0.0f) {
            Uint8 flash = (Uint8)effect->flash_alpha;
            a_DrawFilledRect((aRectf_t){shake_x, shake_y, ICON_SIZE, ICON_SIZE},
                            (aColor_t){255, 0, 0, flash});
        }

        // Draw effect icon (custom 2-letter abbreviation)
        const char* icon_text = GetStatusEffectAbbreviation(effect->type);

        aTextStyle_t icon_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {255, 255, 255, 255},
            .bg = {0, 0, 0, 0},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.0f,
            .padding = 0
        };
        a_DrawText(icon_text,
                        shake_x + ICON_SIZE/2,
                        shake_y + ICON_SIZE/2 - 8,
                        icon_config);

        // Draw duration counter (bottom right)
        dString_t* duration_str = d_StringInit();
        d_StringFormat(duration_str, "%d", effect->duration);

        aTextStyle_t duration_config = {
            .type = FONT_GAME,
            .fg = {255, 255, 0, 255},  // Yellow
            .bg = {0, 0, 0, 0},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.0f,
            .padding = 0
        };
        a_DrawText((char*)d_StringPeek(duration_str),
                        shake_x + ICON_SIZE - 10,
                        shake_y + ICON_SIZE - 12,
                        duration_config);
        d_StringDestroy(duration_str);

        // Draw value if applicable (e.g., "10" for chip drain)
        if (effect->value > 0) {
            dString_t* value_str = d_StringInit();
            d_StringFormat(value_str, "%d", effect->value);

            aTextStyle_t value_config = {
                .type = FONT_GAME,
                .fg = {255, 150, 150, 255},
                .bg = {0, 0, 0, 0},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = 0,
                .scale = 0.8f,
                .padding = 0
            };
            a_DrawText((char*)d_StringPeek(value_str),
                            shake_x + 4,
                            shake_y + 4,
                            value_config);
            d_StringDestroy(value_str);
        }

        current_x += ICON_SIZE + ICON_SPACING;
        count++;
    }
}
