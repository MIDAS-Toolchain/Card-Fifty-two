/*
 * Card Fifty-Two - Settings Scene
 */

#include "../../include/common.h"
#include "../../include/scenes/sceneSettings.h"
#include "../../include/scenes/sceneMenu.h"

// Forward declarations
static void SettingsLogic(float dt);
static void SettingsDraw(float dt);

// ============================================================================
// SCENE INITIALIZATION
// ============================================================================

void InitSettingsScene(void) {
    d_LogInfo("Initializing Settings scene");

    // Set scene delegates
    app.delegate.logic = SettingsLogic;
    app.delegate.draw = SettingsDraw;

    d_LogInfo("Settings scene ready");
}

// ============================================================================
// SCENE LOGIC
// ============================================================================

static void SettingsLogic(float dt) {
    (void)dt;

    a_DoInput();

    // ESC - Return to menu
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        d_LogInfo("Returning to menu from settings");
        InitMenuScene();
        return;
    }
}

// ============================================================================
// SCENE RENDERING
// ============================================================================

static void SettingsDraw(float dt) {
    (void)dt;

    // Dark background
    app.background = (aColor_t){30, 30, 50, 255};

    // Title
    a_DrawText("SETTINGS", SCREEN_WIDTH / 2, 150,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Placeholder settings
    a_DrawText("Coming Soon:", SCREEN_WIDTH / 2, 300,
               200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("- Sound Volume", SCREEN_WIDTH / 2, 360,
               180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("- Music Volume", SCREEN_WIDTH / 2, 410,
               180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("- Fullscreen Toggle", SCREEN_WIDTH / 2, 460,
               180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Instructions
    a_DrawText("[ESC] to return to menu",
               SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50,
               150, 150, 150, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
}
