# ADR-018: Interactive Button UX Pattern (Hold-and-Release)

**Status**: Accepted
**Date**: 2025-11-28
**Context**: Post-combat trinket rewards, modal button interactions
**Depends On**: ADR-008 (Modal Design Consistency), ADR-012 (Reward Modal Animation)
**Fitness Function**: FF-018 (architecture/fitness_functions/18_interactive_button_ux.py)

---

## Context

Modal buttons (Equip/Sell, reward choices, event options) need consistent interaction patterns. Initial implementations had inconsistent behavior:

1. **Instant Activation**: Some buttons fired immediately on keypress (felt mechanical, no tactile feedback)
2. **Double Trigger Bug**: Holding key + clicking mouse triggered action twice
3. **No Visual Feedback**: Unclear which button hotkey was targeting
4. **Inconsistent Hotkeys**: ESC used for both "back" and "menu" (conflict)

**Prior Art**:
- ADR-008 established modal color palette and layout patterns
- ADR-012 implemented hold-and-release for reward modal (reference implementation)
- Multiple button components (button.c, sidebarButton.c, deckButton.c) implemented independently

**Requirements**:
- Tactile feedback on button press (hold = highlight, release = activate)
- No double-trigger when combining mouse + keyboard
- Visual consistency (button colors, scale animations)
- Clear hotkey conventions (E = equip/confirm, S = sell/skip, BACKSPACE = back)

---

## Decision

**Constitutional Pattern: Hold-and-Release Button Interaction with Visual Feedback**

All modal buttons must implement:

1. **Hold-to-Highlight, Release-to-Activate**
2. **Mutex Input** (keyboard OR mouse, never both)
3. **Smooth Scale Animation** (lerp, not instant snap)
4. **ADR-008 Color Palette** (consistent blue/red accent colors)
5. **Standard Hotkey Conventions**

---

## Implementation Pattern

### 1. Hold-and-Release State Machine

Track which key was held in previous frame, trigger on release:

```c
typedef struct ModalButtons {
    int key_held_button;  // -1 = none, 0 = first button, 1 = second button, etc.
    float button_scale[2];  // Per-button scale for animation
    bool button_pressed[2];  // Visual pressed state
} ModalButtons_t;

// In input handler (called every frame):
bool HandleModalInput(Modal_t* modal, float dt) {
    // 1. Track previous frame's key state
    int prev_key_held = modal->key_held_button;

    // 2. Check which key is currently held
    modal->key_held_button = -1;
    if (app.keyboard[SDL_SCANCODE_E]) modal->key_held_button = 0;  // Equip
    else if (app.keyboard[SDL_SCANCODE_S]) modal->key_held_button = 1;  // Sell

    // 3. Trigger on RELEASE (was held, now not held)
    if (prev_key_held == 0 && modal->key_held_button != 0) {
        // E released - perform equip action
        PerformEquipAction();
        return true;
    } else if (prev_key_held == 1 && modal->key_held_button != 1) {
        // S released - perform sell action
        PerformSellAction();
        return true;
    }

    // 4. Visual feedback while held (key held = button highlighted)
    bool equip_hovered = (mouse_over_equip_button);
    if (modal->key_held_button == 0) equip_hovered = true;  // Holding E

    // 5. Mutex: Block mouse clicks while key held (prevents double trigger)
    if (equip_hovered && clicked && modal->key_held_button < 0) {
        PerformEquipAction();
    }

    return false;
}
```

**Why This Works**:
- Holding key highlights button (tactile feedback)
- Releasing key performs action (natural release gesture)
- Mouse blocked when key held (prevents double trigger)
- Previous frame comparison detects release (not instant press)

---

### 2. Button Scale Animation (Smooth Lerp)

Buttons scale smoothly between states (no instant snap):

```c
// Update button scale (called every frame)
float target_scale = pressed ? 0.95f : (hovered ? 1.05f : 1.0f);
float lerp_speed = 10.0f;
modal->button_scale += (target_scale - modal->button_scale) * lerp_speed * dt;

// Render button with scale
int scaled_width = (int)(BUTTON_WIDTH * modal->button_scale);
int scaled_height = (int)(BUTTON_HEIGHT * modal->button_scale);
int scaled_x = button_x + (BUTTON_WIDTH - scaled_width) / 2;
int scaled_y = button_y + (BUTTON_HEIGHT - scaled_height) / 2;

a_DrawFilledRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, button_color);
```

**Scale States**:
- Normal: `1.0f` (100% size)
- Hovered: `1.05f` (105% size - enlarged)
- Pressed: `0.95f` (95% size - compressed)

**Lerp Speed**: `10.0f` provides smooth but responsive feel (~0.1s transition)

---

### 3. ADR-008 Color Palette

Consistent colors across all modal buttons:

```c
// Color constants (from ADR-008)
#define COLOR_OVERLAY           ((aColor_t){9, 10, 20, 180})     // Almost black overlay
#define COLOR_PANEL_BG          ((aColor_t){9, 10, 20, 240})     // Almost black panel
#define COLOR_BUTTON_BG         ((aColor_t){37, 58, 94, 255})    // Dark navy (normal)
#define COLOR_BUTTON_HOVER      ((aColor_t){60, 94, 139, 255})   // Medium blue (hovered)
#define COLOR_BUTTON_PRESSED    ((aColor_t){30, 47, 75, 255})    // Darker navy (pressed)

// Accent colors for semantic actions
#define COLOR_EQUIP_ACCENT      ((aColor_t){168, 202, 88, 255})  // Green (confirm/equip)
#define COLOR_SELL_ACCENT       ((aColor_t){207, 87, 60, 255})   // Red-orange (cancel/sell)

// Button color based on state
aColor_t button_color = COLOR_BUTTON_BG;
if (pressed) button_color = COLOR_BUTTON_PRESSED;
else if (hovered) button_color = COLOR_BUTTON_HOVER;

// Accent borders for semantic meaning
a_DrawRect(button_rect, is_equip_button ? COLOR_EQUIP_ACCENT : COLOR_SELL_ACCENT);
```

**Visual Language**:
- **Blue** = Normal interactive element
- **Green border** = Confirm/Equip/Accept action
- **Red border** = Cancel/Sell/Back action

---

### 4. Standard Hotkey Conventions

Consistent hotkeys across all modals:

| Hotkey | Action | Context |
|--------|--------|---------|
| **E** | Equip / Confirm | Primary action (green accent) |
| **S** | Sell / Skip | Secondary action (red accent) |
| **1-6** | Select slot | Multi-choice selection |
| **BACKSPACE** | Back (one level) | Return to previous modal state |
| **ESC** | Menu (global) | Exit to main menu (reserved) |

**Label Format**: Show hotkey in button text:
```c
a_DrawText("EQUIP (E)", button_x, button_y, ...);
a_DrawText("SELL (S)", button_x, button_y, ...);
a_DrawText("BACK (BACKSPACE)", button_x, button_y, ...);
```

**Why BACKSPACE for Back**:
- ESC reserved for global menu (ADR-008)
- BACKSPACE semantic meaning: "go back one step"
- Prevents hotkey conflict

---

## Evidence

### Reference Implementation: [trinketDropModal.c](../../src/scenes/components/trinketDropModal.c)

**Hold-and-Release** (lines 443-462):
```c
// Track which button key is held (reward modal pattern: hold to highlight, release to activate)
int prev_key_held = modal->key_held_button;
modal->key_held_button = -1;
if (e_pressed) modal->key_held_button = 0;  // E = Equip
else if (s_pressed) modal->key_held_button = 1;  // S = Sell

// Trigger on key release (was held, now not held)
if (prev_key_held == 0 && modal->key_held_button != 0) {
    // E released - switch to slot selection
    modal->choosing_slot = true;
    return false;
} else if (prev_key_held == 1 && modal->key_held_button != 1) {
    // S released - sell trinket
    modal->was_equipped = false;
    modal->chips_gained = modal->trinket_drop.sell_value;
    modal->confirmed = true;
    return true;  // Close modal
}
```

**Mutex Input** (lines 499, 506):
```c
// Handle button clicks (ONLY allow mouse if no key is held - prevents double trigger)
if (equip_hovered && modal->key_held_button < 0) {
    if (clicked) {
        modal->choosing_slot = true;
    }
} else if (sell_hovered && modal->key_held_button < 0) {
    if (clicked) {
        modal->was_equipped = false;
        return true;
    }
}
```

**Scale Animation** (lines 489-495):
```c
// Update button scale animations (smooth transitions)
float target_equip_scale = modal->equip_button_pressed ? 0.95f : (equip_hovered ? 1.05f : 1.0f);
float target_sell_scale = modal->sell_button_pressed ? 0.95f : (sell_hovered ? 1.05f : 1.0f);

float lerp_speed = 10.0f;
modal->equip_button_scale += (target_equip_scale - modal->equip_button_scale) * lerp_speed * dt;
modal->sell_button_scale += (target_sell_scale - modal->sell_button_scale) * lerp_speed * dt;
```

**Color Palette** (lines 20-31, 723-724, 756-757):
```c
// ADR-008 color constants
#define COLOR_BUTTON_BG         ((aColor_t){37, 58, 94, 255})
#define COLOR_BUTTON_HOVER      ((aColor_t){60, 94, 139, 255})
#define COLOR_EQUIP_ACCENT      ((aColor_t){168, 202, 88, 255})  // Green border
#define COLOR_SELL_ACCENT       ((aColor_t){207, 87, 60, 255})   // Red border

// Rendering with scale and colors
a_DrawFilledRect(..., equip_bg_color);
a_DrawRect(..., COLOR_EQUIP_ACCENT);  // Green border for equip

a_DrawFilledRect(..., sell_bg_color);
a_DrawRect(..., COLOR_SELL_ACCENT);  // Red border for sell
```

**BACKSPACE for Back** (lines 350, 370):
```c
bool backspace_pressed = app.keyboard[SDL_SCANCODE_BACKSPACE];

// In slot selection mode
if (backspace_pressed) {
    modal->choosing_slot = false;  // Return to button mode
    return false;
}
```

---

## Consequences

### Positive

**1. Tactile Feedback**
- Hold-and-release creates satisfying "button press" feel
- Visual scale animation reinforces interaction
- Clear which button hotkey is targeting

**2. No Double-Trigger Bugs**
- Mutex input (key OR mouse) prevents race conditions
- `modal->key_held_button < 0` guards mouse clicks
- One input method active at a time

**3. Visual Consistency**
- All modal buttons use ADR-008 color palette
- Same scale animation across components
- Green = confirm, Red = cancel (universal language)

**4. Predictable Hotkeys**
- E/S/BACKSPACE conventions easy to remember
- ESC reserved for global menu (no conflict)
- Hotkeys shown in button labels

**5. Grep-able Enforcement**
- `grep "key_held"` finds all implementations
- `grep "COLOR_BUTTON_BG"` verifies color usage
- Fitness function automates compliance checks

### Negative

**1. Slightly Slower Than Instant Click**
- Hold-and-release adds ~100ms vs instant keypress
- **Accepted**: Tactile feedback worth the delay
- **Mitigation**: Lerp speed tuned for responsiveness

**2. Implementation Complexity**
- Requires state tracking (prev_key, key_held_button)
- Animation state (scale, pressed flags)
- **Mitigated**: Pattern well-documented, fitness function enforces

**3. Learning Curve**
- New developers must understand hold-and-release pattern
- **Mitigated**: ADR-018 provides code examples
- **Mitigated**: Fitness function catches violations early

### Accepted Trade-offs

- ✅ Accept 100ms delay for tactile feedback (better UX)
- ✅ Accept state tracking complexity for bug-free interactions
- ✅ Accept BACKSPACE hotkey (ESC reserved for menu)
- ❌ NOT accepting: Instant activation without hold (feels mechanical)
- ❌ NOT accepting: Keyboard + mouse double triggers (bug source)

---

## Alternatives Considered

### 1. Instant Activation (Rejected)

**Proposal**: Fire action immediately on keypress (no hold-and-release)

```c
if (app.keyboard[SDL_SCANCODE_E]) {
    PerformEquipAction();  // Instant
}
```

**Rejected Because**:
- No tactile feedback (feels mechanical)
- Easy to misclick (accidental keypress)
- No visual highlight before action
- **Evidence**: Initial trinket modal felt "cheap" without animation

### 2. Hover-to-Select + Click (Rejected)

**Proposal**: Hover mouse to highlight, click to activate (two-step)

**Rejected Because**:
- Requires mouse precision (keyboard users excluded)
- Extra step slows interaction
- Doesn't work with keyboard-only input
- **Counter**: Hold-and-release works for both mouse and keyboard

### 3. Different Colors Per Modal (Rejected)

**Proposal**: Each modal chooses own color scheme

**Rejected Because**:
- Inconsistent visual language (violates ADR-008)
- No universal meaning (green = confirm in one modal, blue in another)
- Harder to maintain (colors scattered across files)
- **Alternative**: ADR-008 color constants enforced everywhere

### 4. ESC for Back (Rejected)

**Proposal**: Use ESC key for "back one level"

**Rejected Because**:
- ESC already bound to main menu (global action)
- Conflict causes confusion (back one level vs exit to menu)
- Common in other games (ESC = menu, not back)
- **Alternative**: BACKSPACE semantic meaning ("delete last action")

---

## Success Metrics

- ✅ All modal buttons use `key_held_*` field (hold-and-release pattern)
- ✅ Button scale animations use lerp (no instant snapping)
- ✅ ADR-008 color constants used (`COLOR_BUTTON_BG`, `COLOR_BUTTON_HOVER`, etc.)
- ✅ Font scale = 1.0f for button labels (readability, per ADR-008)
- ✅ Hotkeys shown in format "(E)", "(S)", "(BACKSPACE)"
- ✅ Back buttons use BACKSPACE hotkey (not ESC)
- ✅ No double-trigger bugs (mutex input enforced)

**Verification**: Automated via FF-018 (fitness function runs on `make verify`)

---

## Migration Guide

### Adding Hold-and-Release to Existing Button

**Before** (instant activation):
```c
if (app.keyboard[SDL_SCANCODE_E]) {
    PerformAction();
}
```

**After** (hold-and-release):
```c
// Add state to modal struct
typedef struct MyModal {
    int key_held_button;  // -1 = none, 0 = button A, 1 = button B
} MyModal_t;

// In input handler
int prev_key_held = modal->key_held_button;
modal->key_held_button = -1;
if (app.keyboard[SDL_SCANCODE_E]) modal->key_held_button = 0;

if (prev_key_held == 0 && modal->key_held_button != 0) {
    PerformAction();  // Trigger on release
}
```

### Adding Scale Animation to Button

```c
// Add to modal struct
typedef struct MyModal {
    float button_scale;  // 1.0 = normal, 1.05 = hover, 0.95 = press
} MyModal_t;

// Initialize
modal->button_scale = 1.0f;

// Update every frame
float target_scale = pressed ? 0.95f : (hovered ? 1.05f : 1.0f);
modal->button_scale += (target_scale - modal->button_scale) * 10.0f * dt;

// Render with scale
int scaled_w = (int)(BUTTON_WIDTH * modal->button_scale);
int scaled_h = (int)(BUTTON_HEIGHT * modal->button_scale);
int scaled_x = button_x + (BUTTON_WIDTH - scaled_w) / 2;
int scaled_y = button_y + (BUTTON_HEIGHT - scaled_h) / 2;

a_DrawFilledRect((aRectf_t){scaled_x, scaled_y, scaled_w, scaled_h}, color);
```

---

## Related ADRs

- **ADR-003**: Daedalus Types (state stored in modal structs)
- **ADR-008**: Modal Design Consistency (color palette, layout)
- **ADR-012**: Reward Modal Animation (original hold-and-release implementation)

---

## References

**Files**:
- [src/scenes/components/trinketDropModal.c](../../src/scenes/components/trinketDropModal.c) - Reference implementation (lines 337-522, 723-804)
- [src/scenes/components/rewardModal.c](../../src/scenes/components/rewardModal.c) - Original pattern (lines 360-381)
- [include/scenes/components/trinketDropModal.h](../../include/scenes/components/trinketDropModal.h) - State struct with `key_held_button` field

**Fitness Function**:
- [architecture/fitness_functions/18_interactive_button_ux.py](../fitness_functions/18_interactive_button_ux.py)

**Bug Fixes That Led to This ADR**:
- 2025-11-28: Fixed double-trigger bug (hold E + click mouse = action twice)
- 2025-11-28: Changed ESC to BACKSPACE (menu conflict)
- 2025-11-28: Added red back button above slot grid (visibility)
