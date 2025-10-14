# Rendering Pipeline (Archimedes Integration)

## Overview
The rendering system uses **Archimedes** SDL2 framework for all graphics operations. Following a layered approach: background → table → cards → UI → overlays.

## Render Cycle (60 FPS Target)

### Main Loop (Delegate Pattern)

Archimedes uses a **delegate pattern** for scene management. The main loop delegates logic and rendering to scene-specific functions:

```c
void MainLoop(void) {
    a_PrepareScene();

    float dt = a_GetDeltaTime();
    app.delegate.logic(dt);
    app.delegate.draw(dt);

    a_PresentScene();
}
```

### Scene Delegates

Each scene defines its own logic and draw functions:

```c
static void SceneLogic(float dt) {
    // 1. Input processing
    a_DoInput();
    HandleInput(game_context);

    // 2. Logic update
    UpdateGameLogic(game_context, dt);
}

static void SceneDraw(float dt) {
    // Rendering
    RenderGame(game_context, dt);
}
```

### Scene Initialization

Delegates are assigned in a scene initialization function:

```c
void InitScene(void) {
    // Set scene delegates
    app.delegate.logic = SceneLogic;
    app.delegate.draw = SceneDraw;

    // Initialize scene-specific state
    // (game context, player tables, etc.)
}
```

### Multiple Scenes

You can have multiple scenes (menu, game, settings) by creating different Init functions:

```c
void InitMenuScene(void) {
    app.delegate.logic = MenuLogic;
    app.delegate.draw = MenuDraw;
}

void InitGameScene(void) {
    app.delegate.logic = GameLogic;
    app.delegate.draw = GameDraw;
}

// Switch scenes via button press or state change
if (start_button_pressed) {
    InitGameScene();  // Changes delegates
}
```

## Rendering Layers

### Layer 0: Background (Static)
```c
void RenderBackground(void) {
    // Solid color background
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                     10, 80, 30, 255);  // Dark green felt

    // Optional: Table texture
    SDL_Texture* felt = a_LoadTexture("resources/textures/table_felt.png");
    SDL_Rect dest = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
    SDL_RenderCopy(app.renderer, felt, NULL, &dest);
}
```

### Layer 1: Table Elements (UI Zones)
```c
void RenderTableZones(void) {
    // Dealer zone
    a_DrawRect(50, 50, 700, 200, 255, 255, 255, 100);  // Semi-transparent

    // Player zone
    a_DrawRect(50, 400, 700, 200, 255, 255, 255, 100);

    // Deck position
    a_DrawFilledRect(850, 300, CARD_WIDTH + 10, CARD_HEIGHT + 10,
                     100, 100, 100, 255);  // Deck placeholder
}
```

### Layer 2: Cards (Dynamic)
```c
void RenderCard(const Card_t* card) {
    SDL_Texture* texture = card->face_up ?
        card->texture : g_card_back_texture;

    SDL_Rect dest = {
        card->x,
        card->y,
        CARD_WIDTH,
        CARD_HEIGHT
    };

    SDL_RenderCopy(app.renderer, texture, NULL, &dest);

    // Debug: Card outline
    #ifdef DEBUG
    a_DrawRect(card->x, card->y, CARD_WIDTH, CARD_HEIGHT,
               255, 0, 0, 255);
    #endif
}

void RenderHand(const Hand_t* hand, int base_x, int base_y) {
    const int CARD_SPACING = 40;  // Overlap spacing

    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);
        card->x = base_x + (i * CARD_SPACING);
        card->y = base_y;
        RenderCard(card);
    }
}

void RenderAllHands(GameContext_t* game) {
    // Iterate active players
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* player_id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, player_id);
        Player_t* player = *player_ptr;

        int x = player->is_dealer ? 300 : 300;
        int y = player->is_dealer ? 100 : 450;

        RenderHand(player->hand, x, y);
    }
}
```

### Layer 3: Text and UI
```c
void RenderUI(GameContext_t* game) {
    // Player chips (Constitutional pattern: dString_t, not char[])
    Player_t** player = GetCurrentPlayer(game);
    if (player) {
        dString_t* chip_text = d_InitString();
        d_FormatString(chip_text, "Chips: %d", (*player)->chips);
        a_DrawText(d_PeekString(chip_text), 10, SCREEN_HEIGHT - 50,
                   255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
        d_DestroyString(chip_text);
    }

    // Current bet
    dString_t* bet_text = d_InitString();
    d_FormatString(bet_text, "Bet: %d", (*player)->current_bet);
    a_DrawText(d_PeekString(bet_text), 10, SCREEN_HEIGHT - 80,
               255, 255, 0, FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    d_DestroyString(bet_text);

    // Hand value
    dString_t* value_text = d_InitString();
    d_FormatString(value_text, "Hand: %d", (*player)->hand->total_value);
    a_DrawText(d_PeekString(value_text), 300, 600,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_DestroyString(value_text);

    // Game state message
    const char* state_msg = GetStateMessage(game->current_state);
    a_DrawText(state_msg, SCREEN_WIDTH / 2, 20,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
}
```

### Layer 4: Overlays (Modals, Menus)
```c
void RenderMenu(void) {
    // Semi-transparent overlay
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                     0, 0, 0, 200);

    // Menu box
    int menu_w = 400, menu_h = 300;
    int menu_x = (SCREEN_WIDTH - menu_w) / 2;
    int menu_y = (SCREEN_HEIGHT - menu_h) / 2;

    a_DrawFilledRect(menu_x, menu_y, menu_w, menu_h,
                     40, 40, 40, 255);
    a_DrawRect(menu_x, menu_y, menu_w, menu_h,
               200, 200, 200, 255);

    // Menu items (using Archimedes widgets or custom rendering)
    const char* options[] = {"New Game", "Rules", "Quit"};
    for (int i = 0; i < 3; i++) {
        int y = menu_y + 80 + (i * 60);
        aColor_t color = (i == selected_option) ?
            (aColor_t){255, 255, 0, 255} : white;

        a_DrawText(options[i], SCREEN_WIDTH / 2, y,
                   color.r, color.g, color.b,
                   FONT_GAME, TEXT_ALIGN_CENTER, 0);
    }
}
```

## Texture Management

### Card Texture Loading (Cached)
```c
void LoadAllCardTextures(void) {
    for (int suit = 0; suit < SUIT_MAX; suit++) {
        for (int rank = 1; rank <= RANK_KING; rank++) {
            int card_id = (suit * 13) + (rank - 1);

            // Constitutional pattern: dString_t, not char[]
            dString_t* path = d_InitString();
            d_FormatString(path, "resources/textures/cards/%d.png", card_id);
            SDL_Texture* tex = a_LoadTexture(d_PeekString(path));
            d_DestroyString(path);

            d_SetDataInTable(g_card_textures, &card_id, &tex);
        }
    }

    // Card back texture
    g_card_back_texture = a_LoadTexture("resources/textures/cards/back.png");
}
```

### Lazy Loading Alternative
```c
SDL_Texture* GetCardTexture(int card_id) {
    SDL_Texture** cached = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card_id);
    if (cached) return *cached;

    // Load on first access (Constitutional pattern: dString_t)
    dString_t* path = d_InitString();
    d_FormatString(path, "resources/textures/cards/%d.png", card_id);
    SDL_Texture* tex = a_LoadTexture(d_PeekString(path));
    d_DestroyString(path);

    d_SetDataInTable(g_card_textures, &card_id, &tex);
    return tex;
}
```

## Animation System (Simple)

### Card Deal Animation
```c
typedef struct {
    Card_t* card;
    int start_x, start_y;
    int end_x, end_y;
    float progress;  // 0.0 to 1.0
    float speed;     // Units per second
    bool active;
} CardAnimation_t;

dArray_t* g_active_animations;  // Array of CardAnimation_t

void UpdateAnimations(float dt) {
    for (size_t i = 0; i < g_active_animations->count; i++) {
        CardAnimation_t* anim = (CardAnimation_t*)d_IndexDataFromArray(g_active_animations, i);

        if (!anim->active) continue;

        anim->progress += anim->speed * dt;
        if (anim->progress >= 1.0f) {
            anim->progress = 1.0f;
            anim->active = false;
        }

        // Lerp position
        anim->card->x = anim->start_x + (int)((anim->end_x - anim->start_x) * anim->progress);
        anim->card->y = anim->start_y + (int)((anim->end_y - anim->start_y) * anim->progress);
    }

    // Remove completed animations (iterate backwards)
    for (int i = (int)g_active_animations->count - 1; i >= 0; i--) {
        CardAnimation_t* anim = (CardAnimation_t*)d_IndexDataFromArray(g_active_animations, i);
        if (!anim->active) {
            d_RemoveDataFromArray(g_active_animations, i);
        }
    }
}

void AnimateDealCard(Card_t* card, int dest_x, int dest_y) {
    CardAnimation_t anim = {
        .card = card,
        .start_x = 850, .start_y = 300,  // Deck position
        .end_x = dest_x, .end_y = dest_y,
        .progress = 0.0f,
        .speed = 2.0f,  // 0.5 seconds
        .active = true
    };
    d_AppendDataToArray(g_active_animations, &anim);
}
```

## Widget Integration (Archimedes Widgets)

### Button Example
```c
// Using Archimedes widget system (if available)
aWidget_t* hit_button = a_CreateButton(300, 650, 100, 50, "Hit");
aWidget_t* stand_button = a_CreateButton(450, 650, 100, 50, "Stand");

void RenderButtons(void) {
    a_RenderWidget(hit_button);
    a_RenderWidget(stand_button);
}

void HandleButtonInput(void) {
    if (a_WidgetClicked(hit_button)) {
        PlayerHit(current_player);
    }
    if (a_WidgetClicked(stand_button)) {
        PlayerStand(current_player);
    }
}
```

### Custom Button (Manual)
```c
typedef struct {
    int x, y, w, h;
    const char* label;
    bool hovered;
    bool pressed;
} Button_t;

void RenderButton(Button_t* btn) {
    aColor_t bg = btn->hovered ? (aColor_t){100, 100, 200, 255} :
                                  (aColor_t){50, 50, 150, 255};

    a_DrawFilledRect(btn->x, btn->y, btn->w, btn->h,
                     bg.r, bg.g, bg.b, bg.a);
    a_DrawRect(btn->x, btn->y, btn->w, btn->h,
               255, 255, 255, 255);

    a_DrawText(btn->label, btn->x + btn->w/2, btn->y + btn->h/2 - 8,
               255, 255, 255, FONT_GAME, TEXT_ALIGN_CENTER, 0);
}

bool IsButtonClicked(Button_t* btn) {
    int mx = app.mouse.x;
    int my = app.mouse.y;

    btn->hovered = (mx >= btn->x && mx <= btn->x + btn->w &&
                    my >= btn->y && my <= btn->y + btn->h);

    if (btn->hovered && app.mouse.pressed) {
        btn->pressed = true;
        return false;
    }

    if (btn->pressed && !app.mouse.pressed && btn->hovered) {
        btn->pressed = false;
        return true;  // Click complete
    }

    if (!app.mouse.pressed) {
        btn->pressed = false;
    }

    return false;
}
```

## Performance Optimizations

### Dirty Rectangle Rendering (Future)
```c
// Only re-render changed regions
typedef struct {
    SDL_Rect rect;
    bool dirty;
} RenderRegion_t;

dArray_t* g_dirty_regions;  // Array of RenderRegion_t

void MarkDirty(int x, int y, int w, int h) {
    RenderRegion_t region = {{x, y, w, h}, true};
    d_AppendDataToArray(g_dirty_regions, &region);
}

// In render loop
for (size_t i = 0; i < g_dirty_regions->count; i++) {
    RenderRegion_t* region = (RenderRegion_t*)d_IndexDataFromArray(g_dirty_regions, i);
    if (region->dirty) {
        SDL_RenderSetClipRect(app.renderer, &region->rect);
        // Render only this region
        region->dirty = false;
    }
}
```

### Texture Atlas (Future)
```c
// Single texture with all cards
SDL_Texture* g_card_atlas;
SDL_Rect g_card_rects[52];  // Source rectangles for each card

void RenderCardFromAtlas(int card_id, int x, int y) {
    SDL_Rect dest = {x, y, CARD_WIDTH, CARD_HEIGHT};
    SDL_RenderCopy(app.renderer, g_card_atlas, &g_card_rects[card_id], &dest);
}
```

## Debug Visualization

### Draw Bounding Boxes
```c
#ifdef DEBUG
void RenderDebugInfo(GameContext_t* game) {
    // Player hand bounds
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t** player = (Player_t**)d_GetDataFromTable(g_players, id);

        for (size_t j = 0; j < (*player)->hand->cards->count; j++) {
            Card_t* card = (Card_t*)d_IndexDataFromArray((*player)->hand->cards, j);
            a_DrawRect(card->x, card->y, CARD_WIDTH, CARD_HEIGHT,
                       255, 0, 0, 255);
        }
    }

    // FPS counter (Constitutional pattern: dString_t)
    dString_t* fps_text = d_InitString();
    d_FormatString(fps_text, "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText(d_PeekString(fps_text), SCREEN_WIDTH - 10, 10,
               0, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
    d_DestroyString(fps_text);
}
#endif
```

## Render Function Organization

```c
void RenderGame(GameContext_t* game, float dt) {
    // Layer 0
    RenderBackground();

    // Layer 1
    RenderTableZones();

    // Layer 2
    RenderDeck(game->deck);
    RenderAllHands(game);

    // Layer 3
    RenderUI(game);
    RenderButtons();

    // Layer 4 (conditional)
    if (game->current_state == STATE_MENU) {
        RenderMenu();
    }

    // Debug overlay
    #ifdef DEBUG
    RenderDebugInfo(game);
    #endif
}
```

## Color Palette

```c
// Archimedes predefined colors
const aColor_t TABLE_GREEN = {10, 80, 30, 255};
const aColor_t CARD_WHITE = white;
const aColor_t TEXT_YELLOW = yellow;
const aColor_t BUTTON_BLUE = {50, 50, 150, 255};
const aColor_t BUTTON_HOVER = {100, 100, 200, 255};
const aColor_t OVERLAY_DARK = {0, 0, 0, 200};
```

## Cross-Platform Considerations

### Native vs Web
```c
#ifdef __EMSCRIPTEN__
    // Web: Smaller font sizes
    app.font_scale = 0.8f;
#else
    // Desktop: Normal font sizes
    app.font_scale = 1.0f;
#endif
```

### Resolution Scaling
```c
void SetupViewport(void) {
    // Maintain aspect ratio
    float aspect = (float)SCREEN_WIDTH / SCREEN_HEIGHT;
    int window_w, window_h;
    SDL_GetWindowSize(app.window, &window_w, &window_h);

    float window_aspect = (float)window_w / window_h;

    if (window_aspect > aspect) {
        // Letterbox sides
        int viewport_w = (int)(window_h * aspect);
        int offset_x = (window_w - viewport_w) / 2;
        SDL_Rect viewport = {offset_x, 0, viewport_w, window_h};
        SDL_RenderSetViewport(app.renderer, &viewport);
    } else {
        // Letterbox top/bottom
        int viewport_h = (int)(window_w / aspect);
        int offset_y = (window_h - viewport_h) / 2;
        SDL_Rect viewport = {0, offset_y, window_w, viewport_h};
        SDL_RenderSetViewport(app.renderer, &viewport);
    }
}
```
