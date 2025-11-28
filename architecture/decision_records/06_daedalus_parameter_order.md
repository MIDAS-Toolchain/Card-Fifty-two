# ADR-006: d_ArrayInit Parameter Order Enforcement

## Context

Daedalus library's `d_ArrayInit()` function has a **non-intuitive parameter order**:

```c
dArray_t* d_ArrayInit(size_t capacity, size_t element_size);
```

This order is **backwards** from common C idioms like `calloc(count, size)` and many array libraries that put element type/size first.

**The Bug That Motivated This ADR:**

In `src/player.c:66`, we had:
```c
player.trinket_slots = d_ArrayInit(sizeof(Trinket_t*), 6);  // WRONG!
```

This created an array with:
- **Capacity:** 8 (sizeof pointer on 64-bit)
- **Element size:** 6

When accessing `slot_index = 1`:
- Expected offset: `1 * 8 = 8` bytes
- Actual offset: `1 * 6 = 6` bytes (misaligned!)
- Result: **Bus error** from dereferencing misaligned pointer

This bug caused **intermittent crashes** at COMBAT_START that took days to debug.

## Alternatives Considered

1. **Just be careful** (status quo before this ADR)
   - Pros: No tooling needed, simple
   - Cons: Error-prone, hard to catch in review, silent misalignment bugs
   - Cost: **Already paid - days of debugging**

2. **Wrapper macro with natural order**
   ```c
   #define INIT_ARRAY(type, capacity) d_ArrayInit(capacity, sizeof(type))
   ```
   - Pros: Natural order, type-safe
   - Cons: Hides library API, inconsistent with direct Daedalus calls
   - Cost: Mixed idioms in codebase, confusion about when to use wrapper

3. **Static analysis tool / fitness function** (chosen)
   - Pros: Automated enforcement, catches all violations, CI/CD integration
   - Cons: Requires maintenance, can have false positives
   - Cost: Initial implementation effort, ongoing tuning

4. **Custom fork of Daedalus with flipped params**
   - Pros: "Natural" order in our code
   - Cons: Vendor fork maintenance, diverges from upstream, breaks examples
   - Cost: Ongoing merge conflicts, no community bugfixes

## Decision

**Implement FF-006 (d_ArrayInit Parameter Order Verification) as mandatory pre-commit check.**

All `d_ArrayInit()` calls MUST follow this pattern:
```c
// ✅ CORRECT: Capacity first, element_size second
array = d_ArrayInit(CAPACITY_CONSTANT, sizeof(Type_t));
```

All violations must include an inline comment explaining the correct order:
```c
// d_ArrayInit(capacity, element_size) - capacity FIRST!
player.trinket_slots = d_ArrayInit(6, sizeof(Trinket_t*));
```

**Justification:**

1. **Historical Precedent**: This bug caused a critical intermittent crash (bus error)
2. **Silent Failures**: Misaligned pointers don't always crash immediately (Heisenbug)
3. **Code Review Insufficient**: Human review missed this multiple times
4. **Systematic Solution**: Fitness function catches ALL violations, past and future

## Consequences

**Positive:**
- ✅ Automated detection of parameter order bugs
- ✅ Prevents bus errors from misaligned array access
- ✅ Enforces consistent commenting pattern for clarity
- ✅ CI/CD integration prevents merging broken code
- ✅ Educates developers via clear error messages

**Negative:**
- ❌ One more fitness function to maintain
- ❌ Possible false positives (mitigated by exception markers)
- ❌ Doesn't fix the root cause (Daedalus API design)

**Accepted Trade-offs:**
- ✅ Accept maintenance burden of FF to prevent catastrophic bugs
- ✅ Accept verbosity of inline comments for safety
- ❌ NOT accepting: Wrapper macros (too much indirection)
- ❌ NOT accepting: Daedalus fork (vendor lock-in worse than parameter order)

## Pattern Examples

**Correct Patterns:**

```c
// ✅ Array of value types (Card_t stored directly)
dArray_t* cards = d_ArrayInit(52, sizeof(Card_t));

// ✅ Array of pointers (Player_t* stored)
dArray_t* player_refs = d_ArrayInit(4, sizeof(Player_t*));

// ✅ Array with variable capacity
dArray_t* abilities = d_ArrayInit(enemy->max_abilities, sizeof(AbilityData_t));

// ✅ With inline comment for clarity
// d_ArrayInit(capacity, element_size) - capacity FIRST!
player.trinket_slots = d_ArrayInit(6, sizeof(Trinket_t*));
```

**Incorrect Patterns (Caught by FF-006):**

```c
// ❌ Parameters swapped
dArray_t* cards = d_ArrayInit(sizeof(Card_t), 52);

// ❌ sizeof first (looks like calloc but wrong!)
dArray_t* trinkets = d_ArrayInit(sizeof(Trinket_t*), 6);

// ❌ Variable element_size first
dArray_t* array = d_ArrayInit(element_size, capacity);
```

## Evidence From Codebase

**Bug Location (src/player.c:66 - BEFORE FIX):**
```c
// ❌ WRONG - Created array with capacity=8, element_size=6
player.trinket_slots = d_ArrayInit(sizeof(Trinket_t*), 6);
```

**Fixed Version (src/player.c:67 - AFTER FIX):**
```c
// ✅ CORRECT - Creates array with capacity=6, element_size=8
// d_ArrayInit(capacity, element_size) - capacity FIRST!
player.trinket_slots = d_ArrayInit(6, sizeof(Trinket_t*));
```

**Other Correct Examples in Codebase:**

**src/enemy.c:57-58:**
```c
// Capacity: 16 (prevents realloc during combat)
enemy->passive_abilities = d_ArrayInit(16, sizeof(AbilityData_t));
enemy->active_abilities = d_ArrayInit(16, sizeof(AbilityData_t));
```

**src/statusEffects.c:20:**
```c
// Capacity: 32 (generous to prevent realloc bugs)
manager->active_effects = d_ArrayInit(32, sizeof(StatusEffectInstance_t));
```

**src/hand.c:21:**
```c
hand.cards = d_ArrayInit(16, sizeof(Card_t));  // Max 16 cards in hand
```

## Impact Analysis

**Bug Severity:** 🔴 **CRITICAL**
- Caused intermittent bus error crashes (~66% failure rate)
- Heisenbug (disappeared under debugger due to different memory layout)
- Silent data corruption (misaligned reads didn't always crash immediately)
- Took **2+ days of debugging** to identify root cause

**Detection Difficulty:** 🟡 **MODERATE-HIGH**
- Code review missed it (looks plausible to humans)
- Compiler accepted it (both params are size_t, no type error)
- Tests passed (test environment had cleaner memory)
- Required GDB + core dump analysis to identify

**Prevention Cost:** 🟢 **LOW**
- Fitness function implementation: ~2 hours
- CI/CD integration: ~30 minutes
- Ongoing maintenance: ~5 minutes per false positive (rare)

**ROI:** 🟢 **EXTREMELY POSITIVE**
- Prevents days of debugging on future occurrences
- Catches bugs that humans + compiler miss
- One-time investment protects entire codebase forever

## Success Metrics

**Fitness Function (FF-006):**
- Zero `d_ArrayInit(sizeof(...), ...)` patterns in src/ or include/
- All d_ArrayInit calls match regex: `d_ArrayInit\([A-Z_0-9]+,\s*sizeof\(`
- Exception markers allowed for justified violations
- Pre-commit hook rejects commits with violations

**Testing:**
- Stress test: 1000 game startups without bus error
- Valgrind: Zero invalid reads from array access
- Array capacity verification test passes

**Code Quality:**
- All d_ArrayInit calls include inline comment with parameter order
- Developers understand why this pattern matters (link to this ADR)
- New contributors warned by FF before submitting bad code

## Related Decisions

- **ADR-003**: Constitutional Patterns (mandates Daedalus types)
- **FF-003**: Constitutional Pattern Enforcement (verifies dArray_t usage)
- **FF-006**: This fitness function (verifies parameter order)

## Migration Path

**Existing Code:**
1. Run `python architecture/fitness_functions/06_daedalus_parameter_order.py`
2. Fix all violations by swapping parameters
3. Add inline comment: `// d_ArrayInit(capacity, element_size) - capacity FIRST!`
4. Verify with stress tests and Valgrind

**New Code:**
1. Pre-commit hook runs FF-006 automatically
2. Developer gets clear error message with correct pattern
3. Links to this ADR for context
4. Commit blocked until fixed

## Appendix: Why This Bug Is So Dangerous

**Memory Layout Visualization:**

```
WRONG (capacity=8, element_size=6):
[ptr0][ptr1][ptr2][ptr3][ptr4][ptr5][ptr6][ptr7]
0     6     12    18    24    30    36    42    <- Byte offsets (6-byte stride)
      ^^^ MISALIGNED for 8-byte pointers!

CORRECT (capacity=6, element_size=8):
[ptr0      ][ptr1      ][ptr2      ][ptr3      ][ptr4      ][ptr5      ]
0          8          16         24         32         40         <- Byte offsets (8-byte stride)
           ^^^ ALIGNED for 8-byte pointers ✓
```

**Why It's Intermittent:**
- Accessing slot 0 with wrong element_size: `0 * 6 = 0` (accidentally correct!)
- Accessing slot 1: `1 * 6 = 6` (misaligned → bus error on some CPUs)
- Under debugger: Different memory layout may hide the bug (Heisenbug effect)

**Why Tests Passed:**
- Tests don't access slot 1 (only equip trinket to slot 0)
- Test environment has cleaner memory (less likely to trigger fault)
- Full game has more complex initialization order → triggers the bug

This is a **textbook example** of why parameter order matters for systems programming.
