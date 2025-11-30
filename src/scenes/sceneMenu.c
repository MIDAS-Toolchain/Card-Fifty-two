/*
 * Card Fifty-Two - Main Menu Scene
 * Section-based architecture - clean separation of concerns
 */

#include "../../include/common.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/scenes/sceneBlackjack.h"
#include "../../include/scenes/sceneSettings.h"
#include "../../include/stats.h"
#include "../../include/scenes/components/menuItem.h"
#include "../../include/scenes/sections/mainMenuSection.h"
#include "../../include/audioHelper.h"

// Forward declaration for stats screen
static void InitStatsScene(void);

// Forward declarations
static void MenuLogic(float dt);
static void MenuDraw(float dt);
static void CleanupMenuScene(void);

// Menu state
static int selectedOption = 0;  // 0=Play, 1=Tutorial, 2=Stats, 3=Settings, 4=Quit
static const int NUM_OPTIONS = 5;

// UI Components (scene owns these)
static MenuItem_t* menu_items[5] = {NULL};  // Play, Tutorial, Stats, Settings, Quit

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
    const char* labels[] = {"Play", "Tutorial", "Stats", "Settings", "Quit"};
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
                case 0:  // Play (roguelike mode - coming soon!)
                    printf("\n🎮 PLAY MODE - COMING SOON! 🎮\n");
                    printf("This will be the full roguelike experience:\n");
                    printf("  - 3-Act structure\n");
                    printf("  - Procedurally generated encounters\n");
                    printf("  - Persistent progression\n");
                    printf("  - Multiple enemy types\n");
                    printf("\nFor now, try the Tutorial! 📚\n\n");
                    break;
                case 1:  // Tutorial
                    d_LogInfo("Starting Tutorial (clicked)");
                    CleanupMenuScene();
                    InitBlackjackScene();  // Current tutorial implementation
                    return;
                case 2:  // Stats
                    d_LogInfo("Opening Stats (clicked)");
                    CleanupMenuScene();
                    InitStatsScene();
                    return;
                case 3:  // Settings
                    d_LogInfo("Opening Settings (clicked)");
                    CleanupMenuScene();
                    InitSettingsScene();
                    return;
                case 4:  // Quit
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

    // Up - Move selection up
    if (app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_UP] = 0;

        // Deselect current
        SetMenuItemSelected(menu_items[selectedOption], false);

        // Move up
        selectedOption--;
        if (selectedOption < 0) selectedOption = NUM_OPTIONS - 1;

        // Select new
        SetMenuItemSelected(menu_items[selectedOption], true);
        PlayUIHoverSound();
    }

    // Down - Move selection down
    if (app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_DOWN] = 0;

        // Deselect current
        SetMenuItemSelected(menu_items[selectedOption], false);

        // Move down
        selectedOption++;
        if (selectedOption >= NUM_OPTIONS) selectedOption = 0;

        // Select new
        SetMenuItemSelected(menu_items[selectedOption], true);
        PlayUIHoverSound();
    }

    // ENTER/SPACE/NUMPAD_ENTER - Select option
    if (app.keyboard[SDL_SCANCODE_RETURN] ||
        app.keyboard[SDL_SCANCODE_SPACE] ||
        app.keyboard[SDL_SCANCODE_KP_ENTER]) {

        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_SPACE] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

        PlayUIClickSound();

        switch (selectedOption) {
            case 0:  // Play (roguelike mode - coming soon!)
                printf("\n🎮 PLAY MODE - COMING SOON! 🎮\n");
                printf("This will be the full roguelike experience:\n");
                printf("  - 3-Act structure\n");
                printf("  - Procedurally generated encounters\n");
                printf("  - Persistent progression\n");
                printf("  - Multiple enemy types\n");
                printf("\nFor now, try the Tutorial! 📚\n\n");
                break;
            case 1:  // Tutorial
                d_LogInfo("Starting Tutorial");
                CleanupMenuScene();
                InitBlackjackScene();  // Current tutorial implementation
                break;
            case 2:  // Stats
                d_LogInfo("Opening Stats");
                CleanupMenuScene();
                InitStatsScene();
                break;
            case 3:  // Settings
                d_LogInfo("Opening Settings");
                CleanupMenuScene();
                InitSettingsScene();
                break;
            case 4:  // Quit
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
}

// ============================================================================
// STATS SCENE (Standalone)
// ============================================================================

static void StatsSceneLogic(float dt);
static void StatsSceneDraw(float dt);

static void InitStatsScene(void) {
    d_LogInfo("Initializing Stats scene");
    app.delegate.logic = StatsSceneLogic;
    app.delegate.draw = StatsSceneDraw;
}

static void StatsSceneLogic(float dt) {
    (void)dt;
    a_DoInput();

    // ESC - Return to menu
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        InitMenuScene();
    }
}

static void StatsSceneDraw(float dt) {
    (void)dt;
    app.background = (aColor_t){20, 25, 35, 255};

    const GlobalStats_t* stats = Stats_GetCurrent();

    // Title
    a_DrawText("RUN STATISTICS", SCREEN_WIDTH / 2, 80,
               (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,100,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    dString_t* text = d_StringInit();
    int y = 140;

    // Cards
    d_StringFormat(text, "Cards Drawn: %llu", stats->cards_drawn);
    a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y,
               (aTextStyle_t){.type=FONT_GAME, .fg={200,255,200,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    y += 40;

    // Turns
    d_StringFormat(text, "Turns: %llu  Won: %llu  Lost: %llu  Push: %llu",
                   stats->turns_played, stats->turns_won, stats->turns_lost, stats->turns_pushed);
    a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y,
               (aTextStyle_t){.type=FONT_GAME, .fg={180,220,255,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    y += 40;

    // Combat
    d_StringFormat(text, "Combats Won: %llu", stats->combats_won);
    a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y,
               (aTextStyle_t){.type=FONT_GAME, .fg={255,150,150,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    y += 50;

    // Damage
    d_StringFormat(text, "Total Damage: %llu", stats->damage_dealt_total);
    a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y,
               (aTextStyle_t){.type=FONT_GAME, .fg={255,180,100,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    y += 35;

    // Damage breakdown
    for (int i = 0; i < DAMAGE_SOURCE_MAX; i++) {
        uint64_t dmg = stats->damage_by_source[i];
        if (dmg == 0) continue;
        float pct = stats->damage_dealt_total > 0 ?
                    (float)dmg / stats->damage_dealt_total * 100.0f : 0.0f;
        d_StringFormat(text, "  %s: %llu (%.1f%%)",
                       Stats_GetDamageSourceName((DamageSource_t)i), dmg, pct);
        a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y,
                   (aTextStyle_t){.type=FONT_GAME, .fg={200,200,200,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
        y += 32;
    }

    y += 10;

    // Chips
    d_StringFormat(text, "Chips Bet: %llu  Won: %llu  Lost: %llu  Drained: %llu",
                   stats->chips_bet, stats->chips_won, stats->chips_lost, stats->chips_drained);
    a_DrawText((char*)d_StringPeek(text), SCREEN_WIDTH / 2, y,
               (aTextStyle_t){.type=FONT_GAME, .fg={150,255,150,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    d_StringDestroy(text);

    // Instructions
    a_DrawText("[ESC] Back to Menu", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50,
               (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={150,150,150,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
}
