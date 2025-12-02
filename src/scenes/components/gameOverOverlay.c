/*
 * Game Over Overlay Component
 * Fullscreen defeat screen with stats display and multi-stage animation
 * Pattern: Matches VictoryOverlay architecture + RewardModal animation system
 */

#include "../../../include/scenes/components/gameOverOverlay.h"
#include "../../../include/stats.h"
#include "../../../include/tween/tween.h"
#include <math.h>  // For sinf() in flashing prompt

// External tween manager (from sceneBlackjack.c)
extern TweenManager_t g_tween_manager;

// Color palette (from existing components - EXACTLY matching palette)
#define COLOR_TITLE         ((aColor_t){207, 87, 60, 255})   // #cf573c - red-orange (danger)
#define COLOR_MESSAGE       ((aColor_t){168, 181, 178, 255}) // #a8b5b2 - light gray
#define COLOR_STAT_LABEL    ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream
#define COLOR_STAT_VALUE    ((aColor_t){235, 237, 233, 255}) // #ebede9 - off-white
#define COLOR_PROMPT        ((aColor_t){232, 193, 112, 255}) // #e8c170 - gold (flashing)

// ============================================================================
// LIFECYCLE
// ============================================================================

GameOverOverlay_t* CreateGameOverOverlay(void) {
    GameOverOverlay_t* overlay = malloc(sizeof(GameOverOverlay_t));
    if (!overlay) {
        d_LogError("Failed to allocate GameOverOverlay_t");
        return NULL;
    }

    overlay->visible = false;
    overlay->anim_stage = GAME_OVER_ANIM_FADE_IN_OVERLAY;
    overlay->overlay_alpha = 0.0f;
    overlay->title_alpha = 0.0f;
    overlay->stats_alpha = 0.0f;
    overlay->prompt_alpha = 0.0f;
    overlay->prompt_flash_timer = 0.0f;

    overlay->final_damage = 0;
    overlay->enemies_defeated = 0;
    overlay->hands_won = 0;
    overlay->hands_total = 0;
    overlay->win_rate = 0.0f;

    d_LogInfo("Game over overlay created");
    return overlay;
}

void DestroyGameOverOverlay(GameOverOverlay_t** overlay) {
    if (!overlay || !*overlay) return;

    free(*overlay);
    *overlay = NULL;

    d_LogInfo("Game over overlay destroyed");
}

// ============================================================================
// DISPLAY
// ============================================================================

void ShowGameOverOverlay(GameOverOverlay_t* overlay) {
    if (!overlay) return;

    overlay->visible = true;

    // Snapshot stats from global tracker
    const GlobalStats_t* stats = Stats_GetCurrent();
    if (stats) {
        overlay->final_damage = stats->damage_dealt_total;
        overlay->enemies_defeated = stats->combats_won;
        overlay->hands_won = stats->turns_won;
        overlay->hands_total = stats->turns_played;
        overlay->win_rate = (stats->turns_played > 0)
            ? ((float)stats->turns_won / (float)stats->turns_played) * 100.0f
            : 0.0f;
    }

    // Start animation sequence (stage 1: fade in overlay)
    overlay->anim_stage = GAME_OVER_ANIM_FADE_IN_OVERLAY;
    overlay->overlay_alpha = 0.0f;
    TweenFloat(&g_tween_manager, &overlay->overlay_alpha, 220.0f, 0.5f, TWEEN_EASE_OUT_QUAD);

    d_LogInfo("Game over overlay shown with stats");
}

void HideGameOverOverlay(GameOverOverlay_t* overlay) {
    if (!overlay) return;
    overlay->visible = false;
    d_LogInfo("Game over overlay hidden");
}

// ============================================================================
// UPDATE & RENDERING
// ============================================================================

void UpdateGameOverOverlay(GameOverOverlay_t* overlay, float dt) {
    if (!overlay || !overlay->visible) return;

    // Multi-stage animation progression (matches RewardModal pattern)
    switch (overlay->anim_stage) {
        case GAME_OVER_ANIM_FADE_IN_OVERLAY:
            if (overlay->overlay_alpha >= 219.0f) {  // Stage 1 complete
                overlay->anim_stage = GAME_OVER_ANIM_FADE_IN_TITLE;
                overlay->title_alpha = 0.0f;
                TweenFloat(&g_tween_manager, &overlay->title_alpha, 1.0f,
                          0.4f, TWEEN_EASE_OUT_CUBIC);
            }
            break;

        case GAME_OVER_ANIM_FADE_IN_TITLE:
            if (overlay->title_alpha >= 0.99f) {  // Stage 2 complete
                overlay->anim_stage = GAME_OVER_ANIM_FADE_IN_STATS;
                overlay->stats_alpha = 0.0f;
                TweenFloat(&g_tween_manager, &overlay->stats_alpha, 1.0f,
                          0.5f, TWEEN_EASE_OUT_CUBIC);
            }
            break;

        case GAME_OVER_ANIM_FADE_IN_STATS:
            if (overlay->stats_alpha >= 0.99f) {  // Stage 3 complete
                overlay->anim_stage = GAME_OVER_ANIM_FADE_IN_PROMPT;
                overlay->prompt_alpha = 0.0f;
                TweenFloat(&g_tween_manager, &overlay->prompt_alpha, 1.0f,
                          0.3f, TWEEN_EASE_IN_QUAD);
            }
            break;

        case GAME_OVER_ANIM_FADE_IN_PROMPT:
            if (overlay->prompt_alpha >= 0.99f) {  // Stage 4 complete
                overlay->anim_stage = GAME_OVER_ANIM_COMPLETE;
                overlay->prompt_flash_timer = 0.0f;
            }
            break;

        case GAME_OVER_ANIM_COMPLETE:
            // Flash "Press SPACE" prompt (sin wave)
            overlay->prompt_flash_timer += dt;
            break;
    }
}

void RenderGameOverOverlay(const GameOverOverlay_t* overlay) {
    if (!overlay || !overlay->visible) return;

    // Get UI scale multiplier (100%, 125%, or 150%)
    float ui_scale = GetUIScale();

    // Fullscreen overlay (black with animated alpha) - use runtime window size!
    Uint8 overlay_alpha_byte = (Uint8)overlay->overlay_alpha;
    a_DrawFilledRect((aRectf_t){0, 0, GetWindowWidth(), GetWindowHeight()},
                     (aColor_t){0, 0, 0, overlay_alpha_byte});

    // Center position based on current window size (not hardcoded!)
    int center_x = GetWindowWidth() / 2;
    int center_y = GetWindowHeight() / 2;

    // "DEFEAT" title (red-orange, large, centered)
    Uint8 title_alpha = (Uint8)(overlay->title_alpha * 255);
    aTextStyle_t title_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_TITLE.r, COLOR_TITLE.g, COLOR_TITLE.b, title_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.5f * ui_scale  // Apply UI scaling
    };
    a_DrawText("DEFEAT", center_x, center_y - 150, title_style);

    // Message + Stats (fade-in together)
    Uint8 stats_alpha = (Uint8)(overlay->stats_alpha * 255);

    // Message
    aTextStyle_t message_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_MESSAGE.r, COLOR_MESSAGE.g, COLOR_MESSAGE.b, stats_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f * ui_scale  // Apply UI scaling
    };
    a_DrawText("You ran out of chips.", center_x, center_y - 80, message_style);

    // Stats (cream labels, off-white values)
    aTextStyle_t label_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_STAT_LABEL.r, COLOR_STAT_LABEL.g, COLOR_STAT_LABEL.b, stats_alpha},
        .align = TEXT_ALIGN_RIGHT,
        .wrap_width = 0,
        .scale = 0.9f * ui_scale  // Apply UI scaling
    };
    aTextStyle_t value_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_STAT_VALUE.r, COLOR_STAT_VALUE.g, COLOR_STAT_VALUE.b, stats_alpha},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = 0,
        .scale = 0.9f * ui_scale  // Apply UI scaling
    };

    int label_x = center_x - 20;
    int value_x = center_x + 20;
    int y_offset = center_y - 20;
    int line_height = 30;

    // Stat: Final Damage
    dString_t* damage_str = d_StringInit();
    d_StringFormat(damage_str, "%llu", (unsigned long long)overlay->final_damage);
    a_DrawText("Final Damage:", label_x, y_offset, label_style);
    a_DrawText((char*)d_StringPeek(damage_str), value_x, y_offset, value_style);
    d_StringDestroy(damage_str);

    // Stat: Enemies Defeated
    dString_t* enemies_str = d_StringInit();
    d_StringFormat(enemies_str, "%llu", (unsigned long long)overlay->enemies_defeated);
    a_DrawText("Enemies Defeated:", label_x, y_offset + line_height, label_style);
    a_DrawText((char*)d_StringPeek(enemies_str), value_x, y_offset + line_height, value_style);
    d_StringDestroy(enemies_str);

    // Stat: Hands Won / Total
    dString_t* hands_str = d_StringInit();
    d_StringFormat(hands_str, "%llu / %llu",
                   (unsigned long long)overlay->hands_won,
                   (unsigned long long)overlay->hands_total);
    a_DrawText("Hands Won:", label_x, y_offset + (line_height * 2), label_style);
    a_DrawText((char*)d_StringPeek(hands_str), value_x, y_offset + (line_height * 2), value_style);
    d_StringDestroy(hands_str);

    // Stat: Win Rate
    dString_t* winrate_str = d_StringInit();
    d_StringFormat(winrate_str, "%.1f%%", overlay->win_rate);
    a_DrawText("Win Rate:", label_x, y_offset + (line_height * 3), label_style);
    a_DrawText((char*)d_StringPeek(winrate_str), value_x, y_offset + (line_height * 3), value_style);
    d_StringDestroy(winrate_str);

    // "Press SPACE" prompt (flashing after animation complete)
    Uint8 prompt_alpha = stats_alpha;  // Use stats_alpha during fade-in
    if (overlay->anim_stage == GAME_OVER_ANIM_COMPLETE) {
        // Flash between 128-255 (sin wave)
        float flash = 0.5f + 0.5f * sinf(overlay->prompt_flash_timer * 3.0f);
        prompt_alpha = (Uint8)(flash * 255);
    }
    aTextStyle_t prompt_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_PROMPT.r, COLOR_PROMPT.g, COLOR_PROMPT.b, prompt_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 0.85f * ui_scale  // Apply UI scaling
    };
    a_DrawText("(Press SPACE to return to menu)", center_x, center_y + 160, prompt_style);
}

// ============================================================================
// STATE
// ============================================================================

bool IsGameOverOverlayVisible(const GameOverOverlay_t* overlay) {
    return overlay && overlay->visible;
}

bool IsGameOverAnimationComplete(const GameOverOverlay_t* overlay) {
    return overlay && overlay->anim_stage == GAME_OVER_ANIM_COMPLETE;
}

bool HandleGameOverOverlayInput(const GameOverOverlay_t* overlay) {
    if (!overlay || !overlay->visible) {
        return false;
    }

    // Only accept input during COMPLETE stage (after all animations)
    if (overlay->anim_stage != GAME_OVER_ANIM_COMPLETE) {
        return false;
    }

    // SPACE = quit to menu
    if (app.keyboard[SDL_SCANCODE_SPACE]) {
        app.keyboard[SDL_SCANCODE_SPACE] = 0;  // Consume key
        d_LogInfo("Game Over: Player pressed SPACE - returning to menu");
        return true;
    }

    return false;
}
