# Archimedes Feature Request: UI Component Lifecycle Management

**Issue Type**: Feature Request
**Component**: UI System
**Priority**: Medium
**Complexity**: Medium

---

## Problem Statement

Currently, Archimedes provides excellent abstractions for **rendering** (primitives, textures, text) and **layout** (FlexBox), but leaves **UI component lifecycle management** entirely to the application. This creates:

- ❌ **Repetitive boilerplate**: Every section/component needs manual `malloc` in `Create*()` and `free` in `Destroy*()`
- ❌ **No framework enforcement**: Nothing prevents violating the scenes→sections→components hierarchy
- ❌ **Memory management scattered**: Each UI element manages its own lifecycle independently
- ❌ **Inconsistent patterns**: Different projects may handle component lifecycle differently

### Real-World Example (from Card Fifty-Two)

**Current Implementation**: Manual memory management for every section

```c
// titleSection.c
TitleSection_t* CreateTitleSection(void) {
    TitleSection_t* section = malloc(sizeof(TitleSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate TitleSection");
        return NULL;
    }
    section->layout = NULL;
    d_LogInfo("TitleSection created");
    return section;
}

void DestroyTitleSection(TitleSection_t** section) {
    if (!section || !*section) return;
    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);
    }
    free(*section);
    *section = NULL;
    d_LogInfo("TitleSection destroyed");
}
```

**Same pattern repeated for**:
- `PlayerSection`
- `DealerSection`
- `ActionPanel`
- `PauseMenuSection`
- `TopBarSection`
- `DeckViewPanel`
- Every component (Button, MenuItem, MenuItemRow)

**Problem**: 90% of this code is **boilerplate** (`malloc`, NULL checks, `free`, logging). Only 10% is section-specific logic.

---

## Architectural Context

Card Fifty-Two follows a **three-tier UI architecture**:

```
SCENES (sceneBlackjack.c)
  ├─ Orchestrate sections
  ├─ Manage state machine
  └─ Own component/section lifecycle (manual malloc/free)
      ↓
SECTIONS (TitleSection, PlayerSection, ActionPanel)
  ├─ Collections of components + labels
  ├─ Rendering logic
  └─ FlexBox layout (Archimedes manages this!)
      ↓
COMPONENTS (Button, MenuItem)
  ├─ Fully reusable primitives
  └─ Self-contained rendering
```

**Observation**: Archimedes already manages **FlexBox lifecycle** (`a_CreateFlexBox`, `a_DestroyFlexBox`). Why not **section/component lifecycle** too?

---

## Proposed Solution: Archimedes Component Registry

Add a **lightweight component registry** to Archimedes that handles lifecycle management while preserving application-specific initialization logic.

### Core API

```c
/**
 * a_CreateComponent - Allocate and register a UI component
 *
 * @param size Size of component struct (e.g., sizeof(Button_t))
 * @param type_name Component type (for debugging, e.g., "Button")
 * @return Pointer to zero-initialized component, or NULL on failure
 *
 * Archimedes handles:
 * - Memory allocation (malloc)
 * - Zero-initialization
 * - Registration in component registry
 * - Logging
 */
void* a_CreateComponent(size_t size, const char* type_name);

/**
 * a_DestroyComponent - Cleanup and free a UI component
 *
 * @param component Pointer-to-pointer to component (set to NULL after free)
 * @param cleanup_fn Optional cleanup function (called before free)
 *
 * Archimedes handles:
 * - Calls cleanup_fn if provided (for component-specific cleanup)
 * - Unregisters from component registry
 * - Frees memory
 * - Nulls pointer
 * - Logging
 */
void a_DestroyComponent(void** component, void (*cleanup_fn)(void*));
```

---

## Usage Example

### Before (Current Manual Pattern)

**Button Component**:
```c
Button_t* CreateButton(int x, int y, int w, int h, const char* label) {
    Button_t* button = malloc(sizeof(Button_t));  // Boilerplate
    if (!button) {                                  // Boilerplate
        d_LogError("Failed to allocate button");    // Boilerplate
        return NULL;                                // Boilerplate
    }

    // Actual initialization logic
    button->x = x;
    button->y = y;
    button->w = w;
    button->h = h;
    button->enabled = true;
    button->label = d_InitString();
    d_SetString(button->label, label, 0);

    return button;
}

void DestroyButton(Button_t** button) {
    if (!button || !*button) return;              // Boilerplate
    Button_t* btn = *button;                      // Boilerplate

    // Actual cleanup logic
    if (btn->label) {
        d_DestroyString(btn->label);
    }

    free(btn);         // Boilerplate
    *button = NULL;    // Boilerplate
}
```

**14 lines of boilerplate, 8 lines of actual logic.**

---

### After (With Archimedes API)

**Button Component**:
```c
static void InitButton(Button_t* button, int x, int y, int w, int h, const char* label) {
    button->x = x;
    button->y = y;
    button->w = w;
    button->h = h;
    button->enabled = true;
    button->label = d_InitString();
    d_SetString(button->label, label, 0);
}

Button_t* CreateButton(int x, int y, int w, int h, const char* label) {
    Button_t* button = (Button_t*)a_CreateComponent(sizeof(Button_t), "Button");
    if (!button) return NULL;
    InitButton(button, x, y, w, h, label);
    return button;
}

static void CleanupButton(void* ptr) {
    Button_t* button = (Button_t*)ptr;
    if (button->label) {
        d_DestroyString(button->label);
    }
}

void DestroyButton(Button_t** button) {
    a_DestroyComponent((void**)button, CleanupButton);
}
```

**Same functionality, ~30% less code, no boilerplate.**

---

### Section Example (Before vs After)

**Before**:
```c
TitleSection_t* CreateTitleSection(void) {
    TitleSection_t* section = malloc(sizeof(TitleSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate TitleSection");
        return NULL;
    }
    section->layout = NULL;  // Only actual initialization
    d_LogInfo("TitleSection created");
    return section;
}

void DestroyTitleSection(TitleSection_t** section) {
    if (!section || !*section) return;
    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);  // Only actual cleanup
    }
    free(*section);
    *section = NULL;
    d_LogInfo("TitleSection destroyed");
}
```

**After**:
```c
TitleSection_t* CreateTitleSection(void) {
    TitleSection_t* section = (TitleSection_t*)a_CreateComponent(sizeof(TitleSection_t), "TitleSection");
    if (!section) return NULL;
    section->layout = NULL;  // Actual initialization
    return section;
}

static void CleanupTitleSection(void* ptr) {
    TitleSection_t* section = (TitleSection_t*)ptr;
    if (section->layout) {
        a_DestroyFlexBox(&section->layout);  // Actual cleanup
    }
}

void DestroyTitleSection(TitleSection_t** section) {
    a_DestroyComponent((void**)section, CleanupTitleSection);
}
```

**Result**: Focus on **what** the section needs, not **how** to allocate/free it.

---

## Implementation Outline

### Core Data Structures

```c
// In Archimedes internal state
typedef struct {
    void** components;     // Array of registered component pointers
    size_t count;
    size_t capacity;
} ComponentRegistry_t;

static ComponentRegistry_t g_component_registry = {0};
```

### Core Functions

```c
void* a_CreateComponent(size_t size, const char* type_name) {
    // 1. Allocate memory
    void* component = malloc(size);
    if (!component) {
        fprintf(stderr, "Archimedes: Failed to allocate %s\n", type_name);
        return NULL;
    }

    // 2. Zero-initialize
    memset(component, 0, size);

    // 3. Register in component registry
    RegisterComponent(component);

    // 4. Log (optional, can be disabled)
    printf("Archimedes: Created %s at %p\n", type_name, component);

    return component;
}

void a_DestroyComponent(void** component, void (*cleanup_fn)(void*)) {
    if (!component || !*component) return;

    void* comp = *component;

    // 1. Call component-specific cleanup
    if (cleanup_fn) {
        cleanup_fn(comp);
    }

    // 2. Unregister from component registry
    UnregisterComponent(comp);

    // 3. Free memory
    free(comp);

    // 4. Null pointer
    *component = NULL;

    // 5. Log (optional)
    printf("Archimedes: Destroyed component at %p\n", comp);
}
```

### Automatic Cleanup (Optional Advanced Feature)

```c
// Called in a_Quit() - cleanup any leaked components
void a_CleanupAllComponents(void) {
    if (g_component_registry.count > 0) {
        fprintf(stderr, "Archimedes: Warning - %zu components not destroyed\n",
                g_component_registry.count);

        // Optional: Free leaked components
        for (size_t i = 0; i < g_component_registry.count; i++) {
            free(g_component_registry.components[i]);
        }
    }
}
```

---

## Benefits

✅ **Reduces boilerplate**: Eliminates repetitive malloc/free/NULL check code
✅ **Consistent patterns**: All components use same lifecycle API
✅ **Better debugging**: Registry tracks all components, detects leaks
✅ **Framework alignment**: Matches Archimedes' existing API style (a_CreateFlexBox, a_LoadTexture, etc.)
✅ **Zero breaking changes**: Optional API, existing code unaffected
✅ **Enforces architecture**: Makes scenes→sections→components pattern explicit

---

## Alternative Considered

### Option A: Keep manual malloc/free (status quo)

**Rejected because**:
- Boilerplate duplicated across every component
- No framework support for architectural patterns
- Missed opportunity for automatic leak detection

### Option B: Full object-oriented component system

```c
typedef struct Component {
    void (*render)(struct Component* self);
    void (*destroy)(struct Component* self);
} Component_t;
```

**Rejected because**:
- Over-engineered for Archimedes' philosophy
- Forces inheritance-style patterns on C code
- Breaks Archimedes' simple functional API style

**Decision**: Lightweight registry + lifecycle helpers = best balance of power and simplicity.

---

## Migration Path

### Phase 1: Add API (Non-Breaking)
- Implement `a_CreateComponent()` and `a_DestroyComponent()`
- Add to Archimedes header
- Document in Archimedes guide

### Phase 2: Adopt in Card Fifty-Two (Validation)
- Refactor Button, MenuItem, MenuItemRow
- Refactor TitleSection, PlayerSection, ActionPanel
- Measure lines of code reduction
- Gather feedback

### Phase 3: Refine and Stabilize
- Fix edge cases discovered during adoption
- Optimize registry performance if needed
- Finalize API before Archimedes 1.0

---

## Success Metrics

- ✅ Card Fifty-Two eliminates 50+ lines of boilerplate code
- ✅ All components use consistent lifecycle pattern
- ✅ Component leak detection catches bugs in development
- ✅ API adopted by 3+ Archimedes projects

---

## Proposed by

**Project**: Card Fifty-Two
**Context**: Refactoring section-based UI architecture, noticed repetitive malloc/free patterns
**Date**: 2025-10-13

---

**Status**: Awaiting implementation in Archimedes
