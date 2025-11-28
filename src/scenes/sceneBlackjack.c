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
#include "../../include/loaders/enemyLoader.h"
#include "../../include/scenes/sceneBlackjack.h"
#include "../../include/scenes/sceneMenu.h"
#include "../../include/scenes/components/button.h"
#include "../../include/scenes/components/deckButton.h"
#include "../../include/scenes/components/sidebarButton.h"
#include "../../include/scenes/components/rewardModal.h"
#include "../../include/scenes/components/eventPreviewModal.h"
#include "../../include/scenes/components/combatPreviewModal.h"
#include "../../include/scenes/components/introNarrativeModal.h"
#include "../../include/scenes/components/eventModal.h"
#include "../../include/scenes/components/resultScreen.h"
#include "../../include/scenes/components/victoryOverlay.h"
#include "../../include/scenes/components/trinketUI.h"
#include "../../include/scenes/components/visualEffects.h"
#include "../../include/scenes/components/enemyPortraitRenderer.h"
#include "../../include/eventPool.h"
#include "../../include/act.h"
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
#include "../../include/sanityThreshold.h"
#include <math.h>

// External globals from main.c
extern Deck_t g_test_deck;
extern dTable_t* g_card_textures;
extern SDL_Surface* g_card_back_texture;

// ============================================================================
// SCENE-LOCAL STATE
// ============================================================================

GameContext_t g_game;  // Stack-allocated singleton (constitutional pattern) - non-static for terminal access
Player_t* g_dealer = NULL;
Player_t* g_human_player = NULL;  // Non-static for terminal access

// Loading screen state
static bool g_is_loading = false;  // Set to true during asset loading

// FlexBox layout (main vertical container)
static FlexBox_t* g_main_layout = NULL;

// UI Sections (FlexBox items)
static TopBarSection_t* g_top_bar = NULL;
static PauseMenuSection_t* g_pause_menu = NULL;
LeftSidebarSection_t* g_left_sidebar = NULL;  // Non-static: accessed by resultScreen.c
static TitleSection_t* g_title_section = NULL;
static DealerSection_t* g_dealer_section = NULL;
static PlayerSection_t* g_player_section = NULL;
static ActionPanel_t* g_betting_panel = NULL;
static ActionPanel_t* g_action_panel = NULL;
static ActionPanel_t* g_targeting_panel = NULL;
static Terminal_t* g_terminal = NULL;

// Background surfaces
static SDL_Surface* g_background_texture = NULL;
static SDL_Surface* g_table_texture = NULL;

// Modal sections
static DrawPileModalSection_t* g_draw_pile_modal = NULL;
static DiscardPileModalSection_t* g_discard_pile_modal = NULL;
static RewardModal_t* g_reward_modal = NULL;
static EventPreviewModal_t* g_event_preview_modal = NULL;
static CombatPreviewModal_t* g_combat_preview_modal = NULL;
static IntroNarrativeModal_t* g_intro_narrative_modal = NULL;
EventModal_t* g_event_modal = NULL;  // Non-static for terminal command access
static ResultScreen_t* g_result_screen = NULL;
static VictoryOverlay_t* g_victory_overlay = NULL;

// Event system
EventEncounter_t* g_current_event = NULL;  // Non-static for terminal command access
static EventPool_t* g_tutorial_event_pool = NULL;

// Trinket UI component
static TrinketUI_t* g_trinket_ui = NULL;

// Visual effects component (damage numbers + screen shake)
static VisualEffects_t* g_visual_effects = NULL;

// Enemy portrait renderer component
static EnemyPortraitRenderer_t* g_enemy_portrait_renderer = NULL;

// Popup notification state (for invalid target feedback)
static char g_popup_message[128] = {0};
static float g_popup_alpha = 0.0f;
static float g_popup_y = 0.0f;
static bool g_popup_active = false;

// Tutorial start delay (wait for enemy fade-in)
static float g_tutorial_start_delay = 0.5f;  // 0.5s delay
static bool g_tutorial_started = false;

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
TutorialSystem_t* g_tutorial_system = NULL;  // Non-static: accessed by trinketUI.c
static TutorialStep_t* g_tutorial_steps = NULL;

// Tween manager (for smooth HP bar drain and damage numbers)
// Non-static: accessible from game.c for enemy damage effects
TweenManager_t g_tween_manager;

// Card transition manager (for card dealing/discarding animations)
static CardTransitionManager_t g_card_transition_manager;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// Forward declarations
static void BlackjackLogic(float dt);
static void BlackjackDraw(float dt);

// ============================================================================
// TOKEN BLEED TRACKING (Called from statusEffects.c)
// ============================================================================


static void CleanupBlackjackScene(void);

// Layout initialization
static void InitializeLayout(void);
static void CleanupLayout(void);

// Input handlers
static void HandleBettingInput(void);
static void HandlePlayerTurnInput(void);
static void HandleTargetingInput(void);

static void InitializeLayout(void) {
    // Create main vertical FlexBox for GAME AREA (right side, after sidebar)
    // Top bar is independent, layout starts at LAYOUT_TOP_MARGIN (35px)
    // X position = SIDEBAR_WIDTH (280px), Width = remaining screen width
    g_main_layout = a_FlexBoxCreate(GAME_AREA_X, LAYOUT_TOP_MARGIN,
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

    // Load background surfaces (only on first initialization - they persist across scenes)
    if (!g_background_texture) {
        g_background_texture = IMG_Load("resources/textures/background1.png");
        if (!g_background_texture) {
            d_LogError("Failed to load background surface");
        }
    }

    if (!g_table_texture) {
        g_table_texture = IMG_Load("resources/textures/blackjack_table.png");
        if (!g_table_texture) {
            d_LogError("Failed to load table surface");
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

    // Create event modal
    g_event_modal = CreateEventModal();

    // Create event preview modal (created once, content updated on use)
    g_event_preview_modal = CreateEventPreviewModal(&g_game, "Loading...");

    // Create combat preview modal (created once, content updated on use)
    g_combat_preview_modal = CreateCombatPreviewModal(&g_game, "", "");

    // Create intro narrative modal (tutorial story scene)
    g_intro_narrative_modal = CreateIntroNarrativeModal();

    // Create result screen component
    g_result_screen = CreateResultScreen();
    if (g_result_screen) {
        SetGlobalResultScreen(g_result_screen);  // Register for statusEffects.c callbacks
    }

    // Create victory overlay component
    g_victory_overlay = CreateVictoryOverlay();

    // Create trinket UI component
    g_trinket_ui = CreateTrinketUI();

    // Create visual effects component
    g_visual_effects = CreateVisualEffects(&g_tween_manager);

    // Create enemy portrait renderer (400x400 for 0.8x scale)
    int portrait_x = GAME_AREA_X + (GAME_AREA_WIDTH - 400) / 2 + ENEMY_PORTRAIT_X_OFFSET;
    int portrait_y = (SCREEN_HEIGHT - 400) / 2 + ENEMY_PORTRAIT_Y_OFFSET;
    g_enemy_portrait_renderer = CreateEnemyPortraitRenderer(portrait_x, portrait_y, 400, 400, NULL);

    // Create tutorial event pool
    g_tutorial_event_pool = CreateTutorialEventPool();

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
    if (g_event_preview_modal) {
        DestroyEventPreviewModal(&g_event_preview_modal);
    }
    if (g_combat_preview_modal) {
        DestroyCombatPreviewModal(&g_combat_preview_modal);
    }
    if (g_intro_narrative_modal) {
        DestroyIntroNarrativeModal(&g_intro_narrative_modal);
    }
    if (g_event_modal) {
        DestroyEventModal(&g_event_modal);
    }
    if (g_result_screen) {
        SetGlobalResultScreen(NULL);  // Unregister before destroying
        DestroyResultScreen(&g_result_screen);
    }
    if (g_victory_overlay) {
        DestroyVictoryOverlay(&g_victory_overlay);
    }
    if (g_trinket_ui) {
        DestroyTrinketUI(&g_trinket_ui);
    }
    if (g_enemy_portrait_renderer) {
        DestroyEnemyPortraitRenderer(&g_enemy_portrait_renderer);
    }

    // Destroy event pool
    if (g_tutorial_event_pool) {
        DestroyEventPool(&g_tutorial_event_pool);
    }

    // Destroy main FlexBox
    if (g_main_layout) {
        a_FlexBoxDestroy(&g_main_layout);
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

    // Set loading flag (will show loading screen during asset loading)
    g_is_loading = true;

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

    // Immediately transition to intro narrative (before loading assets)
    // This ensures the first render doesn't show the bright game UI
    State_Transition(&g_game, STATE_INTRO_NARRATIVE);

    // Add players to game (dealer first, then player)
    Game_AddPlayerToGame(&g_game, 0);  // Dealer
    Game_AddPlayerToGame(&g_game, 1);  // Player

    // Initialize FlexBox layout (automatic positioning!)
    InitializeLayout();

    // Initialize tutorial system (but don't start yet - wait for enemy fade-in)
    g_tutorial_system = CreateTutorialSystem();
    g_tutorial_steps = CreateBlackjackTutorial();
    g_tutorial_started = false;  // Will start after 0.5s delay

    // Create tutorial act (2 combats, 2 events)
    g_game.current_act = CreateTutorialAct();
    if (!g_game.current_act) {
        d_LogError("Failed to create tutorial act!");
        return;
    }

    // Start first encounter (should be NORMAL: The Didact)
    Encounter_t* first_encounter = GetCurrentEncounter(g_game.current_act);
    if (!first_encounter) {
        d_LogError("Tutorial act has no first encounter!");
        return;
    }

    d_LogInfoF("Starting tutorial act - First encounter: %s",
              GetEncounterTypeName(first_encounter->type));

    // State already set to INTRO_NARRATIVE at the top of InitBlackjackScene
    // Show modal immediately (don't wait for first logic update)
    if (g_intro_narrative_modal) {
        const char* title = "The Casino";

        // Split narrative into 3 blocks
        const char* block1 =
            "You stumble through darkness. The casino floor stretches endlessly.\n\n"
            "Slot machines silent, tables empty, cards scattered like debris.\n\n"
            "The chips in your pocket grow warm.\n\n"
            "You understand without being told: these are all you have.\n\n"
            "When they're gone, so are you.\n\n";

        const char* block2 =
            "Machines begin lighting up, one by one down the hallway.\n\n"
            "The lights reveal a red velvet table at the end of the hall.\n\n"
            "The faceless, lifeless dealer behind \n\n"
            "recites with a mechanical voice: 'The system requires demonstration. Place your bet.'\n\n";

        const char* final_line = "You approach the table.";

        const char* narrative_blocks[] = {block1, block2, final_line};
        const char* portrait = "resources/textures/events/intro_scene.png";

        ShowIntroNarrativeModal(g_intro_narrative_modal, title, narrative_blocks, 3, portrait);
        d_LogInfo("Tutorial intro narrative shown immediately on init");
    }

    // Loading complete - disable loading screen
    g_is_loading = false;

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

    // Cleanup current act
    if (g_game.current_act) {
        DestroyAct(&g_game.current_act);
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

    // Reset card metadata for new game session
    // Tags applied during this run should NOT persist to next run
    CleanupCardMetadata();
    InitCardMetadata();
    d_LogInfo("Card metadata reset for new session");

    d_LogInfo("Blackjack scene cleanup complete");
}

// ============================================================================
// EVENT GENERATION (using EventPool system)
// ============================================================================

static EventEncounter_t* CreatePostCombatEvent(void) {
    // Use tutorial event pool for random selection
    if (!g_tutorial_event_pool) {
        d_LogError("CreatePostCombatEvent: Tutorial event pool not initialized!");
        return NULL;
    }

    EventEncounter_t* event = GetRandomEventFromPool(g_tutorial_event_pool);
    if (!event) {
        d_LogError("Failed to get event from pool");
        return NULL;
    }

    d_LogInfoF("Selected event from pool: %s", d_StringPeek(event->title));
    return event;
}

/**
 * StartNextEncounter - Spawn next enemy or handle event encounter
 *
 * Called after completing reward/event to progress act.
 * Checks current encounter type and spawns enemy if needed.
 */
static void StartNextEncounter(void) {
    if (!g_game.current_act) {
        d_LogError("StartNextEncounter: No current act!");
        return;
    }

    Encounter_t* encounter = GetCurrentEncounter(g_game.current_act);
    if (!encounter) {
        d_LogError("StartNextEncounter: No current encounter (act complete?)");
        return;
    }

    d_LogInfoF("Starting encounter: %s", GetEncounterTypeName(encounter->type));

    // Handle combat encounters (NORMAL, ELITE, BOSS)
    if (encounter->type == ENCOUNTER_NORMAL ||
        encounter->type == ENCOUNTER_ELITE ||
        encounter->type == ENCOUNTER_BOSS) {

        // Destroy previous enemy (if any)
        if (g_game.current_enemy) {
            DestroyEnemy(&g_game.current_enemy);
        }

        // Spawn new enemy
        if (encounter->enemy_key[0] == '\0') {
            d_LogError("Combat encounter has no enemy key!");
            return;
        }

        // Load enemy from DUF database using stored key
        Enemy_t* enemy = LoadEnemyFromDUF(g_enemies_db, encounter->enemy_key);
        if (!enemy) {
            d_LogErrorF("Failed to load enemy '%s' from DUF!", encounter->enemy_key);
            return;
        }

        // Apply HP multiplier from event consequences (if not 1.0)
        if (g_game.next_enemy_hp_multiplier != 1.0f) {
            int original_hp = enemy->max_hp;
            enemy->max_hp = (int)(enemy->max_hp * g_game.next_enemy_hp_multiplier);
            enemy->current_hp = enemy->max_hp;
            enemy->display_hp = (float)enemy->max_hp;
            d_LogInfoF("Applied HP multiplier %.2f: %d → %d HP",
                      g_game.next_enemy_hp_multiplier, original_hp, enemy->max_hp);
            // Reset multiplier for next enemy
            g_game.next_enemy_hp_multiplier = 1.0f;
        }

        // Load portrait
        if (encounter->portrait_path[0] != '\0') {
            if (!LoadEnemyPortrait(enemy, encounter->portrait_path)) {
                d_LogErrorF("Failed to load enemy portrait: %s", encounter->portrait_path);
            }
        }

        g_game.current_enemy = enemy;
        g_game.is_combat_mode = true;

        // Update portrait renderer with new enemy
        if (g_enemy_portrait_renderer) {
            SetEnemyPortraitEnemy(g_enemy_portrait_renderer, enemy);
        }

        // Fade in enemy on spawn (0.0 → 1.0 over 1.0 second)
        TweenFloat(&g_tween_manager, &enemy->defeat_fade_alpha, 1.0f, 1.0f, TWEEN_EASE_OUT_CUBIC);

        // Fire COMBAT_START event
        Game_TriggerEvent(&g_game, GAME_EVENT_COMBAT_START);

        d_LogInfoF("New combat: %s (%d/%d HP)",
                  GetEnemyName(enemy),
                  enemy->current_hp,
                  enemy->max_hp);

        // Start new round and begin betting
        Game_StartNewRound(&g_game);
        State_Transition(&g_game, STATE_BETTING);
    }
    else if (encounter->type == ENCOUNTER_EVENT) {
        // EVENT encounters are handled by the reward->event flow
        // This function shouldn't be called for EVENT type
        d_LogWarning("StartNextEncounter called for EVENT type (should be handled by reward flow)");
    }
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

    // Update visual effects (shake cooldown)
    if (g_visual_effects) {
        UpdateVisualEffects(g_visual_effects, dt);
    }

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

    // Handle intro narrative modal input (BEFORE tutorial to avoid conflicts)
    if (g_game.current_state == STATE_INTRO_NARRATIVE && g_intro_narrative_modal && IsIntroNarrativeModalVisible(g_intro_narrative_modal)) {
        if (HandleIntroNarrativeModalInput(g_intro_narrative_modal, dt)) {
            // Continue button was clicked - hide modal and start encounter directly
            d_LogInfo("Intro narrative: Continue clicked, starting encounter");
            HideIntroNarrativeModal(g_intro_narrative_modal);

            // Start the Didact encounter directly (no combat preview)
            StartNextEncounter();
        }
        return;  // Block all other input while narrative is visible
    }

    // Handle tutorial start delay (wait for enemy fade-in after narrative)
    if (g_tutorial_system && g_tutorial_steps && !g_tutorial_started) {
        g_tutorial_start_delay -= dt;
        if (g_tutorial_start_delay <= 0.0f) {
            StartTutorial(g_tutorial_system, g_tutorial_steps);
            g_tutorial_started = true;
            d_LogInfo("Tutorial started after 0.5s delay (post narrative)");
        } else {
            // Still waiting - don't process any input yet
            return;
        }
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

    // Settings button clicked - Always works (even during tutorial)
    if (IsTopBarSettingsClicked(g_top_bar)) {
        d_LogInfo("Settings button clicked - showing pause menu");
        ShowPauseMenu(g_pause_menu);
        return;
    }

    // ESC - Show skip tutorial modal if tutorial dialogue is visible, otherwise show pause menu
    // This check happens BEFORE tutorial_blocking_input so ESC works on all tutorial steps
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;

        // If tutorial dialogue is visible, show skip confirmation instead of pause menu
        if (g_tutorial_system && IsTutorialActive(g_tutorial_system) &&
            g_tutorial_system->dialogue_visible && !g_tutorial_system->skip_confirmation.visible) {
            // Show skip tutorial confirmation (unless on final step)
            if (g_tutorial_system->current_step && !g_tutorial_system->current_step->is_final_step) {
                g_tutorial_system->skip_confirmation.visible = true;
                d_LogInfo("ESC pressed - showing skip tutorial confirmation");
                return;
            }
            // On final step, ESC finishes tutorial (handled in HandleTutorialInput)
        }

        // Otherwise show pause menu (only if not in tutorial blocking state)
        if (!tutorial_blocking_input) {
            d_LogInfo("ESC pressed - showing pause menu");
            ShowPauseMenu(g_pause_menu);
            return;
        }
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

    // Result screen component lifecycle (matches old pattern)
    static int result_old_chips = 0;  // Chips before round (needed for delta calculation)

    // Capture chips before round (for slot machine animation)
    if (previous_state != STATE_DEALER_TURN && g_game.current_state == STATE_DEALER_TURN) {
        result_old_chips = GetPlayerChips(g_human_player);
        if (g_result_screen) {
            g_result_screen->status_drain = 0;  // Reset for this round
        }
        d_LogInfoF("💰 ENTERING DEALER_TURN: Captured chips=%d, reset status drain", result_old_chips);
    }

    // Show result screen when entering ROUND_END or COMBAT_VICTORY (AFTER payouts)
    if ((previous_state != STATE_ROUND_END && g_game.current_state == STATE_ROUND_END) ||
        (previous_state != STATE_COMBAT_VICTORY && g_game.current_state == STATE_COMBAT_VICTORY)) {
        if (g_result_screen) {
            int current_chips = GetPlayerChips(g_human_player);
            int total_delta = current_chips - result_old_chips;  // Total chip change

            // Separate bet outcome from status drain
            // total_delta = bet_outcome - status_drain
            // Therefore: bet_outcome = total_delta + status_drain
            int chip_delta = total_delta + g_result_screen->status_drain;  // Bet outcome only (win/loss)

            bool is_victory = (g_game.current_state == STATE_COMBAT_VICTORY);
            ShowResultScreen(g_result_screen, result_old_chips, chip_delta, g_result_screen->status_drain, is_victory);
        }

        // Show victory overlay when entering COMBAT_VICTORY
        if (g_game.current_state == STATE_COMBAT_VICTORY && g_victory_overlay) {
            ShowVictoryOverlay(g_victory_overlay, g_game.current_enemy);
        }
    }

    // Update result screen timer (during result screen)
    if (g_game.current_state == STATE_ROUND_END && g_result_screen) {
        UpdateResultScreen(g_result_screen, dt, &g_tween_manager);
    }

    // Show reward modal when entering STATE_REWARD_SCREEN
    if (previous_state != STATE_REWARD_SCREEN && g_game.current_state == STATE_REWARD_SCREEN) {
        // Hide victory overlay (transitioning away from victory state)
        if (g_victory_overlay) {
            HideVictoryOverlay(g_victory_overlay);
        }

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

            // After combat victory rewards, advance to next encounter
            if (g_game.is_combat_mode && g_game.current_enemy && g_game.current_enemy->is_defeated) {
                if (!g_game.current_act) {
                    d_LogError("Combat victory but no act!");
                    return;
                }

                // Advance to next encounter
                AdvanceEncounter(g_game.current_act);

                // Check if act is complete
                if (IsActComplete(g_game.current_act)) {
                    d_LogInfo("Act complete! Returning to menu.");
                    State_Transition(&g_game, STATE_MENU);
                    return;
                }

                // Get next encounter
                Encounter_t* next = GetCurrentEncounter(g_game.current_act);
                if (!next) {
                    d_LogError("Failed to get next encounter");
                    return;
                }

                d_LogInfoF("Next encounter: %s", GetEncounterTypeName(next->type));

                // If next is EVENT, show event preview
                if (next->type == ENCOUNTER_EVENT) {
                    State_Transition(&g_game, STATE_EVENT_PREVIEW);
                } else if (next->type == ENCOUNTER_ELITE || next->type == ENCOUNTER_BOSS) {
                    // Elite/Boss combat - show preview
                    State_Transition(&g_game, STATE_COMBAT_PREVIEW);
                } else {
                    // Normal combat - spawn enemy directly (no preview)
                    StartNextEncounter();
                }
            } else {
                // Practice mode - start new round
                Game_StartNewRound(&g_game);
                State_Transition(&g_game, STATE_BETTING);
            }
        }
    }

    // Create event preview when entering STATE_EVENT_PREVIEW
    if (previous_state != STATE_EVENT_PREVIEW && g_game.current_state == STATE_EVENT_PREVIEW) {
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
            return;
        }

        // Initialize preview state
        g_game.event_preview_timer = 3.0f;      // 3 second countdown
        Game_ResetRerollCost(&g_game);          // Reset to base cost
        g_game.event_rerolls_used = 0;          // Reset reroll counter

        // Update modal content (created once at init, content updated here)
        if (!g_event_preview_modal) {
            d_LogError("Event preview modal not initialized - skipping to event");
            State_Transition(&g_game, STATE_EVENT);
            return;
        }

        UpdateEventPreviewContent(g_event_preview_modal,
                                  d_StringPeek(g_current_event->title),
                                  Game_GetEventRerollCost(&g_game));
        ShowEventPreviewModal(g_event_preview_modal);
        d_LogInfoF("Event preview started: '%s' (3.0s timer)", d_StringPeek(g_current_event->title));
    }

    // Handle event preview modal input
    if (g_game.current_state == STATE_EVENT_PREVIEW && g_event_preview_modal && IsEventPreviewModalVisible(g_event_preview_modal)) {
        // Update fade animation
        UpdateEventPreviewModal(g_event_preview_modal, dt);

        // Note: Timer is decremented by State_UpdateEventPreview() in state.c

        // ENTER key (skip timer, proceed immediately)
        if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
            app.keyboard[SDL_SCANCODE_RETURN] = 0;
            app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;
            d_LogInfo("Event preview: ENTER pressed, proceeding");
            HideEventPreviewModal(g_event_preview_modal);
            State_Transition(&g_game, STATE_EVENT);
        }

        // Check for button clicks (IsButtonClicked handles press/release detection internally)
        // Reroll button
        if (IsButtonClicked(g_event_preview_modal->reroll_button)) {
                Player_t* player = Game_GetPlayerByID(1);  // Human player
                int cost = Game_GetEventRerollCost(&g_game);
                if (player && player->chips >= cost) {
                    // Deduct chips and track stats
                    player->chips -= cost;
                    Stats_RecordChipsSpentEventReroll(cost);
                    g_game.event_rerolls_used++;

                    d_LogInfoF("Event rerolled (cost: %d chips, total rerolls: %d)",
                              cost, g_game.event_rerolls_used);

                    // Double reroll cost for next reroll
                    Game_IncrementRerollCost(&g_game);

                    // Regenerate event (avoid repeating same event immediately)
                    // Copy previous title before destroying event (avoid use-after-free)
                    char previous_title[256] = {0};
                    if (g_current_event && g_current_event->title) {
                        strncpy(previous_title, d_StringPeek(g_current_event->title), sizeof(previous_title) - 1);
                    }

                    if (g_current_event) {
                        DestroyEvent(&g_current_event);
                    }

                    // Use GetDifferentEventFromPool to avoid immediate repeat
                    if (!g_tutorial_event_pool) {
                        d_LogError("Event reroll: Tutorial event pool not initialized!");
                        g_current_event = NULL;
                    } else {
                        const char* prev = (previous_title[0] != '\0') ? previous_title : NULL;
                        g_current_event = GetDifferentEventFromPool(g_tutorial_event_pool, prev);
                    }

                    // Update modal content (avoid destroy/recreate churn)
                    UpdateEventPreviewContent(g_event_preview_modal,
                                              d_StringPeek(g_current_event->title),
                                              Game_GetEventRerollCost(&g_game));

                    // Reset timer
                    g_game.event_preview_timer = 3.0f;

                    d_LogInfoF("New event: '%s' (next reroll cost: %d chips)",
                              d_StringPeek(g_current_event->title), Game_GetEventRerollCost(&g_game));
            } else {
                d_LogWarning("Not enough chips to reroll event");
                // TODO: Show popup notification
            }
        }

        // Continue button (skip timer, proceed immediately)
        if (IsButtonClicked(g_event_preview_modal->continue_button)) {
            d_LogInfo("Continue button clicked - proceeding to event");
            HideEventPreviewModal(g_event_preview_modal);
            State_Transition(&g_game, STATE_EVENT);
        }

        // Auto-proceed when timer hits 0
        if (g_game.event_preview_timer <= 0.0f) {
            d_LogInfo("Event preview: Timer expired, auto-proceeding");
            HideEventPreviewModal(g_event_preview_modal);
            State_Transition(&g_game, STATE_EVENT);
        }
    }

    // Entering STATE_EVENT (event already created by preview, or fallback if skipped preview)
    if (previous_state != STATE_EVENT && g_game.current_state == STATE_EVENT) {
        // Hide event preview modal (if it was shown)
        if (g_event_preview_modal && IsEventPreviewModalVisible(g_event_preview_modal)) {
            HideEventPreviewModal(g_event_preview_modal);
            d_LogInfo("Event preview modal hidden - transitioning to event");
        }

        // Event should already exist from preview phase
        if (!g_current_event) {
            d_LogWarning("Entering STATE_EVENT without event (preview skipped?) - creating fallback");
            g_current_event = CreatePostCombatEvent();
            if (!g_current_event) {
                d_LogError("Failed to create fallback event - skipping to next round");
                Game_StartNewRound(&g_game);
                State_Transition(&g_game, STATE_BETTING);
                return;
            }
        }

        // Show event modal
        if (g_event_modal && g_current_event) {
            ShowEventModal(g_event_modal, g_current_event);
            d_LogInfo("Event modal shown");
        }
    }

    // Handle event modal input
    if (g_game.current_state == STATE_EVENT && g_event_modal && IsEventModalVisible(g_event_modal)) {
        if (HandleEventModalInput(g_event_modal, g_human_player, dt)) {
            // Choice was selected - copy modal's selected_choice to event
            if (g_current_event) {
                int selected_idx = GetSelectedChoiceIndex(g_event_modal);
                if (selected_idx >= 0) {
                    SelectEventChoice(g_current_event, selected_idx);
                }
            }

            // Apply consequences via game logic layer
            if (g_current_event && g_human_player) {
                Game_ApplyEventConsequences(&g_game, g_current_event, g_human_player);
            }

            // Hide modal
            HideEventModal(g_event_modal);

            // Clean up event
            if (g_current_event) {
                DestroyEvent(&g_current_event);
            }

            // Event complete - advance to next encounter
            if (!g_game.current_act) {
                d_LogError("Event complete but no act!");
                return;
            }

            AdvanceEncounter(g_game.current_act);

            // Check if act is complete
            if (IsActComplete(g_game.current_act)) {
                d_LogInfo("Tutorial act complete! Returning to menu.");
                State_Transition(&g_game, STATE_MENU);
                return;
            }

            // Get next encounter and start it
            Encounter_t* next = GetCurrentEncounter(g_game.current_act);
            if (!next) {
                d_LogError("Failed to get next encounter after event");
                return;
            }

            d_LogInfoF("After event, next encounter: %s", GetEncounterTypeName(next->type));

            // Start next encounter (with combat preview for elite/boss)
            if (next->type == ENCOUNTER_ELITE || next->type == ENCOUNTER_BOSS) {
                // Elite/Boss combat - show preview
                State_Transition(&g_game, STATE_COMBAT_PREVIEW);
            } else if (next->type == ENCOUNTER_NORMAL) {
                // Normal combat - spawn directly (no preview)
                StartNextEncounter();
            } else {
                // Unexpected type after event
                d_LogWarning("Next encounter after event is not combat (unexpected)");
                State_Transition(&g_game, STATE_BETTING);
            }
        }
    }

    // Create combat preview when entering STATE_COMBAT_PREVIEW
    // Skip this if coming from intro narrative (modal already set up manually)
    if (previous_state != STATE_COMBAT_PREVIEW && g_game.current_state == STATE_COMBAT_PREVIEW
        && previous_state != STATE_INTRO_NARRATIVE) {
        // Get current encounter
        Encounter_t* encounter = GetCurrentEncounter(g_game.current_act);
        if (!encounter || encounter->enemy_key[0] == '\0') {
            d_LogError("Invalid combat encounter for preview");
            StartNextEncounter();  // Fallback: spawn directly
            return;
        }

        // Get enemy name and type
        const char* encounter_type = (encounter->type == ENCOUNTER_ELITE) ? "Elite Combat" :
                                      (encounter->type == ENCOUNTER_BOSS) ? "Boss Fight" : "Combat";

        // Create temporary enemy to get name (will be destroyed and recreated on continue)
        Enemy_t* temp_enemy = LoadEnemyFromDUF(g_enemies_db, encounter->enemy_key);
        const char* enemy_name = temp_enemy ? GetEnemyName(temp_enemy) : "Unknown";

        // Initialize preview state
        g_game.combat_preview_timer = 3.0f;  // 3 second countdown

        // Update modal content
        if (!g_combat_preview_modal) {
            d_LogError("Combat preview modal not initialized - skipping preview");
            if (temp_enemy) DestroyEnemy(&temp_enemy);
            StartNextEncounter();
            return;
        }

        UpdateCombatPreviewContent(g_combat_preview_modal, enemy_name, encounter_type);
        ShowCombatPreviewModal(g_combat_preview_modal);

        // Cleanup temp enemy
        if (temp_enemy) {
            DestroyEnemy(&temp_enemy);
        }

        d_LogInfoF("Combat preview started: '%s - %s' (3.0s timer)", encounter_type, enemy_name);
    }

    // Handle combat preview modal input
    if (g_game.current_state == STATE_COMBAT_PREVIEW && g_combat_preview_modal && IsCombatPreviewModalVisible(g_combat_preview_modal)) {
        // Update fade animation
        UpdateCombatPreviewModal(g_combat_preview_modal, dt);

        // Decrement timer
        g_game.combat_preview_timer -= dt;

        // ENTER key (skip timer, proceed immediately)
        if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
            app.keyboard[SDL_SCANCODE_RETURN] = 0;
            app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;
            d_LogInfo("Combat preview: ENTER pressed, proceeding");
            HideCombatPreviewModal(g_combat_preview_modal);
            StartNextEncounter();  // Spawn enemy and start combat
        }

        // Continue button (skip timer, proceed immediately)
        if (IsButtonClicked(g_combat_preview_modal->continue_button)) {
            d_LogInfo("Combat preview: Continue clicked");
            HideCombatPreviewModal(g_combat_preview_modal);
            StartNextEncounter();  // Spawn enemy and start combat
        }

        // Auto-proceed when timer hits 0
        if (g_game.combat_preview_timer <= 0.0f) {
            d_LogInfo("Combat preview: Timer expired, auto-proceeding");
            HideCombatPreviewModal(g_combat_preview_modal);
            StartNextEncounter();  // Spawn enemy and start combat
        }
    }

    // Intro narrative modal is shown immediately in InitBlackjackScene
    // No need to show it again here

    // (Intro narrative modal input handling moved to line ~702 for priority)

    // Trigger tutorial events on state changes
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system)) {
        if (g_game.current_state != previous_state) {
            TriggerTutorialEvent(g_tutorial_system, TUTORIAL_EVENT_STATE_CHANGE,
                                (void*)(intptr_t)g_game.current_state);
        }
    }

    // Update trinket hover state ALWAYS (before state-specific input handling)
    // This allows tooltips to appear at any time, while clicks are gated by STATE_PLAYER_TURN
    if (g_trinket_ui && g_human_player && g_human_player->trinket_slots) {
        UpdateTrinketUIHover(g_trinket_ui, g_human_player);
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
// Trinket input handling moved to trinketUI.c

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

    // For Degenerate's Gambit: card rank 2-9 (excluding Aces and 10+ face cards)
    if (trinket->trinket_id == 0) {
        return card->rank >= RANK_TWO && card->rank <= RANK_NINE;
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
                snprintf(g_popup_message, sizeof(g_popup_message), "This ability targets rank 2-9 only");
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
                snprintf(g_popup_message, sizeof(g_popup_message), "This ability targets rank 2-9 only");
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

    // Apply sanity threshold effects to betting
    int bet_amounts[3] = {BET_AMOUNT_MIN, BET_AMOUNT_MED, BET_AMOUNT_MAX};
    bool sanity_enabled[3] = {true, true, true};
    ApplySanityEffectsToBetting(g_human_player, bet_amounts, sanity_enabled);

    // Update button labels to show modified bet amounts
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i]) continue;

        // Update label to show actual bet amount (may be modified by sanity)
        // If player can't afford this bet, show "All In" instead
        dString_t* label = d_StringInit();
        const char* bet_names[] = {"Min", "Med", "Max"};

        if (g_human_player->chips < bet_amounts[i]) {
            // Can't afford this bet - show "All In" with current chips
            d_StringFormat(label, "All In (%d)", g_human_player->chips);
        } else {
            // Can afford - show normal label
            d_StringFormat(label, "%s (%d)", bet_names[i], bet_amounts[i]);
        }

        SetButtonLabel(bet_buttons[i], d_StringPeek(label));
        d_StringDestroy(label);
    }

    // Calculate which buttons are visible (sanity allows AND can afford)
    bool button_visible[3];
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        button_visible[i] = sanity_enabled[i] && CanAffordBet(g_human_player, bet_amounts[i]);
    }

    // Rebuild action panel layout with only visible buttons (dynamic sizing)
    RebuildActionPanelLayout(g_betting_panel, button_visible, 500);  // 500px available width

    // NOTE: Don't call UpdateActionPanelButtons() here!
    // Button positions will be updated in RenderActionPanel() after FlexBox bounds are set

    // Set enabled/selected states
    for (int i = 0; i < NUM_BET_BUTTONS; i++) {
        if (!bet_buttons[i]) continue;
        SetButtonEnabled(bet_buttons[i], button_visible[i]);
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
            // If player can't afford bet amount, bet all chips instead (All In)
            int actual_bet = (g_human_player->chips < bet_amounts[selected_bet_button])
                ? g_human_player->chips
                : bet_amounts[selected_bet_button];
            Game_ProcessBettingInput(&g_game, g_human_player, actual_bet);
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
            // If player can't afford bet amount, bet all chips instead (All In)
            int actual_bet = (g_human_player->chips < bet_amounts[i])
                ? g_human_player->chips
                : bet_amounts[i];
            Game_ProcessBettingInput(&g_game, g_human_player, actual_bet);
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
        // If player can't afford bet amount, bet all chips instead (All In)
        int actual_bet = (g_human_player->chips < bet_amounts[choice])
            ? g_human_player->chips
            : bet_amounts[choice];
        d_LogInfoF("⌨️ KEYBOARD %d released - betting %d", choice + 1, actual_bet);
        Game_ProcessBettingInput(&g_game, g_human_player, actual_bet);
    }
}

static void HandlePlayerTurnInput(void) {
    if (!g_human_player) return;

    // Handle trinket clicks (hover state already updated in main logic loop)
    if (g_trinket_ui && g_human_player->trinket_slots) {
        HandleTrinketUIInput(g_trinket_ui, g_human_player, &g_game);
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

VisualEffects_t* GetVisualEffects(void) {
    return g_visual_effects;
}

// ============================================================================
// Visual effects now in visualEffects.c
// ============================================================================




// ============================================================================
// Trinket tooltip rendering now in trinketUI.c


// ============================================================================
// Trinket UI rendering now in trinketUI.c


// ============================================================================
// SCENE RENDERING
// ============================================================================

static void BlackjackDraw(float dt) {
    (void)dt;

    // Show loading screen during initialization
    if (g_is_loading) {
        // Black background
        a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (aColor_t){0, 0, 0, 255});

        // "Loading..." text (centered)
        aTextStyle_t loading_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {235, 237, 233, 255},  // Off-white
            .bg = {0, 0, 0, 0},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.5f,
            .padding = 0
        };
        a_DrawText("Loading...", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2, loading_config);
        return;  // Skip all other rendering
    }

    // Apply screen shake offset to entire viewport
    if (g_visual_effects) {
        ApplyScreenShakeViewport(g_visual_effects);
    }

    // Draw full-screen background surface first
    if (g_background_texture) {
        a_BlitSurfaceRect(g_background_texture, (aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, 1);
    } else {
        // Fallback to black if texture didn't load
        app.background = (aColor_t){0, 0, 0, 255};
    }

    // Draw enemy portrait as background (if in combat)
    if (g_game.is_combat_mode && g_enemy_portrait_renderer) {
        RenderEnemyPortrait(g_enemy_portrait_renderer);
    }

    // Draw blackjack table sprite at bottom (on top of portrait)
    if (g_table_texture) {
        // Use reasonable dimensions for the table
        int table_width = 1024;
        int table_height = 256;

        // Center horizontally in game area, position at bottom
        int table_x = GAME_AREA_X + (GAME_AREA_WIDTH - table_width) / 2;
        int table_y = SCREEN_HEIGHT - table_height;

        aRectf_t dest = {
            table_x,
            table_y,
            table_width,
            table_height
        };
        a_BlitSurfaceRect(g_table_texture, dest, 1);
    }

    // Skip all game UI rendering during intro narrative (only show modal + overlay)
    if (g_game.current_state != STATE_INTRO_NARRATIVE) {
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
            case STATE_COMBAT_VICTORY:
                if (g_result_screen && g_human_player) {
                    RenderResultScreen(g_result_screen, g_human_player, g_game.current_state);
                }
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
    if (g_trinket_ui && g_human_player) {
        RenderTrinketUI(g_trinket_ui, g_human_player);
    }

    // Render trinket tooltips AFTER card tooltips for proper z-ordering
    if (g_trinket_ui && g_human_player) {
        RenderTrinketTooltips(g_trinket_ui, g_human_player);
    }

    // Render card tooltips AFTER trinket UI for proper z-ordering
    if (g_dealer_section) {
        RenderDealerSectionTooltip(g_dealer_section);
    }
    if (g_player_section) {
        RenderPlayerSectionTooltip(g_player_section);
    }

    // Render targeting arrow (if in targeting mode)
    RenderTargetingArrow();

    // Render victory overlay component (extracted from inline code for maintainability)
    if (g_victory_overlay && IsVictoryOverlayVisible(g_victory_overlay)) {
        RenderVictoryOverlay(g_victory_overlay);
        // FlexBox result screen shows winnings + "Cleansed!" message (rendered by resultScreen.c)
    }

    // Render reward modal (if visible) - BEFORE pause menu
    if (g_reward_modal) {
        RenderRewardModal(g_reward_modal);
    }

    // Render event preview modal (if visible) - BEFORE pause menu
    if (g_event_preview_modal) {
        RenderEventPreviewModal(g_event_preview_modal, &g_game);
    }

    // Render combat preview modal (if visible) - BEFORE pause menu
    if (g_combat_preview_modal) {
        RenderCombatPreviewModal(g_combat_preview_modal, &g_game);
    }

    }  // End skip game UI during intro narrative

    // Render intro narrative modal (if visible) with dark background
    if (g_intro_narrative_modal && IsIntroNarrativeModalVisible(g_intro_narrative_modal)) {
        // Dark semi-transparent overlay (50% black)
        a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (aColor_t){0, 0, 0, 128});

        // Render modal (has its own fade-in)
        RenderIntroNarrativeModal(g_intro_narrative_modal);
    }

    // Render deck/discard modals
    if (g_draw_pile_modal) {
        RenderDrawPileModalSection(g_draw_pile_modal);
    }
    if (g_discard_pile_modal) {
        RenderDiscardPileModalSection(g_discard_pile_modal);
    }

    // Render event modal (if visible) - matches RewardModal pattern
    if (g_event_modal) {
        RenderEventModal(g_event_modal, g_human_player);
    }

    // Render tutorial if active - skip during intro narrative
    if (g_tutorial_system && IsTutorialActive(g_tutorial_system) &&
        g_game.current_state != STATE_INTRO_NARRATIVE) {
        RenderTutorial(g_tutorial_system);
    }

    // Render pause menu if visible (on top of everything, including tutorial)
    if (g_pause_menu) {
        RenderPauseMenuSection(g_pause_menu);
    }

    // Render damage numbers (floating combat text)
    if (g_visual_effects) {
        RenderDamageNumbers(g_visual_effects);
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
        a_DrawFilledRect((aRectf_t){box_x, box_y, box_width, box_height}, (aColor_t){16, 20, 31, bg_alpha});

        // Draw text (#a53030 red with fade)
        aTextStyle_t popup_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {165, 48, 48, (Uint8)(g_popup_alpha * 255)},  // #a53030 with fade
            .bg = {0, 0, 0, 0},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.2f,
            .padding = 0
        };
        a_DrawText(g_popup_message, SCREEN_WIDTH / 2, (int)g_popup_y, popup_config);
    }

    // Render terminal overlay (highest layer - above tutorial)
    if (g_terminal) {
        RenderTerminal(g_terminal);
    }
}

// ============================================================================
// Result screen rendering now in resultScreen.c

