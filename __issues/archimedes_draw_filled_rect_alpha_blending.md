# Archimedes Bug Report: `a_DrawFilledRect()` Ignores Alpha Parameter

**Issue Type**: Bug Fix
**Component**: Rendering/Primitives
**Priority**: High
**Complexity**: Low
**Affects**: All projects using `a_DrawFilledRect()` with alpha < 255

---

## Problem Statement

The `a_DrawFilledRect()` function accepts an **alpha parameter** but does not enable SDL's **blend mode**, causing all alpha values to be **ignored**. Rectangles always render fully opaque regardless of the alpha value passed.

### Current Broken Behavior

```c
// Try to draw 10% opacity white rectangle
a_DrawFilledRect(100, 100, 200, 50, 255, 255, 255, 25);

// Expected: Semi-transparent white rectangle (barely visible)
// Actual:   Fully opaque white rectangle (alpha ignored!)
```

**Root Cause**: SDL's default blend mode is `SDL_BLENDMODE_NONE`, which means:
- RGB values are used
- Alpha values are **ignored**
- Everything renders fully opaque

---

## Current Implementation (BROKEN)

```c
void a_DrawFilledRect(const int x, const int y, const int w, const int h,
                      const int r, const int g, const int b, const int a)
{
  SDL_SetRenderDrawColor(app.renderer, r, g, b, a);  // ← Alpha passed to SDL
  SDL_Rect rect = {x, y, w, h};
  SDL_RenderFillRect(app.renderer, &rect);            // ← But blend mode not set!
  SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
}
```

**Problem**: Function signature promises alpha support, but implementation doesn't deliver.

---

## Proposed Fix

Enable `SDL_BLENDMODE_BLEND` before drawing, then restore blend mode after:

```c
void a_DrawFilledRect(const int x, const int y, const int w, const int h,
                      const int r, const int g, const int b, const int a)
{
  // Enable alpha blending
  SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_BLEND);

  // Draw with alpha
  SDL_SetRenderDrawColor(app.renderer, r, g, b, a);
  SDL_Rect rect = {x, y, w, h};
  SDL_RenderFillRect(app.renderer, &rect);

  // Restore defaults
  SDL_SetRenderDrawColor(app.renderer, 255, 255, 255, 255);
  SDL_SetRenderDrawBlendMode(app.renderer, SDL_BLENDMODE_NONE);
}
```

**Changes**:
1. Set blend mode to `SDL_BLENDMODE_BLEND` before drawing
2. Restore blend mode to `SDL_BLENDMODE_NONE` after drawing
3. No API changes - existing code works without modification

---

## Real-World Example (Card Fifty-Two)

### Use Case: Menu Item Hover Background

**Goal**: Show subtle 10% opacity white background when hovering over menu items to indicate clickable area.

**Current Code (doesn't work)**:
```c
// MenuItem hover background
if (item->is_hovered) {
    // Draws fully opaque white rectangle - looks bad!
    a_DrawFilledRect(left, top, width, height, 255, 255, 255, 25);
}
```

**With Fix (works as expected)**:
```c
// MenuItem hover background
if (item->is_hovered) {
    // Draws subtle 10% opacity white rectangle - perfect!
    a_DrawFilledRect(left, top, width, height, 255, 255, 255, 25);
}
```

**Impact**: Enables modern UI/UX patterns with subtle visual feedback.

---

## Additional Use Cases

### 1. Semi-Transparent Overlays

```c
// Darken screen for pause menu (50% opacity black)
a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 128);
```

### 2. Selection Highlights

```c
// Highlight selected card with subtle blue tint
a_DrawFilledRect(card_x, card_y, CARD_WIDTH, CARD_HEIGHT, 100, 150, 255, 60);
```

### 3. Tooltip Backgrounds

```c
// Semi-transparent background for tooltips
a_DrawFilledRect(tooltip_x, tooltip_y, tooltip_w, tooltip_h, 40, 40, 40, 200);
```

### 4. Modal Dialog Backgrounds

```c
// Dim entire screen behind modal (70% opacity)
a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 180);
```

---

## Performance Considerations

**Question**: Does enabling/disabling blend mode have performance cost?

**Answer**: Minimal. SDL blend mode changes are lightweight state changes.

**Optimization Note**: If drawing many transparent rectangles in sequence, could optimize by:
1. Set blend mode once at start
2. Draw all rectangles
3. Restore blend mode once at end

But for typical usage (1-5 rectangles per frame), the fix as written is fine.

---

## Backward Compatibility

**Breaking Changes**: None

**Existing Code Behavior**:
- Code passing `a = 255` (fully opaque): **No change** - still renders opaque
- Code passing `a < 255` (transparent): **Now works correctly** - renders transparent as intended

**Migration**: Zero. All existing code continues to work, transparency just starts working.

---

## Testing Checklist

- [ ] Fully opaque rectangles (`a = 255`) render correctly
- [ ] Semi-transparent rectangles (`a = 128`) blend with background
- [ ] Very transparent rectangles (`a = 25`) barely visible
- [ ] Completely transparent (`a = 0`) renders nothing
- [ ] Multiple overlapping transparent rectangles blend correctly
- [ ] No visual glitches or artifacts
- [ ] Performance remains acceptable (no measurable degradation)

---

## Alternative Considered

### Option A: Create separate `a_DrawFilledRectAlpha()` function

```c
void a_DrawFilledRectAlpha(int x, int y, int w, int h, int r, int g, int b, int a);
```

**Rejected because**:
- Duplicates existing function
- Splits API unnecessarily
- Doesn't fix the broken promise of existing function
- Existing function signature already has alpha parameter!

**Decision**: Fix the existing function to honor its own API contract.

---

## Benefits

✅ **Fixes broken API contract** - Function signature promises alpha, should deliver alpha
✅ **Enables modern UI/UX** - Semi-transparent overlays, hover effects, tooltips
✅ **Zero breaking changes** - Existing code continues to work
✅ **Simple fix** - Just 2 lines of code
✅ **Consistent with SDL best practices** - How SDL is meant to be used

---

## Discovered By

**Project**: Card Fifty-Two
**Context**: Implementing menu item hover backgrounds with 10% opacity white
**Date**: 2025-10-07
**Reporter**: User implementing section-based menu architecture

---

## References

- SDL2 Documentation: [SDL_SetRenderDrawBlendMode](https://wiki.libsdl.org/SDL_SetRenderDrawBlendMode)
- SDL2 Blend Modes: `SDL_BLENDMODE_NONE`, `SDL_BLENDMODE_BLEND`, `SDL_BLENDMODE_ADD`, `SDL_BLENDMODE_MOD`

---

**Status**: Awaiting fix in Archimedes rendering module
