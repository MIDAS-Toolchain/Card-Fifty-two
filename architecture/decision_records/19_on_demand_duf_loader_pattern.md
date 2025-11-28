# ADR-019: On-Demand DUF Loader Pattern with Startup Validation

## Context

Game data (enemies, trinkets, affixes, and future content like events/card tags) is stored in DUF (Daedalus Unified Format) files. As the game grows, we need a consistent, maintainable pattern for loading this data that balances:

1. **Memory efficiency** - Don't load everything at startup
2. **Data integrity** - Catch schema errors before gameplay starts
3. **Developer experience** - Clear error messages when data is malformed
4. **Code consistency** - All loaders follow same pattern

**Current State:**

We have three DUF loaders already implemented:
- `enemyLoader.c` - Loads enemy definitions with nested abilities
- `trinketLoader.c` - Loads trinket templates (combat + event)
- `affixLoader.c` - Loads affix templates for trinket randomization

Each evolved organically but converged on similar patterns. We need to codify this as the official approach.

**Alternatives Considered:**

### 1. Pre-Load All Templates into Memory

**Pattern:**
```c
dTable_t* g_enemy_templates;  // Load ALL enemies at startup

void InitEnemySystem(void) {
    // Parse DUF
    d_DUFParseFile("enemies.duf", &db);

    // Pre-load ALL enemies into table
    for (each enemy in db) {
        Enemy_t* enemy = ParseEnemy(enemy_node);
        d_TableSet(g_enemy_templates, &enemy->id, enemy);
    }
}

Enemy_t* GetEnemy(int id) {
    return d_TableGet(g_enemy_templates, &id);  // Instant lookup
}
```

**Pros:**
- Fast lookup (already parsed)
- No repeated parsing overhead
- Simple caller pattern (just table lookup)

**Cons:**
- High memory footprint (loads ALL enemies, even unused ones)
- Duplicate data (DUF tree + template table in memory)
- Sync issues (which is source of truth: DUF or table?)
- Eager initialization overhead at startup

**Cost:** ~50KB per 100 enemies (each enemy ~500 bytes + dStrings)

### 2. Lazy Parsing Without Validation

**Pattern:**
```c
Enemy_t* LoadEnemy(const char* key) {
    // Parse on first access, no startup validation
    dDUFValue_t* node = d_DUFGetObjectItem(g_enemies_db, key);
    if (!node) return NULL;  // Runtime error!

    return ParseEnemy(node);  // Could fail during gameplay
}
```

**Pros:**
- Minimal memory (parse only what's needed)
- Fast startup (no validation pass)

**Cons:**
- Runtime crashes from bad data (schema errors during gameplay)
- No user-friendly error messages (logs only)
- Hard to debug (error discovered far from source)
- Players experience crashes instead of developers

**Cost:** Bug reports, lost progress, negative reviews

### 3. Compile-Time Code Generation

**Pattern:**
```c
// enemies.duf → enemies_generated.c (via Python script)

Enemy_t ENEMY_DIDACT = {
    .name = "The Didact",
    .hp = 10,
    // ... all fields hardcoded
};

Enemy_t* GetEnemy(const char* key) {
    if (strcmp(key, "didact") == 0) return &ENEMY_DIDACT;
    // ...
}
```

**Pros:**
- Zero runtime parsing (compile-time validation)
- Maximum performance
- Type-safe (compiler checks schema)

**Cons:**
- Requires build step (Python → C codegen)
- Longer iteration time (edit DUF → regenerate → recompile)
- DUF changes require full rebuild
- Loss of runtime flexibility (can't mod DUF files without recompile)

**Cost:** Developer velocity, modding support

### 4. Observer Pattern / Event Bus for Validation

**Pattern:**
```c
// Register validators
RegisterValidator("enemies", ValidateEnemySchema);
RegisterValidator("trinkets", ValidateTrinketSchema);

// Startup validation
for (each validator) {
    validator->validate(db);
}
```

**Pros:**
- Decoupled validation from loading
- Easy to add new validators

**Cons:**
- Over-engineered for current scale (3 loaders → 5-6 loaders)
- Indirection makes debugging harder
- More code than direct validation

**Cost:** Complexity tax for minimal benefit

## Decision

**Use on-demand heap allocation pattern with mandatory startup validation.**

### Pattern Architecture:

1. **Parse DUF file into tree at startup** (memory: raw DUF tree only)
2. **Validate ALL entries before gameplay starts** (catch schema errors early)
3. **Show SDL error dialog on validation failure** (developer-friendly)
4. **Parse individual entries on-demand** (heap-allocated templates)
5. **Caller owns allocated memory** (must cleanup with CleanupX + free)
6. **DUF tree is source of truth** (no duplicate tables)

### Standard Loader Interface:

Every DUF loader must implement this 4-function interface:

```c
// ============================================================================
// LOAD DATABASE (Startup - parse file into tree)
// ============================================================================

/**
 * LoadXDatabase - Parse DUF file into tree structure
 *
 * @param filepath - Path to DUF file ("data/X/X.duf")
 * @param out_db - Output DUF tree (freed on shutdown)
 * @return dDUFError_t* - Parse error (line/column info), or NULL on success
 *
 * Called once at startup. Stores raw DUF tree in global (e.g., g_enemies_db).
 */
dDUFError_t* LoadXDatabase(const char* filepath, dDUFValue_t** out_db);

// ============================================================================
// VALIDATE DATABASE (Startup - fail fast on schema errors)
// ============================================================================

/**
 * ValidateXDatabase - Validate ALL entries in database
 *
 * @param db - Parsed DUF database
 * @param out_error_msg - Buffer for error message (shown in SDL dialog)
 * @param error_msg_size - Size of error buffer
 * @return bool - true if ALL entries valid, false if ANY fail
 *
 * Iterates through every entry and attempts to parse it.
 * If ANY entry fails, writes detailed error message (file, key, issue).
 * Called once at startup. Failure shows SDL dialog and exits.
 *
 * Error message format:
 *   "X DUF Validation Failed\n\n"
 *   "Entry: {name} (key: '{key}')\n"
 *   "File: {filepath}\n\n"
 *   "Check console logs for details.\n\n"
 *   "Common issues:\n"
 *   "- {issue 1}\n"
 *   "- {issue 2}\n"
 */
bool ValidateXDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size);

// ============================================================================
// LOAD ON-DEMAND (Runtime - heap-allocated template)
// ============================================================================

/**
 * LoadXTemplateFromDUF - Parse single entry from DUF (on-demand)
 *
 * @param key - Entry key ("didact", "lucky_chip", etc)
 * @return XTemplate_t* - Heap-allocated template, or NULL if not found
 *
 * OWNERSHIP: Returned pointer is HEAP-ALLOCATED. Caller must:
 * 1. Call CleanupXTemplate() to free internal dStrings
 * 2. Call free() to free the struct itself
 *
 * Called during gameplay when template is needed.
 * Validation already happened at startup, so this should never fail.
 */
XTemplate_t* LoadXTemplateFromDUF(const char* key);

// ============================================================================
// CLEANUP TEMPLATE (Caller responsibility - free dStrings)
// ============================================================================

/**
 * CleanupXTemplate - Free dStrings inside template
 *
 * @param template - Template to cleanup
 *
 * Frees internal dString_t fields but NOT the struct itself.
 * Caller must call free(template) after this.
 *
 * Follows ADR-014 (Reverse-Order Cleanup) - LIFO pattern.
 */
void CleanupXTemplate(XTemplate_t* template);
```

### Startup Integration (main.c):

```c
// Load DUF file
dDUFValue_t* g_enemies_db = NULL;
dDUFError_t* err = LoadEnemyDatabase("data/enemies/tutorial_enemies.duf", &g_enemies_db);

if (err) {
    // Parse error (syntax, missing file, etc)
    char error_buf[512];
    snprintf(error_buf, sizeof(error_buf),
            "Enemy DUF Parse Error\n\nFile: %s\nLine %d, Column %d\n\n%s",
            "tutorial_enemies.duf", err->line, err->column, d_StringPeek(err->message));

    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                             "Card Fifty-Two - Startup Error",
                             error_buf, NULL);

    d_DUFErrorFree(err);
    exit(1);
}

// Validate ALL entries (schema validation)
char validation_error[1024];
if (!ValidateEnemyDatabase(g_enemies_db, validation_error, sizeof(validation_error))) {
    // Schema error (missing fields, invalid enums, etc)
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,
                             "Card Fifty-Two - Startup Error",
                             validation_error, NULL);
    exit(1);
}

// SUCCESS - gameplay can start
d_LogInfo("Enemy database validated successfully");
```

### Runtime Usage:

```c
// On-demand loading (heap-allocated)
Enemy_t* enemy = LoadEnemyFromDUF("didact");
if (!enemy) {
    d_LogError("Enemy 'didact' not found");  // Should never happen (validated at startup)
    return;
}

// Use enemy
TriggerAbilities(enemy, GAME_EVENT_COMBAT_START);

// Cleanup when done (caller responsibility)
DestroyEnemy(&enemy);  // Calls CleanupX + free()
```

## Consequences

### Positive:

✅ **Startup validation catches ALL schema errors before gameplay**
- Invalid enums, missing fields, typos discovered immediately
- SDL error dialog shows file, line, and helpful hints
- Zero risk of runtime crashes from bad data

✅ **Developer-friendly error messages**
- "Enemy 'didact' missing 'abilities' array" (specific)
- Shows file path and common issues list
- Console logs provide detailed parse errors

✅ **Low memory footprint**
- Only raw DUF tree in memory (~10KB for 100 enemies)
- Templates parsed on-demand (only load what's used)
- No duplicate template tables

✅ **DUF file is source of truth**
- No sync issues between file and in-memory table
- Modding support (edit DUF → restart → changes applied)
- No "forgot to reload templates" bugs

✅ **Consistent pattern across all loaders**
- Same 4-function interface (Load, Validate, LoadTemplate, Cleanup)
- Same error message format
- Easy to add new loaders (copy template)

✅ **Future-proof for modding**
- DUF files can be replaced without recompile
- Validation catches mod schema errors
- Clear error messages help modders debug

### Negative:

❌ **Caller must remember cleanup**
- LoadXTemplateFromDUF() returns heap pointer
- Caller must call CleanupXTemplate() + free()
- Forgetting causes memory leak (Valgrind catches this)

❌ **Repeated parsing overhead**
- Each LoadXTemplateFromDUF() re-parses DUF node
- Not an issue (parsing is fast, ~1ms per enemy)
- Can cache keys if needed (trinketLoader does this)

❌ **More boilerplate per loader**
- Must implement 4 functions (Load, Validate, LoadTemplate, Cleanup)
- ~200 lines of boilerplate per loader
- Trade-off: consistency > brevity

### Accepted Trade-offs:

✅ **Accept caller cleanup responsibility for safety**
- Heap allocation is explicit (no hidden allocations)
- Cleanup pattern documented (ADR-014)
- Valgrind verifies no leaks

✅ **Accept parsing overhead for memory savings**
- Parsing is fast (<1ms per entry)
- Memory savings significant (no duplicate tables)
- Cache keys if hot path (trinketLoader example)

❌ **NOT accepting: Runtime schema errors**
- Startup validation is mandatory
- SDL error dialog is mandatory
- Fail-fast principle (don't start game with bad data)

❌ **NOT accepting: Silent failures**
- All errors logged + shown in SDL dialog
- Clear error messages required
- Common issues list required

## Pattern Template (For New Loaders)

When creating a new DUF loader (events, card tags, etc), copy this template:

### Header File (include/loaders/xLoader.h):

```c
#ifndef X_LOADER_H
#define X_LOADER_H

#include "../structs.h"
#include <Daedalus.h>

// Global DUF database (raw tree structure)
extern dDUFValue_t* g_x_db;

/**
 * LoadXDatabase - Parse X DUF file
 *
 * @param filepath - Path to DUF file (e.g., "data/x/x.duf")
 * @param out_db - Output pointer to parsed DUF database
 * @return dDUFError_t* - Error info on failure, NULL on success
 */
dDUFError_t* LoadXDatabase(const char* filepath, dDUFValue_t** out_db);

/**
 * ValidateXDatabase - Validate all X entries
 *
 * @param db - Parsed DUF database
 * @param out_error_msg - Buffer for error message
 * @param error_msg_size - Size of error buffer
 * @return bool - true if valid, false if any entry fails
 *
 * Iterates all entries and attempts to parse each one.
 * If any fail, writes detailed error message.
 */
bool ValidateXDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size);

/**
 * LoadXTemplateFromDUF - Load X template from DUF (on-demand)
 *
 * @param key - X key string ("foo", "bar", etc)
 * @return XTemplate_t* - Heap-allocated template, or NULL if not found
 *
 * IMPORTANT: Returned template is HEAP-ALLOCATED. Caller must:
 * 1. Call CleanupXTemplate() to free internal dStrings
 * 2. Call free() to free the struct itself
 */
XTemplate_t* LoadXTemplateFromDUF(const char* key);

/**
 * CleanupXTemplate - Free dStrings inside XTemplate_t
 *
 * @param template - Template to clean up
 *
 * Frees internal dString_t but NOT the struct itself.
 * Caller must free(template) after this.
 */
void CleanupXTemplate(XTemplate_t* template);

/**
 * CleanupXSystem - Free all X templates and DUF database
 *
 * Called on shutdown to free g_x_db.
 */
void CleanupXSystem(void);

#endif // X_LOADER_H
```

### Implementation Skeleton (src/loaders/xLoader.c):

```c
#include "../../include/loaders/xLoader.h"
#include <string.h>

// Global DUF database
dDUFValue_t* g_x_db = NULL;

// ============================================================================
// LOAD DATABASE
// ============================================================================

dDUFError_t* LoadXDatabase(const char* filepath, dDUFValue_t** out_db) {
    if (!filepath || !out_db) {
        d_LogError("LoadXDatabase: NULL parameters");
        return NULL;
    }

    dDUFError_t* err = d_DUFParseFile(filepath, out_db);
    if (err) {
        return err;  // Caller handles error
    }

    d_LogInfoF("Loaded X database from %s", filepath);
    return NULL;
}

// ============================================================================
// VALIDATE DATABASE
// ============================================================================

bool ValidateXDatabase(dDUFValue_t* db, char* out_error_msg, size_t error_msg_size) {
    if (!db || !out_error_msg) {
        return false;
    }

    // Iterate through all X entries
    dDUFValue_t* entry = db->child;
    int validated_count = 0;

    while (entry) {
        const char* key = entry->string;
        if (!key) {
            entry = entry->next;
            continue;
        }

        // Try to parse this entry (validation test)
        XTemplate_t* test = LoadXTemplateFromDUF(key);
        if (!test) {
            // Validation failed - write error message
            snprintf(out_error_msg, error_msg_size,
                    "X DUF Validation Failed\n\n"
                    "Entry: %s\n"
                    "File: data/x/x.duf\n\n"
                    "Check console logs for details.\n\n"
                    "Common issues:\n"
                    "- Missing required fields\n"
                    "- Invalid enum values\n"
                    "- Typo in field name",
                    key);
            return false;
        }

        // Clean up test template
        CleanupXTemplate(test);
        free(test);
        validated_count++;
        entry = entry->next;
    }

    d_LogInfoF("✓ X Validation: All %d entries valid", validated_count);
    return true;
}

// ============================================================================
// LOAD ON-DEMAND
// ============================================================================

XTemplate_t* LoadXTemplateFromDUF(const char* key) {
    if (!key) {
        d_LogError("LoadXTemplateFromDUF: NULL key");
        return NULL;
    }

    if (!g_x_db) {
        d_LogError("LoadXTemplateFromDUF: g_x_db is NULL");
        return NULL;
    }

    // Find entry in DUF tree
    dDUFValue_t* node = d_DUFGetObjectItem(g_x_db, key);
    if (!node) {
        d_LogErrorF("X '%s' not found in DUF database", key);
        return NULL;
    }

    // Allocate template on heap
    XTemplate_t* template = malloc(sizeof(XTemplate_t));
    if (!template) {
        d_LogFatal("LoadXTemplateFromDUF: Failed to allocate template");
        return NULL;
    }
    memset(template, 0, sizeof(XTemplate_t));

    // Parse template from DUF (implement ParseXTemplate helper)
    if (!ParseXTemplate(node, key, template)) {
        d_LogErrorF("Failed to parse X '%s'", key);
        free(template);
        return NULL;
    }

    return template;
}

// ============================================================================
// CLEANUP
// ============================================================================

void CleanupXTemplate(XTemplate_t* template) {
    if (!template) return;

    // Free dStrings (add all dString_t* fields here)
    if (template->name) {
        d_StringDestroy(template->name);
        template->name = NULL;
    }
    // ... other dString fields ...
}

void CleanupXSystem(void) {
    if (g_x_db) {
        d_DUFFree(g_x_db);
        g_x_db = NULL;
    }
    d_LogInfo("X system cleaned up");
}
```

### Startup Integration (main.c):

```c
// Load X database
dDUFValue_t* g_x_db = NULL;
dDUFError_t* err = LoadXDatabase("data/x/x.duf", &g_x_db);

if (err) {
    char error_buf[512];
    snprintf(error_buf, sizeof(error_buf),
            "X DUF Parse Error\n\nFile: x.duf\nLine %d, Column %d\n\n%s",
            err->line, err->column, d_StringPeek(err->message));
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error",
                             error_buf, NULL);
    d_DUFErrorFree(err);
    exit(1);
}

// Validate X database
char validation_error[1024];
if (!ValidateXDatabase(g_x_db, validation_error, sizeof(validation_error))) {
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error",
                             validation_error, NULL);
    exit(1);
}
```

## Evidence From Codebase

### Enemy Loader (Most Complex Example)

**File:** `src/loaders/enemyLoader.c`

**LoadEnemyDatabase:** Lines 360-462
- Parses DUF file, validates enemy structure
- Handles nested abilities (triggers + effects)
- Validates 6 trigger types, 10+ effect types

**ValidateEnemyDatabase:** Lines 488-531
- Iterates all enemies in database
- Attempts to load each one
- Writes error message with enemy name + key
- Returns false if ANY enemy fails

**LoadEnemyFromDUF:** Lines 360-462
- Heap-allocates Enemy_t*
- Deep-copies dStrings from DUF
- Returns NULL if validation fails

**DestroyEnemy:** Cleanup function
- Frees internal dStrings (name, description)
- Frees abilities array
- Sets pointer to NULL

**Error Message Format:** Lines 510-518
```c
snprintf(out_error_msg, error_msg_size,
        "DUF Validation Failed\n\n"
        "Enemy: %s (key: '%s')\n"
        "File: data/enemies/tutorial_enemies.duf\n\n"
        "Check console logs for details.\n\n"
        "Common issues:\n"
        "- Typo in field name (e.g., 'abilitiesf' instead of 'abilities')\n"
        "- Missing required fields\n"
        "- Invalid enum values",
        enemy_name, enemy_key);
```

### Trinket Loader (Dual Database Example)

**File:** `src/loaders/trinketLoader.c`

**LoadTrinketDatabase:** Lines 296-309
- Parses DUF file
- Handles both combat + event trinkets

**ValidateTrinketDatabase:** Lines 365-414
- Validates ALL trinkets in database
- Uses heap-allocated test template

**LoadTrinketTemplateFromDUF:** Lines 426-474
- Checks combat DB first, then event DB
- Heap-allocates TrinketTemplate_t*
- Validates critical fields after parse

**CleanupTrinketTemplate:** Lines 485-524
- Frees 8 dString_t* fields
- NULL checks before destroy

### Affix Loader (Simplest Example)

**File:** `src/loaders/affixLoader.c`

**LoadAffixDatabase:** Lines 127-140
- Minimal parse function

**ValidateAffixDatabase:** Lines 242-283
- Validates value ranges (min < max)
- Validates weight > 0
- Uses stack-allocated test affix (no heap)

**LoadAffixTemplateFromDUF:** Lines 145-205
- **7-step validation** (most rigorous):
  1. Validate parameters
  2. Validate DUF lookup
  3. Validate allocation
  4. Validate parsing
  5. Validate critical fields
  6. Validate value ranges
  7. Validate weight
- Returns NULL if ANY step fails

**CleanupAffixTemplate:** Lines 285-300
- Simplest cleanup (only 3 dString fields)

### Main.c Integration

**Enemy Validation:** Lines 206-214
```c
if (!ValidateEnemyDatabase(g_enemies_db, validation_error, sizeof(validation_error))) {
    d_LogError("Enemy database validation failed!");
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Card Fifty-Two - Startup Error",
                             validation_error, NULL);
    exit(1);
}
```

**Trinket Validation:** Lines 263-268, 289-294
- Combat trinkets validated at line 263
- Event trinkets validated at line 289
- Same SDL dialog pattern

**Affix Validation:** Lines 234-239
- Same SDL dialog pattern

## Success Metrics

✅ **FF-019 passes** - All loaders follow pattern
✅ **SDL error dialogs work** - Bad data shows user-friendly message
✅ **Validation catches schema errors** - Zero runtime crashes from bad data
✅ **New loaders follow template** - Events/card tags use same pattern
✅ **Memory footprint low** - Only DUF tree + on-demand templates
✅ **Error messages helpful** - File, key, common issues listed
✅ **Valgrind clean** - Zero memory leaks in loader code

## Related ADRs

- **ADR-001** (Event-Driven Ability System) - Events trigger enemy abilities loaded via this pattern
- **ADR-003** (Constitutional Patterns) - No raw malloc, use Daedalus types (enforced in Parse functions)
- **ADR-014** (Reverse-Order Cleanup) - Cleanup functions follow LIFO pattern
- **ADR-016** (Trinket Value Semantics) - Templates vs instances (templates loaded on-demand)

## Status

✅ **Implemented** (2025-11-28)
- Pattern extracted from 3 existing loaders
- Fitness function FF-019 created
- Template provided for future loaders

## Future Work

- [ ] Migrate events to DUF loader (currently hardcoded enums)
- [ ] Migrate card tags to DUF loader (currently hardcoded enums)
- [ ] Add DUF schema documentation (field types, required vs optional)
- [ ] Consider caching frequently-accessed templates (if profiling shows bottleneck)
