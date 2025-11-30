# ADR-020: Cross-Compiler Portability (GCC vs Clang/Emscripten)

**Status:** Active
**Date:** 2025-11-28
**Context:** Emscripten web build compilation errors

---

## Context

During Emscripten web build setup, we encountered multiple compilation errors that **GCC silently allowed** but **Clang (Emscripten's compiler) rejected**. These errors revealed latent portability bugs:

1. **Typedef/forward declaration mismatch** (`Settings_t`)
2. **Platform-specific struct sizes** (`Card_t` assertion)
3. **Missing function prototypes** (`State_Transition`)
4. **Missing header includes** (`aImage_t` in `structs.h`)

### Why GCC vs Clang Differ

| Aspect | GCC | Clang |
|--------|-----|-------|
| **Philosophy** | Lenient, backward-compatible (GNU extensions) | Strict C standard enforcement |
| **Implicit functions** | Allowed in C89 mode (default) | Forbidden in C99+ |
| **Type compatibility** | "Compatible enough" heuristics | Exact type matching |
| **Platform assumptions** | x86_64-specific defaults | Architecture-agnostic |
| **Const correctness** | Warnings on discards | Strict errors |

### Concrete Example: Settings_t Typedef

**Code that compiled with GCC but failed with Clang:**

```c
// common.h (forward declaration)
typedef struct Settings_t Settings_t;  // Tagged struct
extern Settings_t* g_settings;

// settings.h (definition)
typedef struct {  // ❌ Anonymous struct!
    int sound_volume;
    // ...
} Settings_t;
```

**Clang error:**
```
error: typedef redefinition with different types
('struct Settings_t' vs 'struct Settings_t')
```

**Why GCC allowed it:**
GCC's type compatibility heuristic treated tagged and anonymous structs as "compatible enough" if their members matched. Clang enforces strict type identity.

**Fix:**
```c
// settings.h (definition)
typedef struct Settings_t {  // ✅ Tagged struct matches forward declaration
    int sound_volume;
    // ...
} Settings_t;
```

---

## Decision

**Enforce C99/C11 compliance across all compilers.**

### Rules

1. **Forward declarations MUST match typedef definitions**
   - If forward-declared as `typedef struct Foo_t Foo_t;`, definition MUST use `typedef struct Foo_t { } Foo_t;`
   - Tagged structs required when forward-declaring typedefs

2. **Platform-aware static assertions for pointer-containing structs**
   ```c
   #ifdef __EMSCRIPTEN__
   _Static_assert(sizeof(Card_t) == 28, "Card_t must be 28 bytes on WASM32");
   #else
   _Static_assert(sizeof(Card_t) == 32, "Card_t must be 32 bytes on x64");
   #endif
   ```

3. **All function calls MUST have prototypes**
   - Include headers before calling functions
   - No implicit function declarations

4. **All types MUST be defined before use**
   - `#include <Archimedes.h>` before using `aImage_t`
   - Forward declarations for recursive structs

5. **Const correctness enforced**
   - No implicit `const` → non-`const` casts

---

## Consequences

### Benefits

✅ **Cross-compiler portability** - Builds on GCC, Clang, Emscripten, MSVC
✅ **Early bug detection** - Catches type mismatches at compile time
✅ **Platform agnostic** - No x86_64-specific assumptions
✅ **Standards compliance** - Clean C99/C11 code

### Costs

⚠️ **More verbose code** - Explicit includes, platform guards
⚠️ **Breaking changes** - Existing GCC-only code may need fixes
⚠️ **Build system complexity** - Platform-specific assertions

---

## Evidence from Codebase

### 1. Settings_t Typedef Fix

**Location:** `include/settings.h:14-25`

**Before (anonymous struct):**
```c
typedef struct {
    // Audio settings
    int sound_volume;     // 0-100
    int music_volume;     // 0-100
    bool sound_enabled;
    bool music_enabled;
    // ...
} Settings_t;
```

**After (tagged struct):**
```c
typedef struct Settings_t {
    // Audio settings
    int sound_volume;     // 0-100
    int music_volume;     // 0-100
    bool sound_enabled;
    bool music_enabled;
    // ...
} Settings_t;
```

**Impact:** Matches forward declaration in `common.h:58`: `typedef struct Settings_t Settings_t;`

---

### 2. Platform-Aware Card_t Assertion

**Location:** `include/structs.h:89-94`

```c
// Platform-aware size assertion (pointers differ: x64=8 bytes, WASM32=4 bytes)
#ifdef __EMSCRIPTEN__
_Static_assert(sizeof(Card_t) == 28, "Card_t must be exactly 28 bytes on WASM32");
#else
_Static_assert(sizeof(Card_t) == 32, "Card_t must be exactly 32 bytes on native 64-bit");
#endif
```

**Why needed:** `Card_t` contains `aImage_t* texture` pointer:
- **x64:** 8-byte pointer → 32-byte struct
- **WASM32:** 4-byte pointer → 28-byte struct

**Old code:**
```c
_Static_assert(sizeof(Card_t) == 32, "Card_t size changed!");  // ❌ Fails on WASM32
```

---

### 3. Missing State_Transition Prototype

**Location:** `src/game.c:145` (function call)

**Error:**
```
error: call to undeclared function 'State_Transition'
```

**Fix:** Added `#include "../include/state.h"` at top of `game.c:16`

**Why GCC allowed it:** GCC's C89 mode permits implicit function declarations (assumes `int State_Transition(int)`)

---

### 4. Missing aImage_t Definition

**Location:** `include/structs.h:72` (Card_t definition)

**Error:**
```
error: unknown type name 'aImage_t'
```

**Fix:** Added `#include <Archimedes.h>` at top of `structs.h:8`

**Why GCC allowed it:** GCC found the header through transitive includes (luck). Clang enforces explicit includes.

---

## Related ADRs

- **ADR-003:** Daedalus Memory Management (struct lifetime rules)
- **ADR-006:** d_ArrayInit Parameter Order (platform correctness)
- **ADR-018:** Settings Architecture (Settings_t structure)

---

## Enforcement

**Fitness Function:** `FF-020` verifies:
1. Forward declarations match typedef tags
2. Static assertions use platform guards
3. No function calls without includes
4. Typedef structs use tags when forward-declared

**Run:** `make verify` or `python3 architecture/fitness_functions/20_cross_compiler_portability.py`

---

## Notes

- **Emscripten uses Clang** - All Clang rules apply
- **Test on multiple compilers** - GCC, Clang, Emscripten
- **CI/CD should test web build** - Catches portability issues early
- **WASM32 pointer size** - 4 bytes, not 8 bytes
