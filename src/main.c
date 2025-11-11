/*
 * Card Fifty-Two - Main Entry Point
 * A card game framework demonstrating Archimedes and Daedalus integration
 */

#include "common.h"
#include "card.h"
#include "deck.h"
#include "hand.h"
#include "scenes/sceneMenu.h"

// ============================================================================
// GLOBAL VARIABLES
// ============================================================================

// Player registry (defined in common.h, declared here)
dTable_t* g_players = NULL;

// Texture cache (defined in common.h, declared here)
dTable_t* g_card_textures = NULL;

// Portrait texture cache (defined in common.h, declared here)
dTable_t* g_portraits = NULL;

// Card back texture (defined in common.h, declared here)
SDL_Texture* g_card_back_texture = NULL;

// Test deck (for demonstration)
// Constitutional pattern: Deck_t is value type, not pointer
Deck_t g_test_deck;

// Player hand (for demonstration)
// Constitutional pattern: Hand_t is value type, not pointer
Hand_t g_player_hand;

// ============================================================================
// GLOBAL FONT STYLES
// ============================================================================

aFontConfig_t FONT_STYLE_TITLE = {
    .type = FONT_ENTER_COMMAND,
    .color = {255, 255, 255, 255},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f
};

aFontConfig_t FONT_STYLE_BODY = {
    .type = FONT_GAME,
    .color = {255, 255, 255, 255},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f
};

aFontConfig_t FONT_STYLE_CHIP_COUNT = {
    .type = FONT_GAME,
    .color = {255, 255, 0, 255},  // Yellow
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f
};

aFontConfig_t FONT_STYLE_DEBUG = {
    .type = FONT_GAME,
    .color = {0, 255, 0, 255},  // Green
    .align = TEXT_ALIGN_RIGHT,
    .wrap_width = 0,
    .scale = 1.0f
};

aFontConfig_t FONT_STYLE_DAMAGE = {
    .type = FONT_GAME,
    .color = {255, 0, 0, 255},  // Red (overridden for healing)
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f
};

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

    // Initialize global tables (Constitutional: Store Player_t by value, not pointer)
    g_players = d_InitTable(sizeof(int), sizeof(Player_t),
                            d_HashInt, d_CompareInt, 16);

    g_card_textures = d_InitTable(sizeof(int), sizeof(SDL_Texture*),
                                   d_HashInt, d_CompareInt, 64);

    g_portraits = d_InitTable(sizeof(int), sizeof(SDL_Texture*),
                              d_HashInt, d_CompareInt, 16);

    if (!g_players || !g_card_textures || !g_portraits) {
        fprintf(stderr, "Failed to initialize global tables\n");
        a_Quit();
        exit(1);
    }

    // Load card back texture
    g_card_back_texture = a_LoadTexture("resources/textures/cards/card_back.png");
    if (!g_card_back_texture) {
        d_LogError("Failed to load card back texture");
    } else {
        d_LogInfo("Card back texture loaded");
    }

    // Load 52 card face textures from PNG files (0.png - 51.png)
    // Card ID mapping: 0-12 Hearts, 13-25 Diamonds, 26-38 Spades, 39-51 Clubs
    for (int card_id = 0; card_id < 52; card_id++) {
        dString_t* path = d_StringInit();
        d_StringFormat(path, "resources/textures/cards/%d.png", card_id);

        SDL_Texture* tex = a_LoadTexture((char*)d_StringPeek(path));
        d_StringDestroy(path);

        if (tex) {
            d_SetDataInTable(g_card_textures, &card_id, &tex);
        } else {
            d_LogErrorF("Failed to load texture for card_id %d", card_id);
        }
    }

    d_LogInfoF("Loaded %d card textures", (int)g_card_textures->count);

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
        d_DestroyTable(&g_players);
        d_LogInfo("Player registry destroyed");
    }

    if (g_card_textures) {
        d_DestroyTable(&g_card_textures);
        d_LogInfo("Texture cache destroyed");
    }

    if (g_portraits) {
        d_DestroyTable(&g_portraits);
        d_LogInfo("Portrait cache destroyed");
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
        if (g_card_back_texture) {
            SDL_Rect bg_src = {0, 0, CARD_WIDTH, CARD_HEIGHT};
            aColor_t white = {255, 255, 255, 255};
            a_BlitTextureRect(g_card_back_texture, bg_src, x, y, 1, white);
        }

        // Draw card face if face up
        if (card->face_up && card->texture) {
            // Query actual texture dimensions
            int tex_w = 0, tex_h = 0;
            int query_result = SDL_QueryTexture(card->texture, NULL, NULL, &tex_w, &tex_h);

            if (query_result != 0 || tex_w <= 0 || tex_h <= 0) {
                d_LogErrorF("Invalid texture dimensions: %dx%d (query result: %d)",
                           tex_w, tex_h, query_result);
                continue;
            }

            // Draw card label centered on card
            int label_x = x + CARD_WIDTH / 2 - tex_w / 2;
            int label_y = y + CARD_HEIGHT / 2 - tex_h / 2;

            SDL_Rect src = {0, 0, tex_w, tex_h};
            aColor_t color = {255, 255, 255, 255};
            a_BlitTextureRect(card->texture, src, label_x, label_y, 1, color);
        }
    }
}

// ============================================================================
// SCENE DELEGATES
// ============================================================================

static void SceneLogic(float dt) {
    (void)dt;  // Unused for now

    a_DoInput();

    // ESC to quit
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        d_LogInfo("ESC pressed - quitting");
        app.running = 0;
    }

    // S - Shuffle deck
    if (app.keyboard[SDL_SCANCODE_S]) {
        app.keyboard[SDL_SCANCODE_S] = 0;  // Reset key state
        ShuffleDeck(&g_test_deck);
        d_LogInfo("Deck shuffled manually");
    }

    // D - Deal card to player hand
    if (app.keyboard[SDL_SCANCODE_D]) {
        app.keyboard[SDL_SCANCODE_D] = 0;  // Reset key state
        if (!IsDeckEmpty(&g_test_deck)) {
            Card_t card = DealCard(&g_test_deck);
            if (card.card_id != -1) {
                // Set card to face up and load texture
                card.face_up = true;
                LoadCardTexture(&card);

                // Add to player hand
                AddCardToHand(&g_player_hand, card);

                dString_t* card_str = d_StringInit();
                CardToString(&card, card_str);
                d_LogInfoF("Dealt card to hand: %s (Hand value: %d)",
                          d_StringPeek(card_str), g_player_hand.total_value);
                d_StringDestroy(card_str);
            }
        } else {
            d_LogInfo("Deck is empty - press R to reset");
        }
    }

    // R - Reset deck and hand
    if (app.keyboard[SDL_SCANCODE_R]) {
        app.keyboard[SDL_SCANCODE_R] = 0;  // Reset key state
        ClearHand(&g_player_hand, &g_test_deck);  // Discard cards to deck first
        ResetDeck(&g_test_deck);  // Then reset deck (regenerates 52 cards)
        d_LogInfo("Deck and hand reset manually");
    }
}

static void SceneDraw(float dt) {
    (void)dt;  // Unused for now

    // Set background color (dark green felt)
    app.background = (aColor_t){10, 80, 30, 255};

    // Draw title
    a_DrawText("Card Fifty-Two", SCREEN_WIDTH / 2, 100,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw subtitle (USE FONT_ENTER_COMMAND, BRIGHT WHITE)
    a_DrawText("Tech Demo - Archimedes & Daedalus", SCREEN_WIDTH / 2, 160,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw deck info (Constitutional pattern: dString_t, not char[])
    dString_t* deck_info = d_StringInit();
    d_StringFormat(deck_info, "Deck: %zu cards remaining",
                   GetDeckSize(&g_test_deck));
    a_DrawText((char*)d_StringPeek(deck_info), SCREEN_WIDTH / 2, 220,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_StringDestroy(deck_info);

    // Draw player hand info
    dString_t* hand_info = d_StringInit();
    d_StringFormat(hand_info, "Your Hand - Cards: %zu | Value: %d%s",
                   GetHandSize(&g_player_hand),
                   g_player_hand.total_value,
                   g_player_hand.is_blackjack ? " (BLACKJACK!)" :
                   g_player_hand.is_bust ? " (BUST)" : "");
    a_DrawText((char*)d_StringPeek(hand_info), SCREEN_WIDTH / 2, 280,
               255, 255, 0, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_StringDestroy(hand_info);

    // Render player hand visually
    RenderHand(&g_player_hand, 100, 350);

    // Draw instructions (BRIGHT YELLOW)
    a_DrawText("[S] Shuffle | [D] Deal Card | [R] Reset | [ESC] Quit",
               SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100,
               255, 255, 0, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw status (CYAN instead of green - visible on green bg)
    // Constitutional pattern: dString_t, not char[]
    dString_t* status = d_StringInit();
    d_StringFormat(status, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_StringPeek(status), SCREEN_WIDTH - 10, 10,
               0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
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
    dLogConfig_t config = { .default_level = D_LOG_LEVEL_DEBUG };
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
