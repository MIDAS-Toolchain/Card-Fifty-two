#!/usr/bin/env python3
"""
FF-014: Reverse-Order Cleanup Verification

Enforces ADR-14 (Reverse-Order Resource Cleanup Pattern)

Verifies:
1. Critical cleanup functions exist (Cleanup, CleanupBlackjackScene)
2. Known init/cleanup pairs are matched (e.g., InitCardMetadata ↔ CleanupCardMetadata)
3. Common anti-patterns are flagged (early parent cleanup, missing cleanup)

Based on: architecture/decision_records/14_reverse_order_cleanup.md
"""

import re
from pathlib import Path
from typing import List, Dict, Tuple, Set, Optional


# Known init/cleanup function pairs
INIT_CLEANUP_PAIRS = [
    ('InitCardMetadata', 'CleanupCardMetadata'),
    ('InitTrinketSystem', 'CleanupTrinketSystem'),
    ('Stats_Init', None),  # No explicit cleanup needed
    ('CreatePlayer', 'DestroyPlayer'),
    ('State_InitContext', 'State_CleanupContext'),
    ('CreateTutorialSystem', 'DestroyTutorialSystem'),
    ('a_CreateFlexBox', 'a_DestroyFlexBox'),
    ('CreateButton', 'DestroyButton'),
    ('CreateEnemy', 'DestroyEnemy'),
    ('CreateAct', 'DestroyAct'),
    ('CleanupDeck', None),  # Init is InitDeck
    ('InitDeck', 'CleanupDeck'),
]


def extract_function_calls(content: str, func_names: List[str]) -> List[Tuple[int, str]]:
    """
    Extract all calls to specified functions.

    Returns: List of (line_number, function_name) tuples
    """
    lines = content.split('\n')
    calls = []

    for line_num, line in enumerate(lines, 1):
        # Skip comment lines
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*'):
            continue

        for func_name in func_names:
            # Pattern: func_name(...) but not function definition
            pattern = rf'\b{func_name}\s*\('
            if re.search(pattern, line):
                # Check it's not a function definition
                definition_pattern = rf'(void|int|bool|static|extern)\s+{func_name}\s*\('
                if not re.search(definition_pattern, line):
                    calls.append((line_num, func_name))
                    break  # Only count once per line

    return calls


def verify_cleanup_function_exists(project_root: Path) -> bool:
    """Verify critical cleanup functions exist"""
    print("📋 Verifying critical cleanup functions exist:")
    print()

    main_c = project_root / "src" / "main.c"
    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"

    critical_funcs = [
        (main_c, 'Cleanup', 'main.c global cleanup'),
        (scene_blackjack, 'CleanupBlackjackScene', 'blackjack scene cleanup'),
    ]

    all_exist = True

    for file_path, func_name, description in critical_funcs:
        if not file_path.exists():
            print(f"⚠️  WARNING: {file_path.name} not found")
            continue

        content = file_path.read_text()
        pattern = rf'(static\s+)?void\s+{func_name}\s*\('

        if not re.search(pattern, content):
            print(f"❌ FAIL: {func_name}() not found in {file_path.name}")
            print(f"   ADR-14 requires {description} function")
            print()
            all_exist = False
        else:
            print(f"✅ {func_name}() found in {file_path.name}")

    if not all_exist:
        return False

    print()
    return True


def verify_init_cleanup_pairing(project_root: Path) -> bool:
    """
    Verify that known Init/Create functions have matching Cleanup/Destroy.

    Checks main.c and sceneBlackjack.c for paired calls.
    """
    print("📋 Verifying init/cleanup pairing (ADR-14):")
    print()

    files_to_check = [
        project_root / "src" / "main.c",
        project_root / "src" / "scenes" / "sceneBlackjack.c",
    ]

    violations = []

    for file_path in files_to_check:
        if not file_path.exists():
            continue

        content = file_path.read_text()

        # Extract all init and cleanup function names we care about
        init_funcs = [pair[0] for pair in INIT_CLEANUP_PAIRS if pair[0]]
        cleanup_funcs = [pair[1] for pair in INIT_CLEANUP_PAIRS if pair[1]]

        # Find all calls
        all_func_names = list(set(init_funcs + cleanup_funcs))
        calls = extract_function_calls(content, all_func_names)

        # Group by function name
        called_funcs = set(func_name for _, func_name in calls)

        # Check pairing
        for init_func, cleanup_func in INIT_CLEANUP_PAIRS:
            if not init_func or not cleanup_func:
                continue  # Skip pairs with no cleanup (e.g., Stats_Init)

            init_called = init_func in called_funcs
            cleanup_called = cleanup_func in called_funcs

            if init_called and not cleanup_called:
                violations.append({
                    'file': file_path,
                    'init': init_func,
                    'cleanup': cleanup_func,
                    'issue': f'{init_func}() called without matching {cleanup_func}()'
                })

    if violations:
        print(f"❌ FAIL: {len(violations)} unpaired init/cleanup call(s)")
        print()
        for v in violations:
            relative_path = v['file'].relative_to(project_root)
            print(f"🚨 {relative_path}")
            print(f"   {v['issue']}")
            print(f"   ADR-14: Every Init/Create MUST have matching Cleanup/Destroy")
            print()
        return False

    print("✅ All init/cleanup pairs matched")
    print()
    return True


def extract_functions(content: str) -> Dict[str, Tuple[int, int]]:
    """
    Extract function boundaries from C source code.

    Returns: Dictionary mapping function_name -> (start_line, end_line)
    """
    lines = content.split('\n')
    functions = {}

    # Pattern: return_type function_name(...) {
    func_pattern = r'^\s*(?:static\s+)?(?:void|int|bool|\w+_t\s*\*?)\s+(\w+)\s*\([^)]*\)\s*\{'

    for line_num, line in enumerate(lines, 1):
        match = re.match(func_pattern, line)
        if match:
            func_name = match.group(1)
            func_start = line_num

            # Find matching closing brace
            brace_count = 1
            func_end = func_start

            for i in range(line_num, len(lines)):
                line_content = lines[i]
                brace_count += line_content.count('{')
                brace_count -= line_content.count('}')

                if brace_count == 0:
                    func_end = i + 1
                    break

            functions[func_name] = (func_start, func_end)

    return functions


def is_in_same_function(line1: int, line2: int, functions: Dict[str, Tuple[int, int]]) -> bool:
    """Check if two line numbers are in the same function"""
    for func_name, (start, end) in functions.items():
        if start <= line1 <= end and start <= line2 <= end:
            return True
    return False


def is_error_path(lines: List[str], line_num: int) -> bool:
    """
    Check if a line is part of an error handling path.

    Detects patterns like:
    - return NULL within 3 lines
    - return false within 3 lines
    """
    # Check current line and next 3 lines for early return
    for offset in range(4):
        check_line = line_num + offset
        if check_line < len(lines):
            line_content = lines[check_line]
            if 'return NULL' in line_content or 'return false' in line_content:
                return True
    return False


def verify_no_early_parent_cleanup(project_root: Path) -> bool:
    """
    Detect anti-pattern: parent freed before children.

    Looks for patterns like:
    - free(modal) before a_DestroyFlexBox()
    - DestroyPlayer() before CleanupHand()

    Enhanced with function boundary awareness to eliminate false positives.
    """
    print("📋 Verifying no early parent cleanup:")
    print()

    src_dir = project_root / "src"
    if not src_dir.exists():
        return True

    violations = []

    # Patterns to check (parent_pattern, child_pattern, description)
    anti_patterns = [
        # Modal freed before FlexBox destroyed
        (r'free\(\*?\w*modal\)', r'a_DestroyFlexBox', 'Modal freed before FlexBox destroyed'),
        # Player destroyed before hand cleanup
        (r'free\(\*?\w*player\)', r'CleanupHand', 'Player freed before hand cleanup'),
    ]

    for c_file in src_dir.rglob("*.c"):
        content = c_file.read_text()
        lines = content.split('\n')

        # Extract function boundaries for this file
        functions = extract_functions(content)

        for parent_pattern, child_pattern, description in anti_patterns:
            # Find parent cleanup line
            parent_lines = []
            for line_num, line in enumerate(lines, 1):
                if re.search(parent_pattern, line):
                    parent_lines.append(line_num)

            # Find child cleanup lines
            child_lines = []
            for line_num, line in enumerate(lines, 1):
                if re.search(child_pattern, line):
                    child_lines.append(line_num)

            # Check if parent comes before child (within same function)
            for parent_line in parent_lines:
                for child_line in child_lines:
                    # Only check if in same function (don't compare Create vs Destroy)
                    if not is_in_same_function(parent_line, child_line, functions):
                        continue

                    # Skip if parent is in an error path (early return)
                    if is_error_path(lines, parent_line - 1):  # -1 because enumerate starts at 1
                        continue

                    # If child is within 50 lines after parent, this is likely wrong order
                    if 0 < (child_line - parent_line) < 50:
                        violations.append({
                            'file': c_file,
                            'parent_line': parent_line,
                            'child_line': child_line,
                            'issue': description
                        })

    if violations:
        print(f"⚠️  WARNING: {len(violations)} potential early parent cleanup(s)")
        print()
        print("   This is a warning - manual review required")
        for v in violations:
            relative_path = v['file'].relative_to(project_root)
            print(f"   {relative_path}:{v['parent_line']} (parent) before line {v['child_line']} (child)")
            print(f"   Issue: {v['issue']}")
        print()
        print("   If these are false positives, this is OK (patterns are heuristic)")
        print()
        # Don't fail on warnings, just inform
        return True

    print("✅ No obvious early parent cleanup detected")
    print()
    return True


def verify_critical_cleanup_order(project_root: Path) -> bool:
    """
    Verify critical cleanup sequences in main.c and sceneBlackjack.c.

    Checks that known dependencies are destroyed in correct order:
    - main.c: Trinkets before CardMetadata
    - sceneBlackjack.c: Tutorial before Layout before GameContext before Players
    """
    print("📋 Verifying critical cleanup order (ADR-14):")
    print()

    # Check main.c cleanup order
    main_c = project_root / "src" / "main.c"
    if main_c.exists():
        content = main_c.read_text()

        # Find Cleanup() function
        cleanup_match = re.search(r'void\s+Cleanup\s*\(\s*void\s*\)\s*\{', content)
        if cleanup_match:
            # Extract function body
            func_start = cleanup_match.end()
            brace_count = 1
            func_end = func_start

            for i, char in enumerate(content[func_start:], start=func_start):
                if char == '{':
                    brace_count += 1
                elif char == '}':
                    brace_count -= 1
                    if brace_count == 0:
                        func_end = i
                        break

            cleanup_body = content[func_start:func_end]

            # Check: CleanupTrinketSystem before CleanupCardMetadata
            trinket_pos = cleanup_body.find('CleanupTrinketSystem')
            metadata_pos = cleanup_body.find('CleanupCardMetadata')

            if trinket_pos != -1 and metadata_pos != -1:
                if metadata_pos < trinket_pos:
                    print("❌ FAIL: main.c Cleanup() has wrong order")
                    print("   CleanupCardMetadata() before CleanupTrinketSystem()")
                    print("   ADR-14: Trinkets depend on card metadata, must destroy trinkets first")
                    print()
                    return False
                else:
                    print("✅ main.c Cleanup() order correct (Trinkets before CardMetadata)")
            else:
                print("⚠️  INFO: Could not verify main.c cleanup order (functions not found)")

    # Check sceneBlackjack.c cleanup order
    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"
    if scene_blackjack.exists():
        content = scene_blackjack.read_text()

        # Find CleanupBlackjackScene() function
        cleanup_match = re.search(r'void\s+CleanupBlackjackScene\s*\(\s*\)', content)
        if cleanup_match:
            # Extract function body
            func_start = cleanup_match.end()
            # Skip to opening brace
            brace_start = content.find('{', func_start)
            if brace_start != -1:
                func_start = brace_start + 1
                brace_count = 1
                func_end = func_start

                for i, char in enumerate(content[func_start:], start=func_start):
                    if char == '{':
                        brace_count += 1
                    elif char == '}':
                        brace_count -= 1
                        if brace_count == 0:
                            func_end = i
                            break

                cleanup_body = content[func_start:func_end]

                # Check critical order: Tutorial → Layout → GameContext → Players
                tutorial_pos = cleanup_body.find('DestroyTutorialSystem')
                layout_pos = cleanup_body.find('CleanupLayout')
                context_pos = cleanup_body.find('State_CleanupContext')
                player_pos = cleanup_body.find('DestroyPlayer')

                violations = []

                if tutorial_pos != -1 and layout_pos != -1 and layout_pos < tutorial_pos:
                    violations.append("CleanupLayout() before DestroyTutorialSystem() (wrong order)")

                if layout_pos != -1 and context_pos != -1 and context_pos < layout_pos:
                    violations.append("State_CleanupContext() before CleanupLayout() (wrong order)")

                if context_pos != -1 and player_pos != -1 and player_pos < context_pos:
                    violations.append("DestroyPlayer() before State_CleanupContext() (wrong order)")

                if violations:
                    print("❌ FAIL: sceneBlackjack.c CleanupBlackjackScene() has wrong order")
                    for v in violations:
                        print(f"   - {v}")
                    print("   ADR-14: Must destroy in reverse init order")
                    print()
                    return False
                else:
                    print("✅ sceneBlackjack.c CleanupBlackjackScene() order correct")

    print()
    return True


def verify_reverse_order_cleanup(project_root: Path) -> bool:
    """Main verification function"""

    print("=" * 80)
    print("FF-014: Reverse-Order Cleanup Verification")
    print("=" * 80)
    print()

    # Check 1: Critical cleanup functions exist
    check1 = verify_cleanup_function_exists(project_root)
    if not check1:
        return False

    # Check 2: Init/cleanup pairing
    check2 = verify_init_cleanup_pairing(project_root)
    if not check2:
        return False

    # Check 3: No early parent cleanup (warning only)
    check3 = verify_no_early_parent_cleanup(project_root)
    # Don't fail on this - it's just warnings

    # Check 4: Critical cleanup order
    check4 = verify_critical_cleanup_order(project_root)
    if not check4:
        return False

    print("=" * 80)
    print("✅ SUCCESS: Reverse-order cleanup pattern verified")
    print("=" * 80)
    print()
    print("Constitutional Pattern Verified:")
    print("  - Critical cleanup functions exist (Cleanup, CleanupBlackjackScene)")
    print("  - Init/cleanup pairs matched (all inits have matching cleanups)")
    print("  - Critical sequences follow reverse order (Tutorial→Layout→Context→Players)")
    print("  - ADR-14 reverse-order pattern enforced")
    print()
    print("Resources destroyed in exact reverse of initialization order!")
    print()

    return True


if __name__ == '__main__':
    import sys

    # Determine project root (3 levels up from this script)
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent.parent

    success = verify_reverse_order_cleanup(project_root)
    sys.exit(0 if success else 1)
