#!/usr/bin/env python3
"""
FF-016: Trinket Value Semantics Verification

Ensures trinkets follow value semantics pattern:
- No malloc() for Trinket_t structs
- Template table stores by VALUE (not pointer)
- Players store embedded/array VALUES (not pointers)
- Deep-copy pattern on equip (memcpy + dString copy)

Based on: ADR-016 (Trinket Template & Instance Pattern)

Architecture Characteristic: Consistency, Safety
Domain Knowledge Required: Trinket system architecture
"""

import re
from pathlib import Path
from typing import List, Dict, Any


def scan_for_trinket_malloc(file_path: Path) -> List[Dict[str, Any]]:
    """
    Check for forbidden malloc() of Trinket_t.

    Pattern: malloc(sizeof(Trinket_t)) or malloc(...Trinket_t...)
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception:
        return violations

    for line_num, line in enumerate(lines, 1):
        # Skip comments
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue

        # Check for malloc of Trinket_t
        if re.search(r'malloc\s*\([^)]*Trinket_t', line):
            violations.append({
                'file': file_path,
                'line': line_num,
                'code': line.strip(),
                'pattern': 'malloc(sizeof(Trinket_t)) - use stack allocation + d_TableSet()'
            })

        # Check for calloc of Trinket_t
        if re.search(r'calloc\s*\([^)]*Trinket_t', line):
            violations.append({
                'file': file_path,
                'line': line_num,
                'code': line.strip(),
                'pattern': 'calloc(..., sizeof(Trinket_t)) - use stack allocation + d_TableSet()'
            })

    return violations


def scan_for_double_pointer_access(file_path: Path) -> List[Dict[str, Any]]:
    """
    Check for incorrect double-pointer access to trinket templates.

    Pattern: Trinket_t** ... = (Trinket_t**)d_TableGet(g_trinket_templates, ...)
    This is WRONG because table stores by VALUE, not pointer.
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception:
        return violations

    for line_num, line in enumerate(lines, 1):
        # Skip comments
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue

        # Check for double-pointer dereference from template table
        if re.search(r'Trinket_t\s*\*\*.*d_TableGet\s*\(\s*g_trinket_templates', line):
            violations.append({
                'file': file_path,
                'line': line_num,
                'code': line.strip(),
                'pattern': 'Trinket_t** from g_trinket_templates - should be Trinket_t* (table stores by value)'
            })

    return violations


def scan_for_pointer_storage_in_player(file_path: Path) -> List[Dict[str, Any]]:
    """
    Check that Player_t uses embedded Trinket_t, not Trinket_t*.

    Pattern (BAD): Trinket_t* class_trinket;
    Pattern (GOOD): Trinket_t class_trinket;
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception:
        return violations

    # Only check structs.h
    if 'structs.h' not in str(file_path):
        return violations

    # Find Player struct definition
    player_struct_match = re.search(r'typedef\s+struct\s+Player\s*\{([^}]+)\}\s*Player_t', content, re.DOTALL)
    if not player_struct_match:
        return violations

    player_body = player_struct_match.group(1)

    # Check for pointer-based class_trinket (BAD)
    if re.search(r'Trinket_t\s*\*\s*class_trinket', player_body):
        violations.append({
            'file': file_path,
            'line': 0,
            'code': 'Trinket_t* class_trinket',
            'pattern': 'class_trinket should be embedded Trinket_t, not Trinket_t* (value semantics)'
        })

    # Check for missing has_class_trinket flag
    if 'has_class_trinket' not in player_body:
        violations.append({
            'file': file_path,
            'line': 0,
            'code': 'Player_t struct',
            'pattern': 'Missing has_class_trinket bool flag (needed for embedded trinket pattern)'
        })

    return violations


def scan_for_array_pointer_storage(file_path: Path) -> List[Dict[str, Any]]:
    """
    Check that trinket_slots array stores Trinket_t values, not pointers.

    Pattern (BAD): d_ArrayInit(6, sizeof(Trinket_t*))
    Pattern (GOOD): d_ArrayInit(6, sizeof(Trinket_t))
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception:
        return violations

    for line_num, line in enumerate(lines, 1):
        # Skip comments
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue

        # Check for trinket_slots initialized with pointer size
        if 'trinket_slots' in line and 'd_ArrayInit' in line:
            if re.search(r'sizeof\s*\(\s*Trinket_t\s*\*\s*\)', line):
                violations.append({
                    'file': file_path,
                    'line': line_num,
                    'code': line.strip(),
                    'pattern': 'trinket_slots uses sizeof(Trinket_t*) - should be sizeof(Trinket_t) (value storage)'
                })

    return violations


def scan_for_deep_copy_pattern(file_path: Path) -> List[Dict[str, Any]]:
    """
    Check that EquipClassTrinket and EquipTrinket use deep-copy pattern.

    Must have:
    1. memcpy of Trinket_t struct
    2. d_StringInit() calls after memcpy (for deep-copying dStrings)
    """
    violations = []

    # Only check trinket.c
    if 'trinket.c' not in str(file_path):
        return violations

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception:
        return violations

    # Find EquipClassTrinket function
    equip_class_match = re.search(r'bool\s+EquipClassTrinket\s*\([^)]+\)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}', content, re.DOTALL)
    if equip_class_match:
        func_body = equip_class_match.group(1)

        # Check for memcpy
        has_memcpy = 'memcpy' in func_body and 'class_trinket' in func_body

        # Check for d_StringInit after memcpy (deep-copy pattern)
        has_string_init = 'd_StringInit()' in func_body

        if has_memcpy and not has_string_init:
            violations.append({
                'file': file_path,
                'line': 0,
                'code': 'EquipClassTrinket()',
                'pattern': 'Missing d_StringInit() after memcpy - must deep-copy dStrings'
            })

    # Find EquipTrinket function
    equip_match = re.search(r'bool\s+EquipTrinket\s*\([^)]+\)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}', content, re.DOTALL)
    if equip_match:
        func_body = equip_match.group(1)

        # Check for memcpy
        has_memcpy = 'memcpy' in func_body

        # Check for d_StringInit after memcpy (deep-copy pattern)
        has_string_init = 'd_StringInit()' in func_body

        if has_memcpy and not has_string_init:
            violations.append({
                'file': file_path,
                'line': 0,
                'code': 'EquipTrinket()',
                'pattern': 'Missing d_StringInit() after memcpy - must deep-copy dStrings'
            })

    return violations


def verify_trinket_value_semantics(project_root: Path) -> bool:
    """
    Main verification function for FF-016.

    Returns: True if no violations found, False otherwise
    """
    print("=" * 80)
    print("FF-016: Trinket Value Semantics Verification")
    print("=" * 80)

    src_dir = project_root / "src"
    include_dir = project_root / "include"

    if not src_dir.exists():
        print(f"❌ FAILURE: Could not find {src_dir}")
        return False

    # Find all C source and header files
    c_files = list(src_dir.rglob("*.c"))
    h_files = list(include_dir.rglob("*.h")) if include_dir.exists() else []
    all_files = c_files + h_files

    print(f"📋 Scanning {len(all_files)} files for trinket value semantics violations")
    print()

    # Collect all violations
    all_violations = []

    for file_path in all_files:
        # Skip backup files
        if '.backup' in str(file_path):
            continue

        all_violations.extend(scan_for_trinket_malloc(file_path))
        all_violations.extend(scan_for_double_pointer_access(file_path))
        all_violations.extend(scan_for_pointer_storage_in_player(file_path))
        all_violations.extend(scan_for_array_pointer_storage(file_path))
        all_violations.extend(scan_for_deep_copy_pattern(file_path))

    # Report results
    if all_violations:
        print("❌ FAILURE: Trinket value semantics violations detected!")
        print()
        print(f"Found {len(all_violations)} violation(s):")
        print()

        for v in all_violations:
            if v['line'] > 0:
                relative_path = v['file'].relative_to(project_root)
                print(f"🚨 {relative_path}:{v['line']}")
            else:
                relative_path = v['file'].relative_to(project_root)
                print(f"🚨 {relative_path}")
            print(f"   Code: {v['code'][:80]}{'...' if len(v['code']) > 80 else ''}")
            print(f"   Issue: {v['pattern']}")
            print()

        print("💡 Fix: Use value semantics - stack allocation, d_TableSet(), deep-copy dStrings")
        print("💡 See: ADR-016 (Trinket Template & Instance Pattern)")
        print()
        return False

    print("✅ SUCCESS: Trinket system uses value semantics correctly")
    print()
    print("Verified:")
    print("  ✓ No malloc() for Trinket_t")
    print("  ✓ Template table stores by VALUE")
    print("  ✓ Player uses embedded class_trinket + has_class_trinket flag")
    print("  ✓ trinket_slots stores Trinket_t values (not pointers)")
    print("  ✓ Equip functions use deep-copy pattern (memcpy + d_StringInit)")
    print()

    return True


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_trinket_value_semantics(project_root)
    exit(0 if success else 1)
