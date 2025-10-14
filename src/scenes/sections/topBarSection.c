/*
 * Top Bar Section Implementation
 */

#include "../../../include/scenes/sections/topBarSection.h"
#include "../../../include/scenes/sceneBlackjack.h"

// ============================================================================
// TOP BAR SECTION LIFECYCLE
// ============================================================================

TopBarSection_t* CreateTopBarSection(void) {
    TopBarSection_t* section = malloc(sizeof(TopBarSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate TopBarSection");
        return NULL;
    }

    // Create settings button (positioned at far right)
    // Calculate X position: right-align the button's LEFT edge, not its center
    int button_x = SCREEN_WIDTH - SETTINGS_BUTTON_MARGIN - SETTINGS_BUTTON_WIDTH;
    section->settings_button = CreateButton(button_x, 0,
                                             SETTINGS_BUTTON_WIDTH,
                                             SETTINGS_BUTTON_HEIGHT,
                                             "[S]");

    if (!section->settings_button) {
        free(section);
        d_LogFatal("Failed to create settings button");
        return NULL;
    }

    d_LogInfo("TopBarSection created");
    return section;
}

void DestroyTopBarSection(TopBarSection_t** section) {
    if (!section || !*section) return;

    TopBarSection_t* bar = *section;

    if (bar->settings_button) {
        DestroyButton(&bar->settings_button);
    }

    free(bar);
    *section = NULL;
    d_LogInfo("TopBarSection destroyed");
}

// ============================================================================
// TOP BAR SECTION RENDERING
// ============================================================================

void RenderTopBarSection(TopBarSection_t* section, int y) {
    if (!section) return;

    // Draw top bar background
    a_DrawFilledRect(0, y, SCREEN_WIDTH, TOP_BAR_HEIGHT,
                     TOP_BAR_BG.r, TOP_BAR_BG.g, TOP_BAR_BG.b, TOP_BAR_BG.a);

    // Update settings button Y position (centered in top bar)
    if (section->settings_button) {
        SetButtonPosition(section->settings_button,
                         section->settings_button->x,
                         y + SETTINGS_BUTTON_Y_OFFSET);
        RenderButton(section->settings_button);
    }
}

// ============================================================================
// TOP BAR SECTION INTERACTION
// ============================================================================

bool IsTopBarSettingsClicked(TopBarSection_t* section) {
    if (!section || !section->settings_button) return false;
    return IsButtonClicked(section->settings_button);
}
