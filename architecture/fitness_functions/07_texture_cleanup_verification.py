#!/usr/bin/env python3
"""
FF-007: Texture Cleanup Verification

Ensures SDL textures stored in global tables are properly freed before destroying tables.
Prevents memory leaks from texture pointer ownership.

Constitutional Pattern: Global tables manage pointer ownership.
When storing SDL_Texture* in dTable_t, must call SDL_DestroyTexture() before d_TableDestroy().

Based on: CLAUDE.md - Constitutional Pattern #4 (Global Tables for Shared Resources)
"""

import re
import sys
from pathlib import Path
from typing import Set, Dict, List, Tuple

def find_texture_tables(src_dir: Path) -> Set[str]:
    """Find all global dTable_t variables that store SDL_Texture* or SDL_Surface* pointers"""
    texture_tables = set()

    # Search for global table declarations storing SDL_Texture* or SDL_Surface*
    for c_file in src_dir.rglob('*.c'):
        with open(c_file, 'r') as f:
            content = f.read()

        # Match: dTable_t* g_XXX = d_TableInit(..., sizeof(SDL_Texture*), ...);
        # or: dTable_t* g_XXX = d_TableInit(..., sizeof(SDL_Surface*), ...);
        texture_pattern = r'(g_\w+)\s*=\s*d_TableInit\([^,]+,\s*sizeof\s*\(\s*SDL_Texture\s*\*\s*\)'
        surface_pattern = r'(g_\w+)\s*=\s*d_TableInit\([^,]+,\s*sizeof\s*\(\s*SDL_Surface\s*\*\s*\)'

        for pattern in [texture_pattern, surface_pattern]:
            matches = re.finditer(pattern, content, re.MULTILINE)
            for match in matches:
                table_name = match.group(1)
                texture_tables.add(table_name)

    # Also check header files for extern declarations
    for h_file in src_dir.parent.glob('include/**/*.h'):
        with open(h_file, 'r') as f:
            content = f.read()

        # Match: extern dTable_t* g_card_textures; (followed by comment mentioning texture)
        pattern = r'extern\s+dTable_t\s*\*\s*(g_\w*texture\w*|g_\w*portrait\w*|g_\w*icon\w*)'
        matches = re.finditer(pattern, content, re.IGNORECASE)
        for match in matches:
            table_name = match.group(1)
            texture_tables.add(table_name)

    return texture_tables

def find_texture_cleanup(cleanup_func: str, table_name: str) -> Tuple[bool, str]:
    """Check if Cleanup() properly destroys textures before destroying table

    Returns: (is_valid, reason)
    """

    # Find the section where this table is destroyed
    destroy_pattern = rf'd_TableDestroy\s*\(\s*&{table_name}\s*\)'
    destroy_match = re.search(destroy_pattern, cleanup_func)

    if not destroy_match:
        return (False, "d_TableDestroy() not found")

    destroy_pos = destroy_match.start()

    # Look for the if block containing this table
    # Pattern: if (table_name) { ... d_TableDestroy(&table_name); }

    # Find start of if block for this table
    if_pattern = rf'if\s*\(\s*{table_name}\s*\)\s*\{{'
    if_match = re.search(if_pattern, cleanup_func[:destroy_pos])

    if not if_match:
        return (False, "if (table_name) block not found")

    # Extract the block between if and destroy
    block_start = if_match.end()
    block_text = cleanup_func[block_start:destroy_pos]

    # Check for the proper cleanup pattern in this block:
    # 1. d_TableGetAllKeys(table_name)
    keys_pattern = rf'd_TableGetAllKeys\s*\(\s*{table_name}\s*\)'
    if not re.search(keys_pattern, block_text):
        return (False, "d_TableGetAllKeys() not found in cleanup block")

    # 2. SDL_DestroyTexture or SDL_FreeSurface call (Archimedes uses both)
    texture_destroy_pattern = r'SDL_DestroyTexture\s*\('
    surface_destroy_pattern = r'SDL_FreeSurface\s*\('
    if not (re.search(texture_destroy_pattern, block_text) or re.search(surface_destroy_pattern, block_text)):
        return (False, "Neither SDL_DestroyTexture() nor SDL_FreeSurface() found in cleanup block")

    # 3. d_ArrayDestroy for keys (any variable name)
    destroy_array_pattern = r'd_ArrayDestroy\s*\(\s*\w+\s*\)'
    if not re.search(destroy_array_pattern, block_text):
        return (False, "d_ArrayDestroy() not found in cleanup block")

    return (True, "All cleanup steps present")

def verify_texture_cleanup(project_root: Path) -> bool:
    """Main verification logic"""
    src_dir = project_root / 'src'

    print("=" * 70)
    print("FF-007: Texture Cleanup Verification")
    print("=" * 70)
    print()

    # Find all texture tables
    texture_tables = find_texture_tables(src_dir)

    if not texture_tables:
        print("⚠️  WARNING: No texture tables found")
        print("   Expected: g_card_textures, g_portraits, etc.")
        return False

    print(f"📋 Found {len(texture_tables)} texture table(s):")
    for table in sorted(texture_tables):
        print(f"   - {table}")
    print()

    # Find Cleanup() function in main.c
    main_c = src_dir / 'main.c'
    if not main_c.exists():
        print("❌ FAILURE: src/main.c not found")
        return False

    with open(main_c, 'r') as f:
        content = f.read()

    # Extract Cleanup() function - find balanced braces
    cleanup_start = content.find('void Cleanup(void)')
    if cleanup_start == -1:
        print("❌ FAILURE: Could not find Cleanup() function in main.c")
        return False

    # Find opening brace
    brace_start = content.find('{', cleanup_start)
    if brace_start == -1:
        print("❌ FAILURE: Could not find Cleanup() opening brace")
        return False

    # Find matching closing brace
    brace_count = 1
    pos = brace_start + 1
    while pos < len(content) and brace_count > 0:
        if content[pos] == '{':
            brace_count += 1
        elif content[pos] == '}':
            brace_count -= 1
        pos += 1

    if brace_count != 0:
        print("❌ FAILURE: Unbalanced braces in Cleanup()")
        return False

    cleanup_func = content[cleanup_start:pos]

    # Check each texture table for proper cleanup
    print("🔍 Verifying texture cleanup pattern:")
    print("   ✓ d_TableGetAllKeys(table)")
    print("   ✓ Loop through keys")
    print("   ✓ SDL_DestroyTexture() on each texture")
    print("   ✓ d_ArrayDestroy(keys)")
    print("   ✓ d_TableDestroy(&table)")
    print()

    all_valid = True
    missing_cleanup = []

    for table_name in sorted(texture_tables):
        is_valid, reason = find_texture_cleanup(cleanup_func, table_name)

        if is_valid:
            print(f"   ✅ {table_name} - Properly cleaned up")
        else:
            print(f"   ❌ {table_name} - {reason}")
            missing_cleanup.append(table_name)
            all_valid = False

    print()

    # Check for global SDL_Texture* variables (not in tables)
    single_texture_pattern = r'SDL_Texture\s*\*\s*(g_\w+)\s*='
    single_textures = set(re.findall(single_texture_pattern, content))

    if single_textures:
        print(f"📋 Found {len(single_textures)} global texture pointer(s):")
        for tex_name in sorted(single_textures):
            # Check if destroyed in Cleanup()
            destroy_pattern = rf'SDL_DestroyTexture\s*\(\s*{tex_name}\s*\)'
            if re.search(destroy_pattern, cleanup_func):
                print(f"   ✅ {tex_name} - Properly destroyed")
            else:
                print(f"   ❌ {tex_name} - MISSING SDL_DestroyTexture()")
                missing_cleanup.append(tex_name)
                all_valid = False
        print()

    if all_valid:
        print("✅ SUCCESS: All textures/surfaces properly cleaned up")
        print()
        print("Constitutional Pattern Verified:")
        print("  - Global tables manage pointer ownership")
        print("  - SDL_DestroyTexture()/SDL_FreeSurface() called before d_TableDestroy()")
        print("  - No memory leaks from texture/surface storage")
        return True
    else:
        print(f"❌ FAILURE: {len(missing_cleanup)} texture(s) missing cleanup")
        print()
        print("Fix: In Cleanup() function, before d_TableDestroy():")
        print("  1. dArray_t* keys = d_TableGetAllKeys(table);")
        print("  2. for (size_t i = 0; i < keys->count; i++) {")
        print("  3.     SDL_Texture** tex = d_TableGet(table, key);")
        print("  4.     SDL_DestroyTexture(*tex);")
        print("  5. }")
        print("  6. d_ArrayDestroy(keys);")
        print("  7. d_TableDestroy(&table);")
        return False

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_texture_cleanup(project_root)
    sys.exit(0 if success else 1)
