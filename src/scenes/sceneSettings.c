/*
 * Card Fifty-Two - Settings Scene
 * FlexBox-based beautiful settings UI with keyboard navigation
 */

#include "../../include/common.h"
#include "../../include/scenes/sceneSettings.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/settings.h"
#include "../../include/scenes/sections/settingsMenuSection.h"

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

// ============================================================================
// SCENE LOGIC
// ============================================================================

static void SettingsLogic(float dt) {
    (void)dt;

    a_DoInput();

    // ESC - Return to menu
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

        InitMenuScene();
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
