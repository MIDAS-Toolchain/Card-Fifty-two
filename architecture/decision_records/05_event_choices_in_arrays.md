# ADR-006: Event Choices as Value Types Over Pointer Arrays

## Context

GameEvent_t needs to store multiple EventChoice_t options (2-5 choices per event). Could store as value types in dArray_t or as pointers requiring manual allocation.

**Alternatives Considered:**

1. **Value types in dArray_t** (EventChoice_t copied into array)
   - Pros: Consistent with Card_t pattern, array owns memory, simple cleanup
   - Cons: Struct copies on append, potentially large if choices have text
   - Cost: ~200 bytes per choice copy (4 dString_t pointers + ints)

2. **Pointer array** (dArray_t of EventChoice_t*)
   - Pros: No copy overhead, smaller array entries (8 bytes)
   - Cons: Manual lifetime management, use-after-free risks, scattered allocations
   - Cost: Must track ownership, free each pointer individually

3. **Linked list** (EventChoice_t* next)
   - Pros: No array resizing, easy to prepend
   - Cons: Poor cache locality, must iterate to find choice N
   - Cost: Extra 8 bytes per choice for next pointer

4. **Fixed-size array** (EventChoice_t choices[MAX_CHOICES])
   - Pros: Zero allocation, stack-friendly
   - Cons: Wastes memory, artificial limit
   - Cost: 1KB wasted per event (5 max × 200 bytes)

## Decision

**Store EventChoice_t as value types in dArray_t. GameEvent_t owns the array, DestroyEvent() cleans up recursively.**

Choices copied into array on AddEventChoice(). No manual pointer management. Consistent with Card_t pattern (value types everywhere).

**Justification:**

1. **Consistency**: Matches Card_t storage pattern (value types in arrays)
2. **Ownership Clarity**: Array owns choices, no lifetime ambiguity
3. **Simple Cleanup**: Single d_ArrayDestroy() frees everything
4. **Type Safety**: dArray_t element_size catches mismatched appends
5. **Event Count**: Typically 2-5 choices, copy cost negligible

## Consequences

**Positive:**
- ✅ Zero use-after-free bugs (array owns memory)
- ✅ One-line cleanup in DestroyEvent()
- ✅ Consistent with Card_t pattern (precedent everywhere)
- ✅ Type-safe via dArray_t element_size
- ✅ Simple to iterate (no NULL checks)

**Negative:**
- ❌ Struct copies on AddEventChoice() (~200 bytes)
- ❌ Array realloc if >4 choices (rare)
- ❌ Can't share EventChoice_t across events (each event copies)

**Accepted Trade-offs:**
- ✅ Accept 200-byte copies for safety (events only created at scene init)
- ✅ Accept no sharing for ownership clarity
- ❌ NOT accepting: Pointer arrays - lifetime complexity not worth 8-byte savings

**Pattern Used:**
```c
typedef struct EventChoice {
    dString_t* text;            // Choice button text
    dString_t* result_text;     // Outcome description
    int chips_delta;
    int sanity_delta;
    dArray_t* granted_tags;     // CardTag_t array
    dArray_t* removed_tags;
} EventChoice_t;

typedef struct GameEvent {
    dString_t* title;
    dString_t* description;
    dArray_t* choices;          // Array of EventChoice_t (value types)
    int selected_choice;
    bool is_complete;
} GameEvent_t;

// Add choice - copies struct into array
void AddEventChoice(GameEvent_t* event, const char* text, 
                    const char* result_text, int chips, int sanity) {
    EventChoice_t choice = {
        .text = d_StringInit(),
        .result_text = d_StringInit(),
        .chips_delta = chips,
        .sanity_delta = sanity,
        .granted_tags = d_CreateArray(sizeof(CardTag_t)),
        .removed_tags = d_CreateArray(sizeof(CardTag_t))
    };
    d_StringSet(choice.text, text, 0);
    d_StringSet(choice.result_text, result_text, 0);
    
    d_ArrayAppend(event->choices, &choice);  // Copy into array
}

// Cleanup - array owns choices
void DestroyEvent(GameEvent_t** event) {
    // Free choices array (Daedalus handles nested cleanup)
    d_ArrayDestroy((*event)->choices);
    free(*event);
    *event = NULL;
}
```

## Success Metrics

- Valgrind shows zero memory leaks after event system stress test
- Fitness function: EventChoice_t stored by value in GameEvent_t.choices
- Code review shows no manual malloc/free in event.c
- Event creation takes <1ms (copy overhead negligible)