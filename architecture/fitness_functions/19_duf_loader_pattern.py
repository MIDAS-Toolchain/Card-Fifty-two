#!/usr/bin/env python3
"""
FF-019: DUF Loader Pattern Verification

Ensures all DUF loaders follow the constitutional on-demand loader pattern:
1. LoadXDatabase() - Parse DUF file
2. ValidateXDatabase() - Validate ALL entries at startup
3. LoadXTemplateFromDUF() - On-demand heap allocation
4. CleanupXTemplate() - Free dStrings (not struct)
5. SDL error dialog integration in main.c

Based on: ADR-019 (On-Demand DUF Loader Pattern with Startup Validation)

Architecture Characteristic: Consistency
Domain Knowledge Required: DUF loader architecture
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Set, Optional, Tuple


# ============================================================================
# LOADER DISCOVERY
# ============================================================================

def discover_duf_loaders(project_root: Path) -> List[Dict[str, any]]:
    """
    Discover all DUF loaders by finding *Loader.h files.

    Returns: List of loader info dicts with:
        - name: Loader name (e.g., "enemy", "trinket", "affix")
        - header_path: Path to include/loaders/xLoader.h
        - impl_path: Path to src/loaders/xLoader.c
    """
    loaders = []
    loader_dir = project_root / "include" / "loaders"

    if not loader_dir.exists():
        return loaders

    for header_file in loader_dir.glob("*Loader.h"):
        loader_name = header_file.stem.replace("Loader", "").lower()

        # Find corresponding .c file
        impl_path = project_root / "src" / "loaders" / f"{header_file.stem}.c"

        if impl_path.exists():
            loaders.append({
                'name': loader_name,
                'header_path': header_file,
                'impl_path': impl_path,
                'capitalized': loader_name.capitalize()
            })

    return loaders


# ============================================================================
# FUNCTION EXISTENCE CHECKS
# ============================================================================

def find_function_definition(file_path: Path, function_pattern: str) -> Optional[int]:
    """
    Find function definition in file.

    Returns: Line number if found, None otherwise
    """
    try:
        with open(file_path, 'r') as f:
            for line_num, line in enumerate(f, 1):
                # Match function definition (not just declaration)
                if re.search(function_pattern, line):
                    return line_num
    except:
        pass

    return None


def check_validation_function(loader: Dict) -> Tuple[bool, Optional[str]]:
    """
    Check 1: Validate{X}Database() function exists

    Returns: (success, error_message)
    """
    impl_path = loader['impl_path']
    capitalized = loader['capitalized']

    # Pattern: bool ValidateXDatabase(dDUFValue_t* db, ...)
    pattern = rf'bool\s+Validate{capitalized}Database\s*\('

    line_num = find_function_definition(impl_path, pattern)

    if line_num:
        return (True, None)
    else:
        error = f"Missing: Validate{capitalized}Database() in {impl_path.name}"
        return (False, error)


def check_load_database_function(loader: Dict) -> Tuple[bool, Optional[str]]:
    """
    Check 2: LoadXDatabase() function exists

    Returns: (success, error_message)
    """
    impl_path = loader['impl_path']
    capitalized = loader['capitalized']

    # Pattern: dDUFError_t* LoadXDatabase(...)
    pattern = rf'dDUFError_t\*\s+Load{capitalized}Database\s*\('

    line_num = find_function_definition(impl_path, pattern)

    if line_num:
        return (True, None)
    else:
        # EXCEPTION: enemyLoader doesn't have LoadEnemyDatabase (loads in main.c directly)
        if loader['name'] == 'enemy':
            return (True, None)  # Skip this check for enemyLoader

        error = f"Missing: Load{capitalized}Database() in {impl_path.name}"
        return (False, error)


def check_ondemand_loader_function(loader: Dict) -> Tuple[bool, Optional[str]]:
    """
    Check 3: LoadXTemplateFromDUF() or LoadXFromDUF() function exists

    Returns: (success, error_message)
    """
    impl_path = loader['impl_path']
    name = loader['name']
    capitalized = loader['capitalized']

    # Pattern 1: LoadXTemplateFromDUF (trinket, affix)
    # Pattern 2: LoadXFromDUF (enemy)
    pattern1 = rf'{capitalized}Template_t\*\s+Load{capitalized}TemplateFromDUF\s*\('
    pattern2 = rf'{capitalized}_t\*\s+Load{capitalized}FromDUF\s*\('

    line1 = find_function_definition(impl_path, pattern1)
    line2 = find_function_definition(impl_path, pattern2)

    if line1 or line2:
        return (True, None)
    else:
        error = f"Missing: Load{capitalized}TemplateFromDUF() or Load{capitalized}FromDUF() in {impl_path.name}"
        return (False, error)


def check_cleanup_function(loader: Dict) -> Tuple[bool, Optional[str]]:
    """
    Check 4: CleanupXTemplate() or DestroyX() function exists

    Returns: (success, error_message)
    """
    impl_path = loader['impl_path']
    capitalized = loader['capitalized']

    # Pattern 1: void CleanupXTemplate(...) (trinket, affix)
    # Pattern 2: void DestroyX(...) (enemy)
    pattern1 = rf'void\s+Cleanup{capitalized}Template\s*\('
    pattern2 = rf'void\s+Destroy{capitalized}\s*\('

    line1 = find_function_definition(impl_path, pattern1)
    line2 = find_function_definition(impl_path, pattern2)

    if line1 or line2:
        return (True, None)
    else:
        # EXCEPTION: DestroyEnemy is in enemy.c, not enemyLoader.c
        if loader['name'] == 'enemy':
            # Check if it exists in enemy.c instead
            # impl_path is src/loaders/enemyLoader.c, parent is src/loaders, parent.parent is src
            project_root = impl_path.parent.parent.parent
            enemy_c = project_root / "src" / "enemy.c"

            if enemy_c.exists():
                line3 = find_function_definition(enemy_c, pattern2)
                if line3:
                    return (True, None)  # Found in enemy.c

        error = f"Missing: Cleanup{capitalized}Template() or Destroy{capitalized}() in {impl_path.name}"
        return (False, error)


# ============================================================================
# MAIN.C INTEGRATION CHECKS
# ============================================================================

def check_startup_validation(loader: Dict, main_c_path: Path) -> Tuple[bool, Optional[str]]:
    """
    Check 5: ValidateXDatabase() called in main.c with SDL dialog

    Returns: (success, error_message)
    """
    capitalized = loader['capitalized']

    try:
        with open(main_c_path, 'r') as f:
            main_c_content = f.read()
    except:
        return (False, f"Could not read {main_c_path}")

    # Check 1: ValidateXDatabase() is called
    validate_pattern = rf'Validate{capitalized}Database\s*\('
    if not re.search(validate_pattern, main_c_content):
        error = f"Validate{capitalized}Database() not called in main.c"
        return (False, error)

    # Check 2: SDL_ShowSimpleMessageBox appears near validation call
    # (within 20 lines of ValidateXDatabase call)
    lines = main_c_content.split('\n')
    validate_line = None
    sdl_dialog_line = None

    for i, line in enumerate(lines):
        if re.search(validate_pattern, line):
            validate_line = i

            # Search within +/- 20 lines for SDL dialog
            search_start = max(0, i - 20)
            search_end = min(len(lines), i + 20)

            for j in range(search_start, search_end):
                if 'SDL_ShowSimpleMessageBox' in lines[j]:
                    sdl_dialog_line = j
                    break

    if validate_line is None:
        return (False, f"Validate{capitalized}Database() not found in main.c")

    if sdl_dialog_line is None:
        error = f"SDL_ShowSimpleMessageBox not found near Validate{capitalized}Database() call (line {validate_line + 1})"
        return (False, error)

    return (True, None)


# ============================================================================
# ERROR MESSAGE FORMAT CHECK
# ============================================================================

def check_error_message_format(loader: Dict) -> Tuple[bool, Optional[str]]:
    """
    Check 6: Error message follows standard format

    Expected format:
        "X DUF Validation Failed\\n\\n"
        "Entry: ..." or "Enemy: ..." or "Trinket: ..."
        "File: ..."
        "Common issues:"

    Returns: (success, error_message)
    """
    impl_path = loader['impl_path']

    try:
        with open(impl_path, 'r') as f:
            impl_content = f.read()
    except:
        return (False, f"Could not read {impl_path}")

    # Check for standard error message pattern
    # Looking for: "X DUF Validation Failed\n\n" or "DUF Validation Failed\n\n"
    validation_failed_pattern = r'Validation Failed\\n\\n'

    if not re.search(validation_failed_pattern, impl_content):
        error = f"Error message doesn't follow standard format (missing 'Validation Failed\\n\\n')"
        return (False, error)

    # Check for "Common issues:" section
    if 'Common issues:' not in impl_content and 'common issues' not in impl_content.lower():
        error = f"Error message missing 'Common issues:' section"
        return (False, error)

    return (True, None)


# ============================================================================
# MAIN VERIFICATION
# ============================================================================

def verify_duf_loader_pattern(project_root: Path) -> bool:
    """
    Main verification function for FF-019.

    Returns: True if all loaders pass, False otherwise
    """
    print("=" * 80)
    print("FF-019: DUF Loader Pattern Verification")
    print("=" * 80)
    print()

    # Discover loaders
    loaders = discover_duf_loaders(project_root)

    if not loaders:
        print("❌ FAILURE: No DUF loaders found in include/loaders/")
        return False

    print(f"📋 Discovered DUF loaders: {len(loaders)}")
    for loader in loaders:
        print(f"   - {loader['impl_path'].name}")
    print()

    # Find main.c
    main_c_path = project_root / "src" / "main.c"
    if not main_c_path.exists():
        print(f"❌ FAILURE: Could not find {main_c_path}")
        return False

    # Check each loader
    all_passed = True
    failures = []

    for loader in loaders:
        print(f"Checking: {loader['name']}Loader")

        checks = [
            ("LoadXDatabase() function exists", check_load_database_function(loader)),
            ("ValidateXDatabase() function exists", check_validation_function(loader)),
            ("On-demand loader function exists", check_ondemand_loader_function(loader)),
            ("Cleanup function exists", check_cleanup_function(loader)),
            ("Startup validation with SDL dialog", check_startup_validation(loader, main_c_path)),
            ("Error message format", check_error_message_format(loader)),
        ]

        loader_passed = True

        for check_name, (success, error_msg) in checks:
            if success:
                print(f"   ✅ {check_name}")
            else:
                print(f"   ❌ {check_name}")
                if error_msg:
                    print(f"      → {error_msg}")
                loader_passed = False
                all_passed = False
                failures.append((loader['name'], check_name, error_msg))

        print()

    # Summary
    if all_passed:
        print("✅ SUCCESS: All DUF loaders follow constitutional pattern")
        print(f"✅ Verified {len(loaders)} loader(s): {', '.join([l['name'] for l in loaders])}")
        return True
    else:
        print(f"❌ FAILURE: {len(failures)} check(s) failed")
        print()
        print("Failures:")
        for loader_name, check_name, error_msg in failures:
            print(f"   🚨 {loader_name}Loader - {check_name}")
            if error_msg:
                print(f"      {error_msg}")
        print()
        print("💡 Fix: See ADR-019 for pattern template and examples")
        print(f"💡 Reference: architecture/decision_records/19_on_demand_duf_loader_pattern.md")
        return False


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_duf_loader_pattern(project_root)
    sys.exit(0 if success else 1)
