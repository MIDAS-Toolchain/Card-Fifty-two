# UI Component Architecture

## Overview

**Components** are fully reusable UI primitives (buttons, labels, panels) shared across multiple scenes and sections. Components follow constitutional patterns (dString_t, proper lifecycle) and integrate with FlexBox for automatic positioning.

**Components vs Sections:**
- **Components** = Fully reusable primitives (Button, Label)
- **Sections** = Collections of components + rendering logic + labels (DealerSection, ActionPanel)
- **Scenes** = Orchestrate sections + manage state machines (sceneBlackjack)

See `03_sections.md` for section architecture details.

---

## Component Philosophy

### Reusability Over Duplication

**Before (Scene-Specific Buttons):**
```c
// In sceneBlackjack.c
typedef struct {
    int x, y, w, h;
    char* label;
    bool enabled;
} Button_t;

// In sceneMenu.c
typedef struct {
    int x, y, w, h;
    char* text;
    bool clickable;
} MenuButton_t;
```

**Problem:** Duplicate code, inconsistent behavior, maintenance nightmare.

**After (Shared Component):**
```c
// In src/scenes/components/button.c
Button_t* CreateButton(int x, int y, int w, int h, const char* label);

// Used in all scenes
Button_t* play_btn = CreateButton(0, 0, 150, 60, "Play");
```

**Benefits:** Single implementation, consistent API, easy to maintain.

---

## Component Structure

### Directory Organization

```
src/scenes/components/
├── button.c         # Reusable button widget
├── label.c          # Text label component (future)
└── panel.c          # Container panel (future)

include/scenes/components/
├── button.h
├── label.h
└── panel.h
```

### Component Header Template

```c
// include/scenes/components/button.h

#ifndef BUTTON_H
#define BUTTON_H

#include "../../common.h"

typedef struct Button {
    int x, y, w, h;
    dString_t* label;       // Constitutional: dString_t, not char[]
    dString_t* hotkey_hint; // Optional "[H]" hint
    bool enabled;
    bool was_pressed;
    void* user_data;        // Custom data (optional)
} Button_t;

// Lifecycle
Button_t* CreateButton(int x, int y, int w, int h, const char* label);
void DestroyButton(Button_t** button);

// Configuration
void SetButtonLabel(Button_t* button, const char* label);
void SetButtonHotkey(Button_t* button, const char* hotkey);
void SetButtonEnabled(Button_t* button, bool enabled);
void SetButtonPosition(Button_t* button, int x, int y);

// Interaction
bool IsButtonClicked(Button_t* button);
bool IsButtonHovered(const Button_t* button);

// Rendering
void RenderButton(const Button_t* button);

#endif // BUTTON_H
```

---

## Button Component (Reference Implementation)

### Lifecycle

```c
Button_t* CreateButton(int x, int y, int w, int h, const char* label) {
    Button_t* button = malloc(sizeof(Button_t));
    if (!button) {
        d_LogError("Failed to allocate button");
        return NULL;
    }

    button->x = x;
    button->y = y;
    button->w = w;
    button->h = h;
    button->enabled = true;
    button->was_pressed = false;
    button->user_data = NULL;

    // Constitutional: dString_t for label
    button->label = d_InitString();
    if (!button->label) {
        free(button);
        d_LogError("Failed to allocate button label");
        return NULL;
    }
    d_SetString(button->label, label ? label : "", 0);

    button->hotkey_hint = NULL;  // Optional, created on demand

    return button;
}

void DestroyButton(Button_t** button) {
    if (!button || !*button) return;

    Button_t* btn = *button;

    // Cleanup dString_t members
    if (btn->label) {
        d_DestroyString(btn->label);
    }
    if (btn->hotkey_hint) {
        d_DestroyString(btn->hotkey_hint);
    }

    free(btn);
    *button = NULL;
}
```

**Pattern:** Create/Destroy for heap-allocated components.

### Configuration

```c
void SetButtonLabel(Button_t* button, const char* label) {
    if (!button || !button->label) return;
    d_SetString(button->label, label ? label : "", 0);
}

void SetButtonHotkey(Button_t* button, const char* hotkey) {
    if (!button) return;

    if (!hotkey || strlen(hotkey) == 0) {
        // Remove hotkey hint
        if (button->hotkey_hint) {
            d_DestroyString(button->hotkey_hint);
            button->hotkey_hint = NULL;
        }
        return;
    }

    // Create or update hotkey hint
    if (!button->hotkey_hint) {
        button->hotkey_hint = d_InitString();
    }
    d_SetString(button->hotkey_hint, hotkey, 0);
}

void SetButtonEnabled(Button_t* button, bool enabled) {
    if (!button) return;
    button->enabled = enabled;
}

void SetButtonPosition(Button_t* button, int x, int y) {
    if (!button) return;
    button->x = x;
    button->y = y;
}
```

**Pattern:** Set functions for mutable properties.

### Interaction

```c
bool IsButtonClicked(Button_t* button) {
    if (!button || !button->enabled) {
        return false;
    }

    int mx = app.mouse.x;
    int my = app.mouse.y;

    bool mouse_over = (mx >= button->x && mx <= button->x + button->w &&
                       my >= button->y && my <= button->y + button->h);

    // Press started
    if (app.mouse.pressed && mouse_over && !button->was_pressed) {
        button->was_pressed = true;
        return false;
    }

    // Press released (click complete)
    if (!app.mouse.pressed && button->was_pressed && mouse_over) {
        button->was_pressed = false;
        return true;
    }

    // Reset if released outside
    if (!app.mouse.pressed) {
        button->was_pressed = false;
    }

    return false;
}

bool IsButtonHovered(const Button_t* button) {
    if (!button) return false;

    int mx = app.mouse.x;
    int my = app.mouse.y;

    return (mx >= button->x && mx <= button->x + button->w &&
            my >= button->y && my <= button->y + button->h);
}
```

**Pattern:** Complete click = press inside + release inside (prevents accidental clicks).

### Rendering

```c
void RenderButton(const Button_t* button) {
    if (!button) return;

    // Button background color
    aColor_t bg_color = button->enabled ?
        (aColor_t){50, 100, 200, 255} :    // Blue if enabled
        (aColor_t){80, 80, 80, 255};        // Gray if disabled

    // Hover state
    if (button->enabled && IsButtonHovered(button)) {
        bg_color = (aColor_t){70, 130, 255, 255};  // Lighter blue on hover
    }

    // Draw filled rectangle
    a_DrawFilledRect(button->x, button->y, button->w, button->h,
                     bg_color.r, bg_color.g, bg_color.b, bg_color.a);

    // Draw border
    a_DrawRect(button->x, button->y, button->w, button->h, 255, 255, 255, 255);

    // Draw label
    int text_x = button->x + button->w / 2;
    int text_y = button->y + button->h / 2 - 10;
    a_DrawText((char*)d_PeekString(button->label), text_x, text_y,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw hotkey hint if present
    if (button->hotkey_hint) {
        const char* hint_str = d_PeekString(button->hotkey_hint);
        if (hint_str && hint_str[0] != '\0') {
            int hotkey_y = button->y + button->h + 5;
            a_DrawText((char*)hint_str,
                       text_x, hotkey_y,
                       180, 180, 180, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
        }
    }
}
```

**Pattern:** Self-contained rendering (component handles its own visual state).

---

## FlexBox Integration

### Adding Components to FlexBox

```c
// Create button
Button_t* hit_btn = CreateButton(0, 0, 120, 60, "Hit");
SetButtonHotkey(hit_btn, "[H]");

// Add to FlexBox (FlexBox calculates position)
a_FlexAddItem(g_action_button_row, 120, 60, hit_btn);

// Later: Get calculated position
a_FlexLayout(g_action_button_row);
int x = a_FlexGetItemX(g_action_button_row, 0);
int y = a_FlexGetItemY(g_action_button_row, 0);

// Update button position
SetButtonPosition(hit_btn, x, y);
```

### Helper Function Pattern

```c
static void UpdateButtonPositions(void) {
    if (!g_action_button_row) return;

    a_FlexLayout(g_action_button_row);  // Recalculate if dirty

    for (int i = 0; i < 3; i++) {
        if (action_buttons[i]) {
            int x = a_FlexGetItemX(g_action_button_row, i);
            int y = a_FlexGetItemY(g_action_button_row, i);
            SetButtonPosition(action_buttons[i], x, y);
        }
    }
}
```

**Call before rendering:**
```c
static void SceneDraw(float dt) {
    UpdateButtonPositions();  // Sync FlexBox → Button positions

    for (int i = 0; i < 3; i++) {
        RenderButton(action_buttons[i]);
    }
}
```

---

## Component Usage in Scenes

### Current Architecture: Scenes Own Components, Sections Reference Them

**Important:** In the current section-based architecture, scenes own component lifecycle, sections reference components passed to them.

```c
// sceneBlackjack.c

static Button_t* bet_buttons[4] = {NULL};     // Scene owns buttons
static ActionPanel_t* g_betting_panel = NULL;  // Scene owns section

static void InitializeLayout(void) {
    // Create buttons (scene owns these)
    const char* bet_labels[] = {"10", "25", "50", "100"};
    const char* bet_hotkeys[] = {"[1]", "[2]", "[3]", "[4]"};

    for (int i = 0; i < 4; i++) {
        bet_buttons[i] = CreateButton(0, 0, 100, 60, bet_labels[i]);
        SetButtonHotkey(bet_buttons[i], bet_hotkeys[i]);
    }

    // Create section (references buttons, doesn't own them)
    g_betting_panel = CreateActionPanel("Place Your Bet:", bet_buttons, 4);
}

static void CleanupLayout(void) {
    // Destroy section first (doesn't destroy buttons it references)
    if (g_betting_panel) {
        DestroyActionPanel(&g_betting_panel);
    }

    // Destroy buttons (scene owns these!)
    for (int i = 0; i < 4; i++) {
        if (bet_buttons[i]) {
            DestroyButton(&bet_buttons[i]);
        }
    }
}
```

**Ownership Pattern:**
- Scene creates buttons
- Scene creates section passing button references
- Section uses buttons for rendering/layout
- Section cleanup: destroys its own FlexBoxes and dStrings, but NOT buttons
- Scene cleanup: destroys sections, then destroys buttons

See `03_sections.md` for complete ownership documentation.

### Scene Input Handling (Current Architecture)

```c
static void HandleBettingInput(void) {
    if (!g_human_player) return;

    const int bet_amounts[] = {10, 25, 50, 100};

    // Update button positions from section's FlexBox
    UpdateActionPanelButtons(g_betting_panel);

    // Process button clicks (components handle interaction, scene handles logic)
    for (int i = 0; i < 4; i++) {
        if (!bet_buttons[i]) continue;

        // Enable/disable based on game state
        SetButtonEnabled(bet_buttons[i], CanAffordBet(g_human_player, bet_amounts[i]));

        // Handle click
        if (IsButtonClicked(bet_buttons[i])) {
            ProcessBettingInput(&g_game, g_human_player, bet_amounts[i]);
        }
    }

    // Keyboard shortcuts
    if (app.keyboard[SDL_SCANCODE_1]) {
        app.keyboard[SDL_SCANCODE_1] = 0;
        ProcessBettingInput(&g_game, g_human_player, 10);
    }
    // ...
}
```

### Scene Rendering (Current Architecture)

**Important:** Scenes orchestrate sections, sections handle rendering!

```c
static void BlackjackDraw(float dt) {
    app.background = TABLE_FELT_GREEN;

    if (g_main_layout) {
        a_FlexLayout(g_main_layout);
        int button_y = a_FlexGetItemY(g_main_layout, 3);

        // Section handles all rendering (buttons, text, layout)
        switch (g_game.current_state) {
            case STATE_BETTING:
                RenderActionPanel(g_betting_panel, button_y);
                break;
            case STATE_PLAYER_TURN:
                RenderActionPanel(g_action_panel, button_y);
                break;
        }
    }
}
```

**Old Pattern (❌ DON'T):**
```c
// Scene rendered buttons directly
for (int i = 0; i < 4; i++) {
    RenderButton(bet_buttons[i]);  // DON'T do this in scenes!
}
```

**New Pattern (✅ DO):**
```c
// Section renders buttons
RenderActionPanel(g_betting_panel, button_y);  // Section handles rendering!
```

---

## Future Components

### Label Component

```c
typedef struct Label {
    int x, y;
    dString_t* text;
    aColor_t color;
    TextFont_t font;
    TextAlign_t align;
} Label_t;

Label_t* CreateLabel(int x, int y, const char* text);
void SetLabelText(Label_t* label, const char* text);
void RenderLabel(const Label_t* label);
```

### Panel Component

```c
typedef struct Panel {
    int x, y, w, h;
    aColor_t bg_color;
    aColor_t border_color;
    int border_width;
    FlexBox_t* layout;  // Optional internal FlexBox
} Panel_t;

Panel_t* CreatePanel(int x, int y, int w, int h);
void RenderPanel(const Panel_t* panel);
```

### TextField Component

```c
typedef struct TextField {
    int x, y, w, h;
    dString_t* text;
    dString_t* placeholder;
    bool is_focused;
    int cursor_pos;
} TextField_t;

TextField_t* CreateTextField(int x, int y, int w, int h);
void HandleTextFieldInput(TextField_t* field);
void RenderTextField(const TextField_t* field);
```

---

## Component Best Practices

### ✅ DO

1. **Use dString_t for all text**
   ```c
   typedef struct {
       dString_t* label;  // ✅ Constitutional
   } Component_t;
   ```

2. **Provide Create/Destroy lifecycle**
   ```c
   Component_t* CreateComponent(...);
   void DestroyComponent(Component_t** component);
   ```

3. **Use const for read-only functions**
   ```c
   void RenderComponent(const Component_t* component);
   bool IsComponentHovered(const Component_t* component);
   ```

4. **NULL-check all parameters**
   ```c
   void SetComponentLabel(Component_t* comp, const char* label) {
       if (!comp || !comp->label) return;
       d_SetString(comp->label, label, 0);
   }
   ```

5. **Support FlexBox positioning**
   ```c
   void SetComponentPosition(Component_t* comp, int x, int y);
   ```

### ❌ DON'T

1. **Don't use char[] for labels**
   ```c
   typedef struct {
       char label[64];  // ❌ Fixed size, not constitutional
   } Component_t;
   ```

2. **Don't forget to null pointer after free**
   ```c
   void DestroyComponent(Component_t** comp) {
       free(*comp);
       // Missing: *comp = NULL;  ❌
   }
   ```

3. **Don't render in interaction functions**
   ```c
   bool IsButtonClicked(Button_t* btn) {
       a_DrawRect(...);  // ❌ NO! Rendering in logic function
   }
   ```

4. **Don't hardcode colors**
   ```c
   a_DrawFilledRect(x, y, w, h, 50, 100, 200, 255);  // ❌ Magic numbers

   // Instead:
   aColor_t BUTTON_BLUE = {50, 100, 200, 255};
   a_DrawFilledRect(x, y, w, h, BUTTON_BLUE.r, ...);
   ```

---

## Component Testing Pattern

### Unit Testing Components

```c
// test_button.c

void test_button_lifecycle(void) {
    Button_t* btn = CreateButton(0, 0, 100, 50, "Test");
    assert(btn != NULL);
    assert(btn->enabled == true);
    assert(strcmp(d_PeekString(btn->label), "Test") == 0);

    DestroyButton(&btn);
    assert(btn == NULL);
}

void test_button_click_detection(void) {
    Button_t* btn = CreateButton(100, 100, 50, 50, "Click");

    // Simulate mouse press inside
    app.mouse.x = 120;
    app.mouse.y = 120;
    app.mouse.pressed = true;
    assert(IsButtonClicked(btn) == false);  // Not clicked yet

    // Simulate mouse release inside
    app.mouse.pressed = false;
    assert(IsButtonClicked(btn) == true);   // Click complete!

    DestroyButton(&btn);
}
```

---

## Component Template

### Creating New Components

```c
// 1. Header (include/scenes/components/mycomponent.h)
#ifndef MY_COMPONENT_H
#define MY_COMPONENT_H

#include "../../common.h"

typedef struct MyComponent {
    int x, y, w, h;
    dString_t* text;
    bool enabled;
    void* user_data;
} MyComponent_t;

// Lifecycle
MyComponent_t* CreateMyComponent(int x, int y, int w, int h);
void DestroyMyComponent(MyComponent_t** component);

// Configuration
void SetMyComponentText(MyComponent_t* comp, const char* text);

// Interaction
bool IsMyComponentClicked(MyComponent_t* comp);

// Rendering
void RenderMyComponent(const MyComponent_t* comp);

#endif
```

```c
// 2. Implementation (src/scenes/components/mycomponent.c)
#include "../../../include/scenes/components/mycomponent.h"

MyComponent_t* CreateMyComponent(int x, int y, int w, int h) {
    MyComponent_t* comp = malloc(sizeof(MyComponent_t));
    if (!comp) {
        d_LogError("Failed to allocate MyComponent");
        return NULL;
    }

    comp->x = x;
    comp->y = y;
    comp->w = w;
    comp->h = h;
    comp->enabled = true;
    comp->user_data = NULL;

    comp->text = d_InitString();
    if (!comp->text) {
        free(comp);
        d_LogError("Failed to allocate MyComponent text");
        return NULL;
    }

    return comp;
}

void DestroyMyComponent(MyComponent_t** component) {
    if (!component || !*component) return;

    MyComponent_t* comp = *component;
    if (comp->text) {
        d_DestroyString(comp->text);
    }

    free(comp);
    *component = NULL;
}

// ... implement other functions
```

---

## MenuItem Component (Menu Navigation)

### Purpose
Interactive menu items with selection indicator, hover effects, and keyboard/mouse support. Used in main menu and pause menu for vertical column layouts.

### Structure

```c
// include/scenes/components/menuItem.h

typedef struct MenuItem {
    int x, y, w, h;
    dString_t* label;       // Constitutional: dString_t, not char[]
    dString_t* hotkey;      // Optional hotkey hint
    bool is_selected;
    bool is_hovered;
    bool enabled;
    bool was_clicked;
    void* user_data;
} MenuItem_t;
```

### Key Features

**Visual States:**
- Normal (light cyan #a4dddb)
- Selected (pink #df84a5)
- Hovered (cyan blue #73bed3)
- Selected + Hovered (bright pink-purple #c65197)
- Disabled (dark gray #202e37)

**Selection Indicator:**
- ">" symbol rendered to left of text
- Dynamically positioned based on text width to prevent overlap
- Uses COLOR_INDICATOR (#df84a5 pink)

**Interaction:**
- Complete click detection (press inside + release inside)
- Hover effects with semi-transparent background
- Automatic width calculation from label text

### Usage Example

```c
// Create menu items
MenuItem_t* play_item = CreateMenuItem(SCREEN_WIDTH / 2, 350, 0, 25, "Play");
MenuItem_t* settings_item = CreateMenuItem(SCREEN_WIDTH / 2, 395, 0, 25, "Settings");
MenuItem_t* quit_item = CreateMenuItem(SCREEN_WIDTH / 2, 440, 0, 25, "Quit");

// Mark first item as selected
SetMenuItemSelected(play_item, true);

// Normalize widths for consistent hover areas
int max_width = MAX(play_item->w, MAX(settings_item->w, quit_item->w));
play_item->w = max_width;
settings_item->w = max_width;
quit_item->w = max_width;

// Check for clicks
if (IsMenuItemClicked(play_item)) {
    // Handle play action
}

// Render
RenderMenuItem(play_item);
RenderMenuItem(settings_item);
RenderMenuItem(quit_item);

// Cleanup
DestroyMenuItem(&play_item);
DestroyMenuItem(&settings_item);
DestroyMenuItem(&quit_item);
```

### Layout Constants

```c
#define MENU_ITEM_PADDING         80   // Clickable padding around text
#define AVG_CHAR_WIDTH            15   // Approximate character width
#define MENU_ITEM_TOP_PADDING     3    // Minimal padding above text
#define MENU_ITEM_BOTTOM_PADDING  30   // Padding below text
#define MENU_ITEM_TOTAL_HEIGHT    35   // Total hover bounds height
```

**Rationale:** Generous padding (80px) for comfortable mouse targeting in vertical menu layouts.

---

## MenuItemRow Component (Horizontal Button Layouts)

### Purpose
Specialized variant of MenuItem optimized for horizontal row layouts (confirmation dialogs, side-by-side buttons). Tighter padding prevents overlap.

### Structure

```c
// include/scenes/components/menuItemRow.h

typedef struct MenuItemRow {
    int x, y, w, h;
    dString_t* label;
    dString_t* hotkey;
    bool is_selected;
    bool is_hovered;
    bool enabled;
    bool was_clicked;
    void* user_data;
} MenuItemRow_t;
```

### Key Differences from MenuItem

**Padding:**
- MenuItem: 80px padding (160px total width per button)
- MenuItemRow: 20px padding (40px total width per button)

**Use Cases:**
- Confirmation dialogs ("Yes" / "No")
- Side-by-side action buttons
- Horizontal navigation menus

### Usage Example

```c
// Confirmation dialog buttons
MenuItemRow_t* yes_btn = CreateMenuItemRow(560, 400, 0, 25, "Yes");
MenuItemRow_t* no_btn = CreateMenuItemRow(680, 400, 0, 25, "No");

// 120px spacing prevents overlap with 20px padding
// Total button width: text_width + 40px (2 * 20px padding)

// Mark "No" as selected by default
SetMenuItemRowSelected(no_btn, true);

// Check for clicks
if (IsMenuItemRowClicked(yes_btn)) {
    // Handle confirmation
}

// Render
RenderMenuItemRow(yes_btn);
RenderMenuItemRow(no_btn);

// Cleanup
DestroyMenuItemRow(&yes_btn);
DestroyMenuItemRow(&no_btn);
```

### Layout Constants

```c
#define MENU_ITEM_ROW_PADDING         20   // Tighter padding for horizontal layouts
#define AVG_CHAR_WIDTH                15   // Approximate character width
#define MENU_ITEM_ROW_TOP_PADDING     3    // Minimal padding above text
#define MENU_ITEM_ROW_BOTTOM_PADDING  30   // Padding below text
```

**Rationale:** Tighter 20px padding prevents massive overlap when buttons are placed side-by-side.

---

## Component Comparison Table

| Feature | Button | MenuItem | MenuItemRow |
|---------|--------|----------|-------------|
| **Primary Use** | Game actions (Hit/Stand/Bet) | Vertical menu navigation | Horizontal confirmation dialogs |
| **Padding** | Variable (scene-defined) | 80px (generous) | 20px (tight) |
| **Visual Style** | Filled rectangle + border | Text + selection indicator | Text + selection indicator |
| **Hover Effect** | Background color change | Semi-transparent white overlay | Semi-transparent white overlay |
| **Click Detection** | Press + release inside | Press + release inside | Press + release inside |
| **Selection Indicator** | N/A | ">" symbol (pink) | ">" symbol (pink) |
| **Auto-width** | Fixed width | Calculated from text | Calculated from text |

---

## Summary

**Component Pattern:**
```c
// Create (scene owns component)
Component_t* comp = CreateComponent(x, y, w, h, "Label");

// Configure
SetComponentProperty(comp, value);

// Pass to section (section references, doesn't own)
MySection_t* section = CreateMySection(comp);

// Use in scene logic
if (IsComponentClicked(comp)) { /* handle click */ }

// Section handles rendering
RenderMySection(section, y);  // Section renders component internally

// Destroy (scene destroys components, sections destroy their own resources)
DestroyMySection(&section);   // Section destroys FlexBoxes/dStrings
DestroyComponent(&comp);       // Scene destroys components
```

**Key Concepts:**
- **Reusable across scenes and sections**
- Constitutional patterns (dString_t, proper lifecycle)
- FlexBox integration for positioning
- Self-contained rendering
- State encapsulation

**Architecture Hierarchy:**
```
Component (Button)
  ├─ Fully reusable primitive
  ├─ No dependencies on sections/scenes
  └─ Owned by scenes, referenced by sections
      ↓
Section (ActionPanel)
  ├─ References components
  ├─ Handles rendering
  └─ Owned by scenes
      ↓
Scene (sceneBlackjack)
  ├─ Owns components and sections
  ├─ Routes input to components
  └─ Orchestrates section rendering
```

**Benefits:**
- DRY (Don't Repeat Yourself)
- Consistent behavior across scenes
- Easy to maintain and extend
- Works with section-based architecture
- Clear ownership rules

**See Also:**
- `03_sections.md` - How sections use components
- `01_scenes.md` - How scenes orchestrate sections

---

*Last Updated: 2025-10-07*
*Part of Card Fifty-Two Architecture Documentation*
