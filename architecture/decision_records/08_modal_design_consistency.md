# ADR-008: Consistent Modal Design and Positioning

## Context

UI modals (tooltips, character stats, combat stats, status effects) need to display supplementary information on hover. Each modal was implemented independently, leading to inconsistent sizing, positioning, and styling. The sanity modal overlapped with the sidebar, tooltips had varying font sizes, and there was no clear pattern for full-height vs. auto-sized modals.

**Alternatives Considered:**

1. **Dynamic height modals** (calculate height based on content)
   - Pros: Compact, only takes needed space, follows tooltip paradigm
   - Cons: Varying heights look inconsistent, harder to align side-by-side
   - Cost: Height calculation logic in every modal, alignment math

2. **Full-height modals** (extend from top bar to bottom)
   - Pros: Consistent visual weight, easier alignment, maximizes info space
   - Cons: Wastes space for short content, feels heavy
   - Cost: All modals must handle full height, padding logic

3. **Mixed approach** (tooltips auto-size, panels full-height)
   - Pros: Best of both worlds, tooltips stay compact, info panels feel substantial
   - Cons: Two different patterns to remember, inconsistent feel
   - Cost: Developers must decide which pattern applies

4. **Anchored to trigger** (modal positioned relative to hover target)
   - Pros: Clear spatial relationship, follows mouse proximity
   - Cons: Can overflow screen edges, overlaps game area
   - Cost: Collision detection, boundary clamping logic

## Decision

**Mixed approach: Small tooltips (abilities, status effects, chips) auto-size. Large info panels (character stats, combat stats) use full game area height and anchor to sidebar edge.**

Full-height modals start at `TOP_BAR_HEIGHT` and extend to `SCREEN_HEIGHT`. Positioning is relative to `SIDEBAR_WIDTH`, not hover trigger, to avoid overlap.

**Justification:**

1. **Visual Hierarchy**: Tooltips are transient hints (auto-size). Panels are persistent info displays (full-height).
2. **Predictable Positioning**: Full-height modals anchor to sidebar edge (always `SIDEBAR_WIDTH + gap`), not portrait position.
3. **No Overlap**: Sidebar width is constant (280px), modals start at 290px, preventing overlap.
4. **Font Consistency**: All description text uses `scale = 1.1f` (matching ability/status tooltips).
5. **Side-by-Side**: Full-height modals enable clean horizontal layout (sanity + combat stats).

## Consequences

**Positive:**
- ✅ No sidebar overlap (modals anchored to `SIDEBAR_WIDTH`, not portrait)
- ✅ Consistent font sizes across all tooltips (`scale = 1.1f` for descriptions)
- ✅ Clear distinction: small tooltips vs. large info panels
- ✅ Full-height modals enable multi-column layouts (character + combat stats)
- ✅ Easier to add new info panels (just follow full-height pattern)

**Negative:**
- ❌ Two modal patterns to remember (auto-size vs. full-height)
- ❌ Full-height modals waste space if content is short
- ❌ Developers must decide which pattern applies to new modals

**Accepted Trade-offs:**
- ✅ Accept dual patterns for visual clarity (tooltips ≠ info panels)
- ✅ Accept wasted space for consistency (better than ragged heights)
- ❌ NOT accepting: Modals positioned relative to hover trigger (causes overlap)

**Pattern Used:**

**Small tooltips** (abilities, status effects, chips, trinkets):
```c
// Auto-size based on content
int tooltip_height = padding;
tooltip_height += title_height + spacing;
tooltip_height += description_height + spacing;
tooltip_height += padding;

a_DrawFilledRect(x, y, tooltip_width, tooltip_height, ...);
```

**Large info panels** (character stats, combat stats):
```c
// Full game area height
int modal_height = SCREEN_HEIGHT - TOP_BAR_HEIGHT;

// Anchor to sidebar edge (not hover trigger)
int modal_x = SIDEBAR_WIDTH + 10;
int modal_y = TOP_BAR_HEIGHT;

ShowCharacterStatsModal(modal, modal_x, modal_y);
```

**Font sizing** (all modals):
```c
// Titles: 1.0f scale
aFontConfig_t title_config = {
    .type = FONT_ENTER_COMMAND,
    .scale = 1.0f
};

// Descriptions: 1.1f scale (readability)
aFontConfig_t desc_config = {
    .type = FONT_GAME,
    .scale = 1.1f  // Consistent across all tooltips
};
```

**Color scheme** (modern dark):
```c
#define COLOR_BG       ((aColor_t){20, 20, 30, 230})     // Dark background
#define COLOR_BORDER   ((aColor_t){100, 100, 100, 200})  // Light gray
#define COLOR_TITLE    ((aColor_t){235, 237, 233, 255})  // Off-white
#define COLOR_TEXT     ((aColor_t){200, 200, 200, 255})  // Light gray
```

**Success Metrics:**
- No modals overlap with sidebar or game area
- All description text readable at `scale = 1.1f`
- Full-height modals align top-to-bottom cleanly
- New modals follow one of two clear patterns (tooltip or panel)

**Evidence:**

**Before:** Sanity modal was auto-sized, overlapped sidebar, used `scale = 0.9f` (hard to read).

**After:** Sanity modal is full-height, anchored to `SIDEBAR_WIDTH + 10`, uses `scale = 1.1f`.

```c
// src/scenes/sections/leftSidebarSection.c:465
int sanity_modal_x = SIDEBAR_WIDTH + 10;  // No overlap
int modal_y = TOP_BAR_HEIGHT;

// src/scenes/components/characterStatsModal.c:110
int modal_height = SCREEN_HEIGHT - TOP_BAR_HEIGHT;  // Full height

// src/scenes/components/characterStatsModal.c:268
.scale = 1.1f  // Match ability/status tooltip size
```

**Future Patterns:**
- Small transient info (damage numbers, ability procs) → auto-size tooltip
- Large persistent info (inventory, skill tree, map) → full-height panel anchored to sidebar
