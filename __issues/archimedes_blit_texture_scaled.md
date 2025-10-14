# Archimedes Feature Request: Scaled Texture Rendering Function

**Issue Type**: Feature Request
**Component**: Rendering/Texture System
**Priority**: High
**Complexity**: Low

---

## Problem Statement

Currently, Archimedes only provides `a_BlitTexture()` which renders textures at their **natural pixel size**. When working with sprite assets that need to be scaled (like card PNGs, character sprites, UI elements), there's no built-in way to render them at different sizes without breaking Archimedes' abstraction layer.

### Real-World Example (from Card Fifty-Two)

**Scenario**: Card game using PNG sprite assets

- **Card PNG sprites**: 32×48 pixels (natural size)
- **Need to render at**: 100×140 pixels (`CARD_WIDTH` × `CARD_HEIGHT`)
- **Current options**:
  1. Use `a_BlitTexture()` → Cards render tiny (32×48)
  2. Break abstraction and use raw SDL → Defeats purpose of Archimedes
  3. Pre-scale assets → Inflexible, wastes memory

```c
// Current: Cards render tiny!
a_BlitTexture(card_texture, x, y);  // Only 32×48 on screen

// Workaround: Break Archimedes abstraction
SDL_Rect dst = {x, y, CARD_WIDTH, CARD_HEIGHT};
SDL_RenderCopy(app.renderer, card_texture, NULL, &dst);  // Ugly!
```

**This breaks the Archimedes philosophy** of abstracting SDL2 details.

---

## Proposed Solution: `a_BlitTextureScaled()`

Add a scaled texture rendering function to Archimedes' rendering API:

```c
/**
 * a_BlitTextureScaled - Render texture scaled to specific dimensions
 *
 * @param texture SDL texture to render
 * @param x Screen X position (top-left corner)
 * @param y Screen Y position (top-left corner)
 * @param w Scaled width in pixels
 * @param h Scaled height in pixels
 *
 * Renders texture scaled to fit w×h rectangle. Texture is stretched
 * to exactly match dimensions (no aspect ratio preservation).
 */
void a_BlitTextureScaled(SDL_Texture* texture, int x, int y, int w, int h);
```

### Implementation (suggestion)

```c
void a_BlitTextureScaled(SDL_Texture* texture, int x, int y, int w, int h) {
    if (!texture) {
        return;
    }

    SDL_Rect dst = {x, y, w, h};
    SDL_RenderCopy(app.renderer, texture, NULL, &dst);
}
```

**Simple, clean, consistent with Archimedes API style.**

---

## Usage Example

### Card Rendering

```c
// Load card sprite (32×48 PNG)
SDL_Texture* card_tex = a_LoadTexture("resources/cards/0.png");

// Render scaled to game card size (100×140)
a_BlitTextureScaled(card_tex, x, y, CARD_WIDTH, CARD_HEIGHT);

// Result: Card renders at proper game size, scaled smoothly
```

### Before vs After

**Before (current workaround):**
```c
static void RenderCard(SDL_Texture* tex, int x, int y) {
    // Breaks abstraction - touches SDL directly
    SDL_Rect dst = {x, y, CARD_WIDTH, CARD_HEIGHT};
    SDL_RenderCopy(app.renderer, tex, NULL, &dst);
}
```

**After (with a_BlitTextureScaled):**
```c
static void RenderCard(SDL_Texture* tex, int x, int y) {
    // Clean Archimedes API
    a_BlitTextureScaled(tex, x, y, CARD_WIDTH, CARD_HEIGHT);
}
```

---

## Additional Use Cases

### 1. Character Sprites
```c
// 16×16 character sprite scaled to 64×64 for retro aesthetic
a_BlitTextureScaled(player_sprite, x, y, 64, 64);
```

### 2. UI Icons
```c
// 24×24 icon scaled for high-DPI displays
a_BlitTextureScaled(icon, x, y, icon_size, icon_size);
```

### 3. Responsive UI
```c
// Background image scaled to fill screen
a_BlitTextureScaled(bg_tex, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
```

---

## Alternative Considered

### Option A: Add scale parameter to existing `a_BlitTexture()`

```c
void a_BlitTexture(SDL_Texture* texture, int x, int y, float scale);
```

**Rejected because:**
- Breaks existing API (function signature change)
- Doesn't support non-uniform scaling (stretch width ≠ height)
- Less flexible than explicit width/height

### Option B: `a_BlitTextureRect()` with source and destination rects

Already exists, but:
- More complex API for simple scaling use case
- Requires manual SDL_Rect construction
- Overkill when you just want "render this at WxH"

**Decision**: New `a_BlitTextureScaled()` function is cleanest solution.

---

## Benefits

✅ **Maintains Archimedes abstraction** - No raw SDL in game code
✅ **Simple API** - 5 parameters, intuitive usage
✅ **Flexible** - Supports any scaling ratio
✅ **Consistent** - Follows `a_Blit*` naming convention
✅ **Low complexity** - Trivial 5-line implementation

---

## Implementation Checklist

- [ ] Add function declaration to `Archimedes.h`
- [ ] Implement function in rendering module
- [ ] Add to Archimedes documentation
- [ ] Test with various texture sizes
- [ ] Verify performance (should be identical to manual SDL_RenderCopy)

---

## Success Metrics

- ✅ Card Fifty-Two can render 32×48 sprites at 100×140 without SDL workarounds
- ✅ No raw `SDL_RenderCopy` calls in game code
- ✅ Function adopted in 3+ Archimedes projects

---

## Proposed by

**Project**: Card Fifty-Two
**Context**: Switching from text-generated cards to PNG sprites
**Date**: 2025-10-07

---

**Status**: Awaiting implementation in Archimedes
