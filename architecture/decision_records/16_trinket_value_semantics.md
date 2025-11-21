# ADR-016: Trinket Template & Instance Pattern (Value Semantics)

## Context

Trinkets are equipment items with passive and active effects. The original implementation stored trinkets as malloc'd pointers in a global table, with players holding pointers to these global singletons. This violated constitutional patterns (ADR-003) and caused several critical bugs.

**The Problem: Pointer-Based Trinkets**

Original architecture:
```c
dTable_t* g_trinkets;  // trinket_id → Trinket_t* (POINTER!)

Trinket_t* CreateTrinket(...) {
    Trinket_t* trinket = malloc(sizeof(Trinket_t));  // ❌ Raw malloc!
    d_SetDataInTable(g_trinkets, &id, &trinket);     // Store POINTER
    return trinket;
}

// Player stored pointer to global singleton
typedef struct Player {
    Trinket_t* class_trinket;      // ❌ Points to global template
    dArray_t* trinket_slots;       // ❌ Array of Trinket_t* pointers
} Player_t;
```

**Bugs Caused:**

1. **Segfault on struct field addition** (2025-11-20): Adding `rarity` field to `Trinket_t` shifted all subsequent pointer fields. Code compiled with old offsets read garbage memory when accessing `passive_description`, `active_description` → segfault on hover.

2. **Shared mutable state**: All players referenced same global trinket instance. Per-player stats (damage dealt, cooldowns) would affect all players sharing that trinket.

3. **Constitutional violation**: `malloc(sizeof(Trinket_t))` directly violates ADR-003: "No raw malloc for these types without explicit justification."

4. **Manual cleanup complexity**: Each trinket needed `free()`, table iteration required double-pointer dereference, cleanup order dependencies between templates and player references.

**Alternatives Considered:**

1. **Keep pointer-based, add version field**
   - Pros: Minimal change, version mismatch detection
   - Cons: Still violates constitutional patterns, doesn't fix shared state
   - Cost: Runtime version checks, still crashes on mismatch

2. **Reference counting**
   - Pros: Automatic cleanup when last reference dropped
   - Cons: Complexity, circular reference risk, doesn't fix layout issues
   - Cost: Rewrite trinket system, runtime overhead

3. **Value semantics (template → copy pattern)**
   - Pros: Constitutional compliance, per-player instances, layout changes isolated
   - Cons: Deep-copy overhead on equip, must copy dStrings manually
   - Cost: One-time refactor, slight memory increase (each player has copy)

## Decision

**Trinkets use VALUE SEMANTICS following the constitutional pattern (ADR-003).**

### Architecture:

1. **Global template registry** (`g_trinket_templates`) stores `Trinket_t` BY VALUE
2. **Players store COPIES** - deep-copy template → player's slot on equip
3. **Each player has own instance** with independent stats
4. **No malloc()** - stack allocation + `d_SetDataInTable()` stores by value
5. **Class trinket** embedded directly in `Player_t` struct (stable address)
6. **Regular trinkets** stored in `dArray_t` of `Trinket_t` values

### Pattern:

**Template Creation (NO malloc!):**
```c
Trinket_t* CreateTrinketTemplate(int trinket_id, const char* name, ...) {
    // Build on STACK, not heap
    Trinket_t template = {0};

    template.trinket_id = trinket_id;
    template.name = d_StringInit();
    d_StringSet(template.name, name, 0);
    // ... initialize all fields ...

    // Store BY VALUE in table (memcpy inside)
    d_SetDataInTable(g_trinket_templates, &template.trinket_id, &template);

    // Return pointer to VALUE in table
    return (Trinket_t*)d_GetDataFromTable(g_trinket_templates, &trinket_id);
}
```

**Player Storage (Embedded + Array of VALUES):**
```c
typedef struct Player {
    // ...
    Trinket_t class_trinket;       // ✅ Embedded VALUE (own copy)
    bool has_class_trinket;        // ✅ Flag if equipped
    dArray_t* trinket_slots;       // ✅ Array of Trinket_t VALUES
} Player_t;

// Initialization
player.has_class_trinket = false;
memset(&player.class_trinket, 0, sizeof(Trinket_t));
player.trinket_slots = d_InitArray(6, sizeof(Trinket_t));  // VALUES!
```

**Equipping (DEEP COPY):**
```c
bool EquipClassTrinket(Player_t* player, Trinket_t* template) {
    // Shallow copy struct
    memcpy(&player->class_trinket, template, sizeof(Trinket_t));

    // CRITICAL: Deep-copy dStrings (memcpy copied pointers, not data!)
    player->class_trinket.name = d_StringInit();
    d_StringSet(player->class_trinket.name, d_StringPeek(template->name), 0);

    player->class_trinket.description = d_StringInit();
    d_StringSet(player->class_trinket.description, d_StringPeek(template->description), 0);

    // ... copy other dStrings ...

    // Reset per-instance state (each player starts fresh)
    player->class_trinket.total_damage_dealt = 0;
    player->class_trinket.passive_damage_bonus = 0;
    player->class_trinket.active_cooldown_current = 0;

    player->has_class_trinket = true;
    return true;
}
```

**Accessing (Pointer to VALUE):**
```c
Trinket_t* GetClassTrinket(const Player_t* player) {
    if (!player->has_class_trinket) return NULL;
    return (Trinket_t*)&player->class_trinket;  // Pointer to embedded value
}

Trinket_t* GetEquippedTrinket(const Player_t* player, int slot) {
    Trinket_t* trinket = (Trinket_t*)d_IndexDataFromArray(player->trinket_slots, slot);
    if (!trinket || !trinket->name) return NULL;  // Empty slot check
    return trinket;  // Pointer to value in array
}
```

**Cleanup (ADR-014 compliant):**
```c
void CleanupTrinketValue(Trinket_t* trinket) {
    // Free internal dStrings only (Daedalus owns the struct itself)
    if (trinket->name) d_StringDestroy(trinket->name);
    if (trinket->description) d_StringDestroy(trinket->description);
    if (trinket->passive_description) d_StringDestroy(trinket->passive_description);
    if (trinket->active_description) d_StringDestroy(trinket->active_description);
    // DO NOT free trinket itself - stored by value in table/array/embedded
}
```

## Consequences

**Positive:**
- ✅ Constitutional compliance (no malloc for trinkets)
- ✅ Per-player instance state (cooldowns, damage stats independent)
- ✅ Struct field additions isolated (template changes don't affect existing copies)
- ✅ Simpler cleanup (no pointer tracking, no double-free risk)
- ✅ Stable addresses for class trinket (embedded in Player_t, safe for TweenFloat)
- ✅ Matches g_players pattern (ADR-003 precedent)

**Negative:**
- ❌ Deep-copy overhead on equip (acceptable - rare operation, not per-frame)
- ❌ Must remember to copy dStrings after memcpy (memcpy only copies pointers)
- ❌ Slightly more memory (each player has full trinket copy vs shared pointer)
- ❌ Template changes require re-equip to propagate (by design - instances are snapshots)

**Accepted Trade-offs:**
- ✅ Accept deep-copy complexity for per-player isolation
- ✅ Accept memory overhead for safety and correctness
- ❌ NOT accepting: Shared mutable state (was source of bugs)
- ❌ NOT accepting: Raw malloc for trinkets (constitutional violation)

## Tweening Considerations

**Class Trinket (Embedded):**
- Address is `&player->class_trinket.shake_offset_x`
- Stable address (embedded in Player_t which is in g_players table)
- Safe to use `TweenFloat()` directly

**Regular Trinkets (Array):**
- Address is `&((Trinket_t*)d_IndexDataFromArray(slots, i))->shake_offset_x`
- Address stable as long as array doesn't reallocate
- Prefer `TweenFloatInArray()` with slot_index for extra safety

```c
// Class trinket - direct tweening (stable address)
TweenFloat(tween_mgr, &self->shake_offset_x, 5.0f, 0.15f, TWEEN_EASE_OUT_ELASTIC);

// Regular trinket - use slot_index (defensive)
// TweenFloatInArray(tween_mgr, player->trinket_slots, slot_index,
//                   offsetof(Trinket_t, shake_offset_x), 5.0f, 0.15f, ...);
```

## Evidence From Codebase

**structs.h (Player_t definition):**
```c
typedef struct Player {
    // ...
    Trinket_t class_trinket;       // Embedded VALUE
    bool has_class_trinket;        // Flag if equipped
    dArray_t* trinket_slots;       // Array of Trinket_t VALUES
} Player_t;
```

**trinket.c (CreateTrinketTemplate):**
```c
Trinket_t* CreateTrinketTemplate(int trinket_id, const char* name, ...) {
    Trinket_t template = {0};  // Stack allocation, NO malloc!
    // ... initialize fields ...
    d_SetDataInTable(g_trinket_templates, &template.trinket_id, &template);
    return (Trinket_t*)d_GetDataFromTable(g_trinket_templates, &trinket_id);
}
```

**trinket.c (EquipClassTrinket):**
```c
bool EquipClassTrinket(Player_t* player, Trinket_t* trinket_template) {
    memcpy(&player->class_trinket, trinket_template, sizeof(Trinket_t));

    // Deep-copy dStrings
    player->class_trinket.name = d_StringInit();
    d_StringSet(player->class_trinket.name, d_StringPeek(trinket_template->name), 0);
    // ...

    player->has_class_trinket = true;
    return true;
}
```

## Success Metrics

- ✅ FF-016 passes (no malloc for Trinket_t, correct deep-copy pattern)
- ✅ Zero segfaults on trinket hover after struct changes
- ✅ Per-player stats work correctly (damage counters independent)
- ✅ Valgrind shows zero memory leaks in trinket code
- ✅ New trinket fields can be added at END of struct safely

## Related ADRs

- **ADR-003** (Constitutional Patterns): Mandates Daedalus types over raw malloc
- **ADR-014** (Reverse-Order Cleanup): CleanupTrinketValue follows LIFO pattern
- **This ADR (16)**: Specific application of value semantics to trinket system

**Status:** ✅ Implemented (2025-11-20)
- Refactored from pointer-based to value-based storage
- Fitness function: FF-016 (implemented)
