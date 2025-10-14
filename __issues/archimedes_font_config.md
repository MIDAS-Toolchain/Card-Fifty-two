# Archimedes Font Config API Improvement

**Issue Type:** API Enhancement
**Target Project:** Archimedes
**Severity:** Low (Quality of Life)
**Status:** Proposed

## Problem Statement

The current `a_DrawText()` function has **too many parameters**, making it hard to read and prone to errors:

```c
a_DrawText("Card Fifty-Two", SCREEN_WIDTH / 2, 100,
           255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
           // ↑ R   ↑ G   ↑ B   ↑ font type      ↑ alignment     ↑ wrap width
```

**Issues:**
1. **8 parameters** - cognitive overload
2. **Color as RGB triplet** - should be `aColor_t` or integrated config
3. **Non-obvious ordering** - easy to mix up alignment and wrap width
4. **Repetitive** - same font config repeated for multiple text calls
5. **Hard to read** - function call spans multiple lines
6. **Not extensible** - adding new options means more parameters

## Current API

```c
void a_DrawText(char* text, int x, int y, int r, int g, int b,
                int font_type, int align, int max_width);
```

**Parameters breakdown:**
- `text` - The string to render ✅ (essential)
- `x, y` - Position ✅ (essential)
- `r, g, b` - Color components ❌ (should be struct)
- `font_type` - Font selection ❌ (should be in config)
- `align` - Text alignment ❌ (should be in config)
- `max_width` - Word wrap width ❌ (should be in config)

**Only 3 parameters are truly essential.** The rest should be configuration.

## Proposed Solution

### Option 1: Font Config Struct (Recommended)

```c
// Font configuration structure
typedef struct {
    int type;              // Font type (FONT_ENTER_COMMAND, FONT_GAME, etc.)
    aColor_t color;        // Text color (already an aColor_t)
    int align;             // Alignment (TEXT_ALIGN_LEFT, CENTER, RIGHT)
    int wrap_width;        // Word wrap width (0 = no wrap)
    float scale;           // Font scale multiplier (default: 1.0)
} aFontConfig_t;

// New API - clean and simple
void a_DrawTextStyled(const char* text, int x, int y, const aFontConfig_t* config);

// If config is NULL, use global default
extern aFontConfig_t a_default_font_config;
```

**Usage:**

```c
// Define reusable font styles
aFontConfig_t title_font = {
    .type = FONT_ENTER_COMMAND,
    .color = {255, 255, 255, 255},
    .align = TEXT_ALIGN_CENTER,
    .wrap_width = 0,
    .scale = 1.0f
};

aFontConfig_t body_font = {
    .type = FONT_GAME,
    .color = {200, 200, 200, 255},
    .align = TEXT_ALIGN_LEFT,
    .wrap_width = 400,
    .scale = 1.0f
};

// Clean, readable calls
a_DrawTextStyled("Card Fifty-Two", SCREEN_WIDTH / 2, 100, &title_font);
a_DrawTextStyled("Game instructions...", 50, 200, &body_font);

// Use default font (white, left-aligned, no wrap)
a_DrawTextStyled("Debug info", 10, 10, NULL);
```

### Option 2: Builder Pattern (Alternative)

```c
// Fluent API
aTextBuilder_t* builder = a_BeginText("Card Fifty-Two", x, y);
a_TextBuilder_Font(builder, FONT_ENTER_COMMAND);
a_TextBuilder_Color(builder, white);
a_TextBuilder_Align(builder, TEXT_ALIGN_CENTER);
a_TextBuilder_Draw(builder);  // Renders and frees
```

**Verdict:** Option 1 is simpler and more C-like.

## Benefits

✅ **Reduced cognitive load** - Only 3 params for simple cases
✅ **Reusable configurations** - Define once, use everywhere
✅ **Better defaults** - NULL config = sensible defaults
✅ **Easier to read** - One line for most cases
✅ **Extensible** - Add struct fields without breaking API
✅ **Type safety** - `aColor_t` instead of RGB ints
✅ **Consistent with Archimedes** - Already uses `aColor_t` elsewhere

## Implementation Plan

### Phase 1: Add New API (Non-Breaking)
1. Define `aFontConfig_t` struct in `aText.h`
2. Implement `a_DrawTextStyled()` function
3. Create `a_default_font_config` global
4. Update documentation

### Phase 2: Backward Compatibility
- Keep existing `a_DrawText()` as wrapper:
  ```c
  void a_DrawText(char* text, int x, int y, int r, int g, int b,
                  int font_type, int align, int max_width) {
      aFontConfig_t config = {
          .type = font_type,
          .color = {r, g, b, 255},
          .align = align,
          .wrap_width = max_width,
          .scale = 1.0f
      };
      a_DrawTextStyled(text, x, y, &config);
  }
  ```

### Phase 3: Migration (Future)
- Mark `a_DrawText()` as deprecated
- Update examples to use new API
- Eventually remove old API in next major version

## Example Refactor (Card Fifty-Two)

### Before (Current):
```c
void Render(void) {
    app.background = (aColor_t){10, 80, 30, 255};

    a_DrawText("Card Fifty-Two", SCREEN_WIDTH / 2, 100,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    a_DrawText("Tech Demo - Archimedes & Daedalus", SCREEN_WIDTH / 2, 160,
               200, 200, 200, FONT_GAME, TEXT_ALIGN_CENTER, 0);

    a_DrawText("Press ESC to quit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100,
               255, 255, 0, FONT_GAME, TEXT_ALIGN_CENTER, 0);

    char status[128];
    snprintf(status, sizeof(status), "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawText(status, SCREEN_WIDTH - 10, 10,
               0, 255, 0, FONT_GAME, TEXT_ALIGN_RIGHT, 0);
}
```

### After (Proposed):
```c
// Define font styles once (could be global)
aFontConfig_t title_font = {
    .type = FONT_ENTER_COMMAND,
    .color = white,
    .align = TEXT_ALIGN_CENTER
};

aFontConfig_t subtitle_font = {
    .type = FONT_GAME,
    .color = {200, 200, 200, 255},
    .align = TEXT_ALIGN_CENTER
};

aFontConfig_t warning_font = {
    .type = FONT_GAME,
    .color = yellow,
    .align = TEXT_ALIGN_CENTER
};

aFontConfig_t debug_font = {
    .type = FONT_GAME,
    .color = {0, 255, 0, 255},
    .align = TEXT_ALIGN_RIGHT
};

void Render(void) {
    app.background = (aColor_t){10, 80, 30, 255};

    a_DrawTextStyled("Card Fifty-Two", SCREEN_WIDTH / 2, 100, &title_font);
    a_DrawTextStyled("Tech Demo - Archimedes & Daedalus", SCREEN_WIDTH / 2, 160, &subtitle_font);
    a_DrawTextStyled("Press ESC to quit", SCREEN_WIDTH / 2, SCREEN_HEIGHT - 100, &warning_font);

    char status[128];
    snprintf(status, sizeof(status), "FPS: %.1f", 1.0f / a_GetDeltaTime());
    a_DrawTextStyled(status, SCREEN_WIDTH - 10, 10, &debug_font);
}
```

**Much cleaner!** Font styles are defined once and reused.

## Default Font Config

```c
// In Archimedes
aFontConfig_t a_default_font_config = {
    .type = FONT_GAME,
    .color = {255, 255, 255, 255},  // White
    .align = TEXT_ALIGN_LEFT,
    .wrap_width = 0,
    .scale = 1.0f
};

// Users can modify the default
a_default_font_config.color = yellow;

// Or create custom defaults
aFontConfig_t my_default = a_default_font_config;
my_default.align = TEXT_ALIGN_CENTER;
```

## Alternative: Convenience Macros

For extremely simple cases, provide macros:

```c
#define a_DrawTextSimple(text, x, y) \
    a_DrawTextStyled(text, x, y, NULL)

#define a_DrawTextCentered(text, x, y) \
    a_DrawTextStyled(text, x, y, &(aFontConfig_t){ \
        .type = FONT_GAME, \
        .color = white, \
        .align = TEXT_ALIGN_CENTER \
    })

// Usage
a_DrawTextSimple("Hello", 10, 10);
a_DrawTextCentered("Title", SCREEN_WIDTH/2, 100);
```

## Questions for Archimedes Team

1. Should we use `aColor_t` or keep RGB ints in the config?
2. Should `scale` be part of font config or global `app.font_scale`?
3. Should we add shadow/outline options to the config struct?
4. What's the best name? `a_DrawTextStyled`, `a_DrawTextEx`, `a_DrawTextConfig`?
5. Should NULL config use global default or hardcoded defaults?

## Related Issues

- Font scale is currently global (`app.font_scale`) - should it be per-config?
- Color is already `aColor_t` in Archimedes - use it consistently
- Text alignment could benefit from vertical alignment option

## Conclusion

The current API works but is **unnecessarily verbose**. A font config struct would:
- Reduce parameters from 9 to 4 (text, x, y, config)
- Make code more readable and maintainable
- Allow reusable font styles
- Be more extensible for future features

**Recommendation:** Implement `aFontConfig_t` + `a_DrawTextStyled()` while keeping old API for compatibility.
