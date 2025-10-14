# Daedalus Missing `d_ClearArray()` Function

**Issue Type:** Missing API
**Target Project:** Daedalus
**Severity:** Medium (Quality of Life / API Completeness)
**Status:** Proposed

## Problem Statement

Daedalus provides `dArray_t` as the primary dynamic array abstraction, following the "everything's a table or array" philosophy. However, there is **no function to clear an array's contents** without destroying the array itself.

**Current workarounds:**
1. **Manual pop loop** - inefficient, verbose
2. **Destroy and recreate** - causes unnecessary allocations
3. **Direct manipulation** - breaks encapsulation (setting `array->count = 0` manually)

## Current API Gap

```c
// ✅ Available functions
dArray_t* d_InitArray(size_t capacity, size_t element_size);
int d_DestroyArray(dArray_t* array);
int d_AppendDataToArray(dArray_t* array, void* data);
void* d_PopDataFromArray(dArray_t* array);
int d_RemoveDataFromArray(dArray_t* array, size_t index);

// ❌ Missing function
int d_ClearArray(dArray_t* array);  // NOT AVAILABLE
```

## Use Case: Hand Management in Card Games

**Scenario:** At the end of a card game round, players need to clear their hands for the next round.

### Current Workaround (Inefficient):

```c
void ClearHand(Hand_t* hand) {
    // ❌ Manual loop - O(n) with n function calls
    while (hand->cards->count > 0) {
        d_PopDataFromArray(hand->cards);
    }

    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}
```

**Problems:**
- O(n) function call overhead (one `d_PopDataFromArray()` per element)
- Verbose and error-prone
- Not obvious to new users
- Allocations/deallocations if array resizes during rebuild

### Alternative Workaround (Wasteful):

```c
void ClearHand(Hand_t* hand) {
    // ❌ Destroy and recreate - unnecessary allocations
    d_DestroyArray(hand->cards);
    hand->cards = d_InitArray(sizeof(Card_t), 10);

    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}
```

**Problems:**
- Free + malloc on every clear
- Memory fragmentation
- Lost capacity information (array may have grown to 20+ cards)
- Inefficient for hot paths (clearing hands every round)

### Breaking Encapsulation (Dangerous):

```c
void ClearHand(Hand_t* hand) {
    // ❌ Direct manipulation - breaks Daedalus abstraction
    hand->cards->count = 0;  // Accessing internal struct!

    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}
```

**Problems:**
- Assumes knowledge of `dArray_t` internal structure
- Breaks if Daedalus changes implementation
- No validation or error checking
- Violates "use Daedalus functions, not raw manipulation"

## Proposed Solution

### API Addition: `d_ClearArray()`

```c
/**
 * d_ClearArray - Remove all elements from array without deallocating
 *
 * @param array The array to clear
 * @return 0 on success, -1 on failure
 *
 * Sets array->count to 0, preserving array->capacity
 * Does NOT free the internal buffer - array can be reused immediately
 *
 * Use case: Clearing collections for reuse (hands, queues, temp buffers)
 */
int d_ClearArray(dArray_t* array);
```

**Implementation (inside Daedalus):**

```c
int d_ClearArray(dArray_t* array) {
    if (!array) {
        d_LogError("d_ClearArray: NULL array pointer");
        return -1;
    }

    // Simply reset count - preserve capacity
    array->count = 0;

    return 0;
}
```

**Usage (Card Fifty-Two):**

```c
void ClearHand(Hand_t* hand) {
    // ✅ Clean, efficient, O(1)
    d_ClearArray(hand->cards);

    hand->total_value = 0;
    hand->is_bust = false;
    hand->is_blackjack = false;
}
```

## Benefits

✅ **O(1) complexity** - instant clear, no loops
✅ **Preserves capacity** - no reallocations on next append
✅ **Simple API** - one function call, obvious intent
✅ **Consistent with stdlib** - similar to `vector::clear()` in C++
✅ **Idiomatic Daedalus** - follows "use library functions, not raw manipulation"
✅ **Safe** - validates NULL, can add future safety checks

## Comparison with Other Libraries

| Library | Clear Function | Complexity |
|---------|----------------|------------|
| C++ STL | `vector::clear()` | O(1) |
| Rust Vec | `vec.clear()` | O(1) |
| Python list | `list.clear()` | O(1) |
| Daedalus | ❌ **Missing** | N/A |

**Every major dynamic array library provides a clear function.**

## Alternative Names (If Preferred)

- `d_RemoveAllDataFromArray()` - verbose but explicit
- `d_EmptyArray()` - shorter, less clear
- `d_ResetArray()` - ambiguous (reset capacity too?)
- `d_ClearArray()` - **recommended** (standard convention)

## Implementation Considerations

### Question 1: Should it zero memory?

**Option A:** Just reset count (fast, recommended)
```c
int d_ClearArray(dArray_t* array) {
    array->count = 0;
    return 0;
}
```

**Option B:** Zero the buffer (slower, more secure)
```c
int d_ClearArray(dArray_t* array) {
    memset(array->data, 0, array->count * array->element_size);
    array->count = 0;
    return 0;
}
```

**Recommendation:** Option A (just reset count). If users need zeroed memory, they can use `memset()` manually or we provide a separate `d_ClearAndZeroArray()`.

### Question 2: Should it shrink capacity?

**No.** The point of clear is to preserve capacity for reuse. If users want to shrink, they can call `d_ResizeArray()` afterward.

### Question 3: Should it call destructors?

**No.** `dArray_t` stores **value types** (Card_t, int, etc.), not pointers. Value types don't need destructors. If users store pointers, they should manually free before clearing.

## Related Missing Functions

While adding `d_ClearArray()`, consider also adding:

1. **`d_ClearTable(dTable_t* table)`** - clear hash table without destroying
2. **`d_ShrinkArrayToFit(dArray_t* array)`** - reduce capacity to match count
3. **`d_ReserveArray(dArray_t* array, size_t capacity)`** - preallocate capacity

## Impact on Card Fifty-Two

**Files affected by adding this function:**
- `src/hand.c` - `ClearHand()` can be simplified
- `src/deck.c` - `ResetDeck()` could use `d_ClearArray()`
- Future game modes - any collection reset logic

**Current blocker:**
- Cannot build Card Fifty-Two because `ClearHand()` calls non-existent `d_ClearArray()`
- Forced to use inefficient workarounds

## Conclusion

The lack of `d_ClearArray()` is a **missing fundamental operation** in Daedalus. Every dynamic array library provides this. The implementation is trivial (one line), but the API completeness is critical.

**Recommendation:** Add `d_ClearArray()` to Daedalus core, along with `d_ClearTable()` for consistency.

---

**Filed by:** Card Fifty-Two development team
**Date:** 2025-10-06
**Priority:** Medium (API gap, workarounds exist but inefficient)
