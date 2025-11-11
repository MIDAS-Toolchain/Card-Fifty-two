/*
 * Card Fifty-Two - Main Menu Scene
 * Section-based architecture - clean separation of concerns
 */

#include "../../include/common.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/scenes/sceneBlackjack.h"
#include "../../include/scenes/sceneSettings.h"
#include "../../include/scenes/components/menuItem.h"
#include "../../include/scenes/sections/mainMenuSection.h"

// Forward declarations
static void MenuLogic(float dt);
static void MenuDraw(float dt);
static void CleanupMenuScene(void);

// Menu state
static int selectedOption = 0;  // 0=Play, 1=Settings, 2=Quit
static const int NUM_OPTIONS = 3;

// UI Components (scene owns these)
static MenuItem_t* menu_items[3] = {NULL};  // Play, Settings, Quit

// UI Section
static MainMenuSection_t* main_menu_section = NULL;

// ============================================================================
// SCENE INITIALIZATION
// ============================================================================

void InitMenuScene(void) {
    d_LogInfo("Initializing Menu scene");

    // Set scene delegates
    app.delegate.logic = MenuLogic;
    app.delegate.draw = MenuDraw;

    selectedOption = 0;

    // Create menu item components (scene owns these)
    const char* labels[] = {"Play", "Settings", "Quit"};
    for (int i = 0; i < NUM_OPTIONS; i++) {
        menu_items[i] = CreateMenuItem(SCREEN_WIDTH / 2, 0, 0, 25, labels[i]);
        if (!menu_items[i]) {
            d_LogFatal("Failed to create menu item");
            return;
        }
    }

    // Normalize all menu items to widest width for consistent hover areas
    int max_width = 0;
    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (menu_items[i]->w > max_width) {
            max_width = menu_items[i]->w;
        }
    }
    for (int i = 0; i < NUM_OPTIONS; i++) {
        menu_items[i]->w = max_width;
    }

    // Mark first option as selected
    SetMenuItemSelected(menu_items[0], true);

    // Create main menu section (references menu_items, doesn't own them)
    main_menu_section = CreateMainMenuSection(menu_items, NUM_OPTIONS);
    if (!main_menu_section) {
        d_LogFatal("Failed to create MainMenuSection");
        return;
    }

    d_LogInfo("Menu scene ready");
}

static void CleanupMenuScene(void) {
    d_LogInfo("Cleaning up Menu scene");

    // Destroy section first (doesn't own menu items)
    if (main_menu_section) {
        DestroyMainMenuSection(&main_menu_section);
    }

    // Destroy menu items (scene owns these)
    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (menu_items[i]) {
            DestroyMenuItem(&menu_items[i]);
        }
    }

    d_LogInfo("Menu scene cleanup complete");
}

// ============================================================================
// SCENE LOGIC
// ============================================================================

static void MenuLogic(float dt) {
    (void)dt;

    a_DoInput();

    // ESC - Quit application
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        d_LogInfo("Quitting from menu");
        app.running = 0;
        return;
    }

    // Mouse click activation (check FIRST - before hover)
    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (menu_items[i] && IsMenuItemClicked(menu_items[i])) {
            // Execute action immediately based on which item was clicked
            switch (i) {
                case 0:  // Play
                    d_LogInfo("Starting Blackjack (clicked)");
                    CleanupMenuScene();
                    InitBlackjackScene();
                    return;
                case 1:  // Settings
                    d_LogInfo("Opening Settings (clicked)");
                    CleanupMenuScene();
                    InitSettingsScene();
                    return;
                case 2:  // Quit
                    d_LogInfo("Quitting (clicked)");
                    app.running = 0;
                    return;
            }
        }
    }

    // Mouse hover auto-select (check fresh hover state every frame)
    for (int i = 0; i < NUM_OPTIONS; i++) {
        if (!menu_items[i]) continue;

        bool is_hovered = IsMenuItemHovered(menu_items[i]);

        if (is_hovered) {
            if (i != selectedOption) {
                // Deselect old, select hovered
                SetMenuItemSelected(menu_items[selectedOption], false);
                selectedOption = i;
                SetMenuItemSelected(menu_items[selectedOption], true);
            }
            break;  // Only one item can be hovered
        }
    }

    // W/Up - Move selection up
    if (app.keyboard[SDL_SCANCODE_W] || app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_W] = 0;
        app.keyboard[SDL_SCANCODE_UP] = 0;

        // Deselect current
        SetMenuItemSelected(menu_items[selectedOption], false);

        // Move up
        selectedOption--;
        if (selectedOption < 0) selectedOption = NUM_OPTIONS - 1;

        // Select new
        SetMenuItemSelected(menu_items[selectedOption], true);
    }

    // S/Down - Move selection down
    if (app.keyboard[SDL_SCANCODE_S] || app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_S] = 0;
        app.keyboard[SDL_SCANCODE_DOWN] = 0;

        // Deselect current
        SetMenuItemSelected(menu_items[selectedOption], false);

        // Move down
        selectedOption++;
        if (selectedOption >= NUM_OPTIONS) selectedOption = 0;

        // Select new
        SetMenuItemSelected(menu_items[selectedOption], true);
    }

    // ENTER/SPACE/NUMPAD_ENTER - Select option
    if (app.keyboard[SDL_SCANCODE_RETURN] ||
        app.keyboard[SDL_SCANCODE_SPACE] ||
        app.keyboard[SDL_SCANCODE_KP_ENTER]) {

        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_SPACE] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

        switch (selectedOption) {
            case 0:  // Play
                d_LogInfo("Starting Blackjack");
                CleanupMenuScene();
                InitBlackjackScene();
                break;
            case 1:  // Settings
                d_LogInfo("Opening Settings");
                CleanupMenuScene();
                InitSettingsScene();
                break;
            case 2:  // Quit
                d_LogInfo("Quitting");
                app.running = 0;
                break;
        }
    }
}

// ============================================================================
// SCENE RENDERING
// ============================================================================

static void MenuDraw(float dt) {
    (void)dt;

    // Dark background (from color palette #090a14)
    app.background = (aColor_t){9, 10, 20, 255};

    // Render main menu section (handles everything!)
    if (main_menu_section) {
        RenderMainMenuSection(main_menu_section);
    }

    // FPS counter
    dString_t* fps_str = d_StringInit();
    d_StringFormat(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_StringPeek(fps_str), SCREEN_WIDTH - 10, 10,
               0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
    d_StringDestroy(fps_str);
}
