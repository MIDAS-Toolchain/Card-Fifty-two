# String Handling Decision Guide - Card Fifty-Two Internal Wiki

**Last Updated:** 2025-10-14
**Status:** Living Document

## TL;DR: The Golden Rule

**Ask yourself:** "Am I **storing** a string or **building** a string?"

- **Storing** (labels, names, config) → `char[]` + `strncpy()`
- **Building** (formatting, concatenation) → `dString_t*` + Daedalus API

---

## Part 1: When to Use `char[]` Fixed Buffers

### Pattern: Static Storage

```c
typedef struct {
    char label[256];     // ✅ Button label
    char hotkey[64];     // ✅ Hotkey hint
    char name[128];      // ✅ Player name
} Component_t;
```

### Use `char[]` When:

1. **Component Labels**
   - Button text ("Hit", "Stand", "Bet")
   - MenuItem labels ("Play", "Settings", "Quit")
   - Section titles ("CARD FIFTY-TWO", "Dealer's Hand")

2. **Names & Identifiers**
   - Player names (set once, rarely changed)
   - Configuration strings
   - Static messages

3. **Maximum Length is Known**
   - UI elements have practical limits (256 chars for labels)
   - Names fit in reasonable buffers (128 chars)

4. **Performance Critical**
   - No heap allocation overhead
   - No `d_InitString()` / `d_DestroyString()` calls
   - Cache-friendly (struct-embedded data)

### Code Example: Button Component

```c
// ✅ CORRECT - Static storage
typedef struct Button {
    int x, y, w, h;
    char label[256];       // Static label
    char hotkey[64];       // Static hotkey
    bool enabled;
} Button_t;

Button_t* CreateButton(int x, int y, int w, int h, const char* label) {
    Button_t* btn = malloc(sizeof(Button_t));

    // Safe string copy
    strncpy(btn->label, label ? label : "", sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';  // Guarantee null termination

    btn->hotkey[0] = '\0';  // Empty string
    return btn;
}

void SetButtonLabel(Button_t* btn, const char* label) {
    if (!btn) return;
    strncpy(btn->label, label ? label : "", sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';
}

void DestroyButton(Button_t** btn) {
    if (!btn || !*btn) return;
    free(*btn);           // No string cleanup needed!
    *btn = NULL;
}

// Rendering
void RenderButton(const Button_t* btn) {
    // Cast safe: a_DrawText is read-only
    a_DrawText((char*)btn->label, btn->x, btn->y, ...);
}
```

### Benefits of `char[]`:
- ✅ Simple lifecycle (no d_DestroyString)
- ✅ No heap fragmentation
- ✅ Fast: no malloc/free overhead
- ✅ Predictable memory usage
- ✅ Thread-safe (no shared heap state)

---

## Part 2: When to Use `dString_t*` Dynamic Strings

### Pattern: Dynamic Building

```c
// ✅ CORRECT - Building formatted string
dString_t* message = d_InitString();
d_FormatString(message, "Player: %s | Score: %d | Bet: %d",
               player->name, score, bet);
a_DrawText((char*)d_PeekString(message), x, y, ...);
d_DestroyString(message);
```

### Use `dString_t*` When:

1. **Formatting with Variables**
   - Score displays: `"Score: 21 (BLACKJACK!)"`
   - Status messages: `"Player: Alice | Chips: 500 | Bet: 50"`
   - Dynamic error messages

2. **String Concatenation**
   - Building multi-part messages
   - Joining strings with separators
   - Template substitution

3. **Unknown Final Length**
   - User input parsing
   - Log message assembly
   - File path construction with many parts

4. **Complex String Operations**
   - Padding, centering, formatting
   - Progress bars, tables
   - Multi-line text construction

### Code Example: Player Section Rendering

```c
void RenderPlayerSection(Player_t* player, int y) {
    // ✅ CORRECT - Dynamic formatting
    dString_t* info = d_InitString();
    d_FormatString(info, "%s: %d%s",
                   player->name,           // char[] in struct
                   player->hand.total_value,
                   player->hand.is_blackjack ? " (BLACKJACK!)" :
                   player->hand.is_bust ? " (BUST)" : "");

    // Cast safe: a_DrawText is read-only, dString_t used for dynamic formatting
    a_DrawText((char*)d_PeekString(info), SCREEN_WIDTH / 2, y, ...);
    d_DestroyString(info);

    // Another dynamic string
    dString_t* chips_info = d_InitString();
    d_FormatString(chips_info, "Chips: %d | Bet: %d",
                   player->chips, player->current_bet);
    // Cast safe: a_DrawText is read-only, dString_t used for dynamic formatting
    a_DrawText((char*)d_PeekString(chips_info), SCREEN_WIDTH / 2, y + 30, ...);
    d_DestroyString(chips_info);
}
```

### Daedalus String API Quick Reference

```c
// Creation & Destruction
dString_t* str = d_InitString();           // Create empty
d_DestroyString(str);                      // Free memory

// Setting Content
d_SetString(str, "Hello", 0);              // Replace content
d_ClearString(str);                        // Empty the string

// Appending
d_AppendToString(str, " World", 0);        // Append C-string
d_AppendCharToString(str, '!');            // Append single char
d_AppendIntToString(str, 42);              // Append integer
d_AppendFloatToString(str, 3.14, 2);       // Append float with decimals

// Formatting (like sprintf)
d_FormatString(str, "Score: %d", score);   // Format like printf

// Getting Content
const char* text = d_PeekString(str);      // Read-only pointer (no alloc)
char* copy = d_DumpString(str, NULL);      // Allocate copy (caller must free!)

// Utility
size_t len = d_GetLengthOfString(str);     // Get length
bool empty = d_IsStringInvalid(str);       // Check if NULL/empty
```

### Benefits of `dString_t*`:
- ✅ No buffer overflow (automatic resizing)
- ✅ Complex formatting (`d_FormatString`)
- ✅ String building operations
- ✅ No manual size calculation
- ✅ Encapsulated memory management

---

## Part 3: Common Mistakes & Anti-Patterns

### ❌ WRONG: Using `dString_t*` for Static Labels

```c
// ❌ DON'T DO THIS
typedef struct Button {
    dString_t* label;      // Overkill for static label!
} Button_t;

Button_t* CreateButton(const char* label) {
    Button_t* btn = malloc(sizeof(Button_t));
    btn->label = d_InitString();           // Unnecessary heap allocation
    d_SetString(btn->label, label, 0);     // Just storing a string
    return btn;
}

void DestroyButton(Button_t** btn) {
    d_DestroyString((*btn)->label);        // Extra cleanup step
    free(*btn);
    *btn = NULL;
}
```

**Problems:**
- Heap allocates for every button label
- Requires cleanup in destructor
- Slower than fixed buffer
- Over-engineered for simple use case

**Fix:** Use `char label[256]` instead!

---

### ❌ WRONG: Using `d_DumpString()` Every Frame

```c
// ❌ MEMORY LEAK - Don't do this!
void RenderSection(Section_t* section) {
    char* title = d_DumpString(section->title, NULL);  // Allocates memory
    a_DrawText(title, x, y, ...);
    // Missing: free(title);  ← LEAK!
}
```

**Problems:**
- `d_DumpString()` allocates new memory every call
- Leaks memory if not freed
- Called 60 times per second = massive leak

**Fix:** Use `d_PeekString()` instead (no allocation):

```c
// ✅ CORRECT
void RenderSection(Section_t* section) {
    // Cast safe: a_DrawText is read-only, dString_t used for dynamic formatting
    a_DrawText((char*)d_PeekString(section->title), x, y, ...);
    // No free() needed - d_PeekString doesn't allocate
}
```

---

### ❌ WRONG: Manual `snprintf` with Fixed Buffers

```c
// ❌ OLD WAY - Manual buffer management
char message[128];
snprintf(message, sizeof(message), "Score: %d | Bet: %d", score, bet);
a_DrawText(message, x, y, ...);
```

**Problems:**
- Fixed size (what if it overflows?)
- Manual size tracking
- No bounds checking beyond buffer size

**Fix:** Use `dString_t` for dynamic content:

```c
// ✅ BETTER - Daedalus handles resizing
dString_t* message = d_InitString();
d_FormatString(message, "Score: %d | Bet: %d", score, bet);
a_DrawText((char*)d_PeekString(message), x, y, ...);
d_DestroyString(message);
```

---

## Part 4: Decision Flowchart

```
Is the string content dynamic (contains variables)?
├─ YES → Does it change every frame?
│  ├─ YES → Use dString_t* (allocate/format/destroy per frame)
│  │         Example: score display, status messages
│  └─ NO  → Use dString_t* (allocate once, update as needed)
│            Example: log messages, tooltips
│
└─ NO  → Is the string a static label/name?
   ├─ YES → Use char[] (simple static storage)
   │         Example: button labels, player names
   └─ NO  → Are you building a complex string?
      ├─ YES → Use dString_t* (concatenation, templates)
      │         Example: file paths, multi-part messages
      └─ NO  → Use char[] (default for static text)
               Example: config strings, error codes
```

---

## Part 5: Real-World Examples from Card Fifty-Two

### Example 1: Button Component
**Use Case:** Static label storage

```c
// include/scenes/components/button.h
typedef struct Button {
    int x, y, w, h;
    char label[256];       // ✅ char[] for static storage
    char hotkey_hint[64];  // ✅ char[] for static storage
    bool enabled;
} Button_t;
```

**Rationale:** Button labels like "Hit", "Stand", "Bet" are set once and rarely change.

---

### Example 2: Player Section Rendering
**Use Case:** Dynamic score/chip display

```c
// src/scenes/sections/playerSection.c
void RenderPlayerSection(PlayerSection_t* section, Player_t* player, int y) {
    // ✅ dString_t for dynamic formatting
    dString_t* info = d_InitString();
    d_FormatString(info, "%s: %d%s",
                   player->name,  // Note: player->name is char[128] in struct!
                   player->hand.total_value,
                   player->hand.is_blackjack ? " (BLACKJACK!)" : "");

    a_DrawText((char*)d_PeekString(info), SCREEN_WIDTH / 2, y, ...);
    d_DestroyString(info);
}
```

**Rationale:** Score and status change dynamically, formatted with variables.

---

### Example 3: Player Structure
**Use Case:** Hybrid approach

```c
// include/player.h
typedef struct Player {
    char name[128];        // ✅ char[] for static name (set once)
    int player_id;
    Hand_t hand;
    int chips;             // Used in dynamic formatting
    int current_bet;       // Used in dynamic formatting
    // ...
} Player_t;
```

**Rationale:**
- `name` is static storage (set once at creation)
- `chips` and `current_bet` are formatted into `dString_t*` at render time

---

### Example 4: Main Menu Section
**Use Case:** Static section titles

```c
// include/scenes/sections/mainMenuSection.h
typedef struct MainMenuSection {
    char title[256];           // ✅ "CARD FIFTY-TWO"
    char subtitle[256];        // ✅ "A Blackjack Demo"
    char instructions[512];    // ✅ Static controls text
    FlexBox_t* menu_layout;
    MenuItem_t** menu_items;
    int item_count;
} MainMenuSection_t;
```

**Rationale:** Section titles are static UI text, set once at initialization.

---

## Part 6: Memory & Performance Considerations

### Memory Footprint Comparison

| Approach | Per-Instance Memory | Heap Allocations | Cleanup Required |
|----------|---------------------|------------------|------------------|
| `char label[256]` | 256 bytes (stack/struct) | 0 | No |
| `dString_t* label` | 8 bytes (pointer) + ~280 bytes (heap) | 1 | Yes (d_DestroyString) |

**For 100 buttons:**
- `char[]`: 25.6 KB (contiguous, cache-friendly)
- `dString_t*`: 100 heap allocations + fragmentation

### Performance: Frame-by-Frame Rendering

**Scenario:** Rendering player score 60 times per second

**BAD (memory leak):**
```c
char* score = d_DumpString(score_str, NULL);  // 60 allocs/sec
a_DrawText(score, x, y, ...);
// Missing free(score); ← LEAK 60 times/sec!
```

**GOOD (no leak):**
```c
dString_t* score = d_InitString();           // 1 alloc
d_FormatString(score, "Score: %d", value);   // Reuses buffer
a_DrawText((char*)d_PeekString(score), ...); // No alloc
d_DestroyString(score);                       // 1 free
```

**BEST (per-frame is acceptable):**
```c
// Create/destroy per frame is fine for dynamic content!
dString_t* score = d_InitString();
d_FormatString(score, "Score: %d", player->score);
a_DrawText((char*)d_PeekString(score), x, y, ...);
d_DestroyString(score);
// Modern allocators handle this efficiently
```

---

## Part 7: Interfacing with Archimedes (SDL2)

### The Casting Issue

**Problem:** Archimedes expects `char*`, Daedalus returns `const char*`

```c
// Archimedes signature (can't change - precompiled library)
void a_DrawText(char* text, int x, int y, ...);

// Daedalus signature
const char* d_PeekString(const dString_t* sb);
```

**Solution:** Cast with explanatory comment

```c
// ✅ CORRECT - Cast is safe when function is read-only
dString_t* message = d_InitString();
d_FormatString(message, "Hello %s", name);

// Cast safe: a_DrawText is read-only, dString_t used for dynamic formatting
a_DrawText((char*)d_PeekString(message), x, y, ...);
d_DestroyString(message);
```

**Why it's safe:**
- `a_DrawText()` only reads the string (doesn't modify it)
- We know the function is read-only from its behavior
- Cast removes `const` but function respects it

---

## Part 8: Quick Reference Card

### Use `char[]` Fixed Buffer When:
- ✅ Component labels (Button, MenuItem)
- ✅ Section titles (static UI text)
- ✅ Player/entity names
- ✅ Config strings
- ✅ Maximum length is known
- ✅ Content changes rarely

### Use `dString_t*` Dynamic String When:
- ✅ Formatting with variables (scores, stats)
- ✅ String concatenation
- ✅ Unknown final length
- ✅ Complex string operations
- ✅ Template substitution
- ✅ File path construction

### The `strncpy()` Pattern (for `char[]`):
```c
strncpy(dest, src ? src : "", sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

### The `dString_t` Pattern (for dynamic):
```c
dString_t* str = d_InitString();
d_FormatString(str, "Format: %d", value);
// Use: d_PeekString(str)
d_DestroyString(str);
```

---

## Part 9: Testing Your Decision

**Ask yourself these questions:**

1. **Does this string contain variables that change?**
   - YES → `dString_t*`
   - NO → `char[]`

2. **Will this string be formatted/concatenated?**
   - YES → `dString_t*`
   - NO → `char[]`

3. **Is this a label/name set once and rarely changed?**
   - YES → `char[]`
   - NO → `dString_t*`

4. **Does it need automatic resizing?**
   - YES → `dString_t*`
   - NO → `char[]`

5. **Is it created/destroyed frequently (per frame)?**
   - YES → Consider if `dString_t*` is truly needed
   - NO → Either works, prefer simpler `char[]`

---

## Appendix A: Complete Code Examples

### Complete Button Component (char[] approach)

```c
// button.h
typedef struct Button {
    int x, y, w, h;
    char label[256];
    char hotkey[64];
    bool enabled;
} Button_t;

Button_t* CreateButton(int x, int y, int w, int h, const char* label);
void SetButtonLabel(Button_t* btn, const char* label);
void DestroyButton(Button_t** btn);
void RenderButton(const Button_t* btn);

// button.c
Button_t* CreateButton(int x, int y, int w, int h, const char* label) {
    Button_t* btn = malloc(sizeof(Button_t));
    if (!btn) return NULL;

    btn->x = x; btn->y = y; btn->w = w; btn->h = h;
    btn->enabled = true;

    strncpy(btn->label, label ? label : "", sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';
    btn->hotkey[0] = '\0';

    return btn;
}

void SetButtonLabel(Button_t* btn, const char* label) {
    if (!btn) return;
    strncpy(btn->label, label ? label : "", sizeof(btn->label) - 1);
    btn->label[sizeof(btn->label) - 1] = '\0';
}

void DestroyButton(Button_t** btn) {
    if (!btn || !*btn) return;
    free(*btn);
    *btn = NULL;
}

void RenderButton(const Button_t* btn) {
    if (!btn) return;
    // Cast safe: a_DrawText is read-only
    a_DrawText((char*)btn->label, btn->x, btn->y, ...);
}
```

---

## Appendix B: Migration Guide

**If you have code using `dString_t*` for labels, here's how to migrate:**

### Before (over-engineered):
```c
typedef struct {
    dString_t* label;
} Component_t;

Component_t* Create() {
    Component_t* c = malloc(sizeof(Component_t));
    c->label = d_InitString();
    d_SetString(c->label, "Text", 0);
    return c;
}

void Destroy(Component_t** c) {
    d_DestroyString((*c)->label);
    free(*c);
    *c = NULL;
}

void Render(const Component_t* c) {
    a_DrawText((char*)d_PeekString(c->label), ...);
}
```

### After (simplified):
```c
typedef struct {
    char label[256];
} Component_t;

Component_t* Create() {
    Component_t* c = malloc(sizeof(Component_t));
    strncpy(c->label, "Text", sizeof(c->label) - 1);
    c->label[sizeof(c->label) - 1] = '\0';
    return c;
}

void Destroy(Component_t** c) {
    free(*c);
    *c = NULL;
}

void Render(const Component_t* c) {
    a_DrawText((char*)c->label, ...);
}
```

**Benefits:**
- 3 fewer lines of code
- 1 fewer heap allocation
- Simpler lifecycle
- Faster execution

---

**END OF INTERNAL WIKI**

*This document reflects the current architectural decisions of Card Fifty-Two as of 2025-10-14.*
