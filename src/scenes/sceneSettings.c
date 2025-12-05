/*
 * Card Fifty-Two - Settings Scene
 * FlexBox-based beautiful settings UI with keyboard navigation
 */

#include "../../include/common.h"
#include "../../include/scenes/sceneSettings.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/scenes/sceneBlackjack.h"
#include "../../include/settings.h"
#include "../../include/scenes/sections/settingsMenuSection.h"

// Return context: tracks which scene to return to on ESC
typedef enum SettingsReturnContext {
    SETTINGS_RETURN_MAIN_MENU,    // From main menu (default)
    SETTINGS_RETURN_GAME          // From in-game pause menu
} SettingsReturnContext_t;

static SettingsReturnContext_t g_return_context = SETTINGS_RETURN_MAIN_MENU;

// Forward declarations
static void SettingsLogic(float dt);
static void SettingsDraw(float dt);

// Settings section (local to this scene - uses global g_settings)
static SettingsMenuSection_t* g_settings_section = NULL;

// ============================================================================
// SCENE INITIALIZATION
// ============================================================================

void InitSettingsScene(void) {
    d_LogInfo("Initializing Settings scene");

    // Set scene delegates
    app.delegate.logic = SettingsLogic;
    app.delegate.draw = SettingsDraw;

    // Use global g_settings (already initialized in main.c)
    if (!g_settings) {
        d_LogError("Global g_settings is NULL! Should be initialized in main.c");
        return;
    }

    // Create settings menu section (references global g_settings)
    g_settings_section = CreateSettingsMenuSection(g_settings);
    if (!g_settings_section) {
        d_LogError("Failed to create SettingsMenuSection!");
        return;
    }

    d_LogInfo("Settings scene ready");
}

/**
 * InitSettingsSceneFromMenu - Open settings from main menu
 * Returns to main menu on ESC
 */
void InitSettingsSceneFromMenu(void) {
    g_return_context = SETTINGS_RETURN_MAIN_MENU;
    InitSettingsScene();
}

/**
 * InitSettingsSceneFromGame - Open settings from in-game pause menu
 * Returns to game (leaving it paused) on ESC
 */
void InitSettingsSceneFromGame(void) {
    g_return_context = SETTINGS_RETURN_GAME;
    InitSettingsScene();
}

// ============================================================================
// SCENE LOGIC
// ============================================================================

static void SettingsLogic(float dt) {
    (void)dt;

    a_DoInput();

    // ESC - Return to previous scene
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;

        // Cleanup section
        if (g_settings_section) {
            DestroySettingsMenuSection(&g_settings_section);
        }

        // Save global settings before exiting (but don't destroy - main.c owns it)
        if (g_settings) {
            Settings_Save(g_settings);
        }

        // Return to the scene we came from
        if (g_return_context == SETTINGS_RETURN_GAME) {
            d_LogInfo("Returning to game (still paused)");
            ResumeBlackjackScene();  // Just restore delegates, don't re-init
        } else {
            d_LogInfo("Returning to main menu");
            InitMenuScene();
        }
        return;
    }

    // Handle settings input (navigation, adjustments)
    if (g_settings_section) {
        HandleSettingsInput(g_settings_section);
    }
}

// ============================================================================
// SCENE RENDERING
// ============================================================================

static void SettingsDraw(float dt) {
    (void)dt;

    // Dark background
    app.background = (aColor_t){9, 10, 20, 255};

    // Render settings menu section
    if (g_settings_section) {
        RenderSettingsMenuSection(g_settings_section);
    }
}
