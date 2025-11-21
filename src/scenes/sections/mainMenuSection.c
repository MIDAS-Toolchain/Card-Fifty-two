/*
 * MainMenuSection Implementation
 */

#include "../../../include/scenes/sections/mainMenuSection.h"

// Layout constants (no magic numbers!)
#define MENU_TITLE_Y            150
#define MENU_SUBTITLE_Y         210
#define MENU_START_Y            350
#define MENU_ITEM_HEIGHT        25
#define MENU_ITEM_GAP           35
#define MENU_INSTRUCTIONS_Y     (SCREEN_HEIGHT - 50)

// ============================================================================
// LIFECYCLE
// ============================================================================

MainMenuSection_t* CreateMainMenuSection(MenuItem_t** menu_items, int item_count) {
    if (!menu_items || item_count <= 0) {
        d_LogError("CreateMainMenuSection: Invalid parameters");
        return NULL;
    }

    MainMenuSection_t* section = malloc(sizeof(MainMenuSection_t));
    if (!section) {
        d_LogError("Failed to allocate MainMenuSection");
        return NULL;
    }

    section->menu_items = menu_items;  // Reference, not owned
    section->item_count = item_count;

    // Create FlexBox for menu items (vertical layout)
    section->menu_layout = a_FlexBoxCreate(0, MENU_START_Y,
                                            SCREEN_WIDTH,
                                            item_count * (MENU_ITEM_HEIGHT + MENU_ITEM_GAP));
    if (!section->menu_layout) {
        free(section);
        d_LogError("Failed to create menu FlexBox");
        return NULL;
    }

    a_FlexConfigure(section->menu_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, MENU_ITEM_GAP);

    // Add menu items to FlexBox
    for (int i = 0; i < item_count; i++) {
        a_FlexAddItem(section->menu_layout, SCREEN_WIDTH, MENU_ITEM_HEIGHT, menu_items[i]);
    }

    // Set default text using strncpy (safe string copy)
    strncpy(section->title, "CARD FIFTY-TWO", sizeof(section->title) - 1);
    section->title[sizeof(section->title) - 1] = '\0';

    strncpy(section->subtitle, "A Blackjack Demo for MIDAS", sizeof(section->subtitle) - 1);
    section->subtitle[sizeof(section->subtitle) - 1] = '\0';

    strncpy(section->instructions,
            "[W/S] or [UP/DOWN] to navigate | [ENTER] or [SPACE] to select | [ESC] to quit",
            sizeof(section->instructions) - 1);
    section->instructions[sizeof(section->instructions) - 1] = '\0';

    d_LogInfo("MainMenuSection created");
    return section;
}

void DestroyMainMenuSection(MainMenuSection_t** section) {
    if (!section || !*section) return;

    MainMenuSection_t* sec = *section;

    // Destroy FlexBox
    if (sec->menu_layout) {
        a_FlexBoxDestroy(&sec->menu_layout);
    }

    // NOTE: No string cleanup needed - using fixed buffers!
    // NOTE: Do NOT destroy menu_items - scene owns them!

    free(sec);
    *section = NULL;
    d_LogInfo("MainMenuSection destroyed");
}

// ============================================================================
// CONFIGURATION
// ============================================================================

void SetMainMenuTitle(MainMenuSection_t* section, const char* title) {
    if (!section) return;
    strncpy(section->title, title ? title : "", sizeof(section->title) - 1);
    section->title[sizeof(section->title) - 1] = '\0';
}

void SetMainMenuSubtitle(MainMenuSection_t* section, const char* subtitle) {
    if (!section) return;
    strncpy(section->subtitle, subtitle ? subtitle : "", sizeof(section->subtitle) - 1);
    section->subtitle[sizeof(section->subtitle) - 1] = '\0';
}

void SetMainMenuInstructions(MainMenuSection_t* section, const char* instructions) {
    if (!section) return;
    strncpy(section->instructions, instructions ? instructions : "", sizeof(section->instructions) - 1);
    section->instructions[sizeof(section->instructions) - 1] = '\0';
}

void UpdateMainMenuItemPositions(MainMenuSection_t* section) {
    if (!section || !section->menu_layout) return;

    // Recalculate FlexBox layout
    a_FlexLayout(section->menu_layout);

    // Update each MenuItem position from FlexBox
    for (int i = 0; i < section->item_count; i++) {
        if (!section->menu_items[i]) continue;

        int y = a_FlexGetItemY(section->menu_layout, i);

        // Center X position
        SetMenuItemPosition(section->menu_items[i], SCREEN_WIDTH / 2, y);
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderMainMenuSection(MainMenuSection_t* section) {
    if (!section) return;

    // Title (cream color #e7d5b3 from palette)
    a_DrawText(section->title, SCREEN_WIDTH / 2, MENU_TITLE_Y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={231,213,179,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Subtitle (light cyan #73bed3 from palette)
    a_DrawText(section->subtitle, SCREEN_WIDTH / 2, MENU_SUBTITLE_Y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={115,190,211,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Update menu item positions from FlexBox
    UpdateMainMenuItemPositions(section);

    // Render each menu item
    for (int i = 0; i < section->item_count; i++) {
        if (section->menu_items[i]) {
            RenderMenuItem(section->menu_items[i]);
        }
    }

    // Instructions (medium gray #394a50 from palette)
    a_DrawText(section->instructions, SCREEN_WIDTH / 2, MENU_INSTRUCTIONS_Y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={57,74,80,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
}
