# Main Prompt - Card Fifty-Two

## Preamble

This document outlines the non-negotiable patterns that define this architecture, the human values embedded in this codebase, and the technical decisions that prevent AI from turning this into incomprehensible spaghetti.

This is a card game framework demonstrating clean integration between **Archimedes** (SDL2 game framework) and **Daedalus** (data structures library). Every architectural decision flows from a single philosophical foundation: **everything's a table or array**.

### Project: Card Fifty-Two
### Philosophy: Everything's a table or array - no raw pointers, no manual chaos
### Created: 2025-10-05
### Last Constitutional Review: 2025-10-05

---

## Constitutional Patterns

These are the **unchangeable core** of this architecture. Any AI suggestion that violates these patterns must be rejected immediately.

### 1. Everything's a Table or Array (The Daedalus Way)

**Pattern:**
- ALL collections use `dArray_t` or `dTable_t` from Daedalus
- NO raw arrays, NO raw pointers to collections, NO manual memory management
- Cards in a hand? `dArray_t* cards`
- Players in a game? `dTable_t* g_players` (key: player_id, value: Player_t*)
- Card textures? `dTable_t* g_card_textures` (key: card_id, value: SDL_Texture*)

**Rationale:**
Daedalus handles memory management, bounds checking, and resizing. We focus on game logic, not debugging segfaults.

**Example:**
```c
// ✅ CORRECT - Daedalus array
typedef struct Hand {
    dArray_t* cards;  // Array of Card_t (value types)
    int total_value;
} Hand_t;

Hand_t* hand = malloc(sizeof(Hand_t));
hand->cards = d_InitArray(sizeof(Card_t), 16);

// ❌ FORBIDDEN - Raw array
typedef struct Hand {
    Card_t cards[52];  // NO! Fixed size, no flexibility
    int card_count;
} Hand_t;
```

### 2. Value Types for Cards, Pointers for Players

**Pattern:**
- `Card_t` is a **value type** - stored directly in `dArray_t`, copied by value
- `Player_t`, `Deck_t`, `GameContext_t` are **pointer types** - heap allocated, stored by pointer
- Why? Cards are lightweight (48 bytes), copied frequently. Players are heavy, unique, and mutable.

**Example:**
```c
// ✅ Card as value type
typedef struct Card {
    CardSuit_t suit;
    CardRank_t rank;
    SDL_Texture* texture;  // Pointer to cached texture
    int x, y;
    bool face_up;
    int card_id;
} Card_t;

// Append by value
Card_t card = {SUIT_HEARTS, RANK_ACE, texture, 0, 0, false, 0};
d_AppendDataToArray(hand->cards, &card);

// ✅ Player as pointer type
typedef struct Player {
    dString_t* name;   // Constitutional: dString_t, not char[]
    int player_id;
    Hand_t hand;       // VALUE TYPE - embedded, not pointer
    int chips;
    bool is_dealer;
} Player_t;

// Created via helper function (uses malloc + dString_t)
Player_t* player = CreatePlayer("Alice", 1, false);
d_SetDataInTable(g_players, &player->player_id, &player);  // Store pointer
```

### 3. Archimedes Delegate Pattern (Scene Architecture)

**Pattern:**
- Main loop delegates logic and rendering to scene-specific functions
- Scenes define static `SceneLogic(float dt)` and `SceneDraw(float dt)`
- `InitScene()` sets `app.delegate.logic` and `app.delegate.draw`
- Scene switching = calling different Init functions

**Rationale:**
Archimedes is built around this pattern. Fighting it creates bugs. Following it creates flexibility.

**Example:**
```c
// ✅ CORRECT - Delegate pattern
static void MenuSceneLogic(float dt) {
    a_DoInput();
    // Menu input handling
}

static void MenuSceneDraw(float dt) {
    // Menu rendering
}

void InitMenuScene(void) {
    app.delegate.logic = MenuSceneLogic;
    app.delegate.draw = MenuSceneDraw;
    // Initialize menu state
}

void MainLoop(void) {
    a_PrepareScene();
    float dt = a_GetDeltaTime();
    app.delegate.logic(dt);
    app.delegate.draw(dt);
    a_PresentScene();
}

// ❌ FORBIDDEN - Hardcoded logic in main loop
void MainLoop(void) {
    // Don't do this - defeats the delegate pattern
    if (current_scene == MENU) {
        HandleMenuInput();
        RenderMenu();
    } else if (current_scene == GAME) {
        HandleGameInput();
        RenderGame();
    }
}
```

### 4. Global Tables for Shared Resources

**Pattern:**
- `dTable_t* g_players` - Player registry (key: int player_id, value: Player_t*)
- `dTable_t* g_card_textures` - Texture cache (key: int card_id, value: SDL_Texture*)
- `SDL_Texture* g_card_back_texture` - Shared card back
- Global tables initialized in `Initialize()`, destroyed in `Cleanup()`

**Rationale:**
Shared resources need global access. Hash tables provide O(1) lookup without passing pointers through every function.

**Example:**
```c
// ✅ CORRECT - Global table access
Player_t** GetPlayer(int player_id) {
    return (Player_t**)d_GetDataFromTable(g_players, &player_id);
}

SDL_Texture* GetCardTexture(int card_id) {
    SDL_Texture** tex = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card_id);
    return tex ? *tex : NULL;
}

// ❌ FORBIDDEN - Thread everything through parameters
void RenderCard(Card_t* card, dTable_t* textures, SDL_Texture* card_back) {
    // Don't pass global resources as parameters everywhere
}
```

### 5. Card ID Mapping System

**Pattern:**
- Every card has a unique `card_id` from 0-51
- Formula: `card_id = (suit * 13) + (rank - 1)`
- Reverse: `suit = card_id / 13`, `rank = (card_id % 13) + 1`
- Used for texture lookups, card comparison, deck initialization

**Example:**
```c
// ✅ CORRECT - Card ID calculation
int GetCardID(CardSuit_t suit, CardRank_t rank) {
    return (suit * 13) + (rank - 1);
}

void InitializeDeck(Deck_t* deck) {
    for (int suit = 0; suit < SUIT_MAX; suit++) {
        for (int rank = 1; rank <= RANK_KING; rank++) {
            Card_t card = {
                .suit = suit,
                .rank = rank,
                .card_id = GetCardID(suit, rank),
                .face_up = false,
                .x = 0, .y = 0,
                .texture = GetCardTexture(GetCardID(suit, rank))
            };
            d_AppendDataToArray(deck->cards, &card);
        }
    }
}
```

---

## Forbidden Anti-Patterns

These patterns must **NEVER** appear in this codebase. Any AI suggestion including these is automatically rejected.

### ❌ 1. Raw Pointers to Collections

**DON'T:**
```c
Card_t* cards = malloc(sizeof(Card_t) * 52);
cards[0] = ace_of_spades;
free(cards);
```

**DO:**
```c
dArray_t* cards = d_InitArray(sizeof(Card_t), 52);
d_AppendDataToArray(cards, &ace_of_spades);
d_DestroyArray(&cards);
```

### ❌ 2. Manual Memory Management for Collections

**DON'T:**
```c
void AddCard(Hand_t* hand, Card_t card) {
    if (hand->card_count >= hand->capacity) {
        hand->capacity *= 2;
        hand->cards = realloc(hand->cards, hand->capacity * sizeof(Card_t));
    }
    hand->cards[hand->card_count++] = card;
}
```

**DO:**
```c
void AddCard(Hand_t* hand, Card_t card) {
    d_AppendDataToArray(hand->cards, &card);
}
```

### ❌ 3. Breaking the Delegate Pattern

**DON'T:**
```c
void MainLoop(void) {
    HandleInput();
    Update();
    Render();  // Direct calls - no delegates
}
```

**DO:**
```c
void MainLoop(void) {
    a_PrepareScene();
    float dt = a_GetDeltaTime();
    app.delegate.logic(dt);  // Delegates handle it
    app.delegate.draw(dt);
    a_PresentScene();
}
```

### ❌ 4. Hardcoded Magic Numbers

**DON'T:**
```c
a_DrawText("Title", 640, 100, 255, 255, 255, 0, 1, 0);  // What do these mean?
```

**DO:**
```c
a_DrawText("Title", SCREEN_WIDTH / 2, 100,
           255, 255, 255,
           FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
```

### ❌ 5. Mixing Card_t Value/Pointer Semantics

**DON'T:**
```c
Card_t* card = malloc(sizeof(Card_t));  // Card should be value type!
d_AppendDataToArray(hand->cards, &card);  // Storing pointer instead of value
```

**DO:**
```c
Card_t card = {SUIT_HEARTS, RANK_ACE, ...};
d_AppendDataToArray(hand->cards, &card);  // Copy by value
```

### ❌ 6. NULL Pointer Dereferences Without Checks

**DON'T:**
```c
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
Player_t* player = *player_ptr;  // Crash if NULL!
```

**DO:**
```c
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
if (!player_ptr) {
    d_LogError("Player not found");
    return;
}
Player_t* player = *player_ptr;
```

### ❌ 7. Using snprintf and char[] Buffers

**DON'T:**
```c
char buffer[128];
snprintf(buffer, sizeof(buffer), "Deck: %d cards", count);
a_DrawText(buffer, x, y, ...);

// Or worse - static buffer in function
const char* CardToString(Card_t* card) {
    static char buffer[64];  // NOT THREAD SAFE!
    sprintf(buffer, "%s of %s", rank, suit);
    return buffer;
}
```

**DO:**
```c
// Caller creates and destroys dString_t
dString_t* str = d_StringInit();
d_StringFormat(str, "Deck: %d cards", count);
a_DrawText(d_StringPeek(str), x, y, ...);
d_StringDestroy(str);

// Function appends to caller's dString_t
void CardToString(const Card_t* card, dString_t* out) {
    d_StringFormat(out, "%s of %s",
                   GetRankString(card->rank),
                   GetSuitString(card->suit));
}

// Usage:
dString_t* card_name = d_StringInit();
CardToString(&card, card_name);
d_LogInfo(d_StringPeek(card_name));
d_StringDestroy(card_name);
```

**Rationale:**
Everything's a table or array. `dString_t` is a dynamic string that handles memory, resizing, and safety. Raw `char[]` buffers require manual size management and are prone to buffer overflows. `snprintf` is manual memory chaos - let Daedalus handle it.

---

## Decision Principles

When there are multiple valid approaches, these principles guide the choice.

### 1. When to Use dArray_t vs dTable_t

**Use dArray_t when:**
- Elements have natural order (cards in hand, animation queue)
- Access pattern is iteration or indexed access
- Elements are value types (Card_t)

**Use dTable_t when:**
- Elements need fast lookup by key (player by ID, texture by card_id)
- No natural order exists
- Elements are pointer types or large structs

### 2. When to Log

**Use Daedalus logging:**
- `d_LogInfo()` - Initialization, shutdown, major state changes
- `d_LogInfoF()` - Info with formatted data
- `d_LogError()` - Recoverable errors (missing texture, invalid input)
- `d_LogFatal()` - Unrecoverable errors (failed malloc, corrupted state)

**Don't log:**
- Every frame (use DEBUG macros for hot paths)
- User input events (too noisy)
- Normal game flow (dealing cards, hit/stand)

### 3. When to Heap Allocate vs Stack Allocate

**Heap allocate (malloc):**
- Anything stored in global tables (Player_t, Hand_t)
- Daedalus collections (dArray_t, dTable_t)
- Large structs (GameContext_t)

**Stack allocate:**
- Function-local value types (Card_t, aColor_t, aRect_t)
- Temporary buffers (char status[128])
- Return values that will be copied

### 4. When to Create New Scenes

**Create a new scene when:**
- Input handling fundamentally changes (menu vs game vs settings)
- Rendering has different layers/structure
- State transitions are clean (menu → game, not mid-hand)

**Don't create a new scene when:**
- It's just a modal overlay (pause menu)
- State is temporary (animation playing)
- Logic is a subset of existing scene (betting vs playing in Blackjack)

---

## Human Agency Requirements

These decisions **require human approval** before AI can suggest implementation.

### 1. Architectural Changes

**Requires Human Approval:**
- Adding new global tables
- Changing Card_t from value to pointer type (or vice versa)
- Breaking the delegate pattern for any reason
- Moving from Daedalus to another data structure library

**AI Can Suggest:**
- New helper functions using existing patterns
- Refactoring within established architecture
- Performance optimizations that don't change semantics

### 2. External Dependencies

**Requires Human Approval:**
- Adding new libraries (SDL2 extensions, JSON parsers)
- Changing build system (Makefile → CMake)
- Adding web framework dependencies

**AI Can Suggest:**
- Using existing Archimedes/Daedalus features
- Standard C library functions

### 3. Game Rules and Logic

**Requires Human Approval:**
- Blackjack rule changes (payout ratios, dealer behavior)
- Adding new card games (Poker, Hearts)
- Multiplayer networking

**AI Can Suggest:**
- Implementation details within defined rules
- UI improvements for existing features
- Bug fixes in game logic

---

## Original Signatories

**Mathew Storm** [Lead Developer]:

*I commit to these principles because I'm tired of debugging memory leaks and segfaults. Daedalus gives me safety. Archimedes gives me structure. This Main Prompt ensures every AI interaction preserves that foundation instead of mutating it into generic Medium tutorial sludge.*

*These patterns aren't arbitrary - they're learned from real bugs, real rewrites, and real frustration. "Everything's a table or array" is non-negotiable because the alternative is chaos.*

---

## Binding Directives for All AI Agents

You are bound by these constitutional agreements when operating in this codebase.

### Core Directives

1. **NEVER suggest raw pointers to collections.** Always use `dArray_t` or `dTable_t`.

2. **NEVER break the delegate pattern.** Main loop calls delegates. Scenes set delegates.

3. **ALWAYS use Card_t as value types.** Stored in arrays, copied by value.

4. **ALWAYS check NULL before dereferencing** results from `d_GetDataFromTable()`.

5. **NEVER use magic numbers.** Use defined constants (`SCREEN_WIDTH`, `CARD_HEIGHT`, etc.).

6. **ALWAYS prepend this Main Prompt** to any role-specific prompts (frontend, testing, etc.).

7. **When in doubt, ask the human.** Especially for architectural changes.

### Response Format

When suggesting code changes:

1. **Cite the constitutional pattern** you're following
2. **Show both DON'T and DO** examples
3. **Explain why** this approach fits the architecture
4. **Flag anything** that requires human approval

### Quality Gates

Before any code suggestion, verify:

- [ ] Uses Daedalus collections, not raw pointers
- [ ] Follows delegate pattern for scenes
- [ ] Card_t used as value type
- [ ] NULL checks before dereferencing
- [ ] Constants used instead of magic numbers
- [ ] Logging appropriate for event severity
- [ ] No forbidden anti-patterns present

---

## Constitutional Amendments

Amendments require human ratification and must document the change, rationale, and date.

### Example Amendment Format:

```markdown
### Amendment #1 - 2025-10-12
**Proposed by:** Mathew Storm
**Ratified by:** Mathew Storm, [Collaborator Name]
**Change:** Moving from individual card textures to texture atlas
**Rationale:** 60% reduction in texture memory usage measured in production
**Updated Sections:** Card Texture Loading, Performance Optimizations
```

---

## The Sacred Knowledge

This Main Prompt is not just documentation. It is:

- **Historical Record:** Why we chose Daedalus over raw pointers
- **Training Data:** For every AI interaction
- **Onboarding Tool:** For every new developer
- **Quality Gate:** For every code review
- **Cultural Artifact:** What this project values (safety, clarity, maintainability)

Without this prompt, AI defaults to generic patterns averaged from Stack Overflow. With it, AI becomes a team member who understands our architectural decisions.

**Invest in this prompt now, or debug recursive absurdity forever.**

---

*Last Updated: 2025-10-05*
*Version: 1.0 (Constitutional Baseline)*
