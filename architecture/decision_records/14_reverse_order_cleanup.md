# ADR-14: Reverse-Order Resource Cleanup Pattern

## Context

C lacks RAII (Resource Acquisition Is Initialization) - destructors aren't called automatically like in C++/Rust. Manual cleanup means developers must track dependencies and call cleanup functions in correct order. Resources often have **hidden dependencies**: child resources reference parent memory, modals hold FlexBox layouts, players contain trinket slots, game context tracks active players.

**The Problem: Cleanup Order Matters**

Wrong cleanup order causes:
- **Use-after-free crashes**: Child accessing destroyed parent pointer
- **Memory leaks**: Parent destroyed while children still reference it
- **Dangling pointers**: References to freed memory
- **GPU texture leaks**: Textures not freed before table destruction
- **State corruption**: Active game references to freed players

**Historical Evidence:**

1. **ADR-007 Texture Bug**: Background textures destroyed in wrong order during scene transitions, causing use-after-free when cached pointers accessed
2. **Player Cleanup Crash** (2025-01-19): User reported "corrupted size vs. prev_size" when quitting to menu after losing all chips - caused by cleanup order investigation

**Alternatives Considered:**

1. **No enforced order** (current implicit pattern)
   - Pros: Flexible, no documentation burden
   - Cons: Completely invisible rule, easy to violate, crashes are cryptic
   - Cost: Days debugging use-after-free bugs, no way to prevent violations

2. **Reference counting** (like `Py_INCREF`/`Py_DECREF`)
   - Pros: Automatic cleanup when last reference dropped
   - Cons: Runtime overhead, complex bookkeeping, circular reference leaks
   - Cost: Rewrite entire resource system, still need manual cleanup for cycles

3. **Garbage collection** (Boehm GC, etc.)
   - Pros: Eliminates manual cleanup entirely
   - Cons: Non-deterministic timing, pauses gameplay, incompatible with SDL/GPU resources
   - Cost: Cannot track GPU textures (SDL_Texture*), game loop stutters

4. **Smart pointers** (custom C implementation)
   - Pros: Automatic cleanup with RAII-like pattern
   - Cons: Verbose syntax, still need manual order for shared resources
   - Cost: Rewrite all code, doesn't solve order dependencies (two smart pointers still need ordering)

5. **Explicit reverse-order rule** (document + enforce via fitness function)
   - Pros: Zero runtime cost, explicit in code, verifiable automatically
   - Cons: Manual discipline required, must track init order when writing cleanup
   - Cost: One-time documentation + fitness function implementation

## Decision

**ALL cleanup functions MUST destroy resources in exact reverse order of initialization (LIFO - Last In, First Out).**

**Rule**: If resource `A` is initialized before resource `B`, then resource `B` MUST be destroyed before resource `A`.

**Pattern:**
```c
// Initialization order
InitA();
InitB();
InitC();

// Cleanup order (EXACT REVERSE)
CleanupC();
CleanupB();
CleanupA();
```

**Justification:**

1. **Dependency Safety**: Children always destroyed before parents
2. **Pointer Validity**: References valid until cleanup, never after
3. **Memory Correctness**: No use-after-free if order respected
4. **Zero Runtime Cost**: Compile-time pattern, no overhead
5. **Verifiable**: Fitness function can parse and validate order

## Consequences

**Positive:**
- ✅ Eliminates entire class of use-after-free bugs
- ✅ Makes cleanup order explicit and reviewable
- ✅ Automatically verifiable via fitness function
- ✅ No runtime overhead (compile-time pattern)
- ✅ Works with GPU resources (SDL textures)

**Negative:**
- ❌ Developers must mentally track initialization order
- ❌ Refactoring init order requires refactoring cleanup order
- ❌ Not enforced by compiler (manual discipline + FF)

**Accepted Trade-offs:**
- ✅ Accept manual tracking for simplicity (better than reference counting complexity)
- ✅ Accept fitness function enforcement over compiler (C has no destructor ordering)
- ❌ NOT accepting: No order enforcement (leads to cryptic crashes)

**Pattern Examples:**

### Example 1: main.c Global Initialization/Cleanup

**Initialization (main.c:160-170):**
```c
// 1. Card metadata (tag definitions)
InitCardMetadata();

// 2. Trinket system (uses card metadata for tag checks)
InitTrinketSystem();

// 3. Stats system (independent)
Stats_Init();
```

**Cleanup (main.c:250-253 - REVERSE ORDER):**
```c
// 3. Stats system (independent) - destroyed first
// (implicit, no cleanup needed)

// 2. Trinket system - destroyed second
CleanupTrinketSystem();

// 1. Card metadata - destroyed last (trinkets depended on it)
CleanupCardMetadata();
```

**Why This Order:**
- Trinkets reference card tags → card metadata must outlive trinkets
- If cleanup reversed: trinket checks `HasCardTag()` after `CleanupCardMetadata()` → use-after-free

### Example 2: sceneBlackjack.c Scene Lifecycle

**Initialization (sceneBlackjack.c:377-452):**
```c
// 1. Create players (populate g_players table)
CreatePlayer("Dealer", 0, true);
CreatePlayer("Player", 1, false);

// 2. Initialize game context (references g_players table)
State_InitContext(&g_game, &g_test_deck);

// 3. Add players to game (stores player IDs in g_game.active_players)
Game_AddPlayerToGame(&g_game, 0);
Game_AddPlayerToGame(&g_game, 1);

// 4. Initialize layout (references g_players for sections)
InitializeLayout();

// 5. Initialize tutorial (references layout sections)
g_tutorial_system = CreateTutorialSystem();
```

**Cleanup (sceneBlackjack.c:505-564 - REVERSE ORDER):**
```c
// 5. Destroy tutorial (references layout) - destroyed first
if (g_tutorial_system) {
    DestroyTutorialSystem(&g_tutorial_system);
}

// 4. Cleanup layout (references g_players) - destroyed second
CleanupLayout();

// 3. Cleanup game context (destroys active_players array) - destroyed third
State_CleanupContext(&g_game);

// 2 & 1. Destroy players (removes from g_players table) - destroyed fourth/fifth
if (g_dealer) {
    DestroyPlayer(0);
}
if (g_human_player) {
    DestroyPlayer(1);
}
```

**Why This Order:**
- Tutorial → Layout → Game Context → Players
- Tutorial references layout sections → must destroy tutorial before layout
- Layout references player portraits → must destroy layout before players
- Game context stores player IDs → must destroy context before players
- If cleanup reversed: `DestroyPlayer()` removes from g_players while game context still references it → dangling pointer

### Example 3: Modal Component Pattern

**Initialization (rewardModal.c:30-80):**
```c
RewardModal_t* modal = malloc(sizeof(RewardModal_t));

// 1. Create buttons (child resources)
modal->continue_button = CreateButton(...);
modal->skip_button = CreateButton(...);

// 2. Create FlexBox layouts (parent containers)
modal->card_layout = a_CreateFlexBox(...);
modal->info_layout = a_CreateFlexBox(...);

// 3. Add buttons to layouts (buttons referenced by FlexBox)
a_AddFlexItem(modal->info_layout, modal->continue_button, ...);
```

**Cleanup (rewardModal.c:116-133 - REVERSE ORDER):**
```c
void DestroyRewardModal(RewardModal_t** modal) {
    // 3. (No explicit removal from FlexBox - destroyed with parent)

    // 2. Destroy FlexBox layouts (parent containers) - destroyed first
    if ((*modal)->card_layout) {
        a_DestroyFlexBox(&(*modal)->card_layout);
    }
    if ((*modal)->info_layout) {
        a_DestroyFlexBox(&(*modal)->info_layout);
    }

    // 1. Buttons automatically freed when FlexBox destroyed
    // (FlexBox owns button rendering, but buttons still exist)
    // This is safe because FlexBox doesn't free button memory, just stops referencing

    // Finally free modal struct (parent of everything)
    free(*modal);
    *modal = NULL;
}
```

**Why This Order:**
- FlexBox holds references to buttons → destroy FlexBox before buttons
- If cleanup reversed: `DestroyButton()` frees memory while FlexBox still references it → use-after-free on next render

## Verification Pattern

Fitness function (FF-014) will verify:

1. **Structural Pairing**: For every `Init*()` / `Create*()`, verify matching `Cleanup*()` / `Destroy*()` exists
2. **Order Verification**: Extract initialization sequence, verify cleanup is exact reverse
3. **Common Init/Cleanup Pairs**:
   - `InitCardMetadata()` ↔ `CleanupCardMetadata()`
   - `InitTrinketSystem()` ↔ `CleanupTrinketSystem()`
   - `CreatePlayer()` ↔ `DestroyPlayer()`
   - `State_InitContext()` ↔ `State_CleanupContext()`
   - `a_CreateFlexBox()` ↔ `a_DestroyFlexBox()`
   - `CreateButton()` ↔ `DestroyButton()`

4. **Violation Examples**:
```c
// ❌ BAD: Wrong cleanup order
InitA();
InitB();
InitC();

CleanupA();  // ← Too early! B and C still reference A
CleanupB();
CleanupC();

// ✅ GOOD: Correct reverse order
InitA();
InitB();
InitC();

CleanupC();  // ← Correct! C destroyed first
CleanupB();
CleanupA();  // ← A destroyed last (everyone who depended on A is gone)
```

## Dependency Types

Resources have three types of dependencies:

1. **Hard Dependency (MUST respect order)**:
   - Child holds pointer to parent data
   - Example: `g_game.active_players` references `g_players` table entries
   - Violation: Use-after-free crash

2. **Soft Dependency (SHOULD respect order)**:
   - Child uses parent API/functions
   - Example: Trinket code calls `HasCardTag()` which reads card metadata
   - Violation: Logic errors, may not crash immediately

3. **Independent (any order)**:
   - No shared state or references
   - Example: Stats system and trinket system are independent
   - Safe: Cleanup order doesn't matter

**Fitness function focuses on Hard Dependencies** (pointer-based) but warns on Soft Dependencies.

## Common Cleanup Anti-Patterns

### Anti-Pattern 1: Early Parent Cleanup
```c
// ❌ BAD
CreateModal();      // Creates buttons inside modal
CreateFlexBox();    // Adds buttons to FlexBox

free(modal);        // ← Frees modal too early!
a_DestroyFlexBox(); // ← FlexBox references freed modal buttons
```

### Anti-Pattern 2: Missing Cleanup Entirely
```c
// ❌ BAD
InitCardMetadata();
InitTrinketSystem();

CleanupTrinketSystem();
// ← Missing CleanupCardMetadata() - memory leak!
```

### Anti-Pattern 3: Partial Reverse Order
```c
// ❌ BAD
InitA();
InitB();
InitC();
InitD();

CleanupD();  // ← Correct so far
CleanupB();  // ← Wrong! Should be CleanupC()
CleanupC();
CleanupA();
```

## Success Metrics

- ✅ Zero use-after-free bugs in cleanup code (verified by valgrind)
- ✅ FF-014 passes on all init/cleanup function pairs
- ✅ Scene transitions clean (no crashes on Quit to Menu)
- ✅ Modal destruction safe (no dangling pointer errors)
- ✅ New developers guided by ADR documentation

## Related ADRs

- **ADR-003** (Constitutional Patterns): Uses Daedalus types for automatic cleanup within structures
- **ADR-007** (Texture Cleanup): Specific case of cleanup order (textures before table destruction)
- **This ADR (14)**: General pattern covering ALL resource cleanup across codebase

**Status:** ✅ Implemented (pattern exists, now documented and enforced)
- Documented: 2025-01-19
- Fitness function: FF-014 (to be implemented)
