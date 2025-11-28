/*
 * Result Screen Component Implementation
 */

#include "../../../include/scenes/components/resultScreen.h"
#include "../../../include/scenes/sections/leftSidebarSection.h"
#include "../../../include/random.h"
#include <math.h>

// External references
extern LeftSidebarSection_t* g_left_sidebar;  // For bet animation callback
extern TweenManager_t g_tween_manager;         // For smooth animations

// ============================================================================
// OVERLAY CONSTANTS
// ============================================================================

#define OVERLAY_ALPHA 128  // Semi-transparent black overlay

// ============================================================================
// LIFECYCLE
// ============================================================================

ResultScreen_t* CreateResultScreen(void) {
    ResultScreen_t* screen = malloc(sizeof(ResultScreen_t));
    if (!screen) {
        d_LogError("Failed to allocate ResultScreen_t");
        return NULL;
    }

    // Initialize scalar fields
    screen->display_chips = 0.0f;
    screen->old_chips = 0;
    screen->chip_delta = 0;
    screen->status_drain = 0;
    screen->timer = 0.0f;
    screen->winloss_started = false;
    screen->winloss_alpha = 0.0f;
    screen->winloss_offset_x = 0.0f;
    screen->winloss_offset_y = 0.0f;
    screen->bleed_started = false;
    screen->bleed_alpha = 0.0f;
    screen->bleed_offset_x = 0.0f;
    screen->bleed_offset_y = 0.0f;

    // Create FlexBox layouts for effect stacking (pattern from leftSidebarSection.c)
    // Positive effects box (center-right, next to "YOU WIN" message)
    screen->positive_effects_box = a_FlexBoxCreate(
        SCREEN_WIDTH / 2 + 120,  // X: Just to the right of center message
        SCREEN_HEIGHT / 2 - 60,  // Y: Aligned with "YOU WIN" message
        250,                     // Width (room for "+50 Win" etc)
        200                      // Height (room for 3-4 effects)
    );
    a_FlexConfigure(screen->positive_effects_box,
                   FLEX_DIR_COLUMN,        // Stack vertically
                   FLEX_JUSTIFY_START,     // Top-aligned
                   8);                     // 8px gap between items

    // Negative effects box (center-right, stacked below positive effects)
    screen->negative_effects_box = a_FlexBoxCreate(
        SCREEN_WIDTH / 2 + 120,  // X: Same alignment as positive box
        SCREEN_HEIGHT / 2 + 20,  // Y: Just below center, stacks under positive
        250,                     // Width (same as positive)
        200                      // Height (room for 3-4 effects)
    );
    a_FlexConfigure(screen->negative_effects_box,
                   FLEX_DIR_COLUMN,        // Stack vertically
                   FLEX_JUSTIFY_START,     // Top-aligned (stack downward)
                   8);                     // 8px gap between items

    // Create dynamic effect arrays (Daedalus pattern)
    screen->positive_effects = d_ArrayInit(4, sizeof(EffectDisplay_t));  // Capacity: 4 effects
    screen->negative_effects = d_ArrayInit(4, sizeof(EffectDisplay_t));  // Capacity: 4 effects

    d_LogInfo("Result screen created with FlexBox effect stacking");
    return screen;
}

void DestroyResultScreen(ResultScreen_t** screen) {
    if (!screen || !*screen) return;

    // Clean up FlexBoxes
    if ((*screen)->positive_effects_box) {
        a_FlexBoxDestroy(&(*screen)->positive_effects_box);
    }
    if ((*screen)->negative_effects_box) {
        a_FlexBoxDestroy(&(*screen)->negative_effects_box);
    }

    // Clean up effect arrays (no label cleanup needed - static strings!)
    if ((*screen)->positive_effects) {
        d_ArrayDestroy((*screen)->positive_effects);
        (*screen)->positive_effects = NULL;
    }

    if ((*screen)->negative_effects) {
        d_ArrayDestroy((*screen)->negative_effects);
        (*screen)->negative_effects = NULL;
    }

    free(*screen);
    *screen = NULL;
    d_LogInfo("Result screen destroyed");
}

// ============================================================================
// DISPLAY
// ============================================================================

void ShowResultScreen(ResultScreen_t* screen, int old_chips, int chip_delta, int status_drain, bool is_victory) {
    if (!screen) return;

    screen->old_chips = old_chips;
    screen->chip_delta = chip_delta;
    screen->status_drain = status_drain;
    screen->display_chips = (float)old_chips;  // Start slot animation from old value
    screen->timer = 0.0f;

    // Reset DEPRECATED animation flags (for backward compat)
    screen->winloss_started = false;
    screen->bleed_started = false;
    screen->winloss_offset_x = GetRandomFloat(-30.0f, 30.0f);
    screen->winloss_offset_y = GetRandomFloat(-20.0f, 20.0f);
    screen->bleed_offset_x = GetRandomFloat(-30.0f, 30.0f);
    screen->bleed_offset_y = GetRandomFloat(-20.0f, 20.0f);

    // ========================================================================
    // NEW: Populate FlexBox effects (auto-positioned, no collision detection!)
    // ========================================================================

    // Clear old FlexBox items and effects (no heap cleanup needed - static labels!)
    a_FlexClearItems(screen->positive_effects_box);
    a_FlexClearItems(screen->negative_effects_box);
    d_ArrayClear(screen->positive_effects);
    d_ArrayClear(screen->negative_effects);

    // Add win/loss effect
    if (chip_delta > 0) {
        // POSITIVE: "+50 Win" (green, center-right)
        EffectDisplay_t effect = {
            .label = "Win",  // Static string, no heap allocation
            .amount = chip_delta,
            .alpha = 255.0f,
            .color = {117, 255, 67, 255}  // Bright green
        };
        d_ArrayAppend(screen->positive_effects, &effect);
        a_FlexAddItem(screen->positive_effects_box, 180, 35, NULL);  // 180px wide, 35px tall
        d_LogInfoF("📊 Added WIN effect: +%d chips", chip_delta);
    } else if (chip_delta < 0) {
        // NEGATIVE: "-50 Loss" (red, center-right)
        EffectDisplay_t effect = {
            .label = "Loss",  // Static string, no heap allocation
            .amount = chip_delta,  // Already negative
            .alpha = 255.0f,
            .color = {255, 50, 50, 255}  // Bright red
        };
        d_ArrayAppend(screen->negative_effects, &effect);
        a_FlexAddItem(screen->negative_effects_box, 180, 35, NULL);
        d_LogInfoF("📊 Added LOSS effect: %d chips", chip_delta);
    }

    // Add status drain effect (RAKE or CHIP_DRAIN)
    if (status_drain > 0) {
        // Determine label and color based on drain type
        const char* label;
        aColor_t color;

        if (screen->drain_type == STATUS_RAKE) {
            label = "RAKE";
            color = (aColor_t){255, 215, 0, 255};  // Gold
        } else {
            label = "Drain";
            color = (aColor_t){255, 165, 0, 255};  // Bright orange
        }

        EffectDisplay_t effect = {
            .label = label,
            .amount = -status_drain,  // Make negative for display
            .alpha = 255.0f,
            .color = color
        };
        d_ArrayAppend(screen->negative_effects, &effect);
        a_FlexAddItem(screen->negative_effects_box, 180, 35, NULL);
        d_LogInfoF("📊 Added %s effect: -%d chips", label, status_drain);
    }

    // Add "Cleansed!" bonus message if this is a victory and no status drain occurred
    if (is_victory && status_drain == 0) {
        // POSITIVE: "Cleansed!" (cyan/light blue, celebrating status effect removal)
        EffectDisplay_t effect = {
            .label = "Cleansed!",  // Static string, no heap allocation
            .amount = 0,  // No chip value
            .alpha = 255.0f,
            .color = {100, 200, 255, 255}  // Light blue/cyan
        };
        d_ArrayAppend(screen->positive_effects, &effect);
        a_FlexAddItem(screen->positive_effects_box, 180, 35, NULL);
        d_LogInfo("📊 Added CLEANSED effect (victory with no status drain)");
    }

    // Tween all effects to fade out over 2 seconds
    for (size_t i = 0; i < screen->positive_effects->count; i++) {
        EffectDisplay_t* effect = (EffectDisplay_t*)d_ArrayGet(screen->positive_effects, i);
        TweenFloat(&g_tween_manager, &effect->alpha, 0.0f, 2.0f, TWEEN_EASE_OUT_QUAD);
    }
    for (size_t i = 0; i < screen->negative_effects->count; i++) {
        EffectDisplay_t* effect = (EffectDisplay_t*)d_ArrayGet(screen->negative_effects, i);
        TweenFloat(&g_tween_manager, &effect->alpha, 0.0f, 2.0f, TWEEN_EASE_OUT_QUAD);
    }

    d_LogInfoF("🎬 RESULT SCREEN: chip_delta=%d, status_drain=%d (%zu positive, %zu negative effects)",
               chip_delta, status_drain, screen->positive_effects->count, screen->negative_effects->count);
}

// ============================================================================
// UPDATE & RENDERING
// ============================================================================

void UpdateResultScreen(ResultScreen_t* screen, float dt, TweenManager_t* tween_mgr) {
    if (!screen) return;

    screen->timer += dt;
}

void RenderResultScreen(ResultScreen_t* screen, Player_t* player, GameState_t state) {
    if (!screen || !player) return;

    // Semi-transparent overlay
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (aColor_t){0, 0, 0, OVERLAY_ALPHA});

    // Determine result message
    const char* message = "";
    aColor_t msg_color = {255, 255, 255, 255};

    switch (player->state) {
        case PLAYER_STATE_WON:
            message = "YOU WIN!";
            msg_color = (aColor_t){0, 255, 0, 255};  // Green
            break;
        case PLAYER_STATE_LOST:
            message = "YOU LOSE";
            msg_color = (aColor_t){255, 0, 0, 255};  // Red
            break;
        case PLAYER_STATE_PUSH:
            message = "PUSH";
            msg_color = (aColor_t){255, 255, 0, 255};  // Yellow
            break;
        case PLAYER_STATE_BLACKJACK:
            if (player->state == PLAYER_STATE_WON) {
                message = "BLACKJACK!";
                msg_color = (aColor_t){0, 255, 0, 255};
            }
            break;
        default:
            message = "Round Complete";
            break;
    }

    // Draw result message
    aTextStyle_t result_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = msg_color,
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText((char*)message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40, result_config);

    // ========================================================================
    // RESULT SCREEN ANIMATIONS (Win/Loss + Status Drain)
    // ========================================================================

    int current_chips = GetPlayerChips(player);

    // Start win/loss animation IMMEDIATELY (0.0s, stays visible)
    if (!screen->winloss_started && screen->chip_delta != 0) {
        screen->winloss_started = true;
        d_LogInfoF("💰 WIN/LOSS ANIMATION: delta=%+d", screen->chip_delta);
        screen->winloss_alpha = 255.0f;
        TweenFloat(&g_tween_manager, &screen->winloss_alpha, 0.0f, 2.0f, TWEEN_EASE_OUT_QUAD);  // Longer fade (2s)
    }

    // Start status drain animation DELAYED (0.5s)
    if (screen->timer > 0.5f && !screen->bleed_started) {
        screen->bleed_started = true;
        d_LogInfoF("💰 STATUS DRAIN ANIMATION: drain=%d", screen->status_drain);
        // Tween chip counter (slot machine effect!)
        TweenFloat(&g_tween_manager, &screen->display_chips, (float)current_chips, 0.8f, TWEEN_EASE_OUT_CUBIC);
        // Fade out status drain text
        if (screen->status_drain > 0) {
            screen->bleed_alpha = 255.0f;
            TweenFloat(&g_tween_manager, &screen->bleed_alpha, 0.0f, 1.5f, TWEEN_EASE_OUT_QUAD);
        }
    }

    // Render chips with slot machine animation
    dString_t* chip_info = d_StringInit();
    d_StringFormat(chip_info, "Chips: %d", (int)screen->display_chips);

    // Measure chip text width for positioning damage text
    int chip_text_width = 0;
    int chip_text_height = 0;
    a_CalcTextDimensions((char*)d_StringPeek(chip_info), FONT_STYLE_TITLE.type, &chip_text_width, &chip_text_height);
    int chip_center_x = SCREEN_WIDTH / 2;
    int chip_y = SCREEN_HEIGHT / 2 + 20;

    // Draw chip count (centered)
    a_DrawText((char*)d_StringPeek(chip_info), chip_center_x, chip_y, FONT_STYLE_TITLE);
    d_StringDestroy(chip_info);

    // Track win/loss message bounds for collision detection
    int winloss_y = 0;
    int winloss_height = 0;
    bool has_winloss = false;

    // Draw win/loss text (IMMEDIATE, 0-0.5s, shows net chip change from betting)
    if (screen->chip_delta != 0 && screen->winloss_alpha > 0.0f) {
        dString_t* winloss_text = d_StringInit();
        d_StringFormat(winloss_text, "%+d chips", screen->chip_delta);

        // Calculate text dimensions for collision detection
        int winloss_width = 0;
        a_CalcTextDimensions((char*)d_StringPeek(winloss_text), FONT_GAME, &winloss_width, &winloss_height);
        winloss_height = (int)((float)winloss_height * 2.0f);  // Account for scale = 2.0f

        // Position with random offset (wins float higher)
        int base_y_offset = (screen->chip_delta > 0) ? -30 : 0;  // Wins start 30px higher
        int winloss_x = chip_center_x + (chip_text_width / 2) + 20 + (int)screen->winloss_offset_x;
        winloss_y = chip_y + base_y_offset + (int)screen->winloss_offset_y;
        has_winloss = true;

        // Bright green for wins, red for losses
        Uint8 r = (screen->chip_delta > 0) ? 117 : 255;
        Uint8 g = (screen->chip_delta > 0) ? 255 : 0;
        Uint8 b = (screen->chip_delta > 0) ? 67 : 0;

        aTextStyle_t winloss_config = {
            .type = FONT_GAME,
            .fg = {r, g, b, (Uint8)screen->winloss_alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 2.0f  // Bigger for readability
        };
        a_DrawText((char*)d_StringPeek(winloss_text), winloss_x, winloss_y, winloss_config);
        d_StringDestroy(winloss_text);
    }

    // Draw status drain text (DELAYED, 0.5s+, shows chip drain from status effects)
    if (screen->timer > 0.5f && screen->status_drain > 0 && screen->bleed_alpha > 0.0f) {
        dString_t* bleed_text = d_StringInit();
        d_StringFormat(bleed_text, "-%d chip drain", screen->status_drain);

        // Calculate text dimensions for collision detection
        int bleed_width = 0;
        int bleed_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(bleed_text), FONT_GAME, &bleed_width, &bleed_height);
        bleed_height = (int)((float)bleed_height * 2.0f);  // Account for scale = 2.0f

        // Position with random offset (different from win/loss!)
        int bleed_x = chip_center_x + (chip_text_width / 2) + 20 + (int)screen->bleed_offset_x;
        int bleed_y = chip_y + (int)screen->bleed_offset_y;

        // COLLISION DETECTION: Check overlap with win/loss message
        if (has_winloss) {
            const int PADDING = 10;  // Visual breathing room
            int overlap_threshold = (winloss_height + bleed_height) / 2 + PADDING;
            int y_distance = abs(winloss_y - bleed_y);

            if (y_distance < overlap_threshold) {
                // PUSH-BACK: Move status drain message below win/loss message
                bleed_y = winloss_y + winloss_height + PADDING;
            }
        }

        aTextStyle_t bleed_config = {
            .type = FONT_GAME,
            .fg = {255, 0, 0, (Uint8)screen->bleed_alpha},  // Always red
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 2.0f  // Bigger for readability
        };
        a_DrawText((char*)d_StringPeek(bleed_text), bleed_x, bleed_y, bleed_config);
        d_StringDestroy(bleed_text);
    }

    // ========================================================================
    // NEW: Render FlexBox effects (auto-positioned, no collision detection!)
    // ========================================================================

    // Render positive effects (center-right, next to "YOU WIN")
    a_FlexLayout(screen->positive_effects_box);
    for (size_t i = 0; i < screen->positive_effects->count; i++) {
        EffectDisplay_t* effect = (EffectDisplay_t*)d_ArrayGet(screen->positive_effects, i);
        if (!effect || effect->alpha < 1.0f) continue;  // Skip fully faded

        int item_x = a_FlexGetItemX(screen->positive_effects_box, i);
        int item_y = a_FlexGetItemY(screen->positive_effects_box, i);

        dString_t* text = d_StringInit();
        if (effect->amount == 0) {
            // Label only (e.g. "Cleansed!")
            d_StringFormat(text, "%s", effect->label);
        } else {
            // Amount + label (e.g. "+50 Win")
            d_StringFormat(text, "+%d %s", effect->amount, effect->label);
        }

        // Calculate text dimensions for background box
        float text_width = 0, text_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(text), FONT_GAME, &text_width, &text_height);

        // Draw semi-transparent black background with 2px padding
        a_DrawFilledRect(
            (aRectf_t){
                item_x - 2,
                item_y - 2,
                text_width * 2.0f + 4,  // Scale matches text scale + padding
                text_height * 2.0f + 4
            },
            (aColor_t){0, 0, 0, 128}  // Black at 50% opacity
        );

        aTextStyle_t style = {
            .type = FONT_GAME,
            .fg = {effect->color.r, effect->color.g, effect->color.b, (Uint8)effect->alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 2.0f
        };
        a_DrawText((char*)d_StringPeek(text), item_x, item_y, style);
        d_StringDestroy(text);
    }

    // Render negative effects (center-right, below positive effects)
    a_FlexLayout(screen->negative_effects_box);
    for (size_t i = 0; i < screen->negative_effects->count; i++) {
        EffectDisplay_t* effect = (EffectDisplay_t*)d_ArrayGet(screen->negative_effects, i);
        if (!effect || effect->alpha < 1.0f) continue;  // Skip fully faded

        int item_x = a_FlexGetItemX(screen->negative_effects_box, i);
        int item_y = a_FlexGetItemY(screen->negative_effects_box, i);

        dString_t* text = d_StringInit();
        d_StringFormat(text, "%d %s", effect->amount, effect->label);

        // Calculate text dimensions for background box
        float text_width = 0, text_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(text), FONT_GAME, &text_width, &text_height);

        // Draw semi-transparent black background with 2px padding
        a_DrawFilledRect(
            (aRectf_t){
                item_x - 2,
                item_y - 2,
                text_width * 2.0f + 4,  // Scale matches text scale + padding
                text_height * 2.0f + 4
            },
            (aColor_t){0, 0, 0, 128}  // Black at 50% opacity
        );

        aTextStyle_t style = {
            .type = FONT_GAME,
            .fg = {effect->color.r, effect->color.g, effect->color.b, (Uint8)effect->alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 2.0f
        };
        a_DrawText((char*)d_StringPeek(text), item_x, item_y, style);
        d_StringDestroy(text);
    }

    // Next round prompt
    aTextStyle_t gray_text = {
        .type = FONT_ENTER_COMMAND,
        .fg = {200, 200, 200, 255},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };

    // Reset viewport after screen shake (if needed)
    // Note: Screen shake is still handled in sceneBlackjack.c
}

// ============================================================================
// EXTERNAL CALLBACKS (for statusEffects.c)
// ============================================================================

// Global reference to result screen (set by sceneBlackjack.c)
static ResultScreen_t* g_global_result_screen = NULL;

void SetGlobalResultScreen(ResultScreen_t* screen) {
    g_global_result_screen = screen;
}

void SetStatusEffectDrainAmount(int drain_amount, StatusEffect_t effect_type) {
    if (g_global_result_screen) {
        g_global_result_screen->status_drain = drain_amount;
        g_global_result_screen->drain_type = effect_type;
        d_LogInfoF("💰 STATUS DRAIN TRACKED: %d chips from %s",
                   drain_amount,
                   effect_type == STATUS_RAKE ? "RAKE" : "CHIP_DRAIN");
    }
}

void TriggerSidebarBetAnimation(int bet_amount) {
    if (g_left_sidebar) {
        ShowSidebarBetDamage(g_left_sidebar, bet_amount);
    }
}
