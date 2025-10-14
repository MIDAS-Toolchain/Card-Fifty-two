# Scene Architecture

## Overview

Scenes are self-contained game states (Menu, Blackjack Game, Settings) that **orchestrate sections** and manage game logic. Card Fifty-Two uses **Archimedes' delegate pattern** for scene switching.

**Key Principle:** Scenes should orchestrate sections rather than rendering directly. Sections handle rendering, scenes handle state machines and input routing.

---

## Scene Lifecycle

### Archimedes Delegate Pattern

```c
// In Archimedes.h
typedef struct {
    void (*logic)(float dt);   // Scene-specific logic function
    void (*draw)(float dt);    // Scene-specific render function
} Delegate_t;

extern struct App {
    Delegate_t delegate;
    int running;
    // ... other fields
} app;
```

**Concept:** Main loop delegates logic and rendering to the active scene's functions.

### Scene Structure

```c
// src/scenes/sceneBlackjack.c

// Scene-local state (static to this file)
static GameContext_t g_game;
static Player_t* g_dealer = NULL;
static Player_t* g_human_player = NULL;
static FlexBox_t* g_main_layout = NULL;

// UI Sections (see 03_sections.md)
static TitleSection_t* g_title_section = NULL;
static DealerSection_t* g_dealer_section = NULL;
static PlayerSection_t* g_player_section = NULL;
static ActionPanel_t* g_betting_panel = NULL;
static ActionPanel_t* g_action_panel = NULL;

// Reusable Button components
static Button_t* bet_buttons[4] = {NULL};
static Button_t* action_buttons[3] = {NULL};

// Forward declarations
static void BlackjackLogic(float dt);
static void BlackjackDraw(float dt);
static void CleanupBlackjackScene(void);

// Initialization (called from menu or main)
void InitBlackjackScene(void) {
    // 1. Set delegates (Archimedes pattern)
    app.delegate.logic = BlackjackLogic;
    app.delegate.draw = BlackjackDraw;

    // 2. Initialize scene-local state
    g_dealer = CreatePlayer("Dealer", 0, true);
    g_human_player = CreatePlayer("Player", 1, false);
    InitGameContext(&g_game, &g_test_deck);

    // 3. Initialize layout (FlexBox + Sections)
    InitializeLayout();

    // 4. Transition to initial state
    TransitionState(&g_game, STATE_BETTING);

    d_LogInfo("Blackjack scene initialized");
}

// Logic (called every frame by main loop)
static void BlackjackLogic(float dt) {
    a_DoInput();  // Process SDL events (REQUIRED!)

    // Handle ESC key
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        CleanupBlackjackScene();
        InitMenuScene();  // Switch scene
        return;
    }

    // State-specific logic
    switch (g_game.current_state) {
        case STATE_BETTING:
            HandleBettingInput();
            break;
        case STATE_PLAYER_TURN:
            HandlePlayerTurnInput();
            break;
        // ...
    }
}

// Rendering (called every frame by main loop)
static void BlackjackDraw(float dt) {
    app.background = (aColor_t){10, 80, 30, 255};  // Dark green

    // Get FlexBox-calculated positions for all sections
    if (g_main_layout) {
        a_FlexLayout(g_main_layout);

        int title_y = a_FlexGetItemY(g_main_layout, 0);
        int dealer_y = a_FlexGetItemY(g_main_layout, 1);
        int player_y = a_FlexGetItemY(g_main_layout, 2);
        int button_y = a_FlexGetItemY(g_main_layout, 3);

        // Render sections (sections handle all rendering!)
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
                RenderResultOverlay();  // Full-screen overlay
                break;
        }
    }

    // FPS counter
    dString_t* fps_str = d_InitString();
    d_FormatString(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_PeekString(fps_str), SCREEN_WIDTH - 10, 10,
               0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
    d_DestroyString(fps_str);
}

// Cleanup (called before switching scenes)
static void CleanupBlackjackScene(void) {
    CleanupLayout();
    CleanupGameContext(&g_game);
    DestroyPlayer(&g_dealer);
    DestroyPlayer(&g_human_player);
}
```

---

## Scene Switching

### Pattern

```c
void SwitchToMenuScene(void) {
    // 1. Cleanup current scene
    CleanupBlackjackScene();

    // 2. Initialize new scene (sets new delegates)
    InitMenuScene();
}
```

**Key Point:** `InitXXXScene()` sets `app.delegate.logic` and `app.delegate.draw`, so the main loop automatically starts calling the new scene's functions.

### Main Loop (in main.c)

```c
while (app.running) {
    a_PrepareScene();
    float dt = a_GetDeltaTime();

    app.delegate.logic(dt);  // Calls active scene's logic function
    app.delegate.draw(dt);   // Calls active scene's render function

    a_PresentScene();
}
```

**Delegate Resolution:**
- If `InitMenuScene()` was last called: `app.delegate.logic = MenuLogic`
- If `InitBlackjackScene()` was last called: `app.delegate.logic = BlackjackLogic`

---

## Scene Organization

### Directory Structure

```
src/scenes/
├── sceneMenu.c          # Main menu (play, settings, quit)
├── sceneBlackjack.c     # Blackjack game scene
├── sceneSettings.c      # Settings menu
└── components/          # Shared UI components
    └── button.c         # Reusable button widget

include/scenes/
├── sceneMenu.h
├── sceneBlackjack.h
├── sceneSettings.h
└── components/
    └── button.h
```

### Scene Header (sceneBlackjack.h)

```c
#ifndef SCENE_BLACKJACK_H
#define SCENE_BLACKJACK_H

// Public initialization function (called from other scenes)
void InitBlackjackScene(void);

#endif // SCENE_BLACKJACK_H
```

**Note:** Logic, draw, and cleanup functions are `static` (private to scene file).

---

## Scene State Management

### State Machines Within Scenes

```c
typedef enum {
    STATE_BETTING,
    STATE_DEALING,
    STATE_PLAYER_TURN,
    STATE_DEALER_TURN,
    STATE_ROUND_END
} GameState_t;

static GameContext_t g_game;  // Contains current_state

void TransitionState(GameContext_t* game, GameState_t new_state) {
    d_LogInfoF("State: %s -> %s",
               StateToString(game->current_state),
               StateToString(new_state));

    game->current_state = new_state;

    // State entry logic
    switch (new_state) {
        case STATE_DEALING:
            DealInitialCards(game);
            break;
        case STATE_DEALER_TURN:
            DealerPlay(game);
            break;
        // ...
    }
}
```

**Pattern:** Each scene manages its own internal state machine.

---

## Scene Rendering Patterns

### Section-Based Rendering (Current Architecture)

**Principle:** Scenes orchestrate sections, sections handle actual rendering.

```c
static void BlackjackDraw(float dt) {
    // Layer 0: Background
    app.background = (aColor_t){10, 80, 30, 255};

    // Layer 1: Section-based rendering (FlexBox positions)
    if (g_main_layout) {
        a_FlexLayout(g_main_layout);

        int title_y = a_FlexGetItemY(g_main_layout, 0);
        int dealer_y = a_FlexGetItemY(g_main_layout, 1);
        int player_y = a_FlexGetItemY(g_main_layout, 2);
        int button_y = a_FlexGetItemY(g_main_layout, 3);

        // Sections handle all rendering internally
        RenderTitleSection(g_title_section, &g_game, title_y);
        RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
        RenderPlayerSection(g_player_section, g_human_player, player_y);

        // State-specific section
        switch (g_game.current_state) {
            case STATE_BETTING:
                RenderActionPanel(g_betting_panel, button_y);
                break;
            case STATE_PLAYER_TURN:
                RenderActionPanel(g_action_panel, button_y);
                break;
        }
    }

    // Layer 2: Full-screen overlays (outside FlexBox)
    if (g_game.current_state == STATE_ROUND_END) {
        RenderResultOverlay();  // Covers entire screen
    }

    // Layer 3: Debug/HUD (always on top)
    dString_t* fps_str = d_InitString();
    d_FormatString(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_PeekString(fps_str), SCREEN_WIDTH - 10, 10, ...);
    d_DestroyString(fps_str);
}
```

**Benefits:**
- FlexBox guarantees no overlap between sections
- Sections encapsulate rendering logic
- Scene remains clean orchestration code
- Easy to add/remove/reorder sections

**See Also:** `03_sections.md` for section implementation details

---

## FlexBox Layout in Scenes

### Layout Constants

**Defined in:** `include/scenes/sceneBlackjack.h`

```c
// Layout dimensions
#define LAYOUT_TOP_MARGIN       0      // Top edge margin (no gap needed)
#define LAYOUT_BOTTOM_CLEARANCE 30     // Bottom clearance (IMPORTANT!)
#define LAYOUT_GAP              10     // Gap between sections
#define TOP_BAR_HEIGHT          35
#define TITLE_AREA_HEIGHT       30
#define DEALER_AREA_HEIGHT      165
#define PLAYER_AREA_HEIGHT      240
#define BUTTON_AREA_HEIGHT      100
```

**Why LAYOUT_BOTTOM_CLEARANCE = 30?**
- Prevents UI elements from being cramped against the screen bottom edge
- Provides visual breathing room for button sections
- Ensures text and buttons remain fully visible (not cut off)
- Was previously 0, causing layout issues fixed in recent improvements

**FlexBox Height Calculation:**
```c
// Total available height for FlexBox:
int flexbox_height = SCREEN_HEIGHT - LAYOUT_TOP_MARGIN - LAYOUT_BOTTOM_CLEARANCE;
// Example: 720 - 0 - 30 = 690px available for sections
```

### Layout Initialization (Section-Based Architecture)

```c
static FlexBox_t* g_main_layout = NULL;

// UI Sections
static TitleSection_t* g_title_section = NULL;
static DealerSection_t* g_dealer_section = NULL;
static PlayerSection_t* g_player_section = NULL;
static ActionPanel_t* g_betting_panel = NULL;
static ActionPanel_t* g_action_panel = NULL;

// Reusable Button components (owned by scene, referenced by panels)
static Button_t* bet_buttons[4] = {NULL};
static Button_t* action_buttons[3] = {NULL};

static void InitializeLayout(void) {
    // Main vertical FlexBox (4 sections)
    // LAYOUT_BOTTOM_CLEARANCE = 30 ensures proper spacing from screen bottom edge
    // Prevents UI elements from being cut off or cramped at bottom
    g_main_layout = a_CreateFlexBox(0, LAYOUT_TOP_MARGIN,
                                     SCREEN_WIDTH,
                                     SCREEN_HEIGHT - LAYOUT_BOTTOM_CLEARANCE);
    a_FlexSetDirection(g_main_layout, FLEX_DIR_COLUMN);
    a_FlexSetJustify(g_main_layout, FLEX_JUSTIFY_SPACE_BETWEEN);
    a_FlexSetGap(g_main_layout, LAYOUT_GAP);

    // Add 4 FlexBox items (FlexBox calculates positions automatically!)
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, TITLE_AREA_HEIGHT, NULL);
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, DEALER_AREA_HEIGHT, NULL);
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, PLAYER_AREA_HEIGHT, NULL);
    a_FlexAddItem(g_main_layout, SCREEN_WIDTH, BUTTON_AREA_HEIGHT, NULL);

    a_FlexLayout(g_main_layout);  // Calculate positions

    // Create UI sections (see 03_sections.md)
    g_title_section = CreateTitleSection();
    g_dealer_section = CreateDealerSection();
    g_player_section = CreatePlayerSection();

    // Create betting buttons
    const char* bet_labels[] = {"10", "25", "50", "100"};
    const char* bet_hotkeys[] = {"[1]", "[2]", "[3]", "[4]"};
    for (int i = 0; i < 4; i++) {
        bet_buttons[i] = CreateButton(0, 0, BET_BUTTON_WIDTH,
                                      BET_BUTTON_HEIGHT, bet_labels[i]);
        SetButtonHotkey(bet_buttons[i], bet_hotkeys[i]);
    }

    // Create betting panel (references bet_buttons, doesn't own them)
    g_betting_panel = CreateActionPanel("Place Your Bet:", bet_buttons, 4);

    // Create action buttons
    const char* action_labels[] = {"Hit", "Stand", "Double"};
    const char* action_hotkeys[] = {"[H]", "[S]", "[D]"};
    for (int i = 0; i < 3; i++) {
        action_buttons[i] = CreateButton(0, 0, ACTION_BUTTON_WIDTH,
                                         ACTION_BUTTON_HEIGHT, action_labels[i]);
        SetButtonHotkey(action_buttons[i], action_hotkeys[i]);
    }

    // Create action panel (references action_buttons, doesn't own them)
    g_action_panel = CreateActionPanel("Your Turn - Choose Action:",
                                       action_buttons, 3);
}
```

**Key Changes from Old Architecture:**
- FlexBox defines 4 sections instead of 3 (added TitleSection)
- Sections handle their own rendering (no inline code in SceneDraw)
- ActionPanel is reusable (betting + player actions use same section type)
- Scene owns buttons, panels reference them

### Layout Cleanup

```c
static void CleanupLayout(void) {
    // Destroy UI sections (sections destroy their own FlexBoxes)
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

    // Destroy main FlexBox
    if (g_main_layout) {
        a_DestroyFlexBox(&g_main_layout);
    }

    // Destroy button components (panels don't own these!)
    for (int i = 0; i < 4; i++) {
        if (bet_buttons[i]) {
            DestroyButton(&bet_buttons[i]);
        }
    }
    for (int i = 0; i < 3; i++) {
        if (action_buttons[i]) {
            DestroyButton(&action_buttons[i]);
        }
    }
}
```

**Ownership Pattern:**
- Scene owns: main FlexBox, sections, buttons
- Sections own: their internal FlexBoxes, dStrings
- Sections reference but don't own: buttons passed to them
- Cleanup order: sections → main FlexBox → components

---

## Input Handling in Scenes

### Archimedes Input System

```c
static void BlackjackLogic(float dt) {
    a_DoInput();  // REQUIRED! Processes SDL events

    // Keyboard input
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;  // Clear flag
        TransitionToMenu();
    }

    // Mouse input
    if (app.mouse.pressed) {
        int mx = app.mouse.x;
        int my = app.mouse.y;
        // Check button clicks
    }
}
```

### Input Handling Patterns

```c
// Keyboard shortcut with debounce
static bool h_was_pressed = false;
if (app.keyboard[SDL_SCANCODE_H] && !h_was_pressed) {
    ExecutePlayerAction(ACTION_HIT);
    h_was_pressed = true;
} else if (!app.keyboard[SDL_SCANCODE_H]) {
    h_was_pressed = false;
}

// OR: Clear flag pattern (simpler)
if (app.keyboard[SDL_SCANCODE_H]) {
    app.keyboard[SDL_SCANCODE_H] = 0;  // Prevent repeat
    ExecutePlayerAction(ACTION_HIT);
}
```

---

## Scene Communication

### Passing Data Between Scenes

```c
// Global state (in main.c or common.h)
extern int g_player_chips;
extern GameSettings_t g_settings;

// From menu to game
void InitBlackjackScene(void) {
    g_human_player = CreatePlayer("Player", 1, false);
    g_human_player->chips = g_player_chips;  // Load from global state
}

// From game back to menu
static void CleanupBlackjackScene(void) {
    g_player_chips = g_human_player->chips;  // Save to global state
    DestroyPlayer(&g_human_player);
}
```

**Alternative: Scene Context Struct**

```c
typedef struct {
    int player_chips;
    int difficulty;
    bool sound_enabled;
} GameContext_t;

extern GameContext_t g_shared_context;

void InitBlackjackScene(void) {
    // Read from shared context
}

void CleanupBlackjackScene(void) {
    // Write back to shared context
}
```

---

## Example Scenes

### Menu Scene (Simple)

```c
// src/scenes/sceneMenu.c

static int selected_option = 0;
static const char* menu_options[] = {"Play", "Settings", "Quit"};

void InitMenuScene(void) {
    app.delegate.logic = MenuLogic;
    app.delegate.draw = MenuDraw;
    selected_option = 0;
}

static void MenuLogic(float dt) {
    a_DoInput();

    if (app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_UP] = 0;
        selected_option = (selected_option - 1 + 3) % 3;
    }

    if (app.keyboard[SDL_SCANCODE_RETURN]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        switch (selected_option) {
            case 0: InitBlackjackScene(); break;
            case 1: InitSettingsScene(); break;
            case 2: app.running = 0; break;
        }
    }
}

static void MenuDraw(float dt) {
    app.background = (aColor_t){20, 20, 40, 255};

    a_DrawText("Card Fifty-Two", SCREEN_WIDTH / 2, 100,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    for (int i = 0; i < 3; i++) {
        aColor_t color = (i == selected_option) ?
            (aColor_t){255, 215, 0, 255} :    // Gold if selected
            (aColor_t){180, 180, 180, 255};   // Gray otherwise

        int y = 300 + (i * 60);
        a_DrawText((char*)menu_options[i], SCREEN_WIDTH / 2, y,
                   color.r, color.g, color.b, FONT_ENTER_COMMAND,
                   TEXT_ALIGN_CENTER, 0);
    }
}
```

---

### Settings Scene (Placeholder)

**Purpose:** Display settings menu for future configuration options.

```c
// src/scenes/sceneSettings.c

void InitSettingsScene(void) {
    app.delegate.logic = SettingsLogic;
    app.delegate.draw = SettingsDraw;
}

static void SettingsLogic(float dt) {
    a_DoInput();

    // ESC - Return to menu
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        InitMenuScene();
        return;
    }
}

static void SettingsDraw(float dt) {
    app.background = (aColor_t){30, 30, 50, 255};  // Dark blue-gray

    // Title
    a_DrawText("SETTINGS", SCREEN_WIDTH / 2, 150,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Placeholder settings
    a_DrawText("Coming Soon:", SCREEN_WIDTH / 2, 300,
               200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("- Sound Volume", SCREEN_WIDTH / 2, 360,
               180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("- Music Volume", SCREEN_WIDTH / 2, 410,
               180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("- Fullscreen Toggle", SCREEN_WIDTH / 2, 460,
               180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Instructions
    a_DrawText("[ESC] to return to menu",
               SCREEN_WIDTH / 2, SCREEN_HEIGHT - 50,
               150, 150, 150, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
}
```

**Key Patterns:**
- Minimal scene (no sections, no FlexBox needed for placeholder)
- Direct rendering for simplicity
- ESC key returns to menu
- Dark background to differentiate from main menu

**Future Expansion:**
- Add SettingsSection with actual configuration options
- Use components (sliders, checkboxes, dropdowns)
- Persist settings to file using Daedalus data structures

---

## Best Practices

### ✅ DO

1. **Use static for scene-local state**
   ```c
   static GameContext_t g_game;  // Only this scene can access
   ```

2. **Orchestrate sections, don't render directly**
   ```c
   // ✅ GOOD - Scene orchestrates sections
   static void SceneDraw(float dt) {
       RenderTitleSection(g_title_section, &g_game, title_y);
       RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
   }

   // ❌ BAD - Scene renders directly
   static void SceneDraw(float dt) {
       a_DrawText(g_dealer->name, x, y, ...);
       RenderHand(&g_dealer->hand, x, y + 50);
   }
   ```

3. **Initialize layout in InitScene**
   ```c
   void InitBlackjackScene(void) {
       InitializeLayout();  // Create FlexBoxes, sections, buttons
   }
   ```

4. **Call a_DoInput() in every logic function**
   ```c
   static void SceneLogic(float dt) {
       a_DoInput();  // REQUIRED for keyboard/mouse to work
   }
   ```

5. **Cleanup in reverse order of initialization**
   ```c
   static void CleanupScene(void) {
       CleanupLayout();         // Sections, buttons, FlexBoxes
       CleanupGameContext();    // Game state
       DestroyPlayers();        // Players last
   }
   ```

6. **Use state machines for complex scene flow**
   ```c
   switch (g_game.current_state) { /* ... */ }
   ```

### ❌ DON'T

1. **Don't forget to set delegates**
   ```c
   void InitScene(void) {
       // REQUIRED!
       app.delegate.logic = SceneLogic;
       app.delegate.draw = SceneDraw;
   }
   ```

2. **Don't skip cleanup**
   ```c
   void SwitchScene(void) {
       CleanupOldScene();  // Memory leak if skipped!
       InitNewScene();
   }
   ```

3. **Don't access other scene's static variables**
   ```c
   // sceneMenu.c
   extern static int g_selected_option;  // ERROR: Can't access!
   ```

4. **Don't render in logic functions**
   ```c
   static void SceneLogic(float dt) {
       a_DrawText("Test", 0, 0, ...);  // NO! Do this in SceneDraw
   }
   ```

5. **Don't render directly in scenes (use sections)**
   ```c
   // ❌ BAD - Scene has rendering logic
   static void SceneDraw(float dt) {
       a_DrawText(dealer->name, 100, dealer_y, ...);
       for (size_t i = 0; i < dealer->hand.cards->count; i++) {
           // 20 lines of card rendering code...
       }
   }

   // ✅ GOOD - Section handles rendering
   static void SceneDraw(float dt) {
       RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
   }
   ```

---

## Scene Checklist

When creating a new scene:

- [ ] Create `src/scenes/sceneXXX.c` and `include/scenes/sceneXXX.h`
- [ ] Define `static void XXXLogic(float dt)`
- [ ] Define `static void XXXDraw(float dt)`
- [ ] Define `static void CleanupXXXScene(void)`
- [ ] Implement `void InitXXXScene(void)` (public, in header)
- [ ] Set `app.delegate.logic` and `app.delegate.draw` in Init
- [ ] Call `a_DoInput()` in logic function
- [ ] Handle ESC key to return to menu
- [ ] Initialize FlexBox layout with sections (see 03_sections.md)
- [ ] Create sections for different UI areas
- [ ] Orchestrate sections in SceneDraw (don't render directly!)
- [ ] Cleanup sections, FlexBoxes, and components
- [ ] Add to Makefile (should auto-detect with wildcard)

---

## Summary

**Scene Pattern (Section-Based Architecture):**
```c
void InitScene(void) {
    app.delegate.logic = SceneLogic;
    app.delegate.draw = SceneDraw;

    // Initialize sections
    g_title_section = CreateTitleSection();
    g_dealer_section = CreateDealerSection();
    // ...
}

static void SceneLogic(float dt) {
    a_DoInput();
    // Update state, route input
}

static void SceneDraw(float dt) {
    // Orchestrate sections (don't render directly!)
    RenderTitleSection(g_title_section, &g_game, title_y);
    RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
    // ...
}

static void CleanupScene(void) {
    // Destroy sections first
    DestroyTitleSection(&g_title_section);
    DestroyDealerSection(&g_dealer_section);
    // Then FlexBoxes, components, game state
}
```

**Key Concepts:**
- Delegate pattern for scene switching
- Static state for scene encapsulation
- **Section-based rendering** (scenes orchestrate, sections render)
- FlexBox layout for UI positioning (no overlap!)
- State machines for scene flow
- Cleanup before scene transitions

**Architecture Hierarchy:**
```
Scene (sceneBlackjack.c)
  ├─ Orchestrates Sections
  ├─ Manages game state
  └─ Routes input
      ↓
Sections (titleSection, dealerSection, etc.)
  ├─ Handle rendering
  ├─ Manage internal FlexBoxes
  └─ Reference components
      ↓
Components (Button)
  └─ Fully reusable primitives
```

**See Also:**
- `03_sections.md` - Section architecture details
- `02_components.md` - Component implementation patterns
- `00_rendering_pipeline.md` - Overall rendering flow

---

*Last Updated: 2025-10-07*
*Part of Card Fifty-Two Architecture Documentation*
