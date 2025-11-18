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
#include "../../include/state.h"
#include "../../include/scenes/sceneBlackjack.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/scenes/components/button.h"
#include "../../include/scenes/components/deckButton.h"
#include "../../include/scenes/components/sidebarButton.h"
#include "../../include/scenes/components/rewardModal.h"
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
#include "../../include/random.h"
#include "../../include/event.h"
#include "../../include/trinket.h"
#include "../../include/stats.h"
#include <math.h>

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
static ActionPanel_t* g_targeting_panel = NULL;
static Terminal_t* g_terminal = NULL;

// Background textures
static SDL_Texture* g_background_texture = NULL;
static SDL_Texture* g_table_texture = NULL;

// Modal sections
static DrawPileModalSection_t* g_draw_pile_modal = NULL;
static DiscardPileModalSection_t* g_discard_pile_modal = NULL;
static RewardModal_t* g_reward_modal = NULL;

// Current event encounter
static EventEncounter_t* g_current_event = NULL;

// Trinket hover state
static int g_hovered_trinket_slot = -1;  // -1 = none, 0-5 = slot index
static bool g_hovered_class_trinket = false;  // Class trinket hover state

// Popup notification state (for invalid target feedback)
static char g_popup_message[128] = {0};
static float g_popup_alpha = 0.0f;
static float g_popup_y = 0.0f;
static bool g_popup_active = false;

// Cached cursors (created once, reused)
static SDL_Cursor* g_cursor_arrow = NULL;
static SDL_Cursor* g_cursor_hand = NULL;

// Reusable Button components
static Button_t* bet_buttons[NUM_BET_BUTTONS] = {NULL};
static Button_t* action_buttons[NUM_ACTION_BUTTONS] = {NULL};
static SidebarButton_t* sidebar_buttons[NUM_DECK_BUTTONS] = {NULL};  // [0]=Draw Pile, [1]=Discard Pile

// Keyboard navigation state
static int selected_bet_button = 0;   // 0=Bet 10, 1=Bet 50, 2=Bet 100
static int selected_action_button = 0;  // 0=Hit, 1=Stand, 2=Double
static int key_held_bet_index = -1;   // -1=none, 0-2=which bet key is held (1/2/3)
static int key_held_action_index = -1; // -1=none, 0-2=which action key is held (1/2/3)

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

// ============================================================================
// RESULT SCREEN ANIMATIONS
// ============================================================================

static float g_result_display_chips = 0.0f;    // Tweened chip counter (slot machine effect)
static float g_result_screen_timer = 0.0f;     // Delay timer
static int g_result_old_chips = 0;             // Chips before round (for slot animation)
static int g_result_chip_delta = 0;            // Win/loss amount (bet outcome)
static int g_result_status_drain = 0;          // Status effect chip drain (CHIP_DRAIN only)

// Win/loss animation (immediate, 0-0.5s)
static bool g_result_winloss_started = false;
static float g_result_winloss_alpha = 0.0f;
static float g_result_winloss_offset_x = 0.0f;
static float g_result_winloss_offset_y = 0.0f;

// Status drain animation (delayed, 0.5s+)
static bool g_result_bleed_started = false;
static float g_result_bleed_alpha = 0.0f;
static float g_result_bleed_offset_x = 0.0f;
static float g_result_bleed_offset_y = 0.0f;

// Forward declarations
static void BlackjackLogic(float dt);
static void BlackjackDraw(float dt);

// ============================================================================
// TOKEN BLEED TRACKING (Called from statusEffects.c)
// ============================================================================

void SetStatusEffectDrainAmount(int drain_amount) {
    g_result_status_drain = drain_amount;
    d_LogInfoF("💰 STATUS DRAIN TRACKED: %d chips", drain_amount);
}

void TriggerSidebarBetAnimation(int bet_amount) {
    if (g_left_sidebar) {
        ShowSidebarBetDamage(g_left_sidebar, bet_amount);
    }
}
static void CleanupBlackjackScene(void);

// Layout initialization
static void InitializeLayout(void);
static void CleanupLayout(void);

// Input handlers
static void HandleBettingInput(void);
static void HandlePlayerTurnInput(void);
static void HandleTrinketInput(void);
static void HandleTargetingInput(void);

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

    // Load background textures (only on first initialization - they persist across scenes)
    if (!g_background_texture) {
        g_background_texture = a_LoadTexture("resources/textures/background1.png");
        if (!g_background_texture) {
            d_LogError("Failed to load background texture");
        }
    }

    if (!g_table_texture) {
        g_table_texture = a_LoadTexture("resources/textures/blackjack_table.png");
        if (!g_table_texture) {
            d_LogError("Failed to load table texture");
        }
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
    const char* action_hotkeys[] = {"[1]", "[2]", "[3]"};
    for (int i = 0; i < NUM_ACTION_BUTTONS; i++) {
        action_buttons[i] = CreateButton(0, 0, ACTION_BUTTON_WIDTH, ACTION_BUTTON_HEIGHT, action_labels[i]);
        SetButtonHotkey(action_buttons[i], action_hotkeys[i]);
    }

    // Create action panel section
    g_action_panel = CreateActionPanel("Your Turn - Choose Action:", action_buttons, NUM_ACTION_BUTTONS);

    // Create targeting panel section (no buttons, just instruction text)
    g_targeting_panel = CreateActionPanel("Right-click to exit targeting mode", NULL, 0);

    // Create deck/discard pile modals
    g_draw_pile_modal = CreateDrawPileModalSection(&g_test_deck);
    g_discard_pile_modal = CreateDiscardPileModalSection(&g_test_deck);

    // Create reward modal
    g_reward_modal = CreateRewardModal();

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
    if (g_targeting_panel) {
        DestroyActionPanel(&g_targeting_panel);
    }

    // Destroy modals
    if (g_draw_pile_modal) {
        DestroyDrawPileModalSection(&g_draw_pile_modal);
    }
    if (g_discard_pile_modal) {
        DestroyDiscardPileModalSection(&g_discard_pile_modal);
    }
    if (g_reward_modal) {
        DestroyRewardModal(&g_reward_modal);
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

    // Reset stats for new tutorial run
    Stats_Reset();

    // Set scene delegates
    app.delegate.logic = BlackjackLogic;
    app.delegate.draw = BlackjackDraw;

    // Initialize cursors (create once, reuse)
    if (!g_cursor_arrow) {
        g_cursor_arrow = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    }
    if (!g_cursor_hand) {
        g_cursor_hand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
    }

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
    g_dealer = Game_GetPlayerByID(0);

    // Create human player (player_id = 1)
    if (!CreatePlayer("Player", 1, false)) {
        d_LogFatal("Failed to create human player");
        return;
    }
    g_human_player = Game_GetPlayerByID(1);

    // Set player class to Degenerate (starter class)
    if (g_human_player) {
        g_human_player->class = PLAYER_CLASS_DEGENERATE;

        // Auto-equip Degenerate's Gambit as CLASS trinket
        Trinket_t* class_trinket = GetTrinketByID(0);  // ID 0 = Degenerate's Gambit
        if (class_trinket) {
            if (EquipClassTrinket(g_human_player, class_trinket)) {
                d_LogInfo("Auto-equipped Degenerate's Gambit as CLASS trinket");
            }
        }
    }

    // Initialize game context
    State_InitContext(&g_game, &g_test_deck);

    // Add players to game (dealer first, then player)
    Game_AddPlayerToGame(&g_game, 0);  // Dealer
    Game_AddPlayerToGame(&g_game, 1);  // Player

    // Initialize FlexBox layout (automatic positioning!)
    InitializeLayout();

    // Initialize tutorial system
    g_tutorial_system = CreateTutorialSystem();
    g_tutorial_steps = CreateBlackjackTutorial();
    if (g_tutorial_system && g_tutorial_steps) {
        StartTutorial(g_tutorial_system, g_tutorial_steps);
        d_LogInfo("Tutorial started");
    }

    // Create first enemy: The Didact (tutorial enemy with 3 teaching abilities)
    Enemy_t* enemy = CreateTheDidact();
    if (enemy) {
        // Load enemy portrait
        if (!LoadEnemyPortrait(enemy, "resources/enemies/didact.png")) {
            d_LogError("Failed to load The Didact portrait");
        }

        g_game.current_enemy = enemy;
        g_game.is_combat_mode = true;

        // Fire ROUND_START event on combat initialization
        Game_TriggerEvent(&g_game, GAME_EVENT_COMBAT_START);

        d_LogInfo("Combat initialized: The Didact (100 HP, 10 chip threat, 3 abilities)");
    }

    // Start in betting state
    State_Transition(&g_game, STATE_BETTING);

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

    // NOTE: Background/table textures are NOT destroyed here - they persist across scene changes
    // Archimedes' a_LoadTexture() has internal caching that reuses texture pointers
    // Destroying them causes use-after-free when the cached pointer is returned on reload
    // These textures are destroyed in main.c Cleanup() when the application exits

    // Cleanup combat enemy
    if (g_game.current_enemy) {
        DestroyEnemy(&g_game.current_enemy);
        g_game.is_combat_mode = false;
    }

    // Cleanup game context (destroys internal tables/arrays)
    State_CleanupContext(&g_game);

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
// EVENT GENERATION
// ============================================================================

static EventEncounter_t* CreatePostCombatEvent(void) {
    // Simple post-combat event with 2 choices
    EventEncounter_t* event = CreateEvent(
        "Victory Spoils",
        "You search through the defeated enemy's belongings. "
        "Among scattered cards and chips, you find something interesting...",
        EVENT_TYPE_CHOICE
    );

    if (!event) {
        d_LogError("Failed to create post-combat event");
        return NULL;
    }

    // Choice 1: Take chips (safe)
    AddEventChoice(event,
                   "Take the chips (+15 chips)",
                   "You pocket the chips and move on.",
                   15,  // +15 chips
                   0);  // No sanity change

    // Choice 2: Rest and recover (risky sanity cost for chip bonus)
    AddEventChoice(event,
                   "Rest at the table (+25 chips, risky)",
                   "You take a moment to rest, but something feels... off. Still, the chips are worth it.",
                   25,  // +25 chips
                   0);  // No sanity change (for now)

    d_LogInfo("Created post-combat event with 2 choices");
    return event;
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

    // Update popup notification (fade out and float up)
    if (g_popup_active) {
        g_popup_alpha -= dt * 1.5f;  // Fade over ~0.67 seconds
        g_popup_y -= dt * 50.0f;     // Float upward at 50px/sec

        if (g_popup_alpha <= 0.0f) {
            g_popup_active = false;
        }
    }

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

    // Notify tutorial system of game state changes (for waiting to advance)
    if (g_tutorial_system) {
        CheckTutorialGameState(g_tutorial_system, g_game.current_state);
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

        // Check for Betting Power (chips) hover event (for tutorial step 3)
        if (g_left_sidebar && UpdateLeftSidebarChipsHover(g_left_sidebar, 0, dt)) {
            static int chips_hover_id = 3;
            TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_HOVER, (void*)(intptr_t)chips_hover_id);
            d_LogInfo("Tutorial: Betting Power (chips) hover triggered after 1 second");
        }

        // Check for Active Bet hover event (for tutorial step 4)
        if (g_left_sidebar && UpdateLeftSidebarHoverTracking(g_left_sidebar, 0, dt)) {
            static int bet_hover_id = 1;
            TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_HOVER, (void*)(intptr_t)bet_hover_id);
            d_LogInfo("Tutorial: Active Bet hover triggered after 1 second");
        }

        // Check for ability hover event (for tutorial step 5)
        if (g_dealer_section && g_game.current_enemy &&
            UpdateDealerAbilityHoverTracking(g_dealer_section, g_game.current_enemy, dt)) {
            static int ability_hover_id = 2;
            TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_HOVER, (void*)(intptr_t)ability_hover_id);
            d_LogInfo("Tutorial: All abilities hovered");
        }

        // Note: Don't return - allow game input to continue
    }

    // Check if tutorial is active and blocking input
    // Block input ONLY during steps 3, 4, 5 (hover tutorial steps)
    // Steps 1 and 2 allow normal gameplay (betting and player actions)
    bool tutorial_blocking_input = (g_tutorial_system && IsTutorialActive(g_tutorial_system) &&
                                     g_tutorial_system->current_step &&
                                     g_tutorial_system->current_step_number >= 3 &&
                                     g_tutorial_system->current_step_number <= 5);

    // Debug: Log when blocking changes
    static bool last_blocking_state = false;
    if (tutorial_blocking_input != last_blocking_state) {
        d_LogInfoF("Tutorial input blocking: %s (step %d)",
                   tutorial_blocking_input ? "BLOCKED" : "ALLOWED",
                   g_tutorial_system ? g_tutorial_system->current_step_number : 0);
        last_blocking_state = tutorial_blocking_input;
    }

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
    State_UpdateLogic(&g_game, dt);

    // Capture chips before round (for slot machine animation)
    if (previous_state != STATE_DEALER_TURN && g_game.current_state == STATE_DEALER_TURN) {
        g_result_old_chips = GetPlayerChips(g_human_player);
        g_result_status_drain = 0;  // Reset for this round
        d_LogInfoF("💰 ENTERING DEALER_TURN: Captured chips=%d, reset status drain", g_result_old_chips);
    }

    // Initialize result screen animations when entering ROUND_END or COMBAT_VICTORY (AFTER payouts)
    if ((previous_state != STATE_ROUND_END && g_game.current_state == STATE_ROUND_END) ||
        (previous_state != STATE_COMBAT_VICTORY && g_game.current_state == STATE_COMBAT_VICTORY)) {
        int current_chips = GetPlayerChips(g_human_player);
        int total_delta = current_chips - g_result_old_chips;  // Total chip change

        // Separate bet outcome from status drain
        // total_delta = bet_outcome - status_drain
        // Therefore: bet_outcome = total_delta + status_drain
        g_result_chip_delta = total_delta + g_result_status_drain;  // Bet outcome only (win/loss)

        g_result_display_chips = (float)g_result_old_chips;  // Start slot animation from old value
        g_result_screen_timer = 0.0f;

        // Reset animation flags
        g_result_winloss_started = false;
        g_result_bleed_started = false;

        // Random offsets for both animations (high-quality RNG)
        g_result_winloss_offset_x = GetRandomFloat(-30.0f, 30.0f);
        g_result_winloss_offset_y = GetRandomFloat(-20.0f, 20.0f);
        g_result_bleed_offset_x = GetRandomFloat(-30.0f, 30.0f);
        g_result_bleed_offset_y = GetRandomFloat(-20.0f, 20.0f);

        const char* state_name = (g_game.current_state == STATE_ROUND_END) ? "ROUND_END" : "COMBAT_VICTORY";
        d_LogInfoF("🎬 ENTERING %s: total_delta=%d, bet_delta=%d, status_drain=%d",
                   state_name, total_delta, g_result_chip_delta, g_result_status_drain);
    }

    // Token bleed timer update (during result screen)
    if (g_game.current_state == STATE_ROUND_END) {
        g_result_screen_timer += dt;
    }

    // Reset result screen animations when exiting
    if (previous_state == STATE_ROUND_END && g_game.current_state != STATE_ROUND_END) {
        g_result_winloss_started = false;
        g_result_bleed_started = false;
        g_result_screen_timer = 0.0f;
        g_result_winloss_alpha = 0.0f;
        g_result_bleed_alpha = 0.0f;
        d_LogInfo("🎬 EXITING STATE_ROUND_END: Reset animation state");
    }

    // Show reward modal when entering STATE_REWARD_SCREEN
    if (previous_state != STATE_REWARD_SCREEN && g_game.current_state == STATE_REWARD_SCREEN) {
        if (g_reward_modal) {
            if (!ShowRewardModal(g_reward_modal)) {
                // No untagged cards - skip reward
                d_LogInfo("No untagged cards, skipping reward");
                Game_StartNewRound(&g_game);
                State_Transition(&g_game, STATE_BETTING);
            }
        }
    }

    // Handle reward modal input and check if it wants to close
    if (g_game.current_state == STATE_REWARD_SCREEN && g_reward_modal && IsRewardModalVisible(g_reward_modal)) {
        if (HandleRewardModalInput(g_reward_modal, dt)) {
            // Modal wants to close
            HideRewardModal(g_reward_modal);

            // After combat victory rewards, transition to event encounter
            if (g_game.is_combat_mode && g_game.current_enemy && g_game.current_enemy->is_defeated) {
                d_LogInfo("Combat victory complete - transitioning to post-combat event");
                State_Transition(&g_game, STATE_EVENT);
            } else {
                // Practice mode - start new round
                Game_StartNewRound(&g_game);
                State_Transition(&g_game, STATE_BETTING);
            }
        }
    }

    // Create event when entering STATE_EVENT
    if (previous_state != STATE_EVENT && g_game.current_state == STATE_EVENT) {
        // Clean up any previous event
        if (g_current_event) {
            DestroyEvent(&g_current_event);
        }

        // Create new post-combat event
        g_current_event = CreatePostCombatEvent();
        if (!g_current_event) {
            d_LogError("Failed to create event - skipping to next round");
            Game_StartNewRound(&g_game);
            State_Transition(&g_game, STATE_BETTING);
        }
    }

    // Handle event input (click choices)
    if (g_game.current_state == STATE_EVENT && g_current_event) {
        if (app.mouse.pressed) {

            int mx = app.mouse.x;
            int my = app.mouse.y;

            // Check if clicked on any choice button
            int panel_height = 500;
            int panel_y = (SCREEN_HEIGHT - panel_height) / 2;
            int button_width = 500;
            int button_height = 60;
            int button_spacing = 20;
            int choices_start_y = panel_y + 250;

            int choice_count = (int)g_current_event->choices->count;
            for (int i = 0; i < choice_count; i++) {
                int button_y = choices_start_y + i * (button_height + button_spacing);
                int button_x = (SCREEN_WIDTH - button_width) / 2;

                if (mx >= button_x && mx <= button_x + button_width &&
                    my >= button_y && my <= button_y + button_height) {
                    // Clicked this choice!
                    EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(g_current_event->choices, i);
                    if (choice) {
                        d_LogInfoF("Event choice selected: %s", d_StringPeek(choice->text));

                        // Apply outcome
                        if (choice->chips_delta != 0 && g_human_player) {
                            g_human_player->chips += choice->chips_delta;
                            d_LogInfoF("Chips changed by %d (now: %d)",
                                       choice->chips_delta, g_human_player->chips);
                        }

                        // Clean up event
                        DestroyEvent(&g_current_event);

                        // Start new round and return to betting
                        Game_StartNewRound(&g_game);
                        State_Transition(&g_game, STATE_BETTING);
                    }
                }
            }
        }
    }

    // Trigger tutorial events on state changes
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        if (g_game.current_state != previous_state) {
            TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_STATE_CHANGE,
                                (void*)(intptr_t)g_game.current_state);
        }
    }

    // Handle input based on current state (blocked during tutorial steps 3, 4, 5)
    if (!tutorial_blocking_input) {
        switch (g_game.current_state) {
            case STATE_BETTING:
                HandleBettingInput();
                break;

            case STATE_PLAYER_TURN:
                HandlePlayerTurnInput();
                break;

            case STATE_TARGETING:
                HandleTargetingInput();
                break;

            default:
                // Other states handled by state machine
                break;
        }
    }
}

// ============================================================================
// INPUT HANDLERS
// ============================================================================

// ============================================================================
// TRINKET INPUT HANDLING
// ============================================================================

/**
 * HandleTrinketInput - Check for trinket clicks to enter targeting mode
 *
 * Called during STATE_PLAYER_TURN. If player clicks on a trinket slot
 * with an off-cooldown active ability, enter STATE_TARGETING.
 */
static void HandleTrinketInput(void) {
    if (!g_human_player || !g_human_player->trinket_slots) return;

    // Only allow trinket use during player turn
    if (g_game.current_state != STATE_PLAYER_TURN) return;

    // Check for mouse click (using pressed flag like buttons)
    if (app.mouse.pressed) {
        // Check if clicked on class trinket
        if (g_hovered_class_trinket) {
            Trinket_t* trinket = GetClassTrinket(g_human_player);

            if (trinket && trinket->active_cooldown_current == 0 && trinket->active_target_type == TRINKET_TARGET_CARD) {
                // Trigger tutorial event (class trinket clicked)
                if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
                    TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_FUNCTION_CALL, (void*)0x1000);
                }

                // Enter targeting mode (use slot_index = -1 for class trinket)
                StateData_SetInt(&g_game.state_data, "targeting_trinket_slot", -1);  // -1 = class trinket
                StateData_SetInt(&g_game.state_data, "targeting_player_id", g_human_player->player_id);
                State_Transition(&g_game, STATE_TARGETING);

                d_LogInfoF("Entered targeting mode for CLASS trinket (player ID %d)",
                          g_human_player->player_id);
            }
        }
        // Check if clicked on regular trinket slot
        else if (g_hovered_trinket_slot >= 0) {
            // Defensive: Verify g_human_player exists
            if (!g_human_player) {
                d_LogError("HandleTrinketClick: g_human_player is NULL");
                return;
            }

            Trinket_t* trinket = GetEquippedTrinket(g_human_player, g_hovered_trinket_slot);

            if (trinket && trinket->active_cooldown_current == 0 && trinket->active_target_type == TRINKET_TARGET_CARD) {
                // Enter targeting mode (store state in int_values table)
                StateData_SetInt(&g_game.state_data, "targeting_trinket_slot", g_hovered_trinket_slot);
                StateData_SetInt(&g_game.state_data, "targeting_player_id", g_human_player->player_id);
                State_Transition(&g_game, STATE_TARGETING);

                d_LogInfoF("Entered targeting mode for trinket slot %d (player ID %d)",
                          g_hovered_trinket_slot, g_human_player->player_id);
            }
        }
    }
}

/**
 * IsCardValidTarget - Check if a card can be targeted by the active trinket
 *
 * @param card - Card to check
 * @param trinket_slot - Trinket slot index (-1 for class trinket, 0-5 for regular slots)
 * @return true if card can be targeted
 */
bool IsCardValidTarget(const Card_t* card, int trinket_slot) {
    if (!card || !g_human_player) return false;

    // Get trinket (either class trinket or regular slot)
    Trinket_t* trinket;
    if (trinket_slot == -1) {
        trinket = GetClassTrinket(g_human_player);
    } else {
        trinket = GetEquippedTrinket(g_human_player, trinket_slot);
    }

    if (!trinket) return false;

    // For Degenerate's Gambit: card rank 2-5 only (no Aces!)
    if (trinket->trinket_id == 0) {
        return card->rank >= RANK_TWO && card->rank <= RANK_FIVE;
    }

    // Unknown trinket or no targeting
    return false;
}

/**
 * HandleTargetingInput - Handle input during STATE_TARGETING
 *
 * Right-click or ESC cancels targeting, left-click selects target card.
 */
static void HandleTargetingInput(void) {
    // ESC = cancel targeting
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        d_LogInfo("Cancelled targeting mode (ESC)");
        State_Transition(&g_game, STATE_PLAYER_TURN);
        return;
    }

    // Right-click = cancel targeting (button is set on mouse UP event)
    if (app.mouse.button == SDL_BUTTON_RIGHT) {
        app.mouse.button = 0;  // Consume the event
        d_LogInfo("Cancelled targeting mode (Right-click)");
        State_Transition(&g_game, STATE_PLAYER_TURN);
        return;
    }

    // Left-click = confirm selection of hovered card (ADR-11)
    if (!app.mouse.pressed) return;

    int targeting_trinket_slot = StateData_GetInt(&g_game.state_data, "targeting_trinket_slot", -999);
    if (targeting_trinket_slot == -999) return;  // Invalid state

    // Get trinket (either class trinket or regular slot)
    Trinket_t* trinket;
    if (targeting_trinket_slot == -1) {
        trinket = GetClassTrinket(g_human_player);
    } else if (targeting_trinket_slot >= 0 && targeting_trinket_slot < 6) {
        trinket = GetEquippedTrinket(g_human_player, targeting_trinket_slot);
    } else {
        return;  // Invalid slot
    }

    if (!trinket || !trinket->active_effect) return;

    // ADR-11: Use hover state as single source of truth for card selection
    // Hover system already handles z-index priority, animations, and position calculation

    // Check player section hover state
    if (g_player_section && g_human_player) {
        int player_hovered = g_player_section->hover_state.hovered_card_index;
        if (player_hovered >= 0) {
            Hand_t* hand = &g_human_player->hand;
            const Card_t* card = GetCardFromHand(hand, (size_t)player_hovered);

            if (card && IsCardValidTarget(card, targeting_trinket_slot)) {
                d_LogInfoF("Selected player card (hovered): %s of %s",
                          GetRankString(card->rank), GetSuitString(card->suit));

                // Execute trinket active ability
                trinket->active_effect(g_human_player, &g_game, (void*)card, trinket, (size_t)targeting_trinket_slot);

                // Apply cooldown
                trinket->active_cooldown_current = trinket->active_cooldown_max;

                // Exit targeting mode
                State_Transition(&g_game, STATE_PLAYER_TURN);
                return;
            } else if (card) {
                d_LogWarning("Invalid target card for this trinket");

                // Show popup notification
                snprintf(g_popup_message, sizeof(g_popup_message), "This ability targets 5 or less");
                g_popup_alpha = 1.0f;
                g_popup_y = SCREEN_HEIGHT / 2;
                g_popup_active = true;

                return;
            }
        }
    }

    // Check dealer section hover state
    if (g_dealer_section && g_dealer) {
        int dealer_hovered = g_dealer_section->hover_state.hovered_card_index;
        if (dealer_hovered >= 0) {
            Hand_t* hand = &g_dealer->hand;
            const Card_t* card = GetCardFromHand(hand, (size_t)dealer_hovered);

            // Skip face-down cards (can't target hidden dealer card)
            if (card && card->face_up && IsCardValidTarget(card, targeting_trinket_slot)) {
                d_LogInfoF("Selected dealer card (hovered): %s of %s",
                          GetRankString(card->rank), GetSuitString(card->suit));

                // Execute trinket active ability
                trinket->active_effect(g_human_player, &g_game, (void*)card, trinket, (size_t)targeting_trinket_slot);

                // Apply cooldown
                trinket->active_cooldown_current = trinket->active_cooldown_max;

                // Exit targeting mode
                State_Transition(&g_game, STATE_PLAYER_TURN);
                return;
            } else if (card) {
                d_LogWarning("Invalid target card for this trinket");

                // Show popup notification
                snprintf(g_popup_message, sizeof(g_popup_message), "This ability targets 5 or less");
                g_popup_alpha = 1.0f;
                g_popup_y = SCREEN_HEIGHT / 2;
                g_popup_active = true;

                return;
            }
        }
    }

    // Click outside any hovered card = no-op (don't cancel targeting)
}

/**
 * RenderTargetingArrow - Draw arrow from trinket to mouse cursor
 */
static void RenderTargetingArrow(void) {
    if (g_game.current_state != STATE_TARGETING) return;

    int targeting_trinket_slot = StateData_GetInt(&g_game.state_data, "targeting_trinket_slot", -999);
    if (targeting_trinket_slot == -999) return;  // Invalid state

    // Get trinket icon position
    int trinket_center_x, trinket_center_y;

    if (targeting_trinket_slot == -1) {
        // Class trinket
        trinket_center_x = CLASS_TRINKET_X + CLASS_TRINKET_SIZE / 2;
        trinket_center_y = CLASS_TRINKET_Y + CLASS_TRINKET_SIZE / 2;
    } else if (targeting_trinket_slot >= 0 && targeting_trinket_slot < 6) {
        // Regular trinket slot
        int row = targeting_trinket_slot / 3;
        int col = targeting_trinket_slot % 3;
        int trinket_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int trinket_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        trinket_center_x = trinket_x + TRINKET_SLOT_SIZE / 2;
        trinket_center_y = trinket_y + TRINKET_SLOT_SIZE / 2;
    } else {
        return;  // Invalid slot
    }

    // Get mouse position
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);

    // Calculate angle from trinket to mouse
    float angle = atan2f((float)(mouse_y - trinket_center_y), (float)(mouse_x - trinket_center_x));
    float distance = sqrtf((float)((mouse_x - trinket_center_x) * (mouse_x - trinket_center_x) +
                                    (mouse_y - trinket_center_y) * (mouse_y - trinket_center_y)));

    // Draw thick curved line using multiple parallel lines
    const int line_thickness = 3;
    SDL_SetRenderDrawColor(app.renderer, 255, 220, 0, 255);  // Bright yellow

    for (int t = -line_thickness; t <= line_thickness; t++) {
        // Offset perpendicular to line direction for thickness
        int offset_x = (int)(t * sinf(angle));
        int offset_y = (int)(-t * cosf(angle));

        // Draw slightly curved line using 10 segments
        const int segments = 10;
        for (int i = 0; i < segments; i++) {
            float t1 = (float)i / segments;
            float t2 = (float)(i + 1) / segments;

            // Bezier curve control points (slight arc)
            int mid_x = (trinket_center_x + mouse_x) / 2;
            int mid_y = (trinket_center_y + mouse_y) / 2 - (int)(distance * 0.1f);  // Arc upward

            // Linear interpolation along curve
            int x1 = (int)(trinket_center_x * (1-t1) * (1-t1) + mid_x * 2 * (1-t1) * t1 + mouse_x * t1 * t1) + offset_x;
            int y1 = (int)(trinket_center_y * (1-t1) * (1-t1) + mid_y * 2 * (1-t1) * t1 + mouse_y * t1 * t1) + offset_y;
            int x2 = (int)(trinket_center_x * (1-t2) * (1-t2) + mid_x * 2 * (1-t2) * t2 + mouse_x * t2 * t2) + offset_x;
            int y2 = (int)(trinket_center_y * (1-t2) * (1-t2) + mid_y * 2 * (1-t2) * t2 + mouse_y * t2 * t2) + offset_y;

            SDL_RenderDrawLine(app.renderer, x1, y1, x2, y2);
        }
    }

    // Draw arrowhead at mouse position (pointing FORWARD from trinket)
    const int arrow_size = 18;
    const float arrow_angle = 0.5f;  // Angle of arrow wings

    // Arrow points forward along the line direction
    // Left wing (angle - arrow_angle)
    int left_x = mouse_x - (int)(arrow_size * cosf(angle - arrow_angle));
    int left_y = mouse_y - (int)(arrow_size * sinf(angle - arrow_angle));

    // Right wing (angle + arrow_angle)
    int right_x = mouse_x - (int)(arrow_size * cosf(angle + arrow_angle));
    int right_y = mouse_y - (int)(arrow_size * sinf(angle + arrow_angle));

    // Draw filled triangle arrowhead
    SDL_SetRenderDrawColor(app.renderer, 255, 220, 0, 255);
    SDL_RenderDrawLine(app.renderer, mouse_x, mouse_y, left_x, left_y);
    SDL_RenderDrawLine(app.renderer, mouse_x, mouse_y, right_x, right_y);
    SDL_RenderDrawLine(app.renderer, left_x, left_y, right_x, right_y);

    // Fill the triangle (simple scanline fill)
    for (int i = 0; i < arrow_size; i++) {
        float t = (float)i / arrow_size;
        int fill_x1 = mouse_x - (int)(i * cosf(angle - arrow_angle));
        int fill_y1 = mouse_y - (int)(i * sinf(angle - arrow_angle));
        int fill_x2 = mouse_x - (int)(i * cosf(angle + arrow_angle));
        int fill_y2 = mouse_y - (int)(i * sinf(angle + arrow_angle));
        SDL_RenderDrawLine(app.renderer, fill_x1, fill_y1, fill_x2, fill_y2);
    }
}

// ============================================================================
// BETTING INPUT HANDLING
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
            Game_ProcessBettingInput(&g_game, g_human_player, bet_amounts[selected_bet_button]);
        }
    }

    // DEBUG: Log when mouse is pressed to diagnose click issues
    if (app.mouse.pressed) {
    }

    // Mouse input - check each betting button
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i]) {
            continue;
        }

        // Process click - game.c validates and executes
        if (IsButtonClicked(bet_buttons[i])) {
            d_LogInfoF("✅ BET BUTTON %d CLICKED", i);
            Game_ProcessBettingInput(&g_game, g_human_player, bet_amounts[i]);
        }
    }

    // Keyboard shortcuts: 1/2/3 for bet amounts
    // Keyboard hotkeys (1, 2, 3) - hold to preview, release to confirm
    int prev_key_held = key_held_bet_index;

    // Check which key is currently held
    key_held_bet_index = -1;
    if (app.keyboard[SDL_SCANCODE_1] && bet_buttons[0] && bet_buttons[0]->enabled) key_held_bet_index = 0;
    else if (app.keyboard[SDL_SCANCODE_2] && bet_buttons[1] && bet_buttons[1]->enabled) key_held_bet_index = 1;
    else if (app.keyboard[SDL_SCANCODE_3] && bet_buttons[2] && bet_buttons[2]->enabled) key_held_bet_index = 2;

    // Update selected button based on key held (for preview effect)
    if (key_held_bet_index != -1) {
        selected_bet_button = key_held_bet_index;
    }

    // Trigger on key release (was held, now not held)
    if (prev_key_held != -1 && key_held_bet_index == -1) {
        int choice = prev_key_held;
        d_LogInfoF("⌨️ KEYBOARD %d released - betting %d", choice + 1, bet_amounts[choice]);
        Game_ProcessBettingInput(&g_game, g_human_player, bet_amounts[choice]);
    }
}

static void HandlePlayerTurnInput(void) {
    if (!g_human_player) return;

    // Check for trinket clicks FIRST (before button handling consumes mouse input)
    // HandleTrinketInput checks g_human_player internally, but we already checked above
    if (g_human_player->trinket_slots) {
        HandleTrinketInput();
    }

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
            Game_ProcessPlayerTurnInput(&g_game, g_human_player, actions[selected_action_button]);
            action_taken = true;
        }
    }

    // Mouse input - Hit button
    if (action_buttons[0] && IsButtonClicked(action_buttons[0])) {
        Game_ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_HIT);
        action_taken = true;
    }

    // Stand button
    if (action_buttons[1] && IsButtonClicked(action_buttons[1])) {
        Game_ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_STAND);
        action_taken = true;
    }

    // Double button
    if (action_buttons[2] && IsButtonClicked(action_buttons[2])) {
        Game_ProcessPlayerTurnInput(&g_game, g_human_player, ACTION_DOUBLE);
        action_taken = true;
    }

    // Keyboard hotkeys (1, 2, 3) - hold to preview, release to confirm
    int prev_key_held = key_held_action_index;

    // Check which key is currently held
    key_held_action_index = -1;
    if (app.keyboard[SDL_SCANCODE_1] && action_buttons[0] && action_buttons[0]->enabled) key_held_action_index = 0;
    else if (app.keyboard[SDL_SCANCODE_2] && action_buttons[1] && action_buttons[1]->enabled) key_held_action_index = 1;
    else if (app.keyboard[SDL_SCANCODE_3] && action_buttons[2] && action_buttons[2]->enabled) key_held_action_index = 2;

    // Update selected button based on key held (for preview effect)
    if (key_held_action_index != -1) {
        selected_action_button = key_held_action_index;
    }

    // Trigger on key release (was held, now not held)
    if (prev_key_held != -1 && key_held_action_index == -1) {
        int choice = prev_key_held;
        PlayerAction_t actions[] = {ACTION_HIT, ACTION_STAND, ACTION_DOUBLE};
        d_LogInfoF("⌨️ ACTION KEY %d released - performing action", choice + 1);
        Game_ProcessPlayerTurnInput(&g_game, g_human_player, actions[choice]);
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
// TRINKET TOOLTIP RENDERING
// ============================================================================

static void RenderTrinketTooltip(const Trinket_t* trinket, int slot_index) {
    if (!trinket) return;

    // Determine if this is a class trinket (slot_index == -1)
    bool is_class_trinket = (slot_index == -1);

    // Calculate tooltip position
    int slot_x, slot_y, slot_size;
    if (is_class_trinket) {
        slot_x = CLASS_TRINKET_X;
        slot_y = CLASS_TRINKET_Y;
        slot_size = CLASS_TRINKET_SIZE;
    } else {
        int row = slot_index / 3;
        int col = slot_index % 3;
        slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        slot_size = TRINKET_SLOT_SIZE;
    }

    int tooltip_width = 340;
    int tooltip_x = slot_x - tooltip_width - 10;  // Left of slot
    int tooltip_y = slot_y;

    // If too far left, show on right instead
    if (tooltip_x < 10) {
        tooltip_x = slot_x + slot_size + 10;
    }

    // Get trinket info
    const char* name = GetTrinketName(trinket);
    const char* description = GetTrinketDescription(trinket);
    const char* passive_desc = trinket->passive_description ? d_StringPeek(trinket->passive_description) : "No passive";
    const char* active_desc = trinket->active_description ? d_StringPeek(trinket->active_description) : "No active";

    int padding = 16;
    int content_width = tooltip_width - (padding * 2);

    // Measure text heights
    int name_height = a_GetWrappedTextHeight((char*)name, FONT_ENTER_COMMAND, content_width);
    int desc_height = a_GetWrappedTextHeight((char*)description, FONT_GAME, content_width);
    int passive_height = a_GetWrappedTextHeight((char*)passive_desc, FONT_GAME, content_width);
    int active_height = a_GetWrappedTextHeight((char*)active_desc, FONT_GAME, content_width);

    // Calculate tooltip height
    int tooltip_height = padding;

    // Add space for "CLASS TRINKET" header if applicable
    if (is_class_trinket) {
        tooltip_height += 20;  // CLASS TRINKET header
    }

    tooltip_height += name_height + 10;
    tooltip_height += desc_height + 12;
    tooltip_height += 1 + 12;  // Divider
    tooltip_height += passive_height + 8;
    tooltip_height += active_height + 8;

    // Show cooldown/status
    if (trinket->active_cooldown_current > 0) {
        tooltip_height += 20;
    }

    // Show damage counter (if any)
    if (trinket->total_damage_dealt > 0) {
        tooltip_height += 20;
    }

    tooltip_height += padding;

    // Clamp to screen
    if (tooltip_y + tooltip_height > SCREEN_HEIGHT - 10) {
        tooltip_y = SCREEN_HEIGHT - tooltip_height - 10;
    }
    if (tooltip_y < TOP_BAR_HEIGHT + 10) {
        tooltip_y = TOP_BAR_HEIGHT + 10;
    }

    // Background
    a_DrawFilledRect(tooltip_x, tooltip_y, tooltip_width, tooltip_height,
                    20, 20, 30, 230);
    a_DrawRect(tooltip_x, tooltip_y, tooltip_width, tooltip_height,
              255, 255, 255, 255);

    // Render content
    int content_x = tooltip_x + padding;
    int current_y = tooltip_y + padding;

    // Class trinket header (if applicable)
    if (is_class_trinket) {
        aFontConfig_t class_header_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {200, 180, 50, 255},  // Gold
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 0.6f
        };
        a_DrawTextStyled("CLASS TRINKET", content_x, current_y, &class_header_config);
        current_y += 20;
    }

    // Title
    aFontConfig_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .color = {232, 193, 112, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)name, content_x, current_y, &title_config);
    current_y += name_height + 10;

    // Description
    aFontConfig_t desc_config = {
        .type = FONT_GAME,
        .color = {180, 180, 180, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)description, content_x, current_y, &desc_config);
    current_y += desc_height + 12;

    // Divider
    a_DrawFilledRect(content_x, current_y, content_width, 1, 100, 100, 100, 200);
    current_y += 12;

    // Passive
    aFontConfig_t passive_config = {
        .type = FONT_GAME,
        .color = {168, 202, 88, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)passive_desc, content_x, current_y, &passive_config);
    current_y += passive_height + 8;

    // Active
    aFontConfig_t active_config = {
        .type = FONT_GAME,
        .color = {207, 87, 60, 255},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawTextStyled((char*)active_desc, content_x, current_y, &active_config);
    current_y += active_height + 8;

    // Cooldown status
    if (trinket->active_cooldown_current > 0) {
        dString_t* cd_text = d_StringInit();
        d_StringFormat(cd_text, "Cooldown: %d turns remaining", trinket->active_cooldown_current);

        aFontConfig_t cd_config = {
            .type = FONT_GAME,
            .color = {255, 100, 100, 255},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(cd_text), content_x, current_y, &cd_config);
        d_StringDestroy(cd_text);
        current_y += 20;
    }

    // Total damage dealt (if any)
    if (trinket->total_damage_dealt > 0) {
        dString_t* dmg_text = d_StringInit();
        d_StringFormat(dmg_text, "Total Damage Dealt: %d", trinket->total_damage_dealt);

        aFontConfig_t dmg_config = {
            .type = FONT_GAME,
            .color = {255, 255, 255, 255},  // White text
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(dmg_text), content_x, current_y, &dmg_config);
        d_StringDestroy(dmg_text);
    }
}

// ============================================================================
// TRINKET UI RENDERING
// ============================================================================

static void RenderTrinketUI(void) {
    if (!g_human_player || !g_human_player->trinket_slots) {
        return;
    }

    // Reset hover state
    g_hovered_trinket_slot = -1;
    g_hovered_class_trinket = false;

    // Get mouse position for hover detection
    int mouse_x = app.mouse.x;
    int mouse_y = app.mouse.y;

    // ========================================================================
    // RENDER CLASS TRINKET (LEFT SIDE, BIGGER SLOT)
    // ========================================================================
    Trinket_t* class_trinket = GetClassTrinket(g_human_player);

    // Check if mouse is hovering over class trinket slot
    bool class_hovered = (mouse_x >= CLASS_TRINKET_X && mouse_x < CLASS_TRINKET_X + CLASS_TRINKET_SIZE &&
                          mouse_y >= CLASS_TRINKET_Y && mouse_y < CLASS_TRINKET_Y + CLASS_TRINKET_SIZE);
    if (class_hovered) {
        g_hovered_class_trinket = true;
    }

    if (class_trinket) {
        // Class trinket equipped - draw with gold styling
        int bg_r = 50, bg_g = 50, bg_b = 30;  // Slightly golden tint
        int border_r = 200, border_g = 180, border_b = 50;  // Gold border

        // Check if on cooldown
        bool on_cooldown = class_trinket->active_cooldown_current > 0;
        bool is_clickable = !on_cooldown && class_trinket->active_target_type == TRINKET_TARGET_CARD;

        if (on_cooldown) {
            // Red tint (same red as cooldown indicator box)
            bg_r = 60; bg_g = 20; bg_b = 20;
            border_r = 255; border_g = 100; border_b = 100;
        }

        // Hover effect (brighter if ready to use)
        if (class_hovered && is_clickable) {
            bg_r += 20; bg_g += 20; bg_b += 10;  // Brighten background
            border_r = 255; border_g = 220; border_b = 100;  // Brighter gold border
        }

        // Apply shake offsets (tweened during proc)
        int shake_x = CLASS_TRINKET_X + (int)class_trinket->shake_offset_x;
        int shake_y = CLASS_TRINKET_Y + (int)class_trinket->shake_offset_y;

        // Background
        a_DrawFilledRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                       bg_r, bg_g, bg_b, 255);

        // Gold border (thicker, 2px)
        a_DrawRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                  border_r, border_g, border_b, 255);  // Gold
        a_DrawRect(shake_x + 1, shake_y + 1, CLASS_TRINKET_SIZE - 2, CLASS_TRINKET_SIZE - 2,
                  border_r, border_g, border_b, 255);  // Double border for emphasis

        // Hover glow effect (subtle overlay)
        if (class_hovered && is_clickable) {
            a_DrawFilledRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                           255, 255, 200, 30);  // Subtle yellow glow
        }

        // Draw red flash overlay when trinket procs
        if (class_trinket->flash_alpha > 0.0f) {
            Uint8 flash = (Uint8)class_trinket->flash_alpha;
            a_DrawFilledRect(shake_x, shake_y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                           255, 0, 0, flash);
        }

        // "CLASS" label at top
        aFontConfig_t class_label_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {200, 180, 50, 255},  // Gold text
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.5f
        };
        a_DrawTextStyled("CLASS",
                       shake_x + CLASS_TRINKET_SIZE / 2,
                       shake_y + 6,
                       &class_label_config);

        // Draw trinket name
        const char* trinket_name = GetTrinketName(class_trinket);
        aFontConfig_t name_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {255, 255, 255, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = CLASS_TRINKET_SIZE - 8,
            .scale = 0.6f
        };
        a_DrawTextStyled((char*)trinket_name,
                       shake_x + CLASS_TRINKET_SIZE / 2,
                       shake_y + 24,
                       &name_config);

        // Draw indicator boxes below trinket (cooldown left, damage bonus right)
        const int indicator_width = 40;
        const int indicator_height = 34;
        const int indicator_gap = 6;
        const int indicator_y_offset = 6;  // Gap between trinket and indicators

        int indicators_y = CLASS_TRINKET_Y + CLASS_TRINKET_SIZE + indicator_y_offset;
        int left_indicator_x = CLASS_TRINKET_X + (CLASS_TRINKET_SIZE / 2) - indicator_width - (indicator_gap / 2);
        int right_indicator_x = CLASS_TRINKET_X + (CLASS_TRINKET_SIZE / 2) + (indicator_gap / 2);

        // Left indicator: Cooldown
        if (on_cooldown) {
            // Red background for cooldown
            a_DrawFilledRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                           60, 20, 20, 255);
            a_DrawRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                      255, 100, 100, 255);

            char cooldown_text[8];
            snprintf(cooldown_text, sizeof(cooldown_text), "%d", class_trinket->active_cooldown_current);

            aFontConfig_t cd_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {255, 150, 150, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 0.8f
            };
            a_DrawTextStyled(cooldown_text,
                           left_indicator_x + indicator_width / 2,
                           indicators_y + 3,
                           &cd_config);
        } else {
            // Dimmed background when ready
            a_DrawFilledRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                           30, 30, 30, 255);
            a_DrawRect(left_indicator_x, indicators_y, indicator_width, indicator_height,
                      80, 80, 80, 255);

            aFontConfig_t ready_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {100, 255, 100, 255},  // Green for ready
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 0.8f
            };
            a_DrawTextStyled("0",
                           left_indicator_x + indicator_width / 2,
                           indicators_y + 3,
                           &ready_config);
        }

        // Right indicator: Damage bonus (shows scaling bonus, not base damage)
        // Green background for damage
        a_DrawFilledRect(right_indicator_x, indicators_y, indicator_width, indicator_height,
                       20, 60, 20, 255);
        a_DrawRect(right_indicator_x, indicators_y, indicator_width, indicator_height,
                  100, 255, 100, 255);

        char bonus_text[16];
        snprintf(bonus_text, sizeof(bonus_text), "+%d", class_trinket->passive_damage_bonus);

        aFontConfig_t bonus_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {150, 255, 150, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.8f
        };
        a_DrawTextStyled(bonus_text,
                       right_indicator_x + indicator_width / 2,
                       indicators_y + 3,
                       &bonus_config);
    } else {
        // Empty class trinket slot - draw dimmed gold outline
        a_DrawRect(CLASS_TRINKET_X, CLASS_TRINKET_Y, CLASS_TRINKET_SIZE, CLASS_TRINKET_SIZE,
                  100, 90, 25, 128);  // Dimmed gold
    }

    // ========================================================================
    // RENDER REGULAR TRINKET GRID (3x2 grid, RIGHT SIDE)
    // ========================================================================
    // 3x2 grid layout (slots 0-2 top row, 3-5 bottom row)
    for (int slot_index = 0; slot_index < 6; slot_index++) {
        int row = slot_index / 3;  // 0 or 1
        int col = slot_index % 3;  // 0, 1, or 2

        int slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

        // Check if mouse is hovering over this slot
        bool is_hovered = (mouse_x >= slot_x && mouse_x < slot_x + TRINKET_SLOT_SIZE &&
                          mouse_y >= slot_y && mouse_y < slot_y + TRINKET_SLOT_SIZE);
        if (is_hovered) {
            g_hovered_trinket_slot = slot_index;
        }

        Trinket_t* trinket = GetEquippedTrinket(g_human_player, slot_index);

        if (trinket) {
            // Trinket equipped - draw filled slot with cooldown overlay
            int bg_r = 40, bg_g = 40, bg_b = 50;

            // Check if on cooldown
            bool on_cooldown = trinket->active_cooldown_current > 0;
            if (on_cooldown) {
                bg_r = 60; bg_g = 30; bg_b = 30;  // Reddish tint
            }

            // Apply shake offsets (tweened during proc - matches status effects)
            int shake_x = slot_x + (int)trinket->shake_offset_x;
            int shake_y = slot_y + (int)trinket->shake_offset_y;

            a_DrawFilledRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                           bg_r, bg_g, bg_b, 255);
            a_DrawRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                      200, 200, 200, 255);

            // Draw red flash overlay when trinket procs (matches status effects)
            if (trinket->flash_alpha > 0.0f) {
                Uint8 flash = (Uint8)trinket->flash_alpha;
                a_DrawFilledRect(shake_x, shake_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                               255, 0, 0, flash);
            }

            // Draw trinket name (truncated to fit) - use shake offsets
            const char* trinket_name = GetTrinketName(trinket);
            aFontConfig_t name_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {255, 255, 255, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = TRINKET_SLOT_SIZE - 4,
                .scale = 0.5f
            };
            a_DrawTextStyled((char*)trinket_name,
                           shake_x + TRINKET_SLOT_SIZE / 2,
                           shake_y + 8,
                           &name_config);

            // Draw cooldown counter if on cooldown - use shake offsets
            if (on_cooldown) {
                char cooldown_text[8];
                snprintf(cooldown_text, sizeof(cooldown_text), "%d", trinket->active_cooldown_current);

                aFontConfig_t cd_config = {
                    .type = FONT_ENTER_COMMAND,
                    .color = {255, 100, 100, 255},
                    .align = TEXT_ALIGN_CENTER,
                    .wrap_width = 0,
                    .scale = 1.2f
                };
                a_DrawTextStyled(cooldown_text,
                               shake_x + TRINKET_SLOT_SIZE / 2,
                               shake_y + TRINKET_SLOT_SIZE / 2 - 6,
                               &cd_config);
            }

            // Draw damage bonus if trinket has scaling - use shake offsets
            if (trinket->passive_damage_bonus > 0) {
                char bonus_text[16];
                snprintf(bonus_text, sizeof(bonus_text), "+%d", trinket->passive_damage_bonus);

                aFontConfig_t bonus_config = {
                    .type = FONT_ENTER_COMMAND,
                    .color = {100, 255, 100, 255},
                    .align = TEXT_ALIGN_RIGHT,
                    .wrap_width = 0,
                    .scale = 0.6f
                };
                a_DrawTextStyled(bonus_text,
                               shake_x + TRINKET_SLOT_SIZE - 4,
                               shake_y + TRINKET_SLOT_SIZE - 16,
                               &bonus_config);
            }
        } else {
            // Empty slot - draw dimmed outline
            a_DrawRect(slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE,
                      80, 80, 80, 128);
        }
    }

    // Render tooltip if hovering over a trinket
    if (g_hovered_class_trinket && class_trinket) {
        RenderTrinketTooltip(class_trinket, -1);  // -1 = class trinket
    } else if (g_hovered_trinket_slot >= 0) {
        Trinket_t* hovered_trinket = GetEquippedTrinket(g_human_player, g_hovered_trinket_slot);
        if (hovered_trinket) {
            RenderTrinketTooltip(hovered_trinket, g_hovered_trinket_slot);
        }
    }
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

            // Get defeat animation state (fade + zoom out)
            float defeat_alpha = GetEnemyDefeatAlpha(g_game.current_enemy);
            float defeat_scale = GetEnemyDefeatScale(g_game.current_enemy);

            // Apply defeat scale to dimensions
            int final_width = (int)(scaled_width * defeat_scale);
            int final_height = (int)(scaled_height * defeat_scale);

            // Center the scaled portrait (so zoom-out is centered, not top-left)
            int scale_offset_x = (scaled_width - final_width) / 2;
            int scale_offset_y = (scaled_height - final_height) / 2;

            // Apply red tint to texture pixels (not transparent areas)
            if (red_alpha > 0.0f) {
                // Blend white → red based on alpha
                // red_alpha = 0.0 → (255, 255, 255) white (no tint)
                // red_alpha = 1.0 → (255, 0, 0) pure red
                Uint8 g_channel = (Uint8)(255 * (1.0f - red_alpha));
                Uint8 b_channel = (Uint8)(255 * (1.0f - red_alpha));
                SDL_SetTextureColorMod(enemy_portrait, 255, g_channel, b_channel);
            }

            // Apply defeat fade alpha
            if (defeat_alpha < 1.0f) {
                SDL_SetTextureAlphaMod(enemy_portrait, (Uint8)(defeat_alpha * 255));
            }

            // Use a_BlitTextureScaled to avoid int truncation of scale parameter
            // (a_BlitTextureRect takes int scale, which truncates 0.6f to 0)
            a_BlitTextureScaled(enemy_portrait,
                                portrait_x + (int)shake_x + scale_offset_x,
                                ENEMY_PORTRAIT_Y_OFFSET + (int)shake_y + scale_offset_y,
                                final_width, final_height);

            // Reset texture mods after rendering
            if (red_alpha > 0.0f) {
                SDL_SetTextureColorMod(enemy_portrait, 255, 255, 255);
            }
            if (defeat_alpha < 1.0f) {
                SDL_SetTextureAlphaMod(enemy_portrait, 255);
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

            case STATE_TARGETING:
                RenderActionPanel(g_targeting_panel, button_y);
                break;

            case STATE_ROUND_END:
                RenderResultOverlay();
                break;

            default:
                break;
        }
    }

    // Render sidebar modal overlay (character stats) - MUST be after action panels for z-ordering
    if (g_left_sidebar && g_human_player) {
        RenderLeftSidebarModalOverlay(g_left_sidebar, g_human_player, 0, LAYOUT_TOP_MARGIN);
    }

    // Render trinket UI (before pause menu/terminal so they overlay it)
    RenderTrinketUI();

    // Render targeting arrow (if in targeting mode)
    RenderTargetingArrow();

    // Render combat victory overlay (if in victory state)
    // Render combat victory celebration (after round end, before rewards)
    if (g_game.current_state == STATE_COMBAT_VICTORY && g_game.current_enemy) {
        // Dark overlay
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 200);

        // "VICTORY!" text (centered, large, gold)
        int center_x = SCREEN_WIDTH / 2;
        int center_y = SCREEN_HEIGHT / 2 - 50;
        aFontConfig_t gold_title = {
            .type = FONT_ENTER_COMMAND,
            .color = {232, 193, 112, 255},  // Gold
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.5f
        };
        a_DrawTextStyled("VICTORY!", center_x, center_y, &gold_title);

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
        a_DrawTextStyled((char*)d_StringPeek(victory_msg), center_x, center_y + 50, &off_white);
        d_StringDestroy(victory_msg);

        // Show bet winnings (green "+X chips" text)
        if (g_result_chip_delta > 0) {
            dString_t* winnings_text = d_StringInit();
            d_StringFormat(winnings_text, "+%d chips won!", g_result_chip_delta);

            aFontConfig_t green_winnings = {
                .type = FONT_GAME,
                .color = {117, 255, 67, 255},  // Bright green
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = 0,
                .scale = 1.5f
            };
            a_DrawTextStyled((char*)d_StringPeek(winnings_text), center_x, center_y + 110, &green_winnings);
            d_StringDestroy(winnings_text);
        }
    }

    // Render reward modal (if visible) - BEFORE pause menu
    if (g_reward_modal) {
        RenderRewardModal(g_reward_modal);
    }

    // Render deck/discard modals
    if (g_draw_pile_modal) {
        RenderDrawPileModalSection(g_draw_pile_modal);
    }
    if (g_discard_pile_modal) {
        RenderDiscardPileModalSection(g_discard_pile_modal);
    }

    // Render pause menu if visible (on top of everything)
    if (g_pause_menu) {
        RenderPauseMenuSection(g_pause_menu);
    }

    // Render event screen (if in STATE_EVENT)
    if (g_game.current_state == STATE_EVENT && g_current_event) {
        // Dark overlay
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 220);

        // Event panel (centered modal)
        int panel_width = 700;
        int panel_height = 500;
        int panel_x = (SCREEN_WIDTH - panel_width) / 2;
        int panel_y = (SCREEN_HEIGHT - panel_height) / 2;

        // Panel background
        a_DrawFilledRect(panel_x, panel_y, panel_width, panel_height, 20, 20, 30, 240);
        a_DrawRect(panel_x, panel_y, panel_width, panel_height, 255, 255, 255, 255);

        // Event title
        aFontConfig_t title_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {232, 193, 112, 255},  // Gold
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.3f
        };
        a_DrawTextStyled((char*)d_StringPeek(g_current_event->title),
                         SCREEN_WIDTH / 2, panel_y + 40, &title_config);

        // Event description
        aFontConfig_t desc_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {235, 237, 233, 255},  // Off-white
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = panel_width - 80,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(g_current_event->description),
                         SCREEN_WIDTH / 2, panel_y + 100, &desc_config);

        // Choice buttons
        int choice_count = (int)g_current_event->choices->count;
        int button_width = 500;
        int button_height = 60;
        int button_spacing = 20;
        int choices_start_y = panel_y + 250;

        for (int i = 0; i < choice_count; i++) {
            EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(g_current_event->choices, i);
            if (!choice || !choice->text) continue;

            int button_y = choices_start_y + i * (button_height + button_spacing);
            int button_x = (SCREEN_WIDTH - button_width) / 2;

            // Button background
            a_DrawFilledRect(button_x, button_y, button_width, button_height, 50, 50, 60, 255);
            a_DrawRect(button_x, button_y, button_width, button_height, 200, 200, 200, 255);

            // Choice text
            const char* choice_text = d_StringPeek(choice->text);
            if (!choice_text) continue;

            aFontConfig_t choice_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {255, 255, 255, 255},
                .align = TEXT_ALIGN_CENTER,
                .wrap_width = button_width - 40,
                .scale = 0.9f
            };
            a_DrawTextStyled((char*)choice_text,
                             SCREEN_WIDTH / 2, button_y + 20, &choice_config);
        }

        // Instruction text
        aFontConfig_t instruction_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {180, 180, 180, 255},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.8f
        };
        a_DrawTextStyled("Click a choice to continue",
                         SCREEN_WIDTH / 2, panel_y + panel_height - 40, &instruction_config);
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

    // Render popup notification (above tutorial, below terminal)
    if (g_popup_active && g_popup_alpha > 0.0f) {
        // Calculate background box dimensions based on text
        // Approximate: 12px per character at scale 1.2 (FONT_ENTER_COMMAND is ~10px base)
        int text_length = strlen(g_popup_message);
        int estimated_width = text_length * 12;
        int padding_x = 2;  // 2px horizontal padding
        int box_width = estimated_width + (padding_x * 2);
        int box_height = 50;
        int box_x = (SCREEN_WIDTH - box_width) / 2;
        int box_y = (int)g_popup_y - 4;  // Move background DOWN 6px (was -10, now -4)

        // Draw background box (#10141f at 30% opacity with fade)
        Uint8 bg_alpha = (Uint8)(0.3f * g_popup_alpha * 255);
        a_DrawFilledRect(box_x, box_y, box_width, box_height, 16, 20, 31, bg_alpha);

        // Draw text (#a53030 red with fade)
        aFontConfig_t popup_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {165, 48, 48, (Uint8)(g_popup_alpha * 255)},  // #a53030 with fade
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.2f
        };
        a_DrawTextStyled(g_popup_message, SCREEN_WIDTH / 2, (int)g_popup_y, &popup_config);
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

    // ========================================================================
    // RESULT SCREEN ANIMATIONS (Win/Loss + Status Drain)
    // ========================================================================

    int current_chips = GetPlayerChips(g_human_player);

    // Start win/loss animation IMMEDIATELY (0.0s, stays visible)
    if (!g_result_winloss_started && g_result_chip_delta != 0) {
        g_result_winloss_started = true;
        d_LogInfoF("💰 WIN/LOSS ANIMATION: delta=%+d", g_result_chip_delta);
        g_result_winloss_alpha = 255.0f;
        TweenFloat(&g_tween_manager, &g_result_winloss_alpha, 0.0f, 2.0f, TWEEN_EASE_OUT_QUAD);  // Longer fade (2s)
    }

    // Start status drain animation DELAYED (0.5s)
    if (g_result_screen_timer > 0.5f && !g_result_bleed_started) {
        g_result_bleed_started = true;
        d_LogInfoF("💰 STATUS DRAIN ANIMATION: drain=%d", g_result_status_drain);
        // Tween chip counter (slot machine effect!)
        TweenFloat(&g_tween_manager, &g_result_display_chips, (float)current_chips, 0.8f, TWEEN_EASE_OUT_CUBIC);
        // Fade out status drain text
        if (g_result_status_drain > 0) {
            g_result_bleed_alpha = 255.0f;
            TweenFloat(&g_tween_manager, &g_result_bleed_alpha, 0.0f, 1.5f, TWEEN_EASE_OUT_QUAD);
        }
    }

    // Render chips with slot machine animation
    dString_t* chip_info = d_StringInit();
    d_StringFormat(chip_info, "Chips: %d", (int)g_result_display_chips);

    // Measure chip text width for positioning damage text
    int chip_text_width = 0;
    int chip_text_height = 0;
    a_CalcTextDimensions((char*)d_StringPeek(chip_info), FONT_STYLE_TITLE.type, &chip_text_width, &chip_text_height);
    int chip_center_x = SCREEN_WIDTH / 2;
    int chip_y = SCREEN_HEIGHT / 2 + 20;

    // Draw chip count (centered)
    a_DrawTextStyled((char*)d_StringPeek(chip_info), chip_center_x, chip_y, &FONT_STYLE_TITLE);
    d_StringDestroy(chip_info);

    // Track win/loss message bounds for collision detection
    int winloss_y = 0;
    int winloss_height = 0;
    bool has_winloss = false;

    // Draw win/loss text (IMMEDIATE, 0-0.5s, shows net chip change from betting)
    if (g_result_chip_delta != 0 && g_result_winloss_alpha > 0.0f) {
        dString_t* winloss_text = d_StringInit();
        d_StringFormat(winloss_text, "%+d chips", g_result_chip_delta);

        // Calculate text dimensions for collision detection
        int winloss_width = 0;
        a_CalcTextDimensions((char*)d_StringPeek(winloss_text), FONT_GAME, &winloss_width, &winloss_height);
        winloss_height = (int)((float)winloss_height * 2.0f);  // Account for scale = 2.0f

        // Position with random offset (wins float higher)
        int base_y_offset = (g_result_chip_delta > 0) ? -30 : 0;  // Wins start 30px higher
        int winloss_x = chip_center_x + (chip_text_width / 2) + 20 + (int)g_result_winloss_offset_x;
        winloss_y = chip_y + base_y_offset + (int)g_result_winloss_offset_y;
        has_winloss = true;

        // Bright green for wins, red for losses
        Uint8 r = (g_result_chip_delta > 0) ? 117 : 255;
        Uint8 g = (g_result_chip_delta > 0) ? 255 : 0;
        Uint8 b = (g_result_chip_delta > 0) ? 67 : 0;

        aFontConfig_t winloss_config = {
            .type = FONT_GAME,
            .color = {r, g, b, (Uint8)g_result_winloss_alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 2.0f  // Bigger for readability
        };
        a_DrawTextStyled((char*)d_StringPeek(winloss_text), winloss_x, winloss_y, &winloss_config);
        d_StringDestroy(winloss_text);
    }

    // Draw status drain text (DELAYED, 0.5s+, shows chip drain from status effects)
    if (g_result_screen_timer > 0.5f && g_result_status_drain > 0 && g_result_bleed_alpha > 0.0f) {
        dString_t* bleed_text = d_StringInit();
        d_StringFormat(bleed_text, "-%d chip drain", g_result_status_drain);

        // Calculate text dimensions for collision detection
        int bleed_width = 0;
        int bleed_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(bleed_text), FONT_GAME, &bleed_width, &bleed_height);
        bleed_height = (int)((float)bleed_height * 2.0f);  // Account for scale = 2.0f

        // Position with random offset (different from win/loss!)
        int bleed_x = chip_center_x + (chip_text_width / 2) + 20 + (int)g_result_bleed_offset_x;
        int bleed_y = chip_y + (int)g_result_bleed_offset_y;

        // COLLISION DETECTION: Check overlap with win/loss message
        if (has_winloss) {
            const int PADDING = 10;  // Visual breathing room
            int overlap_threshold = (winloss_height + bleed_height) / 2 + PADDING;
            int y_distance = abs(winloss_y - bleed_y);

            if (y_distance < overlap_threshold) {
                // PUSH-BACK: Move status drain message below win/loss message
                bleed_y = winloss_y + winloss_height + PADDING;
            }
        }

        aFontConfig_t bleed_config = {
            .type = FONT_GAME,
            .color = {255, 0, 0, (Uint8)g_result_bleed_alpha},  // Always red
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 2.0f  // Bigger for readability
        };
        a_DrawTextStyled((char*)d_StringPeek(bleed_text), bleed_x, bleed_y, &bleed_config);
        d_StringDestroy(bleed_text);
    }

    // Next round prompt
    aFontConfig_t gray_text = {
        .type = FONT_ENTER_COMMAND,
        .color = {200, 200, 200, 255},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
}
