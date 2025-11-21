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

    // Initialize to zero
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

    return screen;
}

void DestroyResultScreen(ResultScreen_t** screen) {
    if (!screen || !*screen) return;
    free(*screen);
    *screen = NULL;
}

// ============================================================================
// DISPLAY
// ============================================================================

void ShowResultScreen(ResultScreen_t* screen, int old_chips, int chip_delta, int status_drain) {
    if (!screen) return;

    screen->old_chips = old_chips;
    screen->chip_delta = chip_delta;
    screen->status_drain = status_drain;
    screen->display_chips = (float)old_chips;  // Start slot animation from old value
    screen->timer = 0.0f;

    // Reset animation flags
    screen->winloss_started = false;
    screen->bleed_started = false;

    // Random offsets for both animations (high-quality RNG)
    screen->winloss_offset_x = GetRandomFloat(-30.0f, 30.0f);
    screen->winloss_offset_y = GetRandomFloat(-20.0f, 20.0f);
    screen->bleed_offset_x = GetRandomFloat(-30.0f, 30.0f);
    screen->bleed_offset_y = GetRandomFloat(-20.0f, 20.0f);

    d_LogInfoF("🎬 RESULT SCREEN: chip_delta=%d, status_drain=%d", chip_delta, status_drain);
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

void SetStatusEffectDrainAmount(int drain_amount) {
    if (g_global_result_screen) {
        g_global_result_screen->status_drain = drain_amount;
        d_LogInfoF("💰 STATUS DRAIN TRACKED: %d chips", drain_amount);
    }
}

void TriggerSidebarBetAnimation(int bet_amount) {
    if (g_left_sidebar) {
        ShowSidebarBetDamage(g_left_sidebar, bet_amount);
    }
}
