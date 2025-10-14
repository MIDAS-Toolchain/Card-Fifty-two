# Input Handling Design

## Input System (Archimedes)

### Core Input Processing
```c
void HandleInput(GameContext_t* game) {
    a_DoInput();  // Process SDL events

    // Global shortcuts
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        if (game->current_state == STATE_MENU) {
            app.running = 0;  // Quit
        } else {
            TransitionState(game, STATE_MENU);  // Back to menu
        }
    }

    // State-specific input
    switch (game->current_state) {
        case STATE_MENU:
            HandleMenuInput(game);
            break;
        case STATE_BETTING:
            HandleBettingInput(game);
            break;
        case STATE_PLAYER_TURN:
            HandlePlayerTurnInput(game);
            break;
        default:
            break;
    }
}
```

## Button Click Detection

### Button Structure
```c
typedef struct {
    int x, y, w, h;
    const char* label;
    bool enabled;
    bool was_pressed;
} Button_t;

bool IsButtonClicked(Button_t* btn) {
    int mx = app.mouse.x;
    int my = app.mouse.y;

    bool mouse_over = (mx >= btn->x && mx <= btn->x + btn->w &&
                       my >= btn->y && my <= btn->y + btn->h);

    if (!btn->enabled) return false;

    // Press started
    if (app.mouse.pressed && mouse_over && !btn->was_pressed) {
        btn->was_pressed = true;
        return false;
    }

    // Press released (click complete)
    if (!app.mouse.pressed && btn->was_pressed && mouse_over) {
        btn->was_pressed = false;
        return true;
    }

    // Reset if released outside
    if (!app.mouse.pressed) {
        btn->was_pressed = false;
    }

    return false;
}
```

## Game-Specific Input Handlers

### Betting Input
```c
Button_t bet_buttons[4] = {
    {300, 600, 80, 50, "10", true, false},
    {400, 600, 80, 50, "25", true, false},
    {500, 600, 80, 50, "50", true, false},
    {600, 600, 80, 50, "100", true, false}
};

void HandleBettingInput(GameContext_t* game) {
    Player_t* player = GetCurrentPlayer(game);
    if (!player || player->is_ai) return;

    int bet_amounts[] = {10, 25, 50, 100};

    for (int i = 0; i < 4; i++) {
        if (IsButtonClicked(&bet_buttons[i])) {
            if (PlaceBet(player, bet_amounts[i])) {
                d_LogInfoF("Player bet %d chips", bet_amounts[i]);
            }
        }
    }
}
```

### Player Turn Input
```c
Button_t action_buttons[3] = {
    {300, 650, 100, 50, "Hit", true, false},
    {450, 650, 100, 50, "Stand", true, false},
    {600, 650, 100, 50, "Double", true, false}
};

void HandlePlayerTurnInput(GameContext_t* game) {
    Player_t* player = GetCurrentPlayer(game);
    if (!player || player->is_ai || player->is_dealer) return;

    // Disable double if not first turn or insufficient chips (Hand_t is embedded value type)
    action_buttons[2].enabled = (player->hand.cards->count == 2 &&
                                  player->chips >= player->current_bet);

    if (IsButtonClicked(&action_buttons[0])) {  // Hit
        ExecutePlayerAction(game, player, ACTION_HIT);
    }

    if (IsButtonClicked(&action_buttons[1])) {  // Stand
        ExecutePlayerAction(game, player, ACTION_STAND);
    }

    if (IsButtonClicked(&action_buttons[2])) {  // Double
        ExecutePlayerAction(game, player, ACTION_DOUBLE);
    }
}
```

### Keyboard Shortcuts
```c
void HandleKeyboardShortcuts(GameContext_t* game) {
    Player_t* player = GetCurrentPlayer(game);

    if (game->current_state == STATE_PLAYER_TURN && player && !player->is_ai) {
        // H = Hit
        if (app.keyboard[SDL_SCANCODE_H]) {
            ExecutePlayerAction(game, player, ACTION_HIT);
            app.keyboard[SDL_SCANCODE_H] = 0;  // Prevent repeat
        }

        // S = Stand
        if (app.keyboard[SDL_SCANCODE_S]) {
            ExecutePlayerAction(game, player, ACTION_STAND);
            app.keyboard[SDL_SCANCODE_S] = 0;
        }

        // D = Double (Hand_t is embedded value type)
        if (app.keyboard[SDL_SCANCODE_D] && player->hand.cards->count == 2) {
            ExecutePlayerAction(game, player, ACTION_DOUBLE);
            app.keyboard[SDL_SCANCODE_D] = 0;
        }
    }
}
```

## Menu Navigation

### Menu State
```c
int selected_option = 0;
const char* menu_options[] = {"New Game", "Settings", "Quit"};
const int menu_option_count = 3;

void HandleMenuInput(GameContext_t* game) {
    // Arrow keys for selection
    static bool up_was_pressed = false;
    if (app.keyboard[SDL_SCANCODE_UP] && !up_was_pressed) {
        selected_option = (selected_option - 1 + menu_option_count) % menu_option_count;
        up_was_pressed = true;
    } else if (!app.keyboard[SDL_SCANCODE_UP]) {
        up_was_pressed = false;
    }

    static bool down_was_pressed = false;
    if (app.keyboard[SDL_SCANCODE_DOWN] && !down_was_pressed) {
        selected_option = (selected_option + 1) % menu_option_count;
        down_was_pressed = true;
    } else if (!app.keyboard[SDL_SCANCODE_DOWN]) {
        down_was_pressed = false;
    }

    // Enter to select
    static bool enter_was_pressed = false;
    if (app.keyboard[SDL_SCANCODE_RETURN] && !enter_was_pressed) {
        switch (selected_option) {
            case 0:  // New Game
                TransitionState(game, STATE_BETTING);
                break;
            case 1:  // Settings
                // TODO: Settings menu
                break;
            case 2:  // Quit
                app.running = 0;
                break;
        }
        enter_was_pressed = true;
    } else if (!app.keyboard[SDL_SCANCODE_RETURN]) {
        enter_was_pressed = false;
    }

    // Mouse click on menu items
    for (int i = 0; i < menu_option_count; i++) {
        int y = 300 + (i * 60);
        if (app.mouse.x >= 400 && app.mouse.x <= 880 &&
            app.mouse.y >= y && app.mouse.y <= y + 40) {

            selected_option = i;

            if (app.mouse.button == SDL_BUTTON_LEFT &&
                app.mouse.state == SDL_RELEASED) {
                // Same logic as Enter key
            }
        }
    }
}
```

## Card Selection (Future - Poker)

```c
int selected_card_index = -1;

void HandleCardSelection(Hand_t* hand) {
    for (size_t i = 0; i < hand->cards->count; i++) {
        Card_t* card = (Card_t*)d_IndexDataFromArray(hand->cards, i);

        // Check if mouse over card
        if (app.mouse.x >= card->x && app.mouse.x <= card->x + CARD_WIDTH &&
            app.mouse.y >= card->y && app.mouse.y <= card->y + CARD_HEIGHT) {

            if (app.mouse.button == SDL_BUTTON_LEFT &&
                app.mouse.state == SDL_RELEASED) {
                selected_card_index = (int)i;
                card->y -= 20;  // Visual feedback (lift card)
            }
        }
    }
}
```

## Input Summary

| Input | Action | State |
|-------|--------|-------|
| **ESC** | Back to menu / Quit | All states |
| **H** | Hit | Player turn |
| **S** | Stand | Player turn |
| **D** | Double down | Player turn (first 2 cards) |
| **Mouse Click** | Button/card selection | Context-dependent |
| **Arrow Keys** | Menu navigation | Menu |
| **Enter** | Menu selection | Menu |

## Best Practices

✅ **Single-press detection** - Use static flags to prevent key repeat
✅ **Button state tracking** - Track `was_pressed` to detect complete clicks
✅ **Disable invalid actions** - Gray out buttons that can't be used
✅ **Visual feedback** - Highlight on hover, animate on click
✅ **Keyboard shortcuts** - Support both mouse and keyboard

❌ **Don't use raw SDL events** - Always use `a_DoInput()` and `app.keyboard`/`app.mouse`
❌ **Don't forget to clear key states** - Prevent accidental repeats with `= 0`
❌ **Don't ignore disabled state** - Check `enabled` flag before processing input
