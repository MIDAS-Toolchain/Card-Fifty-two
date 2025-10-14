# Rendering Strategy

## Layout Constants

**Critical Constants for Screen Layout:**

```c
// Screen dimensions (from Archimedes)
#define SCREEN_WIDTH  1280
#define SCREEN_HEIGHT 720

// Layout spacing (sceneBlackjack.h)
#define LAYOUT_TOP_MARGIN       0      // No gap at top
#define LAYOUT_BOTTOM_CLEARANCE 30     // Essential bottom spacing!
#define LAYOUT_GAP              10     // Gap between sections
```

**LAYOUT_BOTTOM_CLEARANCE = 30 (Critical!):**
- Prevents UI elements from being cramped at screen bottom
- Provides visual breathing room
- Ensures buttons and text remain fully visible
- **Was previously 0, causing layout issues** - fixed in recent improvements

**Available Layout Height:**
```c
int layout_height = SCREEN_HEIGHT - LAYOUT_TOP_MARGIN - LAYOUT_BOTTOM_CLEARANCE;
// 720 - 0 - 30 = 690px available for FlexBox sections
```

**Section Heights (must sum to ≤ 690px with gaps):**
```c
#define TOP_BAR_HEIGHT      35   // Optional persistent top bar
#define TITLE_AREA_HEIGHT   30   // Game title/status
#define DEALER_AREA_HEIGHT  165  // Dealer cards + info
#define PLAYER_AREA_HEIGHT  240  // Player cards + info
#define BUTTON_AREA_HEIGHT  100  // Action buttons
```

**Why These Matter:**
- Layout constants define the visual structure
- Incorrect values cause UI overlap or cutoff
- FlexBox relies on these for positioning
- Documented here as baseline for all scenes

---

## Visual Layout

```
┌────────────────────────────────────────┐
│  Dealer: 17          [Deck: 45]        │  ← Top area
│  [🂡][🂵][🂨]                            │
│                                        │
│                                        │
│         ╔════ Table Felt ═════╗        │  ← Center
│         ║                     ║        │
│         ╚═════════════════════╝        │
│                                        │
│  Player 1: 19                          │  ← Bottom area
│  [🃑][🃉]                                │
│                                        │
│  Chips: 950  Bet: 50  [Hit] [Stand]   │  ← UI controls
└────────────────────────────────────────┘
```

## Render Layers (Back to Front)

### Layer 0: Background
```c
void RenderBackground(void) {
    // Table felt texture or solid color
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 10, 80, 30, 255);
}
```

### Layer 1: Table Elements
```c
void RenderTableElements(void) {
    // Deck position
    a_DrawFilledRect(850, 300, CARD_WIDTH + 10, CARD_HEIGHT + 10,
                     40, 40, 40, 255);

    // Player zones (outlines)
    a_DrawRect(50, 400, 700, 200, 255, 255, 255, 80);  // Player area
    a_DrawRect(50, 50, 700, 200, 255, 255, 255, 80);   // Dealer area
}
```

### Layer 2: Cards
```c
void RenderAllCards(GameContext_t* game) {
    // Render deck
    RenderDeck(game->deck, 850, 300);

    // Render each player's hand
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* player = GetPlayerByID(*id);

        int x = player->is_dealer ? 100 : 100;
        int y = player->is_dealer ? 80 : 450;

        RenderHand(&player->hand, x, y, !player->is_dealer);  // Hand_t is embedded value type
    }
}
```

### Layer 3: UI Text & Buttons
```c
void RenderUI(GameContext_t* game) {
    // Player info (Constitutional: dString_t, not char[])
    Player_t* player = GetCurrentPlayer(game);
    if (player && !player->is_dealer) {
        dString_t* text = d_InitString();
        d_FormatString(text, "Chips: %d  Bet: %d", player->chips, player->current_bet);
        a_DrawText(d_PeekString(text), 10, SCREEN_HEIGHT - 30, 255, 255, 255, FONT_GAME, TEXT_ALIGN_LEFT, 0);
        d_DestroyString(text);
    }

    // Hand values
    for (size_t i = 0; i < game->active_players->count; i++) {
        int* id = (int*)d_IndexDataFromArray(game->active_players, i);
        Player_t* p = GetPlayerByID(*id);

        // Constitutional: dString_t for formatting, d_PeekString for player name
        dString_t* value_text = d_InitString();
        d_FormatString(value_text, "%s: %d", d_PeekString(p->name), p->hand.total_value);

        int y = p->is_dealer ? 30 : SCREEN_HEIGHT - 200;
        a_DrawText(d_PeekString(value_text), 100, y, 255, 255, 255, FONT_GAME, TEXT_ALIGN_LEFT, 0);
        d_DestroyString(value_text);
    }

    // Action buttons (if player turn)
    if (game->current_state == STATE_PLAYER_TURN) {
        RenderActionButtons();
    }
}
```

### Layer 4: Overlays
```c
void RenderOverlays(GameContext_t* game) {
    if (game->current_state == STATE_ROUND_END) {
        // Semi-transparent overlay
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 150);

        // Result message
        Player_t* player = GetPlayerByID(1);
        const char* msg = GetResultMessage(player->state);
        a_DrawText(msg, SCREEN_WIDTH/2, SCREEN_HEIGHT/2,
                   255, 255, 0, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    }
}
```

## Card Animation

### Deal Animation
```c
typedef struct {
    Card_t* card;
    int start_x, start_y;
    int end_x, end_y;
    float progress;  // 0.0 to 1.0
    bool active;
} CardAnimation_t;

dArray_t* g_active_animations;  // Array of CardAnimation_t

void AnimateDeal(Card_t* card, int dest_x, int dest_y) {
    CardAnimation_t anim = {
        .card = card,
        .start_x = 850, .start_y = 300,  // Deck position
        .end_x = dest_x, .end_y = dest_y,
        .progress = 0.0f,
        .active = true
    };
    d_AppendDataToArray(g_active_animations, &anim);
}

void UpdateAnimations(float dt) {
    for (size_t i = 0; i < g_active_animations->count; i++) {
        CardAnimation_t* anim = (CardAnimation_t*)d_IndexDataFromArray(g_active_animations, i);

        if (anim->active) {
            anim->progress += 3.0f * dt;  // Speed
            if (anim->progress >= 1.0f) {
                anim->progress = 1.0f;
                anim->active = false;
            }

            // Lerp position
            anim->card->x = (int)(anim->start_x + (anim->end_x - anim->start_x) * anim->progress);
            anim->card->y = (int)(anim->start_y + (anim->end_y - anim->start_y) * anim->progress);
        }
    }
}
```

## Button Rendering

```c
typedef struct {
    int x, y, w, h;
    const char* label;
    bool enabled;
} Button_t;

void RenderButton(const Button_t* btn) {
    aColor_t bg = btn->enabled ?
        (aColor_t){50, 100, 200, 255} :
        (aColor_t){100, 100, 100, 255};

    a_DrawFilledRect(btn->x, btn->y, btn->w, btn->h, bg.r, bg.g, bg.b, bg.a);
    a_DrawRect(btn->x, btn->y, btn->w, btn->h, 255, 255, 255, 255);

    a_DrawText(btn->label, btn->x + btn->w/2, btn->y + btn->h/2 - 8,
               255, 255, 255, FONT_GAME, TEXT_ALIGN_CENTER, 0);
}

Button_t hit_btn = {300, 650, 100, 50, "Hit", true};
Button_t stand_btn = {450, 650, 100, 50, "Stand", true};
```

## Color Scheme

```c
const aColor_t TABLE_GREEN = {10, 80, 30, 255};
const aColor_t CARD_WHITE = {255, 255, 255, 255};
const aColor_t TEXT_GOLD = {255, 215, 0, 255};
const aColor_t BUTTON_BLUE = {50, 100, 200, 255};
const aColor_t OVERLAY_DARK = {0, 0, 0, 180};
```

## Performance

- **Texture caching**: All card textures loaded once
- **Sprite batching**: Group SDL_RenderCopy calls
- **Dirty rectangles**: Only redraw changed regions (future)
- **Target FPS**: 60 FPS, render budget ~16ms per frame
