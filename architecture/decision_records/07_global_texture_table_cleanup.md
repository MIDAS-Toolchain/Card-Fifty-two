# ADR-007: Global Texture Table Cleanup Pattern

## Context

SDL textures are opaque GPU resources managed via pointers (`SDL_Texture*`). When stored in global `dTable_t*` containers (e.g., `g_card_textures`, `g_portraits`), the table owns the **pointers** but not the underlying GPU memory. Must explicitly free textures before destroying the table.

**Problem:** Daedalus tables store data by **value** (memcpy), not by reference. When storing `SDL_Texture*`, the table holds **copies of pointers**, not the actual textures. Simply calling `d_TableDestroy()` frees the table's internal memory but **leaks GPU textures**.

**Alternatives Considered:**

1. **Manual cleanup before d_TableDestroy()** (iterate keys → SDL_DestroyTexture → destroy table)
   - Pros: Explicit ownership model, grep-able pattern, no hidden magic
   - Cons: Verbose, easy to forget, must repeat for every texture table
   - Cost: ~15 lines per table, developer must remember cleanup order

2. **Custom table destructor callback** (register cleanup function in d_TableInit)
   - Pros: Automatic cleanup, DRY (Don't Repeat Yourself)
   - Cons: Requires Daedalus API change, hides ownership model, callback Hell
   - Cost: Library modification, non-obvious destructor registration

3. **Wrapper struct with destructor** (TexHandle_t with cleanup function)
   - Pros: RAII-like pattern in C, encapsulates ownership
   - Cons: Extra indirection, more complex type system, still needs manual trigger
   - Cost: New type for every resource type (TexHandle_t, AudioHandle_t, etc.)

4. **Don't use tables for textures** (global SDL_Texture* array indexed by card_id)
   - Pros: Simpler cleanup (just loop and destroy)
   - Cons: Loses hash table performance, abandons constitutional pattern (Daedalus everywhere)
   - Cost: Inconsistent with rest of codebase, forces linear search for lookups

## Decision

**Enforce manual texture cleanup before table destruction as constitutional pattern.**

All global texture tables (`g_card_textures`, `g_portraits`, `g_ability_icons`) MUST follow this pattern in `Cleanup()`:

```c
if (g_texture_table) {
    // 1. Get all keys from table
    dArray_t* keys = d_TableGetAllKeys(g_texture_table);

    if (keys) {
        // 2. Iterate through keys and destroy each texture
        for (size_t i = 0; i < keys->count; i++) {
            int* key = (int*)d_ArrayGet(keys, i);
            if (key) {
                // 3. Get texture pointer from table (returns SDL_Texture**)
                SDL_Texture** tex_ptr = (SDL_Texture**)d_TableGet(g_texture_table, key);
                if (tex_ptr && *tex_ptr) {
                    SDL_DestroyTexture(*tex_ptr);  // Free GPU resource
                }
            }
        }
        // 4. Destroy keys array
        d_ArrayDestroy(keys);
    }

    // 5. Destroy table (now safe - no dangling textures)
    d_TableDestroy(&g_texture_table);
}
```

**Justification:**

1. **Explicit Ownership**: Code clearly shows table owns pointers, textures need separate cleanup
2. **Grep-able Pattern**: `d_TableGetAllKeys` + `SDL_DestroyTexture` is searchable
3. **Constitutional Consistency**: Uses Daedalus for iteration (d_TableGetAllKeys returns dArray_t*)
4. **No Library Changes**: Works with current Daedalus API
5. **Fitness Function Enforcement**: FF-007 verifies this pattern exists for all texture tables

## Consequences

**Positive:**
- ✅ Zero GPU memory leaks (textures explicitly freed)
- ✅ Ownership model is obvious (manual cleanup = manual ownership)
- ✅ Works with existing Daedalus API (no library modifications)
- ✅ Automated verification (FF-007 checks pattern exists)
- ✅ Consistent with constitutional pattern (uses d_TableGetAllKeys, not raw iteration)

**Negative:**
- ❌ Verbose (15 lines per texture table)
- ❌ Easy to forget (must manually add for new texture tables)
- ❌ Copy-paste risk (developers might skip steps)

**Accepted Trade-offs:**
- ✅ Accept verbosity for explicit ownership clarity
- ✅ Accept manual pattern for grep-able violations
- ✅ Accept repetition for architectural consistency
- ❌ NOT accepting: Hidden cleanup (destroys debug-ability)
- ❌ NOT accepting: Raw pointer iteration (violates constitutional pattern)

## Evidence From Codebase

**main.c (Lines 193-209) - g_card_textures cleanup:**
```c
// Free all card textures before destroying table
if (g_card_textures) {
    dArray_t* card_ids = d_TableGetAllKeys(g_card_textures);
    if (card_ids) {
        for (size_t i = 0; i < card_ids->count; i++) {
            int* card_id = (int*)d_ArrayGet(card_ids, i);
            if (card_id) {
                SDL_Texture** tex_ptr = (SDL_Texture**)d_TableGet(g_card_textures, card_id);
                if (tex_ptr && *tex_ptr) {
                    SDL_DestroyTexture(*tex_ptr);  // Free GPU resource
                }
            }
        }
        d_ArrayDestroy(card_ids);
    }
    d_TableDestroy(&g_card_textures);
    d_LogInfo("Texture cache destroyed");
}
```

**main.c (Lines 212-228) - g_portraits cleanup:**
```c
// Free all portrait textures before destroying table
if (g_portraits) {
    dArray_t* player_ids = d_TableGetAllKeys(g_portraits);
    if (player_ids) {
        for (size_t i = 0; i < player_ids->count; i++) {
            int* player_id = (int*)d_ArrayGet(player_ids, i);
            if (player_id) {
                SDL_Texture** tex_ptr = (SDL_Texture**)d_TableGet(g_portraits, player_id);
                if (tex_ptr && *tex_ptr) {
                    SDL_DestroyTexture(*tex_ptr);  // Free GPU resource
                }
            }
        }
        d_ArrayDestroy(player_ids);
    }
    d_TableDestroy(&g_portraits);
    d_LogInfo("Portrait cache destroyed");
}
```

**main.c (Lines 231-247) - g_ability_icons cleanup:**
```c
// Free all ability icon textures before destroying table
if (g_ability_icons) {
    dArray_t* keys = d_TableGetAllKeys(g_ability_icons);
    if (keys) {
        for (size_t i = 0; i < keys->count; i++) {
            int* ability_id = (int*)d_ArrayGet(keys, i);
            if (ability_id) {
                SDL_Texture** tex_ptr = (SDL_Texture**)d_TableGet(g_ability_icons, ability_id);
                if (tex_ptr && *tex_ptr) {
                    SDL_DestroyTexture(*tex_ptr);  // Free GPU resource
                }
            }
        }
        d_ArrayDestroy(keys);
    }
    d_TableDestroy(&g_ability_icons);
    d_LogInfo("Ability icon cache destroyed");
}
```

**FF-007 Verification (Lines 75-94):**
```python
# Check for the proper cleanup pattern in this block:
# 1. d_TableGetAllKeys(table_name)
keys_pattern = rf'd_TableGetAllKeys\s*\(\s*{table_name}\s*\)'
if not re.search(keys_pattern, block_text):
    return (False, "d_TableGetAllKeys() not found in cleanup block")

# 2. SDL_DestroyTexture call
sdl_destroy_pattern = r'SDL_DestroyTexture\s*\('
if not re.search(sdl_destroy_pattern, block_text):
    return (False, "SDL_DestroyTexture() not found in cleanup block")

# 3. d_ArrayDestroy for keys (any variable name)
destroy_array_pattern = r'd_ArrayDestroy\s*\(\s*\w+\s*\)'
if not re.search(destroy_array_pattern, block_text):
    return (False, "d_ArrayDestroy() not found in cleanup block")
```

## Pattern Details

### Why SDL_Texture** (Double Pointer)?

`d_TableGet()` returns `void*` pointing to the **stored data inside the table**. Since the table stores `SDL_Texture*` (pointer to texture), the return value is a **pointer to that pointer** → `SDL_Texture**`.

```c
// Table stores: sizeof(SDL_Texture*) = 8 bytes (on 64-bit)
d_TableInit(sizeof(int), sizeof(SDL_Texture*), ...);

// Get returns pointer to the stored pointer
SDL_Texture** tex_ptr = (SDL_Texture**)d_TableGet(table, &key);

// Dereference once to get actual texture pointer
SDL_Texture* texture = *tex_ptr;
```

### Why Not Store SDL_Texture Directly?

SDL textures are **opaque handles** (SDL owns the struct). You can't memcpy the texture itself, only the pointer. Storing `sizeof(SDL_Texture)` would require knowing SDL's internal struct size (which is not public API).

### Cleanup Order Matters

1. **MUST** call `SDL_DestroyTexture()` before `d_TableDestroy()`
2. **MUST** destroy keys array (`d_ArrayDestroy(keys)`) before destroying table
3. **MUST** check NULL before dereferencing (`if (tex_ptr && *tex_ptr)`)

Violating this order causes:
- GPU memory leaks (textures not freed)
- Use-after-free (destroying table before reading keys)
- Segfaults (dereferencing NULL pointers)

## Success Metrics

- **FF-007 passes**: All texture tables have proper cleanup pattern in `Cleanup()`
- **Valgrind shows zero GPU leaks**: SDL reports no unreleased textures on exit
- **New texture tables follow pattern**: Code review rejects PRs missing cleanup
- **Documentation references this ADR**: Comments in code cite ADR-007 for justification

## Related Decisions

- **ADR-003**: Daedalus types constitutional pattern (why we use dTable_t)
- **FF-007**: Automated verification of this cleanup pattern
- **CLAUDE.md**: Constitutional Pattern #4 (Global Tables for Shared Resources)
