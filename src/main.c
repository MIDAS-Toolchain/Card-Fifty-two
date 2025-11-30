/*
 * Card Fifty-Two - Main Entry Point
 * A card game framework demonstrating Archimedes and Daedalus integration
 */

#include "common.h"
#include "card.h"
#include "deck.h"
#include "hand.h"
#include "cardTags.h"
#include "trinket.h"
#include "stats.h"
#include "settings.h"
#include "scenes/sceneMenu.h"
#include "loaders/enemyLoader.h"
#include "loaders/affixLoader.h"
#include "loaders/trinketLoader.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Player registry (defined in common.h, declared here)
dTable_t* g_players = NULL;

// Texture cache (defined in common.h, declared here)
dTable_t* g_card_textures = NULL;

// Portrait texture cache (defined in common.h, declared here)
dTable_t* g_portraits = NULL;

// Card back surface (defined in common.h, declared here)
aImage_t* g_card_back_texture = NULL;

// Ability icon textures (defined in common.h, declared here)
dTable_t* g_ability_icons = NULL;

// Enemy database (defined in common.h, declared here)
dDUFValue_t* g_enemies_db = NULL;

// Global settings (defined in common.h, declared here)
struct Settings_t* g_settings = NULL;

// UI sound effects (defined in common.h, declared here)
aSoundEffect_t g_ui_hover_sound;
aSoundEffect_t g_ui_click_sound;

// Card slide sound effects (8 variants)
aSoundEffect_t g_card_slide_sounds[CARD_SLIDE_SOUND_COUNT];
int g_last_card_slide_index = -1;  // Track last played sound for no-repeat

// Test deck (for demonstration)
// Constitutional pattern: Deck_t is value type, not pointer
Deck_t g_test_deck;

// Player hand (for demonstration)
// Constitutional pattern: Hand_t is value type, not pointer
Hand_t g_player_hand;

// ============================================================================
// GLOBAL FONT STYLES
// ============================================================================

aTextStyle_t FONT_STYLE_TITLE = {
    .type = FONT_ENTER_COMMAND,
    .fg = {255, 255, 255, 255},
    .bg = {0, 0, 0, 0},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
};

aTextStyle_t FONT_STYLE_BODY = {
    .type = FONT_GAME,
    .fg = {255, 255, 255, 255},
    .bg = {0, 0, 0, 0},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
};

aTextStyle_t FONT_STYLE_CHIP_COUNT = {
    .type = FONT_GAME,
    .fg = {255, 255, 0, 255},  // Yellow
    .bg = {0, 0, 0, 0},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
};

aTextStyle_t FONT_STYLE_DEBUG = {
    .type = FONT_GAME,
    .fg = {0, 255, 0, 255},  // Green
    .bg = {0, 0, 0, 0},
    .align = TEXT_ALIGN_RIGHT,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
};

aTextStyle_t FONT_STYLE_DAMAGE = {
    .type = FONT_GAME,
    .fg = {255, 0, 0, 255},  // Red (overridden for healing)
    .bg = {0, 0, 0, 0},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f,
    .padding = 0
};

// ============================================================================
// WINDOW SIZE HELPERS (for resolution-independent UI)
// ============================================================================

int GetWindowWidth(void) {
    int w = 0;
    SDL_GetWindowSize(app.window, &w, NULL);
    return w;
}

int GetWindowHeight(void) {
    int h = 0;
    SDL_GetWindowSize(app.window, NULL, &h);
    return h;
}

// ============================================================================
// INITIALIZATION & CLEANUP
// ============================================================================

void Initialize(void) {
    // Initialize Archimedes framework
    if (a_Init(SCREEN_WIDTH, SCREEN_HEIGHT, "Card Fifty-Two") != 0) {
        fprintf(stderr, "Failed to initialize Archimedes\n");
        exit(1);
    }

    d_LogInfo("Archimedes initialized successfully");

    // Initialize audio system (uses default: 16 channels, 44100 Hz)
    if (a_InitAudio() != 0) {
        d_LogError("Failed to initialize audio system");
    } else {
        d_LogInfo("Audio system initialized");

        // Reserve channels 0-1 for UI sounds (prevents auto-allocation)
        a_AudioReserveChannels(2);
        d_LogInfo("Reserved 2 channels for UI audio (HOVER=0, CLICK=1)");
    }

    // Initialize global tables (Constitutional: Store Player_t by value, not pointer)
    g_players = d_TableInit(sizeof(int), sizeof(Player_t),
                            d_HashInt, d_CompareInt, 16);

    g_card_textures = d_TableInit(sizeof(int), sizeof(SDL_Surface*),
                                   d_HashInt, d_CompareInt, 64);

    g_portraits = d_TableInit(sizeof(int), sizeof(SDL_Surface*),
                              d_HashInt, d_CompareInt, 16);

    g_ability_icons = d_TableInit(sizeof(int), sizeof(SDL_Surface*),
                                   d_HashInt, d_CompareInt, 8);

    if (!g_players || !g_card_textures || !g_portraits || !g_ability_icons) {
        fprintf(stderr, "Failed to initialize global tables\n");
        a_Quit();
        exit(1);
    }

    // Load card back image (using Archimedes image cache)
    g_card_back_texture = a_ImageLoad("resources/textures/cards/card_back.png");
    if (!g_card_back_texture) {
        d_LogError("Failed to load card back image");
    } else {
        d_LogInfo("Card back image loaded");
    }

    // Load 52 card face images from PNG files (0.png - 51.png)
    // Card ID mapping: 0-12 Hearts, 13-25 Diamonds, 26-38 Spades, 39-51 Clubs
    for (int card_id = 0; card_id < 52; card_id++) {
        dString_t* path = d_StringInit();
        d_StringFormat(path, "resources/textures/cards/%d.png", card_id);

        aImage_t* img = a_ImageLoad((char*)d_StringPeek(path));
        d_StringDestroy(path);

        if (img) {
            d_TableSet(g_card_textures, &card_id, &img);
        } else {
            d_LogErrorF("Failed to load image for card_id %d", card_id);
        }
    }

    d_LogInfoF("Loaded %d card images", (int)g_card_textures->count);

    // Initialize card metadata system
    InitCardMetadata();

    // Initialize trinket system
    InitTrinketSystem();

    // Initialize global stats system
    Stats_Init();

    // Initialize global settings (load from settings.duf or use defaults)
    g_settings = Settings_Init();
    Settings_Load(g_settings);
    Settings_Apply(g_settings);  // Apply audio volume settings
    d_LogInfo("Global settings initialized and applied");

    // Load UI sound effects (new SDL_mixer API)
    if (a_AudioLoadSound("resources/audio/effects/ui/hover_button.wav", &g_ui_hover_sound) < 0) {
        d_LogError("Failed to load UI hover sound");
    }
    if (a_AudioLoadSound("resources/audio/effects/ui/click_button.wav", &g_ui_click_sound) < 0) {
        d_LogError("Failed to load UI click sound");
    }
    d_LogInfo("UI sound effects loaded (SDL_mixer)");

    // Load card slide sound effects (8 variants)
    for (int i = 0; i < CARD_SLIDE_SOUND_COUNT; i++) {
        dString_t* sound_path = d_StringInit();
        d_StringFormat(sound_path, "resources/audio/effects/game/card-slide-%d.wav", i + 1);

        if (a_AudioLoadSound((char*)d_StringPeek(sound_path), &g_card_slide_sounds[i]) < 0) {
            d_LogErrorF("Failed to load card slide sound %d", i + 1);
        }

        d_StringDestroy(sound_path);
    }
    d_LogInfoF("Card slide sound effects loaded (%d variants)", CARD_SLIDE_SOUND_COUNT);

    // Load enemy database from DUF
    dDUFError_t* err = d_DUFParseFile("data/enemies/tutorial_enemies.duf", &g_enemies_db);
    if (err != NULL) {
        d_LogErrorF("Failed to parse enemy database at %d:%d - %s",
                   err->line, err->column, d_StringPeek(err->message));

        // Show OS error dialog
        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "DUF Parse Error\n\nFile: data/enemies/tutorial_enemies.duf\nLine %d, Column %d\n\n%s",
                err->line, err->column, d_StringPeek(err->message));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", error_buf, NULL);

        d_DUFErrorFree(err);
        a_Quit();
        exit(1);
    }
    d_LogInfo("Enemy database loaded successfully");

    // Validate all enemies in database (checks schema, required fields, enums)
    char validation_error[1024];
    if (!ValidateEnemyDatabase(g_enemies_db, validation_error, sizeof(validation_error))) {
        d_LogError("Enemy database validation failed!");

        // Show OS error dialog with validation details
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", validation_error, NULL);

        a_Quit();
        exit(1);
    }

    // Load affix database from DUF
    err = LoadAffixDatabase("data/affixes/combat_affixes.duf", &g_affixes_db);
    if (err != NULL) {
        d_LogErrorF("Failed to parse affix database at %d:%d - %s",
                   err->line, err->column, d_StringPeek(err->message));

        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "Affix DUF Parse Error\n\nFile: combat_affixes.duf\nLine %d, Column %d\n\n%s",
                err->line, err->column, d_StringPeek(err->message));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", error_buf, NULL);

        d_DUFErrorFree(err);
        a_Quit();
        exit(1);
    }

    // Validate affix database
    if (!ValidateAffixDatabase(g_affixes_db, validation_error, sizeof(validation_error))) {
        d_LogError("Affix database validation failed!");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", validation_error, NULL);
        a_Quit();
        exit(1);
    }

    // Enemy pattern: Affixes parsed on-demand, no table population needed
    d_LogInfo("Affix database loaded successfully (will parse on-demand)");

    // Load combat trinket database from DUF
    dDUFValue_t* combat_trinkets_db = NULL;
    err = LoadTrinketDatabase("data/trinkets/combat_trinkets.duf", &combat_trinkets_db);
    if (err != NULL) {
        d_LogErrorF("Failed to parse combat trinket database at %d:%d - %s",
                   err->line, err->column, d_StringPeek(err->message));

        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "Trinket DUF Parse Error\n\nFile: combat_trinkets.duf\nLine %d, Column %d\n\n%s",
                err->line, err->column, d_StringPeek(err->message));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", error_buf, NULL);

        d_DUFErrorFree(err);
        a_Quit();
        exit(1);
    }

    // Validate combat trinket database
    if (!ValidateTrinketDatabase(combat_trinkets_db, validation_error, sizeof(validation_error))) {
        d_LogError("Combat trinket database validation failed!");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", validation_error, NULL);
        a_Quit();
        exit(1);
    }

    // Load event trinket database from DUF
    dDUFValue_t* event_trinkets_db = NULL;
    err = LoadTrinketDatabase("data/trinkets/event_trinkets.duf", &event_trinkets_db);
    if (err != NULL) {
        d_LogErrorF("Failed to parse event trinket database at %d:%d - %s",
                   err->line, err->column, d_StringPeek(err->message));

        char error_buf[512];
        snprintf(error_buf, sizeof(error_buf),
                "Trinket DUF Parse Error\n\nFile: event_trinkets.duf\nLine %d, Column %d\n\n%s",
                err->line, err->column, d_StringPeek(err->message));
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", error_buf, NULL);

        d_DUFErrorFree(err);
        a_Quit();
        exit(1);
    }

    // Validate event trinket database
    if (!ValidateTrinketDatabase(event_trinkets_db, validation_error, sizeof(validation_error))) {
        d_LogError("Event trinket database validation failed!");
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error", validation_error, NULL);
        a_Quit();
        exit(1);
    }

    // Store combat DUF reference first (needed for on-demand loading)
    g_trinkets_db = combat_trinkets_db;

    // Merge both trinket databases into key cache
    if (!MergeTrinketDatabases(combat_trinkets_db, event_trinkets_db)) {
        d_LogFatal("Failed to populate trinket templates");
        a_Quit();
        exit(1);
    }

    // DUF validation already completed in ValidateTrinketDUF()
    d_LogInfo("✓ Trinket DUF loading validated successfully");

    d_LogInfo("Global tables initialized");
    d_LogInfoF("Screen size: %dx%d", SCREEN_WIDTH, SCREEN_HEIGHT);
}

void Cleanup(void) {
    d_LogInfo("Shutting down...");

    // Cleanup test deck and player hand
    CleanupDeck(&g_test_deck);
    CleanupHand(&g_player_hand);

    // Destroy global tables
    if (g_players) {
        d_TableDestroy(&g_players);
        d_LogInfo("Player registry destroyed");
    }

    // Cleanup trinket loader system (frees g_trinket_key_cache, DUF trees)
    CleanupTrinketLoaderSystem();

    // Cleanup affix system (frees g_affixes_db DUF tree - enemy pattern)
    CleanupAffixSystem();

    // Free enemy database
    if (g_enemies_db) {
        d_DUFFree(g_enemies_db);
        g_enemies_db = NULL;
        d_LogInfo("Enemy database destroyed");
    }

    // Card back image is managed by Archimedes image cache (auto-cleanup)
    g_card_back_texture = NULL;

    // Free all card surfaces before destroying table
    if (g_card_textures) {
        dArray_t* card_ids = d_TableGetAllKeys(g_card_textures);
        if (card_ids) {
            for (size_t i = 0; i < card_ids->count; i++) {
                int* card_id = (int*)d_ArrayGet(card_ids, i);
                if (card_id) {
                    SDL_Surface** surf_ptr = (SDL_Surface**)d_TableGet(g_card_textures, card_id);
                    if (surf_ptr && *surf_ptr) {
                        SDL_FreeSurface(*surf_ptr);
                    }
                }
            }
            d_ArrayDestroy(card_ids);
        }
        d_TableDestroy(&g_card_textures);
        d_LogInfo("Surface cache destroyed");
    }

    // Free all portrait surfaces before destroying table
    if (g_portraits) {
        dArray_t* player_ids = d_TableGetAllKeys(g_portraits);
        if (player_ids) {
            for (size_t i = 0; i < player_ids->count; i++) {
                int* player_id = (int*)d_ArrayGet(player_ids, i);
                if (player_id) {
                    SDL_Surface** surf_ptr = (SDL_Surface**)d_TableGet(g_portraits, player_id);
                    if (surf_ptr && *surf_ptr) {
                        SDL_FreeSurface(*surf_ptr);
                    }
                }
            }
            d_ArrayDestroy(player_ids);
        }
        d_TableDestroy(&g_portraits);
        d_LogInfo("Portrait cache destroyed");
    }

    // Free all ability icon surfaces before destroying table
    if (g_ability_icons) {
        dArray_t* keys = d_TableGetAllKeys(g_ability_icons);
        if (keys) {
            for (size_t i = 0; i < keys->count; i++) {
                int* ability_id = (int*)d_ArrayGet(keys, i);
                if (ability_id) {
                    SDL_Surface** surf_ptr = (SDL_Surface**)d_TableGet(g_ability_icons, ability_id);
                    if (surf_ptr && *surf_ptr) {
                        SDL_FreeSurface(*surf_ptr);
                    }
                }
            }
            d_ArrayDestroy(keys);
        }
        d_TableDestroy(&g_ability_icons);
        d_LogInfo("Ability icon cache destroyed");
    }

    // Cleanup trinket system (ADR-14: destroy in reverse init order)
    CleanupTrinketSystem();

    // Cleanup card metadata system (ADR-14: initialized first, destroyed last)
    CleanupCardMetadata();

    // Save and destroy global settings
    if (g_settings) {
        Settings_Save(g_settings);
        Settings_Destroy(&g_settings);
        d_LogInfo("Settings saved and destroyed");
    }

    // Quit Archimedes
    a_Quit();
    d_LogInfo("Archimedes shutdown complete");
}

// ============================================================================
// RENDERING HELPERS
// ============================================================================

static void RenderHand(const Hand_t* hand, int start_x, int start_y) {
    // Check NULL before dereferencing
    if (!hand || !hand->cards) {
        return;
    }

    // Early return if hand is empty
    if (hand->cards->count == 0) {
        return;
    }

    // Draw each card in hand horizontally with spacing
    const int CARD_SPACING = 120;  // Horizontal spacing between cards

    for (size_t i = 0; i < hand->cards->count; i++) {
        const Card_t* card = GetCardFromHand(hand, i);
        if (!card) continue;

        int x = start_x + (i * CARD_SPACING);
        int y = start_y;

        // Draw card background
        if (g_card_back_texture && g_card_back_texture->surface) {
            aRectf_t src = {0, 0, g_card_back_texture->surface->w, g_card_back_texture->surface->h};
            aRectf_t dest = {x, y, CARD_WIDTH, CARD_HEIGHT};
            a_BlitRect(g_card_back_texture, &src, &dest, 1.0f);
        }

        // Draw card face if face up
        // NOTE: card->texture is SDL_Texture*, but we need aImage_t* for a_BlitRect
        // This is a temporary workaround - cards should store aImage_t* in the future
        if (card->face_up && card->texture) {
            d_LogWarning("Card rendering in main.c needs migration to aImage_t*");
            // TODO: Migrate card textures to aImage_t* system
        }
    }
}

// ============================================================================
// SCENE DELEGATES
// ============================================================================

static void SceneLogic(float dt) {
    (void)dt;  // Unused for now

    a_DoInput();

}

static void SceneDraw(float dt) {
    (void)dt;  // Unused for now

    // Set background color (dark green felt)
    app.background = (aColor_t){10, 80, 30, 255};

    // Draw title
    aTextStyle_t title_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, 255},
        .bg = {0, 0, 0, 0},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f,
        .padding = 0
    };
    a_DrawText("Card Fifty-Two", SCREEN_WIDTH / 2, 100, title_style);

    // Draw subtitle (USE FONT_ENTER_COMMAND, BRIGHT WHITE)
    a_DrawText("Tech Demo - Archimedes & Daedalus", SCREEN_WIDTH / 2, 160, title_style);

    // Draw deck info (Constitutional pattern: dString_t, not char[])
    dString_t* deck_info = d_StringInit();
    d_StringFormat(deck_info, "Deck: %zu cards remaining",
                   GetDeckSize(&g_test_deck));
    a_DrawText((char*)d_StringPeek(deck_info), SCREEN_WIDTH / 2, 220, title_style);
    d_StringDestroy(deck_info);

    // Draw player hand info (yellow)
    aTextStyle_t hand_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 0, 255},
        .bg = {0, 0, 0, 0},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f,
        .padding = 0
    };
    dString_t* hand_info = d_StringInit();
    d_StringFormat(hand_info, "Your Hand - Cards: %zu | Value: %d%s",
                   GetHandSize(&g_player_hand),
                   g_player_hand.total_value,
                   g_player_hand.is_blackjack ? " (BLACKJACK!)" :
                   g_player_hand.is_bust ? " (BUST)" : "");
    a_DrawText((char*)d_StringPeek(hand_info), SCREEN_WIDTH / 2, 280, hand_style);
    d_StringDestroy(hand_info);

    // Render player hand visually
    RenderHand(&g_player_hand, 100, 350);

    // Draw instructions (BRIGHT YELLOW)
    a_DrawText("[S] Shuffle | [D] Deal Card | [R] Reset | [ESC] Quit",
               SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100, hand_style);

    // Draw status (CYAN instead of green - visible on green bg)
    aTextStyle_t status_style = {
        .type = FONT_ENTER_COMMAND,
        .fg = {0, 255, 255, 255},
        .bg = {0, 0, 0, 0},
        .align = TEXT_ALIGN_RIGHT,
        .wrap_width = 0,
        .scale = 1.0f,
        .padding = 0
    };
    dString_t* status = d_StringInit();
    d_StringFormat(status, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_StringPeek(status), SCREEN_WIDTH - 10, 10, status_style);
    d_StringDestroy(status);
}

// ============================================================================
// SCENE INITIALIZATION
// ============================================================================

void InitScene(void) {
    // Set scene delegates
    app.delegate.logic = SceneLogic;
    app.delegate.draw = SceneDraw;

    // Initialize and shuffle test deck
    // Constitutional pattern: Deck_t is value type
    InitDeck(&g_test_deck, 1);  // Single 52-card deck
    ShuffleDeck(&g_test_deck);
    d_LogInfo("Test deck initialized and shuffled");

    // Initialize player hand
    // Constitutional pattern: Hand_t is value type
    InitHand(&g_player_hand);
    d_LogInfo("Player hand initialized");

    d_LogInfo("Scene delegates initialized");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void MainLoop(void) {
    a_PrepareScene();

    float dt = a_GetDeltaTime();
    app.delegate.logic(dt);
    app.delegate.draw(dt);

    a_PresentScene();
}

// ============================================================================
// ENTRY POINT
// ============================================================================

int main(void) {
    // Initialize Daedalus Logger (MUST be first!)
    // Web builds: Use WARNING level to reduce console.log() overhead (WASM→JS boundary expensive)
    // Native builds: Keep INFO level for detailed debugging
#ifdef __EMSCRIPTEN__
    dLogConfig_t config = { .default_level = D_LOG_LEVEL_WARNING };
#else
    dLogConfig_t config = { .default_level = D_LOG_LEVEL_INFO };
#endif
    dLogger_t* logger = d_CreateLogger(config);
    d_SetGlobalLogger(logger);

    d_LogInfo("=== Card Fifty-Two Starting ===");

    // Initialize Archimedes
    Initialize();

    // Start with menu scene
    InitMenuScene();

    // Main loop (cross-platform)
    #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(MainLoop, 0, 1);
    #else
        while (app.running) {
            MainLoop();
        }
    #endif

    // Cleanup
    Cleanup();

    // Clean up the logger before finishing
    d_DestroyLogger(d_GetGlobalLogger());

    return 0;
}
