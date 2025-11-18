/*
 * Card Fifty-Two - Settings Scene
 */

#include "../../include/common.h"
#include "../../include/scenes/sceneSettings.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/stats.h"

// Forward declarations
static void SettingsLogic(float dt);
static void SettingsDraw(float dt);

// Settings state
static bool show_stats = false;

// ============================================================================
// SCENE INITIALIZATION
// ============================================================================

void InitSettingsScene(void) {
    d_LogInfo("Initializing Settings scene");

    // Set scene delegates
    app.delegate.logic = SettingsLogic;
    app.delegate.draw = SettingsDraw;

    // Reset stats view
    show_stats = false;

    d_LogInfo("Settings scene ready");
}

// ============================================================================
// SCENE LOGIC
// ============================================================================

static void SettingsLogic(float dt) {
    (void)dt;

    a_DoInput();

    // ESC - Return to menu (or back to settings if viewing stats)
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        if (show_stats) {
            // Back to settings menu
            show_stats = false;
        } else {
            // Back to main menu
            d_LogInfo("Returning to menu from settings");
            InitMenuScene();
        }
        return;
    }

    // S - Toggle stats view
    if (app.keyboard[SDL_SCANCODE_S]) {
        app.keyboard[SDL_SCANCODE_S] = 0;
        show_stats = !show_stats;
        d_LogInfoF("Stats view: %s", show_stats ? "ON" : "OFF");
    }
}

// ============================================================================
// SCENE RENDERING
// ============================================================================

static void SettingsDraw(float dt) {
    (void)dt;

    // Dark background
    app.background = (aColor_t){30, 30, 50, 255};

    if (show_stats) {
        // ========================================================================
        // STATS VIEW
        // ========================================================================
        const GlobalStats_t* stats = Stats_GetCurrent();

        // Title
        a_DrawText("STATISTICS", SCREEN_WIDTH / 2, 100,
                   255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Cards drawn
        dString_t* text = d_StringInit();
        d_StringFormat(text, "Cards Drawn: %llu", stats->cards_drawn);
        a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, 200,
                   200, 255, 200, FONT_GAME, TEXT_ALIGN_CENTER, 0);

        // Total damage
        d_StringFormat(text, "Total Damage: %llu", stats->damage_dealt_total);
        a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, 260,
                   255, 200, 100, FONT_GAME, TEXT_ALIGN_CENTER, 0);

        // Damage breakdown header
        a_DrawText("Damage by Source:", SCREEN_WIDTH / 2, 320,
                   180, 180, 180, FONT_GAME, TEXT_ALIGN_CENTER, 0);

        // Damage by source breakdown
        int y_offset = 370;
        for (int i = 0; i < DAMAGE_SOURCE_MAX; i++) {
            const char* source_name = Stats_GetDamageSourceName((DamageSource_t)i);
            uint64_t damage = stats->damage_by_source[i];

            // Calculate percentage
            float percentage = 0.0f;
            if (stats->damage_dealt_total > 0) {
                percentage = (float)damage / (float)stats->damage_dealt_total * 100.0f;
            }

            d_StringFormat(text, "%s: %llu (%.1f%%)", source_name, damage, percentage);

            // Color based on amount
            int r = 150 + (damage > 0 ? 100 : 0);
            int g = 150;
            int b = 150;

            a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y_offset,
                       r, g, b, FONT_GAME, TEXT_ALIGN_CENTER, 0);
            y_offset += 45;
        }

        d_StringDestroy(text);

        // Instructions
        a_DrawText("[ESC] to return to settings",
                   SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50,
                   150, 150, 150, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    } else {
        // ========================================================================
        // SETTINGS MENU
        // ========================================================================

        // Title
        a_DrawText("SETTINGS", SCREEN_WIDTH / 2, 150,
                   255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Stats option (now functional!)
        a_DrawText("[S] View Statistics", SCREEN_WIDTH / 2, 300,
                   100, 255, 100, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Placeholder settings
        a_DrawText("Coming Soon:", SCREEN_WIDTH / 2, 380,
                   200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        a_DrawText("- Sound Volume", SCREEN_WIDTH / 2, 430,
                   180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        a_DrawText("- Music Volume", SCREEN_WIDTH / 2, 480,
                   180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Instructions
        a_DrawText("[ESC] to return to menu",
                   SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50,
                   150, 150, 150, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    }
}
