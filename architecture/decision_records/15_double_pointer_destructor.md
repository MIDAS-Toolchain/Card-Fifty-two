# ADR-015: Double-Pointer Destructor Pattern for Heap-Allocated Components

**Status:** Accepted

**Date:** 2025-11-20

**Context:** Memory safety in manual C lifecycle management

---

## Problem Statement

C lacks automatic memory management (RAII, garbage collection). Without strict patterns, heap-allocated components lead to three critical bug classes:

1. **Use-After-Free**: Accessing memory after `free()` causes undefined behavior
2. **Double-Free**: Calling `free()` twice on same pointer crashes program
3. **Dangling Pointers**: Pointers that reference freed memory remain valid-looking but dangerous

**Example of the danger:**

```c
// Without pattern:
Modal_t* modal = CreateModal();
free(modal);              // Memory freed
RenderModal(modal);       // USE-AFTER-FREE CRASH! (modal still points to freed memory)

// Or:
free(modal);              // First free
free(modal);              // DOUBLE-FREE CRASH!
```

**Root cause**: C doesn't automatically null pointers after `free()`, and calling code can't distinguish freed from valid pointers.

---

## Decision

**All heap-allocated components MUST use double-pointer destructors that automatically null the caller's pointer.**

### Mandatory Destructor Signature

```c
void DestroyComponentType(ComponentType_t** ptr);
```

**Key:** Double pointer (`**ptr`) allows function to modify caller's pointer.

### Mandatory Implementation Pattern

**Every `Destroy*()` function MUST contain this exact sequence:**

```c
void DestroyComponentType(ComponentType_t** ptr) {
    // 1. NULL guard (protect against NULL input or double-destroy)
    if (!ptr || !*ptr) {
        return;
    }

    // 2. Destroy child resources (reverse init order - see ADR-14)
    if ((*ptr)->child_resource) {
        DestroyChildResource(&(*ptr)->child_resource);
    }

    // 3. Free heap memory
    free(*ptr);

    // 4. Null caller's pointer (prevents use-after-free and double-free)
    *ptr = NULL;
}
```

**Order is non-negotiable:**
1. NULL guard first (safety)
2. Destroy children before parent (ADR-14)
3. Free memory
4. Null pointer last (caller safety)

### Mandatory Call-Site Pattern

**Always use address-of operator (`&`):**

```c
Modal_t* modal = CreateModal();
DestroyModal(&modal);  // Pass address of pointer
// modal is now NULL automatically
```

**WRONG:**
```c
DestroyModal(modal);   // ❌ Passes pointer value, can't null caller's variable
```

---

## Context: Heap vs Value Types

### Heap-Allocated Types (malloc/free)

**Characteristics:**
- Created with `malloc()`
- Independent lifetime from parent
- Owned by pointer in parent struct

**Pattern:** `Destroy**(ComponentType_t** ptr)`

**Examples:**
- UI Components: `Button_t`, `Modal_t`, `TrinketUI_t`
- Sections: `PlayerSection_t`, `DealerSection_t`
- Scene Data: `ResultScreen_t`, `MenuItem_t`

**Destructor Names:**
- `DestroyButton(&button)`
- `DestroyModal(&modal)`
- `DestroyPlayerSection(&section)`

### Value Types (Embedded in Parent)

**Characteristics:**
- Embedded directly in parent struct (not pointers)
- Lifetime tied to parent
- Memory managed by parent's malloc/free

**Pattern:** `Cleanup*(ComponentType_t* ptr)` (single pointer)

**Examples:**
- `Hand_t` (embedded in `Player_t`)
- `Deck_t` (embedded in game state)
- `TweenManager_t` (embedded in scene)

**Cleanup Names:**
- `CleanupHand(&player->hand)` (single pointer, no nulling)
- `CleanupDeck(&game->deck)`
- `CleanupTweenManager(&scene->tween_mgr)`

**Why no double pointer?** Parent owns the memory - cleanup only destroys internal resources (e.g., dArray_t inside the struct), not the struct itself.

---

## Examples

### Example 1: RewardModal (Heap-Allocated)

**Declaration** (rewardModal.h):
```c
typedef struct RewardModal RewardModal_t;

RewardModal_t* CreateRewardModal(void);
void DestroyRewardModal(RewardModal_t** modal);  // Double pointer
```

**Implementation** (rewardModal.c):
```c
RewardModal_t* CreateRewardModal(void) {
    RewardModal_t* modal = malloc(sizeof(RewardModal_t));
    if (!modal) return NULL;

    // Initialize fields...
    modal->layout = a_CreateFlexBox(x, y, w, h);
    return modal;
}

void DestroyRewardModal(RewardModal_t** modal) {
    if (!modal || !*modal) return;  // NULL guard

    RewardModal_t* m = *modal;

    // Destroy children (reverse init order)
    if (m->layout) {
        a_DestroyFlexBox(&m->layout);
    }

    // Free heap memory
    free(*modal);

    // Null caller's pointer
    *modal = NULL;
}
```

**Usage** (sceneBlackjack.c):
```c
void CleanupBlackjackScene(void) {
    if (g_reward_modal) {
        DestroyRewardModal(&g_reward_modal);  // Pass &pointer
        // g_reward_modal is now NULL
    }
}
```

### Example 2: Button (Heap-Allocated)

**Declaration** (button.h):
```c
typedef struct Button Button_t;

Button_t* CreateButton(int x, int y, int w, int h, const char* text);
void DestroyButton(Button_t** button);  // Double pointer
```

**Implementation** (button.c):
```c
Button_t* CreateButton(int x, int y, int w, int h, const char* text) {
    Button_t* button = malloc(sizeof(Button_t));
    if (!button) return NULL;

    button->label = d_StringInit();
    d_StringSet(button->label, text);
    button->rect = (SDL_Rect){x, y, w, h};
    return button;
}

void DestroyButton(Button_t** button) {
    if (!button || !*button) return;  // NULL guard

    Button_t* b = *button;

    // Destroy children
    if (b->label) {
        d_DestroyString(&b->label);
    }

    // Free heap memory
    free(*button);

    // Null caller's pointer
    *button = NULL;
}
```

### Example 3: Hand (Value Type, NOT Heap-Allocated)

**Declaration** (player.h):
```c
typedef struct Hand {
    dArray_t* cards;  // Internal heap resource
    int value;
} Hand_t;

void CleanupHand(Hand_t* hand);  // Single pointer (value type)
```

**Implementation** (player.c):
```c
void CleanupHand(Hand_t* hand) {
    if (!hand) return;

    // Destroy internal resources only
    if (hand->cards) {
        d_ArrayDestroy(&hand->cards);
    }

    // Does NOT free 'hand' itself (parent owns it)
    // Does NOT null 'hand' (parent owns the pointer/struct)
}
```

**Usage** (player.c):
```c
typedef struct Player {
    Hand_t hand;  // Embedded, not pointer
} Player_t;

void DestroyPlayer(Player_t** player) {
    if (!player || !*player) return;

    Player_t* p = *player;

    // Cleanup embedded value type
    CleanupHand(&p->hand);  // Single pointer, no nulling

    // Free parent
    free(*player);
    *player = NULL;
}
```

---

## Anti-Patterns

### ❌ Anti-Pattern 1: Single-Pointer Destructor

```c
void DestroyModal(Modal_t* modal) {  // Single pointer!
    free(modal);
    // Caller's pointer NOT nulled!
}

// Caller:
Modal_t* modal = CreateModal();
DestroyModal(modal);    // modal still points to freed memory
if (modal) {            // Pointer is NOT NULL!
    RenderModal(modal); // USE-AFTER-FREE CRASH!
}
```

**Why it's wrong:** Cannot modify caller's pointer, leaves dangling pointer.

### ❌ Anti-Pattern 2: Missing NULL Guard

```c
void DestroyModal(Modal_t** modal) {
    free(*modal);       // CRASH if modal is NULL or *modal is NULL!
    *modal = NULL;
}

// Caller:
Modal_t* modal = NULL;
DestroyModal(&modal);   // SEGFAULT! Dereferencing NULL pointer
```

**Why it's wrong:** Crashes on NULL input or double-destroy attempts.

### ❌ Anti-Pattern 3: Forgot to Null Pointer

```c
void DestroyModal(Modal_t** modal) {
    if (!modal || !*modal) return;
    free(*modal);
    // Missing: *modal = NULL;
}

// Caller:
DestroyModal(&modal);   // Memory freed, but modal not nulled
DestroyModal(&modal);   // DOUBLE-FREE CRASH! (NULL guard fails)
if (modal) {            // Pointer is NOT NULL!
    RenderModal(modal); // USE-AFTER-FREE CRASH!
}
```

**Why it's wrong:** Double-free and use-after-free still possible.

### ❌ Anti-Pattern 4: Wrong Call-Site Syntax

```c
Modal_t* modal = CreateModal();
DestroyModal(modal);    // ❌ Missing & operator
```

**Why it's wrong:** Passes pointer value (not address), function can't null caller's variable.

### ❌ Anti-Pattern 5: Using Destroy* for Value Types

```c
// Hand_t is embedded in Player_t (value type)
void DestroyHand(Hand_t** hand) {  // ❌ WRONG! Hand is NOT heap-allocated
    // ...
}
```

**Why it's wrong:** Value types don't own their memory - parent does. Use `Cleanup*()` instead.

---

## Special Cases

### Linked-List / Collection Walkers

**Exception to double-pointer rule:** Utility functions that walk and destroy collections of nodes.

**Example:** `DestroyBlackjackTutorial(TutorialStep_t* first_step)`

```c
void DestroyBlackjackTutorial(TutorialStep_t* first_step) {
    if (!first_step) return;

    TutorialStep_t* current = first_step;
    TutorialStep_t* next = NULL;

    // Walk through linked list and destroy each node
    while (current) {
        next = current->next_step;
        DestroyTutorialStep(&current);  // Individual nodes use double-pointer
        current = next;
    }
}

// Caller manually nulls the pointer
g_tutorial_steps = NULL;  // Manual nulling after walker completes
```

**Why this is acceptable:**

1. Walker function destroys **collection**, not individual nodes
2. Individual node destructors (`DestroyTutorialStep`) still use double-pointer pattern
3. Caller has more complex ownership (e.g., head of linked list)
4. Manual nulling happens at call site

**When to use this pattern:**

- Linked lists (walk next pointers, destroy each)
- Tree structures (recursive walkers)
- Collections with complex traversal

**Rule:** Walker uses single pointer, but nodes within collection use double-pointer.

---

## Rationale

### Why Double Pointer?

**Problem:** C functions receive arguments by value:

```c
void SetToNull(int* ptr) {
    ptr = NULL;  // Only modifies LOCAL copy of pointer
}

int* original = malloc(100);
SetToNull(original);
// original is STILL not NULL!
```

**Solution:** Pass pointer to pointer:

```c
void SetToNull(int** ptr) {
    *ptr = NULL;  // Modifies caller's variable via indirection
}

int* original = malloc(100);
SetToNull(&original);  // Pass address of pointer
// original is now NULL!
```

**Applied to destructors:**

```c
void DestroyModal(Modal_t** modal) {
    free(*modal);
    *modal = NULL;  // Nulls caller's variable
}
```

### Benefits

**1. Prevents Use-After-Free**

```c
Modal_t* modal = CreateModal();
DestroyModal(&modal);   // modal becomes NULL
RenderModal(modal);     // NULL check catches this - no crash
```

**2. Prevents Double-Free**

```c
DestroyModal(&modal);   // First call: frees memory, sets modal = NULL
DestroyModal(&modal);   // Second call: NULL guard returns early - safe!
```

**3. Prevents Dangling Pointers**

```c
Modal_t* modal = CreateModal();
Modal_t* alias = modal;  // Alias created
DestroyModal(&modal);    // modal becomes NULL, but...
RenderModal(alias);      // ❌ STILL DANGEROUS! Alias not nulled

// Best practice: don't create aliases, or null them manually
alias = NULL;
```

**4. Makes Cleanup Idempotent**

```c
// Safe to call multiple times
DestroyModal(&modal);
DestroyModal(&modal);  // No-op, no crash
DestroyModal(&modal);  // Still no-op
```

**5. Clear Intent in Code**

```c
DestroyModal(&modal);  // Address-of operator signals: "this will be modified"
```

---

## Trade-Offs

### Accepted

✅ **Double-pointer syntax verbosity** for automatic memory safety
- `DestroyModal(&modal)` vs hypothetical `DestroyModal(modal)`
- Trade-off: Slight verbosity for crash prevention

✅ **Manual discipline** (no compiler enforcement) for simplicity
- C won't stop you from calling `DestroyModal(modal)` (missing `&`)
- Fitness function catches violations
- Trade-off: Manual verification vs language complexity

✅ **NULL guard overhead** (extra if-check) for robustness
- Every destructor checks `if (!ptr || !*ptr)`
- Negligible performance cost
- Trade-off: Tiny overhead for safety

### Rejected

❌ **Single-pointer destructors** - too dangerous
- Can't null caller's pointer
- Allows use-after-free and double-free
- Unacceptable risk

❌ **Smart pointers / RAII wrappers** - too complex
- Would require C++ or macro magic
- Violates constitutional simplicity (ADR-003)
- Adds cognitive load

❌ **Reference counting** - overkill
- Card Fifty-Two has clear ownership (no shared pointers)
- Adds complexity without benefit
- Performance overhead

---

## Integration with Existing ADRs

**ADR-003 (Constitutional Patterns):**
- Value types use Daedalus (dString_t, dArray_t)
- Heap types use double-pointer destructors
- Complementary patterns for different ownership models

**ADR-014 (Reverse Order Cleanup):**
- Double-pointer pattern works with reverse-order destruction
- Children destroyed before parent in destructor body
- Pointer nulling happens last

**ADR-007 (Texture Cleanup):**
- SDL_DestroyTexture() called before nulling
- Follows same principle: cleanup resources before freeing parent

---

## Enforcement

**Manual Code Review:**
- Check `Destroy*()` signatures use double pointer
- Verify implementation has all 4 steps
- Verify call sites use `&` operator

**Fitness Function (FF-015):**
- Automatically verifies all `Create*()` have matching `Destroy**()` signatures
- Checks destructor implementations contain required sequence
- Flags violations for fix

**Build System:**
- `make verify` runs FF-015 as part of architecture verification suite

---

## Evidence

**Components Following Pattern (33+ found):**

**Modals (13):**
- RewardModal, EventModal, CharacterStatsModal, CombatStatsModal, EventPreviewModal, CombatPreviewModal, CardGridModal, IntroNarrativeModal, AbilityTooltipModal, StatusEffectTooltipModal, CardTooltipModal, BetTooltipModal, ChipsTooltipModal

**UI Components (10):**
- Button, TrinketUI, ResultScreen, SidebarButton, DeckButton, VisualEffects, EnemyHealthBar, MenuItem, AbilityDisplay, MenuItemRow

**Sections (10+):**
- PlayerSection, DealerSection, LeftSidebarSection, TopBarSection, MainMenuSection, PauseMenuSection, TitleSection, DrawPileModalSection, DiscardPileModalSection, etc.

**All use double-pointer destructors with pointer nulling.**

**Value Types Using Cleanup* (NOT Destroy**):**
- Hand_t, Deck_t, TweenManager_t, CardTransitionManager_t

---

## Summary

**What:** All heap-allocated components use double-pointer destructors that automatically null caller's pointer.

**Why:** Prevents use-after-free, double-free, and dangling pointer bugs.

**How:**
1. Signature: `void Destroy*(ComponentType_t** ptr)`
2. Implementation: NULL guard → destroy children → free → null pointer
3. Call site: `Destroy*(&variable)`

**Constitutional Significance:**
- Universal enforcement (33+ components)
- Memory safety critical (crashes prevented)
- Complements ADR-003 (value types) and ADR-014 (cleanup order)
- Not style - architectural semantics for C lifecycle management
