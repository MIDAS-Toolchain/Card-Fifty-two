#!/usr/bin/env python3
"""
FF-013: Universal Damage Modifier Verification

Enforces ADR-13 (Universal Damage Modifier Application)

Verifies:
1. All TakeDamage() calls use ApplyPlayerDamageModifiers() first
2. No direct RollForCrit() calls in damage code (old pattern)
3. No manual damage_percent/damage_flat application (duplication)
4. ApplyPlayerDamageModifiers() exists and is properly declared

Based on: architecture/decision_records/13_universal_damage_modifiers.md
"""

import re
from pathlib import Path
from typing import List, Dict, Any, Optional, Tuple


def find_function_callsites(content: str, func_name: str) -> List[Tuple[int, str]]:
    """
    Find all callsites of a function in file content.

    Returns: List of (line_number, line_content) tuples
    """
    lines = content.split('\n')
    callsites = []

    # Pattern: func_name(...) - captures function calls
    pattern = rf'\b{func_name}\s*\('

    # Pattern to detect function definitions: void/int/etc TakeDamage(...)
    definition_pattern = rf'(void|int|bool|static)\s+{func_name}\s*\('

    for line_num, line in enumerate(lines, 1):
        # Skip comment lines
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue

        # Skip function definitions (not callsites)
        if re.search(definition_pattern, line):
            continue

        if re.search(pattern, line):
            callsites.append((line_num, line.strip()))

    return callsites


def get_surrounding_lines(content: str, target_line: int, before: int = 10, after: int = 2) -> str:
    """Get lines surrounding a target line number"""
    lines = content.split('\n')
    start = max(0, target_line - before - 1)
    end = min(len(lines), target_line + after)
    return '\n'.join(lines[start:end])


def verify_apply_damage_modifiers_exists(project_root: Path) -> bool:
    """Verify ApplyPlayerDamageModifiers() function exists"""
    print("📋 Verifying ApplyPlayerDamageModifiers() exists:")
    print()

    player_h = project_root / "include" / "player.h"
    player_c = project_root / "src" / "player.c"

    if not player_h.exists():
        print("❌ FATAL: include/player.h not found")
        return False

    if not player_c.exists():
        print("❌ FATAL: src/player.c not found")
        return False

    # Check header declaration
    header_content = player_h.read_text()
    declaration_pattern = r'int\s+ApplyPlayerDamageModifiers\s*\([^)]*Player_t\s*\*[^)]*int\s+base_damage[^)]*bool\s*\*[^)]*\)'

    if not re.search(declaration_pattern, header_content):
        print("❌ FAIL: ApplyPlayerDamageModifiers() not declared in player.h")
        print("   Expected: int ApplyPlayerDamageModifiers(Player_t* player, int base_damage, bool* out_is_crit)")
        print()
        return False

    print("✅ ApplyPlayerDamageModifiers() declared in player.h")

    # Check implementation
    impl_content = player_c.read_text()
    impl_pattern = r'int\s+ApplyPlayerDamageModifiers\s*\([^)]*\)\s*\{'

    if not re.search(impl_pattern, impl_content):
        print("❌ FAIL: ApplyPlayerDamageModifiers() implementation not found in player.c")
        print()
        return False

    print("✅ ApplyPlayerDamageModifiers() implemented in player.c")
    print()

    return True


def verify_take_damage_uses_modifiers(project_root: Path) -> bool:
    """
    Verify all TakeDamage() calls are preceded by ApplyPlayerDamageModifiers().

    Returns: True if all callsites use modifiers, False otherwise
    """
    print("📋 Verifying TakeDamage() uses ApplyPlayerDamageModifiers():")
    print()

    src_dir = project_root / "src"
    if not src_dir.exists():
        print(f"❌ FATAL: {src_dir} not found")
        return False

    # Find all C files
    c_files = list(src_dir.rglob("*.c"))

    violations = []
    total_take_damage_calls = 0

    for file_path in c_files:
        content = file_path.read_text()

        # Find all TakeDamage() callsites
        take_damage_callsites = find_function_callsites(content, 'TakeDamage')

        if not take_damage_callsites:
            continue

        total_take_damage_calls += len(take_damage_callsites)

        for line_num, line_content in take_damage_callsites:
            # Get surrounding lines (10 before, 2 after)
            context = get_surrounding_lines(content, line_num, before=10, after=2)

            # Check if ApplyPlayerDamageModifiers appears in context
            if 'ApplyPlayerDamageModifiers' not in context:
                violations.append({
                    'file': file_path,
                    'line': line_num,
                    'code': line_content,
                    'context': context
                })

    print(f"📊 Found {total_take_damage_calls} TakeDamage() call(s) across {len(c_files)} file(s)")
    print()

    if violations:
        print(f"❌ FAIL: {len(violations)} TakeDamage() call(s) without ApplyPlayerDamageModifiers()")
        print()

        for v in violations:
            relative_path = v['file'].relative_to(project_root)
            print(f"🚨 {relative_path}:{v['line']}")
            print(f"   Code: {v['code'][:80]}")
            print(f"   Issue: TakeDamage() called without ApplyPlayerDamageModifiers()")
            print()
            print("   Context (showing 10 lines before):")
            for ctx_line in v['context'].split('\n')[-12:]:
                print(f"      {ctx_line}")
            print()

        print("💡 Fix: Call ApplyPlayerDamageModifiers() before TakeDamage():")
        print("   int damage = ApplyPlayerDamageModifiers(player, base_damage, &is_crit);")
        print("   TakeDamage(enemy, damage);")
        print()
        return False

    print(f"✅ All {total_take_damage_calls} TakeDamage() call(s) use ApplyPlayerDamageModifiers()")
    print()

    return True


def verify_no_direct_roll_for_crit(project_root: Path) -> bool:
    """
    Verify no damage code calls RollForCrit() directly (should use ApplyPlayerDamageModifiers).

    Exception: player.c is allowed (internal implementation).
    """
    print("📋 Verifying no direct RollForCrit() calls in damage code:")
    print()

    src_dir = project_root / "src"
    if not src_dir.exists():
        return True

    # Find all C files except player.c
    c_files = [f for f in src_dir.rglob("*.c") if f.name != 'player.c']

    violations = []

    for file_path in c_files:
        content = file_path.read_text()

        # Find RollForCrit() callsites
        roll_crit_callsites = find_function_callsites(content, 'RollForCrit')

        for line_num, line_content in roll_crit_callsites:
            # Get surrounding context
            context = get_surrounding_lines(content, line_num, before=5, after=5)

            # If TakeDamage appears nearby, this is likely damage code
            if 'TakeDamage' in context:
                violations.append({
                    'file': file_path,
                    'line': line_num,
                    'code': line_content
                })

    if violations:
        print(f"❌ FAIL: {len(violations)} direct RollForCrit() call(s) in damage code")
        print()
        print("   Damage code should use ApplyPlayerDamageModifiers(), not RollForCrit()")
        print()

        for v in violations:
            relative_path = v['file'].relative_to(project_root)
            print(f"🚨 {relative_path}:{v['line']}")
            print(f"   Code: {v['code'][:80]}")
            print()

        print("💡 Fix: Replace RollForCrit() with ApplyPlayerDamageModifiers():")
        print("   OLD: bool is_crit = RollForCrit(player, damage, &damage);")
        print("   NEW: int damage = ApplyPlayerDamageModifiers(player, base_damage, &is_crit);")
        print()
        return False

    print("✅ No direct RollForCrit() calls in damage code (uses universal modifier)")
    print()

    return True


def verify_no_manual_modifier_application(project_root: Path) -> bool:
    """
    Verify no code manually applies damage_percent or damage_flat (should use ApplyPlayerDamageModifiers).

    Exception: player.c is allowed (contains the implementation).
    """
    print("📋 Verifying no manual damage modifier application:")
    print()

    src_dir = project_root / "src"
    if not src_dir.exists():
        return True

    # Find all C files except player.c
    c_files = [f for f in src_dir.rglob("*.c") if f.name != 'player.c']

    violations = []

    # Pattern that indicates ARITHMETIC on damage_percent/damage_flat
    # This catches: damage += (damage * player->damage_percent) / 100
    # But NOT: d_StringFormat(..., player->damage_percent) or similar display code
    arithmetic_pattern = r'damage\s*[+\-*/]=?\s*.*\b(damage_percent|damage_flat)\b'

    for file_path in c_files:
        content = file_path.read_text()
        lines = content.split('\n')

        for line_num, line in enumerate(lines, 1):
            # Skip comment lines
            stripped = line.strip()
            if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
                continue

            # Check for arithmetic operations with damage modifiers
            match = re.search(arithmetic_pattern, line)
            if match:
                modifier_name = match.group(1)
                violations.append({
                    'file': file_path,
                    'line': line_num,
                    'code': line.strip(),
                    'modifier': modifier_name
                })

    if violations:
        print(f"❌ FAIL: {len(violations)} manual modifier application(s) detected")
        print()
        print("   Modifiers should only be applied in ApplyPlayerDamageModifiers()")
        print()

        for v in violations:
            relative_path = v['file'].relative_to(project_root)
            print(f"🚨 {relative_path}:{v['line']}")
            print(f"   Code: {v['code'][:80]}")
            print(f"   Issue: Manual {v['modifier']} application (duplicates modifier logic)")
            print()

        print("💡 Fix: Remove manual modifier code, use ApplyPlayerDamageModifiers():")
        print("   DELETE: damage += (damage * player->damage_percent) / 100;")
        print("   USE: int damage = ApplyPlayerDamageModifiers(player, base_damage, &is_crit);")
        print()
        return False

    print("✅ No manual modifier application (all use ApplyPlayerDamageModifiers())")
    print()

    return True


def verify_universal_damage_modifiers(project_root: Path) -> bool:
    """Main verification function"""

    print("=" * 80)
    print("FF-013: Universal Damage Modifier Verification")
    print("=" * 80)
    print()

    # Check 1: ApplyPlayerDamageModifiers exists
    check1 = verify_apply_damage_modifiers_exists(project_root)
    if not check1:
        return False

    # Check 2: All TakeDamage() calls use modifiers
    check2 = verify_take_damage_uses_modifiers(project_root)
    if not check2:
        return False

    # Check 3: No direct RollForCrit() in damage code
    check3 = verify_no_direct_roll_for_crit(project_root)
    if not check3:
        return False

    # Check 4: No manual modifier application
    check4 = verify_no_manual_modifier_application(project_root)
    if not check4:
        return False

    print("=" * 80)
    print("✅ SUCCESS: Universal damage modifier pattern verified")
    print("=" * 80)
    print()
    print("Constitutional Pattern Verified:")
    print("  - ApplyPlayerDamageModifiers() exists and is declared")
    print("  - All TakeDamage() calls use ApplyPlayerDamageModifiers()")
    print("  - No direct RollForCrit() calls in damage code")
    print("  - No manual damage_percent/damage_flat application")
    print()
    print("All damage sources (turns, tags, trinkets, abilities) use consistent modifiers!")
    print()

    return True


if __name__ == '__main__':
    import sys

    # Determine project root (3 levels up from this script)
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent.parent

    success = verify_universal_damage_modifiers(project_root)
    sys.exit(0 if success else 1)
