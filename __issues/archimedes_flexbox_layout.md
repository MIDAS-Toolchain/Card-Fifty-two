# Archimedes Feature Request: CSS-Like Flexbox Container System

**Issue Type**: Feature Request
**Component**: UI Layout System
**Priority**: Medium
**Complexity**: Medium-High

---

## Problem Statement

Currently, Archimedes requires **manual positioning** of all UI elements using hardcoded X/Y coordinates. This approach is:

- ❌ **Error-prone**: Easy to create overlapping elements
- ❌ **Brittle**: Changing one element's size breaks the entire layout
- ❌ **Unresponsive**: Doesn't adapt to dynamic content sizes
- ❌ **Tedious**: Requires recalculating positions for every layout change

### Real-World Example (from Card Fifty-Two)

When building a Blackjack game, we had to manually fix overlapping text **three times**:

```c
// Manual positioning - fragile and hard to maintain
RenderPlayerInfo(g_dealer, 100, 80);      // Dealer name/score
RenderHand(&g_dealer->hand, 100, 130);    // Dealer cards (manually calculated gap)

RenderPlayerInfo(g_human_player, 100, 365);  // Player name/score
RenderHand(&g_human_player->hand, 100, 440);  // Player cards

// Problem: If dealer gets 5+ cards, layout breaks!
// Problem: Changing gap requires updating 4+ magic numbers
```

**Issues Encountered**:
1. Dealer text overlapped cards (y=90 → y=80, cards y=120 → y=130)
2. Player text overlapped cards (y=350 → y=365, cards y=380 → y=440)
3. Buttons overlapped cards (y=550 → y=580, y=600 → y=630)

Every time content size changed, manual recalculation was needed.

---

## Proposed Solution: Flexbox Layout System

Implement a **CSS Flexbox-inspired** container system that automatically calculates child positions based on layout rules.

### Core Concept

```
┌─────────────────────────────────────────┐
│  FlexContainer (direction: column)      │
│  ┌───────────────────────────────────┐  │
│  │ Child 1 (auto-positioned)         │  │
│  └───────────────────────────────────┘  │
│           ↓ gap: 20px                   │
│  ┌───────────────────────────────────┐  │
│  │ Child 2 (auto-positioned)         │  │
│  └───────────────────────────────────┘  │
│           ↓ gap: 20px                   │
│  ┌───────────────────────────────────┐  │
│  │ Child 3 (auto-positioned)         │  │
│  └───────────────────────────────────┘  │
└─────────────────────────────────────────┘
```

---

## API Design

### Enums

```c
typedef enum {
    FLEX_DIRECTION_ROW,     // Horizontal layout (→)
    FLEX_DIRECTION_COLUMN   // Vertical layout (↓)
} FlexDirection_t;

typedef enum {
    FLEX_JUSTIFY_START,          // Align to start
    FLEX_JUSTIFY_CENTER,         // Center items
    FLEX_JUSTIFY_END,            // Align to end
    FLEX_JUSTIFY_SPACE_BETWEEN,  // Equal spacing, edges aligned
    FLEX_JUSTIFY_SPACE_AROUND    // Equal spacing, including edges
} FlexJustify_t;

typedef enum {
    FLEX_ALIGN_START,    // Cross-axis start
    FLEX_ALIGN_CENTER,   // Cross-axis center
    FLEX_ALIGN_END       // Cross-axis end
} FlexAlign_t;
```

### Structures

```c
typedef struct FlexChild {
    void* widget;           // Pointer to widget/renderable
    int width, height;      // Child dimensions
    int flex_grow;          // Growth factor (default: 0)
    int flex_shrink;        // Shrink factor (default: 1)
    int calculated_x;       // Auto-calculated position
    int calculated_y;
} FlexChild_t;

typedef struct FlexContainer {
    int x, y;                    // Container position
    int width, height;           // Container size
    FlexDirection_t direction;   // Layout direction
    FlexJustify_t justify;       // Main-axis alignment
    FlexAlign_t align;           // Cross-axis alignment
    int gap;                     // Spacing between children
    int padding_top;
    int padding_right;
    int padding_bottom;
    int padding_left;
    dArray_t* children;          // Array of FlexChild_t
} FlexContainer_t;
```

### Core API

```c
// Container Management
FlexContainer_t* a_CreateFlexContainer(int x, int y, int width, int height);
void a_DestroyFlexContainer(FlexContainer_t* container);

// Container Configuration
void a_FlexSetDirection(FlexContainer_t* container, FlexDirection_t direction);
void a_FlexSetJustify(FlexContainer_t* container, FlexJustify_t justify);
void a_FlexSetAlign(FlexContainer_t* container, FlexAlign_t align);
void a_FlexSetGap(FlexContainer_t* container, int gap);
void a_FlexSetPadding(FlexContainer_t* container, int top, int right, int bottom, int left);

// Child Management
void a_FlexAddChild(FlexContainer_t* container, void* widget, int width, int height);
void a_FlexRemoveChild(FlexContainer_t* container, int index);
void a_FlexClearChildren(FlexContainer_t* container);

// Layout & Rendering
void a_FlexLayout(FlexContainer_t* container);      // Calculate positions
void a_FlexRender(FlexContainer_t* container);      // Render all children
void a_FlexDebugBounds(FlexContainer_t* container); // Draw debug outlines

// Child Queries
int a_FlexGetChildX(FlexContainer_t* container, int child_index);
int a_FlexGetChildY(FlexContainer_t* container, int child_index);
```

---

## Usage Example: Card Game Layout

### Before (Manual Positioning)

```c
// Hard to maintain - overlaps easily
void RenderGameUI(void) {
    RenderPlayerInfo(dealer, 100, 80);
    RenderHand(dealer_hand, 100, 130);

    RenderPlayerInfo(player, 100, 365);
    RenderHand(player_hand, 100, 440);

    RenderBettingButtons(400, 580);  // If cards grow, this breaks!
}
```

### After (Flexbox Layout)

```c
// Automatic, maintainable, no overlaps
FlexContainer_t* game_layout;

void InitGameUI(void) {
    // Main vertical container
    game_layout = a_CreateFlexContainer(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    a_FlexSetDirection(game_layout, FLEX_DIRECTION_COLUMN);
    a_FlexSetJustify(game_layout, FLEX_JUSTIFY_SPACE_BETWEEN);
    a_FlexSetPadding(game_layout, 60, 0, 50, 0);

    // Dealer section
    FlexContainer_t* dealer_section = a_CreateFlexContainer(100, 0, 800, 250);
    a_FlexSetDirection(dealer_section, FLEX_DIRECTION_COLUMN);
    a_FlexSetGap(dealer_section, 20);
    a_FlexAddChild(dealer_section, dealer_info, 600, 30);
    a_FlexAddChild(dealer_section, dealer_hand, 600, 140);

    // Player section
    FlexContainer_t* player_section = a_CreateFlexContainer(100, 0, 800, 250);
    a_FlexSetDirection(player_section, FLEX_DIRECTION_COLUMN);
    a_FlexSetGap(player_section, 20);
    a_FlexAddChild(player_section, player_info, 600, 30);
    a_FlexAddChild(player_section, player_hand, 600, 140);

    // Button row
    FlexContainer_t* button_row = a_CreateFlexContainer(0, 0, SCREEN_WIDTH, 70);
    a_FlexSetDirection(button_row, FLEX_DIRECTION_ROW);
    a_FlexSetJustify(button_row, FLEX_JUSTIFY_CENTER);
    a_FlexSetGap(button_row, 20);
    a_FlexAddChild(button_row, hit_button, 120, 60);
    a_FlexAddChild(button_row, stand_button, 120, 60);
    a_FlexAddChild(button_row, double_button, 120, 60);

    // Add sections to main layout
    a_FlexAddChild(game_layout, dealer_section, 800, 250);
    a_FlexAddChild(game_layout, player_section, 800, 250);
    a_FlexAddChild(game_layout, button_row, SCREEN_WIDTH, 70);
}

void RenderGameUI(void) {
    a_FlexLayout(game_layout);   // Auto-calculate positions
    a_FlexRender(game_layout);   // Render all children
}
```

**Benefits**:
- ✅ No manual Y coordinate calculations
- ✅ Changing `gap` updates entire layout instantly
- ✅ Adding/removing elements doesn't break layout
- ✅ Responsive to content size changes
- ✅ Readable, maintainable code

---

## Implementation Phases

### Phase 1: MVP (Basic Flexbox)
- [ ] Basic `FLEX_DIRECTION_ROW` and `FLEX_DIRECTION_COLUMN`
- [ ] `FLEX_JUSTIFY_START`, `FLEX_JUSTIFY_CENTER`, `FLEX_JUSTIFY_END`
- [ ] Fixed gap spacing
- [ ] Single-level containers (no nesting)
- [ ] Manual width/height for children

### Phase 2: Enhanced Layout
- [ ] `FLEX_JUSTIFY_SPACE_BETWEEN`, `FLEX_JUSTIFY_SPACE_AROUND`
- [ ] `FLEX_ALIGN_START`, `FLEX_ALIGN_CENTER`, `FLEX_ALIGN_END`
- [ ] Padding support (top, right, bottom, left)
- [ ] Nested containers

### Phase 3: Responsive Features
- [ ] `flex_grow` for expandable children
- [ ] `flex_shrink` for collapsible children
- [ ] Auto-sizing children based on content
- [ ] Min/max width/height constraints

### Phase 4: Advanced Features
- [ ] Flex-wrap for multi-line layouts
- [ ] Reverse direction
- [ ] Per-child alignment overrides
- [ ] Gap between rows/columns separately

---

## Technical Considerations

### Data Structures
- Use Daedalus `dArray_t` for children storage
- Store calculated positions in `FlexChild_t` struct
- Lazy calculation: only recalculate on layout changes

### Performance
- Layout calculation is O(n) where n = child count
- Cache calculated positions until container/children change
- Render optimization: batch SDL calls per container

### Memory Management
- Containers own their child array
- Widgets are referenced, not owned (caller manages widget lifetime)
- `a_DestroyFlexContainer()` only frees container + array, not widgets

### Integration with Existing Code
- Non-breaking: Existing manual positioning still works
- Opt-in: Only affects code that uses flex containers
- Compatible with existing Archimedes rendering functions

---

## Alternatives Considered

### 1. Grid Layout System
**Pros**: Better for 2D layouts
**Cons**: More complex, less intuitive for simple vertical/horizontal stacking

### 2. Absolute Positioning with Constraints
**Pros**: More flexible
**Cons**: Still requires manual positioning logic

### 3. Immediate Mode UI (Dear ImGui style)
**Pros**: Very simple API
**Cons**: Different paradigm from Archimedes' retained mode

**Decision**: Flexbox is the best fit - familiar to web developers, handles 90% of game UI layouts, progressive enhancement path.

---

## Success Metrics

- ✅ Card Fifty-Two can refactor to zero hardcoded Y positions
- ✅ Reducing overlap bugs by 100% in projects using flexbox
- ✅ Layout code reduced by 50%+ lines
- ✅ Developer adoption in 3+ Archimedes projects

---

## References

- [CSS Flexbox Specification](https://www.w3.org/TR/css-flexbox-1/)
- [Flutter's Flex Widget](https://api.flutter.dev/flutter/widgets/Flex-class.html)
- [React Native Flexbox](https://reactnative.dev/docs/flexbox)
- Daedalus dArray_t: Used for children storage

---

## Proposed by

**Project**: Card Fifty-Two
**Context**: Spent 3 iterations fixing UI overlaps with manual positioning
**Date**: 2025-10-07

---

**Would you like me to file this as a GitHub issue?** If you provide the Archimedes repository URL, I can help format it for direct submission.
