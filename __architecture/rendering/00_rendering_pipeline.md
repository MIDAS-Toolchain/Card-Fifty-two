# Rendering Pipeline Architecture

## Overview

Card Fifty-Two uses **Archimedes** (SDL2 game framework) for all rendering operations. The rendering pipeline follows Archimedes' frame-based architecture with a clear separation between logic and drawing.

**Current Architecture:** Scenes orchestrate **sections**, sections handle rendering. See `01_scenes.md`, `02_components.md`, and `03_sections.md` for architectural details.

---

## Frame Loop Structure

### Archimedes Frame Cycle

```c
// Main loop (in main.c)
while (app.running) {
    a_PrepareScene();              // 1. Clear screen, handle events

    float dt = a_GetDeltaTime();   // 2. Get delta time since last frame

    app.delegate.logic(dt);        // 3. Update game state (no drawing!)
    app.delegate.draw(dt);         // 4. Render everything

    a_PresentScene();              // 5. Swap buffers, show frame
}
```

### Key Principles

1. **Logic and Rendering are Separate**
   - `logic(dt)` updates state (positions, game logic, input)
   - `draw(dt)` renders state (NO state changes here!)

2. **Delta Time (dt)**
   - Time elapsed since last frame in seconds
   - Used for smooth animations and frame-rate independence
   - Example: `position += velocity * dt`

3. **Double Buffering**
   - `a_PrepareScene()` prepares back buffer
   - `a_PresentScene()` swaps buffers (vsync if enabled)

---

## Rendering Order (Back to Front)

### Current Architecture: Section-Based Rendering

**Rendering happens in layers, with sections handling most of the work:**

```c
void SceneDraw(float dt) {
    // Layer 0: Background
    app.background = TABLE_FELT_GREEN;

    // Layer 1: Section-based rendering (FlexBox-positioned)
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
        RenderResultOverlay();
    }

    // Layer 3: Debug/HUD (always on top)
    dString_t* fps_str = d_InitString();
    d_FormatString(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText((char*)d_PeekString(fps_str), SCREEN_WIDTH - 10, 10, ...);
    d_DestroyString(fps_str);
}
```

### Layer Breakdown

**Layer 0: Background**
- Solid color or texture fill
- Set via `app.background`

**Layer 1: Sections (FlexBox-positioned)**
- TitleSection: Game title + state display
- DealerSection: Dealer name, score, cards
- PlayerSection: Player name, chips, bet, cards
- ActionPanel: Instructions + buttons (betting or actions)
- Each section handles its own rendering internally

**Layer 2: Full-Screen Overlays**
- Result screens (win/lose/blackjack)
- Pause menus
- Covers entire screen, outside FlexBox system

**Layer 3: HUD/Debug**
- FPS counter
- Debug visualizations
- Always on top, never obscured

---

## Archimedes Rendering Functions

### Core Drawing Primitives

```c
// Shapes
void a_DrawRect(int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a);
void a_DrawFilledRect(int x, int y, int w, int h, u8 r, u8 g, u8 b, u8 a);
void a_DrawLine(int x1, int y1, int x2, int y2, u8 r, u8 g, u8 b, u8 a);
void a_DrawCircle(int x, int y, int radius, u8 r, u8 g, u8 b, u8 a);

// Textures
void a_BlitTexture(SDL_Texture* texture, int x, int y);
void a_BlitTextureScaled(SDL_Texture* texture, int x, int y, int w, int h);

// Text
void a_DrawText(char* text, int x, int y, u8 r, u8 g, u8 b,
                TextFont_t font, TextAlign_t align, int wrap_width);
```

### Text Alignment

```c
typedef enum {
    TEXT_ALIGN_LEFT,    // x is left edge
    TEXT_ALIGN_CENTER,  // x is center
    TEXT_ALIGN_RIGHT    // x is right edge
} TextAlign_t;
```

**Example:**
```c
// Centered title
a_DrawText("Blackjack", SCREEN_WIDTH / 2, 30,
           255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

// Right-aligned FPS counter
a_DrawText("FPS: 60", SCREEN_WIDTH - 10, 10,
           0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
```

---

## Texture Management

### Loading Textures

```c
SDL_Texture* LoadCardTexture(int card_id) {
    dString_t* path = d_InitString();
    d_FormatString(path, "gfx/cards/card_%02d.png", card_id);

    SDL_Texture* tex = a_LoadTexture((char*)d_PeekString(path));
    if (!tex) {
        d_LogErrorF("Failed to load card texture: %s", d_PeekString(path));
    }

    d_DestroyString(path);
    return tex;
}
```

### Texture Caching

```c
// Global texture table (in main.c)
dTable_t* g_card_textures;  // Key: int card_id, Value: SDL_Texture*

void InitializeCardTextures(void) {
    g_card_textures = d_InitTable(sizeof(int), sizeof(SDL_Texture*), NULL, NULL);

    for (int card_id = 0; card_id < 52; card_id++) {
        SDL_Texture* tex = LoadCardTexture(card_id);
        if (tex) {
            d_SetDataInTable(g_card_textures, &card_id, &tex);
        }
    }
}

SDL_Texture* GetCardTexture(int card_id) {
    SDL_Texture** tex_ptr = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card_id);
    return tex_ptr ? *tex_ptr : NULL;
}
```

**Rationale:** Load once, use many times. No runtime file I/O during gameplay.

---

## FlexBox Layout Integration

### Layout-Driven Rendering (Section-Based Architecture)

```c
static FlexBox_t* g_main_layout;  // Scene owns main layout

// UI Sections
static TitleSection_t* g_title_section;
static DealerSection_t* g_dealer_section;
static PlayerSection_t* g_player_section;
static ActionPanel_t* g_betting_panel;

void SceneDraw(float dt) {
    app.background = TABLE_FELT_GREEN;

    // Calculate layout (FlexBox auto-spaces sections with gaps)
    if (g_main_layout) {
        a_FlexLayout(g_main_layout);

        // Get calculated positions for each section
        int title_y = a_FlexGetItemY(g_main_layout, 0);
        int dealer_y = a_FlexGetItemY(g_main_layout, 1);
        int player_y = a_FlexGetItemY(g_main_layout, 2);
        int button_y = a_FlexGetItemY(g_main_layout, 3);

        // Sections handle all rendering internally
        RenderTitleSection(g_title_section, &g_game, title_y);
        RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
        RenderPlayerSection(g_player_section, g_human_player, player_y);
        RenderActionPanel(g_betting_panel, button_y);
    }
}
```

**Key Concepts:**
- FlexBox calculates positions ONCE per layout change
- Scene passes `y` position to sections
- Sections calculate internal positions from constants (SECTION_PADDING, ELEMENT_GAP, etc.)
- **No overlap possible:** FlexBox guarantees gaps between sections

**See Also:** `03_sections.md` for section layout constant patterns

### Debug Rendering

```c
#ifdef DEBUG
    a_FlexDebugRender(g_main_layout);  // Show FlexBox bounds (white/yellow/cyan)
#endif
```

---

## Performance Optimization

### Dirty Flags

```c
typedef struct {
    bool needs_redraw;
    int cached_hand_value;
} RenderCache_t;

void UpdateHandValue(Hand_t* hand) {
    int new_value = CalculateBlackjackValue(hand);
    if (new_value != cache.cached_hand_value) {
        cache.cached_hand_value = new_value;
        cache.needs_redraw = true;
    }
}
```

**Principle:** Only recalculate/redraw when state actually changes.

### Texture Batching

```c
// Good: Group texture draws together
for (int i = 0; i < hand->cards->count; i++) {
    Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
    a_BlitTexture(card->texture, card->x, card->y);
}

// Bad: Interleave texture draws with primitives (state changes!)
for (int i = 0; i < count; i++) {
    a_BlitTexture(tex, x, y);
    a_DrawRect(x, y, w, h, ...);  // Forces GPU state change
}
```

**Rationale:** Minimize GPU state changes by batching similar operations.

---

## Frame Budget

### Target Performance

| Metric | Target | Notes |
|--------|--------|-------|
| **FPS** | 60 | Vsync locked to 60Hz monitor |
| **Frame Time** | 16.67ms | 1000ms / 60fps |
| **Logic Budget** | 5ms | State updates, input, AI |
| **Render Budget** | 10ms | Draw calls, texture uploads |
| **Overhead** | 1.67ms | SDL/OS overhead |

### Measuring Performance

```c
// FPS display (in every scene)
dString_t* fps_str = d_InitString();
d_FormatString(fps_str, "FPS: %.1f", 1.0f / a_GetDeltaTime());
a_DrawText((char*)d_PeekString(fps_str), SCREEN_WIDTH - 10, 10,
           0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
d_DestroyString(fps_str);
```

---

## Archimedes Color System

### aColor_t Structure

```c
typedef struct {
    u8 r, g, b, a;  // Red, Green, Blue, Alpha (0-255)
} aColor_t;
```

### Common Colors

```c
const aColor_t TABLE_GREEN    = {10, 80, 30, 255};   // Dark green felt
const aColor_t CARD_WHITE     = {255, 255, 255, 255}; // Pure white
const aColor_t TEXT_GOLD      = {255, 215, 0, 255};   // Gold for wins
const aColor_t BUTTON_BLUE    = {50, 100, 200, 255};  // Primary button
const aColor_t OVERLAY_DARK   = {0, 0, 0, 180};       // Semi-transparent black
```

---

## Common Rendering Patterns

### Centered Text

```c
a_DrawText("Title", SCREEN_WIDTH / 2, y,
           255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
```

### Right-Aligned Text

```c
a_DrawText("Score: 100", SCREEN_WIDTH - 10, y,
           255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
```

### Semi-Transparent Overlay

```c
a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 150);
```

### Hover Effect

```c
aColor_t color = IsButtonHovered(btn) ?
    (aColor_t){70, 130, 255, 255} :   // Lighter blue
    (aColor_t){50, 100, 200, 255};     // Normal blue

a_DrawFilledRect(btn->x, btn->y, btn->w, btn->h,
                 color.r, color.g, color.b, color.a);
```

---

## Best Practices

### ✅ DO

1. **Separate logic and rendering**
   ```c
   void SceneLogic(float dt) { /* Update state, route input */ }
   void SceneDraw(float dt)  { /* Orchestrate sections */ }
   ```

2. **Orchestrate sections, don't render directly in scenes**
   ```c
   // ✅ GOOD - Scene orchestrates sections
   void SceneDraw(float dt) {
       RenderTitleSection(g_title_section, &g_game, title_y);
       RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
   }

   // ❌ BAD - Scene renders directly
   void SceneDraw(float dt) {
       a_DrawText(g_dealer->name, x, y, ...);  // DON'T!
       RenderHand(&g_dealer->hand, x, y + 50);  // Let sections handle this!
   }
   ```

3. **Use FlexBox for layout**
   ```c
   int y = a_FlexGetItemY(layout, index);  // Calculated once
   RenderMySection(section, y);            // Pass to section
   ```

4. **Cache textures globally**
   ```c
   SDL_Texture* tex = GetCardTexture(card_id);  // O(1) lookup
   ```

5. **Use dString_t for formatted text**
   ```c
   dString_t* str = d_InitString();
   d_FormatString(str, "Score: %d", score);
   a_DrawText((char*)d_PeekString(str), ...);
   d_DestroyString(str);
   ```

6. **Use layout constants, not magic numbers**
   ```c
   // ✅ GOOD
   int name_y = y + SECTION_PADDING;
   int cards_y = name_y + TEXT_LINE_HEIGHT + ELEMENT_GAP;

   // ❌ BAD
   int name_y = y + 10;
   int cards_y = y + 40;  // Magic number - why 40?
   ```

### ❌ DON'T

1. **Don't change state in draw functions**
   ```c
   void SceneDraw(float dt) {
       player->x += 5;  // NO! Do this in logic()
   }
   ```

2. **Don't load textures every frame**
   ```c
   void SceneDraw(float dt) {
       SDL_Texture* tex = a_LoadTexture("card.png");  // NO! Load once
   }
   ```

3. **Don't use hardcoded positions**
   ```c
   RenderButton(400, 580);  // NO! Use FlexBox
   ```

4. **Don't use char[] for dynamic text**
   ```c
   char buf[64];  // NO! Use dString_t
   snprintf(buf, 64, "Score: %d", score);
   ```

5. **Don't render directly in scenes (use sections!)**
   ```c
   void SceneDraw(float dt) {
       // ❌ BAD - Scene has rendering code
       a_DrawText("Dealer", 100, 80, ...);
       for (size_t i = 0; i < dealer->hand.cards->count; i++) {
           // 20 lines of card rendering...
       }

       // ✅ GOOD - Section handles rendering
       RenderDealerSection(g_dealer_section, g_dealer, dealer_y);
   }
   ```

---

## Summary

**Archimedes Rendering Pipeline:**
1. `a_PrepareScene()` - Clear screen, handle events
2. `delegate.logic(dt)` - Update state, route input
3. `delegate.draw(dt)` - Orchestrate sections (back to front)
4. `a_PresentScene()` - Swap buffers

**Current Architecture: Section-Based Rendering**
```
Scene (sceneBlackjack.c)
  ├─ Orchestrates sections
  ├─ Gets FlexBox positions
  └─ Passes y position to sections
      ↓
Sections (titleSection, dealerSection, etc.)
  ├─ Calculate internal positions from constants
  ├─ Render text, components, cards
  └─ Respect allocated height bounds
      ↓
FlexBox System
  └─ Guarantees no overlap (LAYOUT_GAP between sections)
```

**Key Concepts:**
- Delegate pattern for scene-specific rendering
- **Section-based architecture** (scenes orchestrate, sections render)
- Layer-based rendering (background → sections → overlays → HUD)
- FlexBox for automatic layout positioning (**no overlap possible**)
- Layout constants instead of magic numbers
- Texture caching for performance
- dString_t for all dynamic text

**Performance:**
- Target 60 FPS (16.67ms per frame)
- Batch similar draw calls
- Cache calculated positions (FlexBox calculates once)
- Use dirty flags to avoid redundant work

**See Also:**
- `01_scenes.md` - Scene orchestration patterns
- `02_components.md` - Reusable component architecture
- `03_sections.md` - Section implementation details

---

*Last Updated: 2025-10-07*
*Part of Card Fifty-Two Architecture Documentation*
