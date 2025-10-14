/*
 * Card Fifty-Two - Blackjack Scene
 * Full state machine-driven Blackjack game with betting and player actions
 * NOW WITH SECTION-BASED ARCHITECTURE - No overlap possible!
 */

#include "../../include/common.h"
#include "../../include/card.h"
#include "../../include/deck.h"
#include "../../include/hand.h"
#include "../../include/player.h"
#include "../../include/game.h"
#include "../../include/scenes/sceneBlackjack.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/scenes/components/button.h"
#include "../../include/scenes/components/deckButton.h"
#include "../../include/scenes/sections/topBarSection.h"
#include "../../include/scenes/sections/pauseMenuSection.h"
#include "../../include/scenes/sections/titleSection.h"
#include "../../include/scenes/sections/dealerSection.h"
#include "../../include/scenes/sections/playerSection.h"
#include "../../include/scenes/sections/actionPanel.h"
#include "../../include/scenes/sections/drawPileModalSection.h"
#include "../../include/scenes/sections/discardPileModalSection.h"
#include "../../include/tutorial/tutorialSystem.h"
#include "../../include/tutorial/blackjackTutorial.h"

// External globals from main.c
extern Deck_t g_test_deck;
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;

// ============================================================================
// SCENE-LOCAL STATE
// ============================================================================

static GameContext_t g_game;  // Stack-allocated singleton (constitutional pattern)
static Player_t* g_dealer = NULL;
static Player_t* g_human_player = NULL;

// FlexBox layout (main vertical container)
static FlexBox_t* g_main_layout = NULL;

// UI Sections (FlexBox items)
static TopBarSection_t* g_top_bar = NULL;
static PauseMenuSection_t* g_pause_menu = NULL;
static TitleSection_t* g_title_section = NULL;
static DealerSection_t* g_dealer_section = NULL;
static PlayerSection_t* g_player_section = NULL;
static ActionPanel_t* g_betting_panel = NULL;
static ActionPanel_t* g_action_panel = NULL;

// Modal sections
static DrawPileModalSection_t* g_draw_pile_modal = NULL;
static DiscardPileModalSection_t* g_discard_pile_modal = NULL;

// Reusable Button components
static Button_t* bet_buttons[NUM_BET_BUTTONS] = {NULL};
static Button_t* action_buttons[NUM_ACTION_BUTTONS] = {NULL};
static DeckButton_t* deck_buttons[NUM_DECK_BUTTONS] = {NULL};  // [0]=View Deck, [1]=View Discard

// Tutorial system
static TutorialSystem_t* g_tutorial_system = NULL;
static TutorialStep_t* g_tutorial_steps = NULL;

// Forward declarations
static void BlackjackLogic(float dt);
static void BlackjackDraw(float dt);
static void CleanupBlackjackScene(void);

// Layout initialization
static void InitializeLayout(void);
static void CleanupLayout(void);

// Input handlers
static void HandleBettingInput(void);
static void HandlePlayerTurnInput(void);

// Rendering
static void RenderResultOverlay(void);

// ============================================================================
// LAYOUT INITIALIZATION
// ============================================================================

static void InitializeLayout(void) {
    // Create main vertical FlexBox (using constants from header)
    // Top bar is now independent, so layout starts at LAYOUT_TOP_MARGIN (35px)
    g_main_layout = a_CreateFlexBox(0, LAYOUT_TOP_MARGIN, SCREEN_WIDTH, SCREEN_HEIGHT - LAYOUT_TOP_MARGIN - LAYOUT_BOTTOM_CLEARANCE);
    a_FlexConfigure(g_main_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_SPACE_BETWEEN, LAYOUT_GAP);
    a_FlexSetPadding(g_main_layout, 0);

    // Add 4 sections to main layout (top bar is now independent!)
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, TITLE_AREA_HEIGHT, NULL);
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, DEALER_AREA_HEIGHT, NULL);
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, PLAYER_AREA_HEIGHT, NULL);
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, BUTTON_AREA_HEIGHT, NULL);

    // Calculate initial layout
    a_FlexLayout(g_main_layout);

    // Create UI sections
    g_top_bar = CreateTopBarSection();
    g_pause_menu = CreatePauseMenuSection();
    g_title_section = CreateTitleSection();
    g_dealer_section = CreateDealerSection();

    // Create deck/discard view buttons FIRST (needed for PlayerSection)
    // Count text is now passed dynamically during render, not stored
    const char* deck_hotkeys[] = {"[V]", "[C]"};
    for (int i = 0; i < NUM_DECK_BUTTONS; i++) {
        deck_buttons[i] = CreateDeckButton(0, 0, deck_hotkeys[i]);
    }

    // Create player section with deck buttons
    g_player_section = CreatePlayerSection(deck_buttons, &g_test_deck);

    // Create betting buttons
    const char* bet_labels[] = {"1", "5", "10"};
    const char* bet_hotkeys[] = {"[1]", "[2]", "[3]"};
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        bet_buttons[i] = CreateButton(0, 0, BET_BUTTON_WIDTH, BET_BUTTON_HEIGHT, bet_labels[i]);
        SetButtonHotkey(bet_buttons[i], bet_hotkeys[i]);
    }

    // Create betting panel section
    g_betting_panel = CreateActionPanel("Place Your Bet:", bet_buttons, NUM_BET_BUTTONS);

    // Create action buttons
    const char* action_labels[] = {"Hit", "Stand", "Double"};
    const char* action_hotkeys[] = {"[H]", "[S]", "[D]"};
    for (int i = 0; i < NUM_ACTION_BUTTONS; i++) {
        action_buttons[i] = CreateButton(0, 0, ACTION_BUTTON_WIDTH, ACTION_BUTTON_HEIGHT, action_labels[i]);
        SetButtonHotkey(action_buttons[i], action_hotkeys[i]);
    }

    // Create action panel section
    g_action_panel = CreateActionPanel("Your Turn - Choose Action:", action_buttons, NUM_ACTION_BUTTONS);

    // Create deck/discard pile modals
    g_draw_pile_modal = CreateDrawPileModalSection(&g_test_deck);
    g_discard_pile_modal = CreateDiscardPileModalSection(&g_test_deck);

    d_LogInfo("Section-based layout initialized");
}

static void CleanupLayout(void) {
    // Destroy UI sections (sections destroy their own FlexBoxes)
    if (g_top_bar) {
        DestroyTopBarSection(&g_top_bar);
    }
    if (g_pause_menu) {
        DestroyPauseMenuSection(&g_pause_menu);
    }
    if (g_title_section) {
        DestroyTitleSection(&g_title_section);
    }
    if (g_dealer_section) {
        DestroyDealerSection(&g_dealer_section);
    }
    if (g_player_section) {
        DestroyPlayerSection(&g_player_section);
    }
    if (g_betting_panel) {
        DestroyActionPanel(&g_betting_panel);
    }
    if (g_action_panel) {
        DestroyActionPanel(&g_action_panel);
    }

    // Destroy modals
    if (g_draw_pile_modal) {
        DestroyDrawPileModalSection(&g_draw_pile_modal);
    }
    if (g_discard_pile_modal) {
        DestroyDiscardPileModalSection(&g_discard_pile_modal);
    }

    // Destroy main FlexBox
    if (g_main_layout) {
        a_DestroyFlexBox(&g_main_layout);
    }

    // Destroy button components (panels don't own these)
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (bet_buttons[i]) {
            DestroyButton(&bet_buttons[i]);
        }
    }
    for (int i = 0; i < NUM_ACTION_BUTTONS; i++) {
        if (action_buttons[i]) {
            DestroyButton(&action_buttons[i]);
        }
    }
    for (int i = 0; i < NUM_DECK_BUTTONS; i++) {
        if (deck_buttons[i]) {
            DestroyDeckButton(&deck_buttons[i]);
        }
    }

    d_LogInfo("Section cleanup complete");
}

// ============================================================================
// SCENE INITIALIZATION
// ============================================================================

void InitBlackjackScene(void) {
    d_LogInfo("Initializing Blackjack scene");

    // Set scene delegates
    app.delegate.logic = BlackjackLogic;
    app.delegate.draw = BlackjackDraw;

    // Initialize and shuffle deck
    InitDeck(&g_test_deck, 1);
    ShuffleDeck(&g_test_deck);

    // Create dealer (player_id = 0)
    if (!CreatePlayer("Dealer", 0, true)) {
        d_LogFatal("Failed to create dealer");
        return;
    }
    g_dealer = GetPlayerByID(0);

    // Create human player (player_id = 1)
    if (!CreatePlayer("Player", 1, false)) {
        d_LogFatal("Failed to create human player");
        return;
    }
    g_human_player = GetPlayerByID(1);

    // Initialize game context
    InitGameContext(&g_game, &g_test_deck);

    // Add players to game (dealer first, then player)
    AddPlayerToGame(&g_game, 0);  // Dealer
    AddPlayerToGame(&g_game, 1);  // Player

    // Initialize FlexBox layout (automatic positioning!)
    InitializeLayout();

    // Initialize tutorial system
    g_tutorial_system = CreateTutorialSystem();
    g_tutorial_steps = CreateBlackjackTutorial();
    if (g_tutorial_system && g_tutorial_steps) {
        StartTutorial(g_tutorial_system, g_tutorial_steps);
        d_LogInfo("Tutorial started");
    }

    // Start in betting state
    TransitionState(&g_game, STATE_BETTING);

    d_LogInfo("Blackjack scene ready");
}

static void CleanupBlackjackScene(void) {
    d_LogInfo("Cleaning up Blackjack scene");

    // Cleanup tutorial system
    if (g_tutorial_steps) {
        DestroyBlackjackTutorial(g_tutorial_steps);
        g_tutorial_steps = NULL;
    }
    if (g_tutorial_system) {
        DestroyTutorialSystem(&g_tutorial_system);
    }

    // Cleanup FlexBox layout and buttons
    CleanupLayout();

    // Cleanup game context (destroys internal tables/arrays)
    CleanupGameContext(&g_game);

    // Destroy players (also removes from g_players table)
    if (g_dealer) {
        DestroyPlayer(0);  // Dealer ID
        g_dealer = NULL;
    }
    if (g_human_player) {
        DestroyPlayer(1);  // Player ID
        g_human_player = NULL;
    }

    // Cleanup deck
    CleanupDeck(&g_test_deck);

    d_LogInfo("Blackjack scene cleanup complete");
}

// ============================================================================
// SCENE LOGIC
// ============================================================================

static void BlackjackLogic(float dt) {
    a_DoInput();

    // Handle modals first (highest priority)
    if (g_draw_pile_modal && IsDrawPileModalVisible(g_draw_pile_modal)) {
        HandleDrawPileModalInput(g_draw_pile_modal);
        return;  // Block all other input
    }
    if (g_discard_pile_modal && IsDiscardPileModalVisible(g_discard_pile_modal)) {
        HandleDiscardPileModalInput(g_discard_pile_modal);
        return;  // Block all other input
    }

    // If tutorial is active, handle tutorial input (non-blocking - game input continues)
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        HandleTutorialInput(g_tutorial_system);
        UpdateTutorialListeners(g_tutorial_system, dt);
        // Note: Don't return - allow game input to continue
    }

    // Check if tutorial is active and we're NOT on the final step (blocks input during steps 1 and 2)
    bool tutorial_blocking_input = (g_tutorial_system && IsTutorialActive(g_tutorial_system) &&
                                     g_tutorial_system->current_step &&
                                     !g_tutorial_system->current_step->is_final_step);

    // If pause menu is visible, handle it exclusively
    if (g_pause_menu && g_pause_menu->is_visible) {
        PauseAction_t action = HandlePauseMenuInput(g_pause_menu);
        switch (action) {
            case PAUSE_ACTION_RESUME:
                HidePauseMenu(g_pause_menu);
                break;
            case PAUSE_ACTION_QUIT_TO_MENU:
                d_LogInfo("Quit to menu confirmed");
                CleanupBlackjackScene();
                InitMenuScene();
                return;
            case PAUSE_ACTION_SETTINGS:
                // TODO: Open settings (future)
                d_LogInfo("Settings not implemented yet");
                break;
            case PAUSE_ACTION_HELP:
                // TODO: Open help (future)
                d_LogInfo("Help not implemented yet");
                break;
            case PAUSE_ACTION_NONE:
            default:
                break;
        }
        return;  // Don't process game logic while paused
    }

    // Settings button clicked - Show pause menu (blocked during tutorial steps 1 and 2)
    if (!tutorial_blocking_input && IsTopBarSettingsClicked(g_top_bar)) {
        d_LogInfo("Settings button clicked - showing pause menu");
        ShowPauseMenu(g_pause_menu);
        return;
    }

    // ESC - Show pause menu (blocked during tutorial steps 1 and 2)
    if (!tutorial_blocking_input && app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        d_LogInfo("ESC pressed - showing pause menu");
        ShowPauseMenu(g_pause_menu);
        return;
    }

    // Deck/Discard view buttons - always available (except during tutorial steps 1 and 2)
    if (!tutorial_blocking_input) {
        // View Deck button (V key)
        if (deck_buttons[0] && IsDeckButtonClicked(deck_buttons[0])) {
            ShowDrawPileModal(g_draw_pile_modal);
        }
        if (app.keyboard[SDL_SCANCODE_V]) {
            app.keyboard[SDL_SCANCODE_V] = 0;
            ShowDrawPileModal(g_draw_pile_modal);
        }

        // View Discard button (C key)
        if (deck_buttons[1] && IsDeckButtonClicked(deck_buttons[1])) {
            ShowDiscardPileModal(g_discard_pile_modal);
        }
        if (app.keyboard[SDL_SCANCODE_C]) {
            app.keyboard[SDL_SCANCODE_C] = 0;
            ShowDiscardPileModal(g_discard_pile_modal);
        }
    }

    // Update game state machine
    GameState_t previous_state = g_game.current_state;
    UpdateGameLogic(&g_game, dt);

    // Trigger tutorial events on state changes
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        if (g_game.current_state != previous_state) {
            TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_STATE_CHANGE,
                                (void*)(intptr_t)g_game.current_state);
        }
    }

    // Handle input based on current state
    switch (g_game.current_state) {
        case STATE_BETTING:
            HandleBettingInput();
            break;

        case STATE_PLAYER_TURN:
            HandlePlayerTurnInput();
            break;

        default:
            // Other states handled by state machine
            break;
    }
}

// ============================================================================
// INPUT HANDLERS
// ============================================================================

static void HandleBettingInput(void) {
    if (!g_human_player) return;

    const int bet_amounts[] = {BET_AMOUNT_MIN, BET_AMOUNT_MED, BET_AMOUNT_MAX};

    // Update button positions from betting panel
    UpdateActionPanelButtons(g_betting_panel);

    // Mouse input - check each betting button
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i]) continue;

        // Enable/disable based on available chips
        SetButtonEnabled(bet_buttons[i], CanAffordBet(g_human_player, bet_amounts[i]));

        // Process click - game.c validates and executes
        if (IsButtonClicked(bet_buttons[i])) {
            ProcessBettingInput(&g_game, g_human_player, bet_amounts[i]);
        }
    }

    // Keyboard shortcuts: 1/2/3 for bet amounts
    if (app.keyboard[SDL_SCANCODE_1]) {
        app.keyboard[SDL_SCANCODE_1] = 0;
        ProcessBettingInput(&g_game, g_human_player, BET_AMOUNT_MIN);
    }
    if (app.keyboard[SDL_SCANCODE_2]) {
        app.keyboard[SDL_SCANCODE_2] = 0;
        ProcessBettingInput(&g_game, g_human_player, BET_AMOUNT_MED);
    }
    if (app.keyboard[SDL_SCANCODE_3]) {
        app.keyboard[SDL_SCANCODE_3] = 0;
        ProcessBettingInput(&g_game, g_human_player, BET_AMOUNT_MAX);
    }
}

static void HandlePlayerTurnInput(void) {
    if (!g_human_player) return;

    // Update button positions from action panel
    UpdateActionPanelButtons(g_action_panel);

    // Enable/disable double button (UI feedback only - game.c validates)
    bool can_double = (g_human_player->hand.cards->count == 2 &&
                       CanAffordBet(g_human_player, g_human_player->current_bet));
    if (action_buttons[2]) {
        SetButtonEnabled(action_buttons[2], can_double);
    }

    // Track if an action was taken (for tutorial step 2)
    bool action_taken = false;

    // Mouse input - Hit button
    if (action_buttons[0] && IsButtonClicked(action_buttons[0])) {
        ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_HIT);
        action_taken = true;
    }

    // Stand button
    if (action_buttons[1] && IsButtonClicked(action_buttons[1])) {
        ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_STAND);
        action_taken = true;
    }

    // Double button
    if (action_buttons[2] && IsButtonClicked(action_buttons[2])) {
        ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_DOUBLE);
        action_taken = true;
    }

    // Keyboard shortcuts
    if (app.keyboard[SDL_SCANCODE_H]) {
        app.keyboard[SDL_SCANCODE_H] = 0;
        ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_HIT);
        action_taken = true;
    }

    if (app.keyboard[SDL_SCANCODE_S]) {
        app.keyboard[SDL_SCANCODE_S] = 0;
        ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_STAND);
        action_taken = true;
    }

    if (app.keyboard[SDL_SCANCODE_D]) {
        app.keyboard[SDL_SCANCODE_D] = 0;
        ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_DOUBLE);
        action_taken = true;
    }

    // If tutorial step 2 is active and an action was taken, advance tutorial
    if (action_taken && g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        // Check if we're on step 2 (not first step, not final step)
        if (g_tutorial_system->current_step &&
            g_tutorial_system->current_step != g_tutorial_system->first_step &&
            !g_tutorial_system->current_step->is_final_step) {
            // Manually trigger tutorial advancement
            g_tutorial_system->current_step->listener.triggered = true;
        }
    }
}

// ============================================================================
// SCENE RENDERING
// ============================================================================

static void BlackjackDraw(float dt) {
    (void)dt;

    // Dark green felt background
    app.background = TABLE_FELT_GREEN;

    // Render top bar at fixed position (independent of FlexBox)
    RenderTopBarSection(g_top_bar, 0);

    // Get FlexBox-calculated positions and render all sections
    if (g_main_layout) {
        a_FlexLayout(g_main_layout);

        // Render 4 sections using FlexBox positions
        int title_y = a_FlexGetItemY(g_main_layout, 0);
        int dealer_y = a_FlexGetItemY(g_main_layout, 1);
        int player_y = a_FlexGetItemY(g_main_layout, 2);
        int button_y = a_FlexGetItemY(g_main_layout, 3);

        RenderTitleSection(g_title_section, &g_game, title_y);
        RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
        RenderPlayerSection(g_player_section, g_human_player, player_y);

        // State-specific panels
        switch (g_game.current_state) {
            case STATE_BETTING:
                RenderActionPanel(g_betting_panel, button_y);
                break;

            case STATE_PLAYER_TURN:
                RenderActionPanel(g_action_panel, button_y);
                break;

            case STATE_ROUND_END:
                RenderResultOverlay();
                break;

            default:
                break;
        }
    }

    // Render pause menu if visible (on top of everything)
    if (g_pause_menu) {
        RenderPauseMenuSection(g_pause_menu);
    }

    // Render deck/discard modals (above pause menu, below tutorial)
    if (g_draw_pile_modal) {
        RenderDrawPileModalSection(g_draw_pile_modal);
    }
    if (g_discard_pile_modal) {
        RenderDiscardPileModalSection(g_discard_pile_modal);
    }

    // Render tutorial if active (absolute top layer)
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        RenderTutorial(g_tutorial_system);
    }

    // FPS
    dString_t* fps_str = d_InitString();
    d_FormatString(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_PeekString(fps_str), SCREEN_WIDTH - 10, 10,
               0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
    d_DestroyString(fps_str);
}

// ============================================================================
// RENDERING HELPERS
// ============================================================================

static void RenderResultOverlay(void) {
    if (!g_human_player) return;

    // Semi-transparent overlay
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, OVERLAY_ALPHA);

    // Determine result message
    const char* message = "";
    aColor_t msg_color = {255, 255, 255, 255};

    switch (g_human_player->state) {
        case PLAYER_STATE_WON:
            message = "YOU WIN!";
            msg_color = (aColor_t){0, 255, 0, 255};  // Green
            break;
        case PLAYER_STATE_LOST:
            message = "YOU LOSE";
            msg_color = (aColor_t){255, 0, 0, 255};  // Red
            break;
        case PLAYER_STATE_PUSH:
            message = "PUSH";
            msg_color = (aColor_t){255, 255, 0, 255};  // Yellow
            break;
        case PLAYER_STATE_BLACKJACK:
            if (g_human_player->state == PLAYER_STATE_WON) {
                message = "BLACKJACK!";
                msg_color = (aColor_t){0, 255, 0, 255};
            }
            break;
        default:
            message = "Round Complete";
            break;
    }

    // Draw result message
    a_DrawText((char*)message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40,
               msg_color.r, msg_color.g, msg_color.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Chips change info
    dString_t* chip_info = d_InitString();
    d_FormatString(chip_info, "Chips: %d", GetPlayerChips(g_human_player));
    a_DrawText((char*)d_PeekString(chip_info), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_DestroyString(chip_info);

    // Next round prompt
    a_DrawText("Next round starting...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 60,
               200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
}
