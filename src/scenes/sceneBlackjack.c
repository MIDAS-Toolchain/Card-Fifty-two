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
#include "../../include/scenes/components/sidebarButton.h"
#include "../../include/scenes/sections/topBarSection.h"
#include "../../include/scenes/sections/pauseMenuSection.h"
#include "../../include/scenes/sections/titleSection.h"
#include "../../include/scenes/sections/dealerSection.h"
#include "../../include/scenes/sections/playerSection.h"
#include "../../include/scenes/sections/actionPanel.h"
#include "../../include/scenes/sections/drawPileModalSection.h"
#include "../../include/scenes/sections/discardPileModalSection.h"
#include "../../include/scenes/sections/leftSidebarSection.h"
#include "../../include/tutorial/tutorialSystem.h"
#include "../../include/tutorial/blackjackTutorial.h"
#include "../../include/terminal/terminal.h"
#include "../../include/tween/tween.h"
#include "../../include/cardAnimation.h"

// External globals from main.c
extern Deck_t g_test_deck;
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;

// ============================================================================
// SCENE-LOCAL STATE
// ============================================================================

GameContext_t g_game;  // Stack-allocated singleton (constitutional pattern) - non-static for terminal access
Player_t* g_dealer = NULL;
Player_t* g_human_player = NULL;  // Non-static for terminal access

// FlexBox layout (main vertical container)
static FlexBox_t* g_main_layout = NULL;

// UI Sections (FlexBox items)
static TopBarSection_t* g_top_bar = NULL;
static PauseMenuSection_t* g_pause_menu = NULL;
static LeftSidebarSection_t* g_left_sidebar = NULL;
static TitleSection_t* g_title_section = NULL;
static DealerSection_t* g_dealer_section = NULL;
static PlayerSection_t* g_player_section = NULL;
static ActionPanel_t* g_betting_panel = NULL;
static ActionPanel_t* g_action_panel = NULL;
static Terminal_t* g_terminal = NULL;

// Background textures
static SDL_Texture* g_background_texture = NULL;
static SDL_Texture* g_table_texture = NULL;

// Modal sections
static DrawPileModalSection_t* g_draw_pile_modal = NULL;
static DiscardPileModalSection_t* g_discard_pile_modal = NULL;

// Reusable Button components
static Button_t* bet_buttons[NUM_BET_BUTTONS] = {NULL};
static Button_t* action_buttons[NUM_ACTION_BUTTONS] = {NULL};
static SidebarButton_t* sidebar_buttons[NUM_DECK_BUTTONS] = {NULL};  // [0]=Draw Pile, [1]=Discard Pile

// Keyboard navigation state
static int selected_bet_button = 0;   // 0=Bet 10, 1=Bet 50, 2=Bet 100
static int selected_action_button = 0;  // 0=Hit, 1=Stand, 2=Double

// Tutorial system
static TutorialSystem_t* g_tutorial_system = NULL;
static TutorialStep_t* g_tutorial_steps = NULL;

// Tween manager (for smooth HP bar drain and damage numbers)
// Non-static: accessible from game.c for enemy damage effects
TweenManager_t g_tween_manager;

// Card transition manager (for card dealing/discarding animations)
static CardTransitionManager_t g_card_transition_manager;

// ============================================================================
// DAMAGE NUMBERS SYSTEM
// ============================================================================

#define MAX_DAMAGE_NUMBERS 16

typedef struct DamageNumber {
    bool active;
    float x, y;           // Position
    float alpha;          // Opacity (1.0 = opaque, 0.0 = invisible)
    int damage;           // Damage amount to display
    bool is_healing;      // true = green, false = red
} DamageNumber_t;

static DamageNumber_t g_damage_numbers[MAX_DAMAGE_NUMBERS];

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
    // Create main vertical FlexBox for GAME AREA (right side, after sidebar)
    // Top bar is independent, layout starts at LAYOUT_TOP_MARGIN (35px)
    // X position = SIDEBAR_WIDTH (280px), Width = remaining screen width
    g_main_layout = a_CreateFlexBox(GAME_AREA_X, LAYOUT_TOP_MARGIN,
                                     GAME_AREA_WIDTH,
                                     SCREEN_HEIGHT - LAYOUT_TOP_MARGIN - LAYOUT_BOTTOM_CLEARANCE);
    a_FlexConfigure(g_main_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_SPACE_BETWEEN, LAYOUT_GAP);
    a_FlexSetPadding(g_main_layout, 0);

    // Add 3 sections to main layout (no title, buttons before player cards)
    a_FlexAddItem(g_main_layout, GAME_AREA_WIDTH, DEALER_AREA_HEIGHT, NULL);   // Dealer now index 0
    a_FlexAddItem(g_main_layout, GAME_AREA_WIDTH, BUTTON_AREA_HEIGHT, NULL);   // Buttons now index 1
    a_FlexAddItem(g_main_layout, GAME_AREA_WIDTH, PLAYER_AREA_HEIGHT, NULL);   // Player now index 2

    // Calculate initial layout
    a_FlexLayout(g_main_layout);

    // Create UI sections
    g_top_bar = CreateTopBarSection();
    g_pause_menu = CreatePauseMenuSection();
    g_title_section = CreateTitleSection();
    g_dealer_section = CreateDealerSection();

    // Create developer terminal
    g_terminal = InitTerminal();

    // Load background textures
    g_background_texture = a_LoadTexture("resources/textures/background1.png");
    if (!g_background_texture) {
        d_LogError("Failed to load background texture");
    }

    g_table_texture = a_LoadTexture("resources/textures/blackjack_table.png");
    if (!g_table_texture) {
        d_LogError("Failed to load blackjack table texture");
    }

    // Create sidebar buttons FIRST (needed for LeftSidebar)
    const char* sidebar_labels[] = {"Draw Pile", "Discard Pile"};
    const char* sidebar_hotkeys[] = {"[V]", "[C]"};
    for (int i = 0; i < NUM_DECK_BUTTONS; i++) {
        sidebar_buttons[i] = CreateSidebarButton(0, 0, sidebar_labels[i], sidebar_hotkeys[i]);
    }

    // Create left sidebar section with sidebar buttons and player reference
    g_left_sidebar = CreateLeftSidebarSection(sidebar_buttons, &g_test_deck, g_human_player);

    // Create player section (NO deck buttons anymore)
    g_player_section = CreatePlayerSection(NULL, NULL);

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
    if (g_left_sidebar) {
        DestroyLeftSidebarSection(&g_left_sidebar);
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
        if (sidebar_buttons[i]) {
            DestroySidebarButton(&sidebar_buttons[i]);
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

    // Initialize tween manager
    InitTweenManager(&g_tween_manager);

    // Initialize card transition manager
    InitCardTransitionManager(&g_card_transition_manager);

    // Initialize damage numbers pool
    memset(g_damage_numbers, 0, sizeof(g_damage_numbers));
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        g_damage_numbers[i].active = false;
    }

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

    // Create first enemy: The Didact (basic enemy: ~3 small hands or 1 blackjack + small hand)
    Enemy_t* enemy = CreateEnemy("The Didact", 300, 5);
    if (enemy) {
        // Load enemy portrait
        if (!LoadEnemyPortrait(enemy, "resources/enemies/didact.png")) {
            d_LogError("Failed to load The Didact portrait");
        }

        g_game.current_enemy = enemy;
        g_game.is_combat_mode = true;
        d_LogInfo("Combat initialized: The Didact (300 HP, 5 chip threat)");
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

    // Cleanup terminal
    if (g_terminal) {
        CleanupTerminal(&g_terminal);
    }

    // Cleanup background textures
    if (g_background_texture) {
        SDL_DestroyTexture(g_background_texture);
        g_background_texture = NULL;
    }

    if (g_table_texture) {
        SDL_DestroyTexture(g_table_texture);
        g_table_texture = NULL;
    }

    // Cleanup combat enemy
    if (g_game.current_enemy) {
        DestroyEnemy(&g_game.current_enemy);
        g_game.is_combat_mode = false;
    }

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

    // Update tween manager (smooth HP bar drain, damage numbers, etc.)
    UpdateTweens(&g_tween_manager, dt);

    // Update card transitions (must happen AFTER UpdateTweens for flip timing)
    UpdateCardTransitions(&g_card_transition_manager, dt);

    // Ctrl + ` to toggle terminal (HIGHEST PRIORITY)
    if ((app.keyboard[SDL_SCANCODE_LCTRL] || app.keyboard[SDL_SCANCODE_RCTRL]) &&
        app.keyboard[SDL_SCANCODE_GRAVE]) {
        app.keyboard[SDL_SCANCODE_GRAVE] = 0;
        if (g_terminal) {
            ToggleTerminal(g_terminal);
        }
    }

    // Update terminal (cursor blink)
    if (g_terminal) {
        UpdateTerminal(g_terminal, dt);
    }

    // If terminal is visible, handle terminal input and block all game input
    if (g_terminal && IsTerminalVisible(g_terminal)) {
        HandleTerminalInput(g_terminal);
        return;
    }

    // Update top bar showcase (combat text rotation)
    if (g_top_bar && g_human_player) {
        UpdateTopBarShowcase(g_top_bar, dt, g_human_player, g_game.current_enemy);
    }

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

    // Sidebar buttons - always available (except during tutorial steps 1 and 2)
    if (!tutorial_blocking_input) {
        // Draw Pile button (V key) - Toggle behavior
        if (sidebar_buttons[0] && IsSidebarButtonClicked(sidebar_buttons[0])) {
            if (IsDrawPileModalVisible(g_draw_pile_modal)) {
                HideDrawPileModal(g_draw_pile_modal);
            } else {
                HideDiscardPileModal(g_discard_pile_modal);  // Close other modal
                ShowDrawPileModal(g_draw_pile_modal);
            }
        }
        if (app.keyboard[SDL_SCANCODE_V]) {
            app.keyboard[SDL_SCANCODE_V] = 0;
            if (IsDrawPileModalVisible(g_draw_pile_modal)) {
                HideDrawPileModal(g_draw_pile_modal);
            } else {
                HideDiscardPileModal(g_discard_pile_modal);  // Close other modal
                ShowDrawPileModal(g_draw_pile_modal);
            }
        }

        // Discard Pile button (C key) - Toggle behavior
        if (sidebar_buttons[1] && IsSidebarButtonClicked(sidebar_buttons[1])) {
            if (IsDiscardPileModalVisible(g_discard_pile_modal)) {
                HideDiscardPileModal(g_discard_pile_modal);
            } else {
                HideDrawPileModal(g_draw_pile_modal);  // Close other modal
                ShowDiscardPileModal(g_discard_pile_modal);
            }
        }
        if (app.keyboard[SDL_SCANCODE_C]) {
            app.keyboard[SDL_SCANCODE_C] = 0;
            if (IsDiscardPileModalVisible(g_discard_pile_modal)) {
                HideDiscardPileModal(g_discard_pile_modal);
            } else {
                HideDrawPileModal(g_draw_pile_modal);  // Close other modal
                ShowDiscardPileModal(g_discard_pile_modal);
            }
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

    // Enable/disable buttons based on available chips + update selection state
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i]) continue;
        SetButtonEnabled(bet_buttons[i], CanAffordBet(g_human_player, bet_amounts[i]));
        SetButtonSelected(bet_buttons[i], (i == selected_bet_button));
    }

    // Mouse hover auto-select (like menu navigation)
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i] || !bet_buttons[i]->enabled) continue;
        if (IsButtonHovered(bet_buttons[i])) {
            if (i != selected_bet_button) {
                selected_bet_button = i;
            }
            break;  // Only one button can be hovered
        }
    }

    // LEFT arrow - Move selection left
    if (app.keyboard[SDL_SCANCODE_LEFT]) {
        app.keyboard[SDL_SCANCODE_LEFT] = 0;

        // Find previous enabled button (wrap around)
        int original = selected_bet_button;
        do {
            selected_bet_button--;
            if (selected_bet_button < 0) selected_bet_button = NUM_BET_BUTTONS - 1;
            if (bet_buttons[selected_bet_button] && bet_buttons[selected_bet_button]->enabled) break;
            if (selected_bet_button == original) break;  // All disabled
        } while (selected_bet_button != original);
    }

    // RIGHT arrow - Move selection right
    if (app.keyboard[SDL_SCANCODE_RIGHT]) {
        app.keyboard[SDL_SCANCODE_RIGHT] = 0;

        // Find next enabled button (wrap around)
        int original = selected_bet_button;
        do {
            selected_bet_button++;
            if (selected_bet_button >= NUM_BET_BUTTONS) selected_bet_button = 0;
            if (bet_buttons[selected_bet_button] && bet_buttons[selected_bet_button]->enabled) break;
            if (selected_bet_button == original) break;  // All disabled
        } while (selected_bet_button != original);
    }

    // ENTER - Select currently highlighted button
    if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

        if (bet_buttons[selected_bet_button] && bet_buttons[selected_bet_button]->enabled) {
            ProcessBettingInput(&g_game, g_human_player, bet_amounts[selected_bet_button]);
        }
    }

    // Mouse input - check each betting button
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i]) continue;

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

    // Update selection state for all buttons
    for (int i = 0; i < NUM_ACTION_BUTTONS; i++) {
        if (action_buttons[i]) {
            SetButtonSelected(action_buttons[i], (i == selected_action_button));
        }
    }

    // Mouse hover auto-select (like menu navigation)
    for (int i = 0; i < NUM_ACTION_BUTTONS; i++) {
        if (!action_buttons[i] || !action_buttons[i]->enabled) continue;
        if (IsButtonHovered(action_buttons[i])) {
            if (i != selected_action_button) {
                selected_action_button = i;
            }
            break;  // Only one button can be hovered
        }
    }

    // LEFT arrow - Move selection left
    if (app.keyboard[SDL_SCANCODE_LEFT]) {
        app.keyboard[SDL_SCANCODE_LEFT] = 0;

        // Find previous enabled button (wrap around)
        int original = selected_action_button;
        do {
            selected_action_button--;
            if (selected_action_button < 0) selected_action_button = NUM_ACTION_BUTTONS - 1;
            if (action_buttons[selected_action_button] && action_buttons[selected_action_button]->enabled) break;
            if (selected_action_button == original) break;  // All disabled
        } while (selected_action_button != original);
    }

    // RIGHT arrow - Move selection right
    if (app.keyboard[SDL_SCANCODE_RIGHT]) {
        app.keyboard[SDL_SCANCODE_RIGHT] = 0;

        // Find next enabled button (wrap around)
        int original = selected_action_button;
        do {
            selected_action_button++;
            if (selected_action_button >= NUM_ACTION_BUTTONS) selected_action_button = 0;
            if (action_buttons[selected_action_button] && action_buttons[selected_action_button]->enabled) break;
            if (selected_action_button == original) break;  // All disabled
        } while (selected_action_button != original);
    }

    // Track if an action was taken (for tutorial step 2)
    bool action_taken = false;

    // ENTER - Select currently highlighted button
    if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

        if (action_buttons[selected_action_button] && action_buttons[selected_action_button]->enabled) {
            PlayerAction_t actions[] = {ACTION_HIT, ACTION_STAND, ACTION_DOUBLE};
            ProcessPlayerTurnInput(&g_game, g_human_player, actions[selected_action_button]);
            action_taken = true;
        }
    }

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
// TWEEN HELPERS (Public API for game.c)
// ============================================================================

void TweenEnemyHP(Enemy_t* enemy) {
    if (!enemy) return;

    // Tween display_hp to current_hp over 0.6 seconds with ease-out cubic
    // This creates a smooth HP bar drain effect
    TweenFloat(&g_tween_manager, &enemy->display_hp, (float)enemy->current_hp,
               0.6f, TWEEN_EASE_OUT_CUBIC);
}

void TweenPlayerHP(Player_t* player) {
    if (!player) return;

    // Tween display_chips to chips over 0.6 seconds with ease-out cubic
    // This creates a smooth HP bar drain effect (chips = HP)
    TweenFloat(&g_tween_manager, &player->display_chips, (float)player->chips,
               0.6f, TWEEN_EASE_OUT_CUBIC);
}

CardTransitionManager_t* GetCardTransitionManager(void) {
    return &g_card_transition_manager;
}

TweenManager_t* GetTweenManager(void) {
    return &g_tween_manager;
}

// ============================================================================
// DAMAGE NUMBERS (Public API for game.c)
// ============================================================================

static void OnDamageNumberComplete(void* user_data) {
    // Mark damage number slot as free
    DamageNumber_t* dmg = (DamageNumber_t*)user_data;
    if (dmg) {
        dmg->active = false;
    }
}

void SpawnDamageNumber(int damage, float world_x, float world_y, bool is_healing) {
    // Find free slot
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        if (!g_damage_numbers[i].active) {
            DamageNumber_t* dmg = &g_damage_numbers[i];

            // Initialize damage number
            dmg->active = true;
            dmg->x = world_x;
            dmg->y = world_y;
            dmg->alpha = 1.0f;
            dmg->damage = damage;
            dmg->is_healing = is_healing;

            // Tween Y position (rise 50px over 1.0s)
            TweenFloat(&g_tween_manager, &dmg->y, world_y - 50.0f,
                       1.0f, TWEEN_EASE_OUT_CUBIC);

            // Tween alpha (fade out over 1.0s)
            TweenFloatWithCallback(&g_tween_manager, &dmg->alpha, 0.0f,
                                   1.0f, TWEEN_LINEAR,
                                   OnDamageNumberComplete, dmg);

            return;  // Spawned successfully
        }
    }

    // Pool full - oldest damage number will be overwritten next frame
}

// ============================================================================
// SCENE RENDERING
// ============================================================================

static void BlackjackDraw(float dt) {
    (void)dt;

    // Draw full-screen background texture first
    if (g_background_texture) {
        a_BlitTextureScaled(g_background_texture, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    } else {
        // Fallback to black if texture didn't load
        app.background = (aColor_t){0, 0, 0, 255};
    }

    // Draw enemy portrait as background (if in combat)
    if (g_game.is_combat_mode && g_game.current_enemy) {
        SDL_Texture* enemy_portrait = GetEnemyPortraitTexture(g_game.current_enemy);
        if (enemy_portrait) {
            // Query portrait texture size
            int portrait_w, portrait_h;
            SDL_QueryTexture(enemy_portrait, NULL, NULL, &portrait_w, &portrait_h);

            // Calculate actual rendered size after scaling
            int scaled_width = (int)(portrait_w * ENEMY_PORTRAIT_SCALE);
            int scaled_height = (int)(portrait_h * ENEMY_PORTRAIT_SCALE);

            // Center the portrait in game area, then apply offset
            int centered_x = GAME_AREA_X + (GAME_AREA_WIDTH - scaled_width) / 2;
            int portrait_x = centered_x + ENEMY_PORTRAIT_X_OFFSET;

            // Get shake offset for damage feedback
            float shake_x, shake_y;
            GetEnemyShakeOffset(g_game.current_enemy, &shake_x, &shake_y);

            // Get red flash alpha for damage feedback
            float red_alpha = GetEnemyRedFlashAlpha(g_game.current_enemy);

            // Apply red tint to texture pixels (not transparent areas)
            if (red_alpha > 0.0f) {
                // Blend white → red based on alpha
                // red_alpha = 0.0 → (255, 255, 255) white (no tint)
                // red_alpha = 1.0 → (255, 0, 0) pure red
                Uint8 g_channel = (Uint8)(255 * (1.0f - red_alpha));
                Uint8 b_channel = (Uint8)(255 * (1.0f - red_alpha));
                SDL_SetTextureColorMod(enemy_portrait, 255, g_channel, b_channel);
            }

            // Use a_BlitTextureScaled to avoid int truncation of scale parameter
            // (a_BlitTextureRect takes int scale, which truncates 0.6f to 0)
            a_BlitTextureScaled(enemy_portrait,
                                portrait_x + (int)shake_x, ENEMY_PORTRAIT_Y_OFFSET + (int)shake_y,
                                scaled_width, scaled_height);

            // Reset texture color mod after rendering
            if (red_alpha > 0.0f) {
                SDL_SetTextureColorMod(enemy_portrait, 255, 255, 255);
            }
        };
    };

    // Draw blackjack table sprite at bottom (on top of portrait)
    if (g_table_texture) {
        int table_y = SCREEN_HEIGHT - 256;  // Bottom 256px of screen
        SDL_Rect src = {0, 0, 1024, 256};    // Full source sprite (512×256)

        // Center in game area (right of sidebar)
        // Game area is 232px wide (512 - 280 sidebar), scale image to fit
        float scale = (float)GAME_AREA_WIDTH / 512.0f;  // Scale to fit game area width

        a_BlitTextureRect(g_table_texture, src,
                          GAME_AREA_X - 32, table_y,  // X=280 (start of game area)
                          scale,                 // Scale to fit game area
                          (aColor_t){255, 255, 255, 255});  // No tint
    }

    // Render top bar at fixed position (independent of FlexBox)
    RenderTopBarSection(g_top_bar, &g_game, 0);

    // Render left sidebar (fixed position on left side)
    if (g_left_sidebar && g_human_player) {
        int sidebar_height = SCREEN_HEIGHT - LAYOUT_TOP_MARGIN - LAYOUT_BOTTOM_CLEARANCE;
        RenderLeftSidebarSection(g_left_sidebar, g_human_player, 0, LAYOUT_TOP_MARGIN, sidebar_height);
    }

    // Get FlexBox-calculated positions and render all game area sections
    if (g_main_layout) {
        a_FlexLayout(g_main_layout);

        // Render 3 sections using FlexBox positions (no title)
        int dealer_y = a_FlexGetItemY(g_main_layout, 0);  // Dealer now index 0
        int button_y = a_FlexGetItemY(g_main_layout, 1);  // Buttons now index 1
        int player_y = a_FlexGetItemY(g_main_layout, 2);  // Player now index 2

        // Pass current enemy to dealer section (NULL if not in combat)
        Enemy_t* current_enemy = g_game.is_combat_mode ? g_game.current_enemy : NULL;
        RenderDealerSection(g_dealer_section, g_dealer, current_enemy, dealer_y);
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

    // Render combat victory overlay (if in victory state)
    if (g_game.current_state == STATE_COMBAT_VICTORY && g_game.current_enemy) {
        // Dark overlay
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 200);

        // "CONGRATS!" text (centered, large, gold)
        int center_x = SCREEN_WIDTH / 2;
        int center_y = SCREEN_HEIGHT / 2 - 50;
        aFontConfig_t gold_title = {
            .type = FONT_ENTER_COMMAND,
            .color = {232, 193, 112, 255},  // Gold
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.0f
        };
        a_DrawTextStyled("CONGRATS!", center_x, center_y, &gold_title);

        // Victory message
        dString_t* victory_msg = d_StringInit();
        d_StringFormat(victory_msg, "You defeated %s!",
                       GetEnemyName(g_game.current_enemy));
        aFontConfig_t off_white = {
            .type = FONT_ENTER_COMMAND,
            .color = {235, 237, 233, 255},  // Off-white
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(victory_msg), center_x, center_y + 40, &off_white);
        d_StringDestroy(victory_msg);
    }

    // Render tutorial if active (absolute top layer)
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        RenderTutorial(g_tutorial_system);
    }

    // Render damage numbers (floating combat text)
    for (int i = 0; i < MAX_DAMAGE_NUMBERS; i++) {
        DamageNumber_t* dmg = &g_damage_numbers[i];
        if (!dmg->active) continue;

        // Choose color based on type (red for damage, green for healing)
        aColor_t color;
        if (dmg->is_healing) {
            color = (aColor_t){117, 167, 67, (int)(dmg->alpha * 255)};  // Green with alpha
        } else {
            color = (aColor_t){165, 48, 48, (int)(dmg->alpha * 255)};   // Red with alpha
        }

        // Format damage text
        dString_t* dmg_text = d_StringInit();
        if (dmg->is_healing) {
            d_StringFormat(dmg_text, "+%d", dmg->damage);
        } else {
            d_StringFormat(dmg_text, "-%d", dmg->damage);
        }

        // Render at current position with alpha
        aFontConfig_t dmg_config = {
            .type = FONT_ENTER_COMMAND,
            .color = color,
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(dmg_text), (int)dmg->x, (int)dmg->y, &dmg_config);

        d_StringDestroy(dmg_text);
    }

    // Render terminal overlay (highest layer - above tutorial)
    if (g_terminal) {
        RenderTerminal(g_terminal);
    }

    // FPS
    dString_t* fps_str = d_StringInit();
    d_StringFormat(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    aFontConfig_t fps_config = {
        .type = FONT_ENTER_COMMAND,
        .color = {0, 255, 255, 255},  // Cyan
        .align = TEXT_ALIGN_RIGHT,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)d_StringPeek(fps_str), SCREEN_WIDTH - 10, 10, &fps_config);
    d_StringDestroy(fps_str);
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
    aFontConfig_t result_config = {
        .type = FONT_ENTER_COMMAND,
        .color = msg_color,
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)message, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40, &result_config);

    // Chips change info
    dString_t* chip_info = d_StringInit();
    d_StringFormat(chip_info, "Chips: %d", GetPlayerChips(g_human_player));
    a_DrawTextStyled((char*)d_StringPeek(chip_info), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 20, &FONT_STYLE_TITLE);
    d_StringDestroy(chip_info);

    // Next round prompt
    aFontConfig_t gray_text = {
        .type = FONT_ENTER_COMMAND,
        .color = {200, 200, 200, 255},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawTextStyled("Next round starting...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 60, &gray_text);
}
