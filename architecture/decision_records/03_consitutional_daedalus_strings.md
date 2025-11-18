# ADR-004: Constitutional Patterns (Daedalus Types Over Raw C)

## Context

C lacks built-in dynamic strings and arrays. Every dynamic data structure requires manual memory management with malloc/realloc/free. Must choose a consistent approach for the entire codebase.

**Alternatives Considered:**

1. **Daedalus library types** (dString_t, dArray_t, dTable_t)
   - Pros: Consistent API, automatic growth, recursive cleanup, type-safe storage
   - Cons: Vendor lock-in, learning curve, less control over allocation strategy
   - Cost: Dependency on external library, must understand Daedalus patterns

2. **Raw malloc/realloc** (manual memory management)
   - Pros: No dependencies, maximum control, standard C idioms
   - Cons: Manual cleanup everywhere, easy to leak, verbose grow logic
   - Cost: Every array needs custom realloc code, every string needs size tracking

3. **C++ std containers** (std::string, std::vector)
   - Pros: Battle-tested, idiomatic C++, rich API
   - Cons: Language change (not C), ABI compatibility issues, different paradigm
   - Cost: Entire codebase becomes C++, mixing C/C++ causes friction

4. **Custom wrapper library** (homegrown mystring_t, myarray_t)
   - Pros: Tailored to exact needs, no external dependencies
   - Cons: Reinventing wheel, testing burden, maintenance overhead
   - Cost: Library code becomes part of project responsibility

## Decision

**Mandate Daedalus types (dString_t, dArray_t, dTable_t) as "constitutional pattern" enforced project-wide.**

All dynamic strings use dString_t. All dynamic arrays use dArray_t. All key-value maps use dTable_t. No raw malloc for these types without explicit justification in comments.

**Justification:**

1. **Grep-able Violations**: `grep "malloc"` shows exceptions, not legitimate string/array use
2. **Automatic Cleanup**: Destroy functions handle nested resources recursively
3. **Type Safety**: dArray_t stores element_size, prevents mismatched append operations
4. **Value Semantics in Tables**: dTable_t stores structs by VALUE via memcpy (no lifetime issues)
5. **Already Committed**: Daedalus is core dependency (not adding new library)

## Consequences

**Positive:**
- ✅ Fewer memory leaks (Daedalus handles realloc correctly)
- ✅ Code review is mechanical (pattern violations obvious)
- ✅ New developers follow existing precedent (examples everywhere)
- ✅ No buffer overflow on string operations (dString_t auto-grows)
- ✅ Tables store by value (no dangling pointer bugs)

**Negative:**
- ❌ Vendor lock-in to Daedalus (can't easily switch)
- ❌ Less control over allocation timing and strategy
- ❌ Extra pointer indirection (library manages memory)

**Accepted Trade-offs:**
- ✅ Accept vendor lock-in for consistency and safety
- ✅ Accept allocation strategy limitations for automatic memory management
- ❌ NOT accepting: Mixing raw pointers with Daedalus types - all or nothing

**Pattern Used:**

**Value types** (Card_t, Hand_t) are stored IN arrays by value:
```c
dArray_t* cards = d_CreateArray(sizeof(Card_t));
Card_t card = {...};
d_AppendDataToArray(cards, &card);  // memcpy into array
```

**Reference types** (Player name) are stored AS library types:
```c
typedef struct Player {
    dString_t* name;  // Heap-allocated dString_t
    // ...
} Player_t;
```

**Tables store entire structs** by value using memcpy:
```c
Player_t player = {...};
d_SetDataInTable(g_players, &player.player_id, &player);  // Copies entire struct
```

## Evidence From Codebase

**player.c (Lines 23-31):**
```c
Player_t player = {0};

// Constitutional pattern: dString_t for name, not char[]
player.name = d_StringInit();
if (!player.name) {
    d_LogFatal("CreatePlayer: Failed to initialize name string");
    return false;
}
d_StringSet(player.name, name, 0);
```

**game.c (Lines 255-256):**
```c
// Constitutional pattern: dArray_t for active player list
dArray_t* active_players;  // Array of int (player IDs)
```

**structs.h (Lines 70-74):**
```c
typedef struct Hand {
    dArray_t* cards;      // dArray of Card_t (value types)
    int total_value;
    bool is_bust;
    bool is_blackjack;
} Hand_t;
```

## Success Metrics

- Fitness function: Zero `malloc/calloc/realloc` for string/array allocation
- Valgrind shows zero memory leaks after 1000-round stress test
- Code review rejects raw malloc for dynamic strings/arrays
- New contributors naturally follow pattern (precedent is clear)
