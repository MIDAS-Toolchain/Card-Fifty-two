#!/usr/bin/env python3
"""
FF-003: Constitutional Pattern Enforcement

Ensures no raw malloc/calloc/realloc for string/array types.
Enforces use of dString_t, dArray_t, dTable_t throughout codebase.

Based on: ADR-003 (Constitutional Patterns - Daedalus Types Over Raw C)

Architecture Characteristic: Consistency
Domain Knowledge Required: None (purely structural analysis)
"""

import re
from pathlib import Path
from typing import List, Dict, Any


# Forbidden patterns (raw allocation for strings/arrays)
FORBIDDEN_PATTERNS = [
    # String allocations
    (r'malloc\s*\(\s*.*\*\s*sizeof\s*\(\s*char\s*\)', 'malloc(n * sizeof(char)) - use dString_t'),
    (r'calloc\s*\([^)]*,\s*sizeof\s*\(\s*char\s*\)', 'calloc(n, sizeof(char)) - use dString_t'),
    (r'realloc\s*\([^)]*,\s*[^)]*\*\s*sizeof\s*\(\s*char\s*\)', 'realloc(..., n * sizeof(char)) - use dString_t'),
    (r'char\s*\*\s*\w+\s*=\s*malloc', 'char* var = malloc(...) - use dString_t'),
    (r'malloc\s*\(\s*strlen\s*\([^)]*\)', 'malloc(strlen(...)) - use dString_t'),

    # Array allocations (generic pattern for any type)
    (r'malloc\s*\(\s*\w+\s*\*\s*sizeof\s*\([A-Z]\w*_t\)', 'malloc(n * sizeof(Type_t)) - use dArray_t'),
    (r'calloc\s*\(\s*\w+\s*,\s*sizeof\s*\([A-Z]\w*_t\)', 'calloc(n, sizeof(Type_t)) - use dArray_t'),
    (r'realloc\s*\([^)]*,\s*\w+\s*\*\s*sizeof\s*\([A-Z]\w*_t\)', 'realloc(..., n * sizeof(Type_t)) - use dArray_t'),
]

# Allowed exception markers in comments
EXCEPTION_MARKERS = [
    '// EXCEPTION:',
    '// FFI:',
    '/* EXCEPTION:',
    '/* FFI:',
]

# Directories to skip (external libraries, vendored code)
SKIP_DIRECTORIES = [
    'external',
    'vendor',
    'third_party',
]


def is_exception_allowed(line: str, prev_line: str = '') -> bool:
    """
    Check if line has an explicit exception marker allowing raw allocation.

    Exception markers can be on the same line or the previous line.
    """
    combined = prev_line + ' ' + line
    return any(marker in combined for marker in EXCEPTION_MARKERS)


def is_comment_line(line: str) -> bool:
    """Check if line is a comment"""
    stripped = line.strip()
    return stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*')


def scan_file_for_violations(file_path: Path) -> List[Dict[str, Any]]:
    """
    Scan a single C file for constitutional pattern violations.

    Returns: List of violation dictionaries with file, line, code, and pattern info
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"⚠ WARNING: Could not read {file_path}: {e}")
        return violations

    prev_line = ''

    for line_num, line in enumerate(lines, 1):
        # Skip comment lines
        if is_comment_line(line):
            prev_line = line
            continue

        # Skip lines with exception markers
        if is_exception_allowed(line, prev_line):
            prev_line = line
            continue

        # Check for forbidden patterns
        for pattern, description in FORBIDDEN_PATTERNS:
            if re.search(pattern, line):
                violations.append({
                    'file': file_path,
                    'line': line_num,
                    'code': line.strip(),
                    'pattern': description
                })

        prev_line = line

    return violations


def should_skip_directory(file_path: Path) -> bool:
    """Check if file is in a directory that should be skipped"""
    parts = file_path.parts
    return any(skip_dir in parts for skip_dir in SKIP_DIRECTORIES)


def scan_file_for_table_misuse(file_path: Path) -> List[Dict[str, Any]]:
    """
    Scan for incorrect d_GetDataFromTable usage patterns.

    Detects: Player_t** when g_players stores Player_t by value (should be Player_t*)

    Returns: List of violation dictionaries
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        return violations

    # Pattern: Player_t** ... = (Player_t**)d_GetDataFromTable(g_players, ...)
    # This is WRONG because g_players stores Player_t by value, not by pointer
    player_double_ptr_pattern = r'Player_t\s*\*\*.*=\s*\(\s*Player_t\s*\*\*\s*\)\s*d_GetDataFromTable\s*\(\s*g_players'

    for line_num, line in enumerate(lines, 1):
        if re.search(player_double_ptr_pattern, line):
            violations.append({
                'file': file_path,
                'line': line_num,
                'code': line.strip(),
                'pattern': 'Player_t** from g_players - should be Player_t* (table stores by value)'
            })

    return violations


def verify_constitutional_pattern_enforcement(project_root: Path) -> bool:
    """
    Main verification function for FF-003.

    Returns: True if no violations found, False otherwise
    """
    print("=" * 80)
    print("FF-003: Constitutional Pattern Enforcement")
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

    # Filter out skipped directories
    all_files = [f for f in all_files if not should_skip_directory(f)]

    print(f"📋 Scanning {len(all_files)} files ({len(c_files)} .c, {len(h_files)} .h)")
    print()

    # Scan all files for violations
    all_violations = []

    for file_path in all_files:
        violations = scan_file_for_violations(file_path)
        all_violations.extend(violations)

        # Also check for table misuse patterns
        table_violations = scan_file_for_table_misuse(file_path)
        all_violations.extend(table_violations)

    # Report results
    if all_violations:
        print("❌ FAILURE: Constitutional pattern violations detected!")
        print()
        print(f"Found {len(all_violations)} violation(s):")
        print()

        for v in all_violations:
            # Show path relative to project root
            relative_path = v['file'].relative_to(project_root)
            print(f"🚨 {relative_path}:{v['line']}")
            print(f"   Code: {v['code'][:80]}{'...' if len(v['code']) > 80 else ''}")
            print(f"   Issue: {v['pattern']}")
            print()

        print("💡 Fix: Replace raw malloc/calloc/realloc with dString_t or dArray_t")
        print("💡 Exception: Add '// EXCEPTION: {reason}' comment if truly necessary")
        print()
        return False

    print("✅ SUCCESS: All string/array allocations use Daedalus types")
    print(f"✅ SUCCESS: Zero violations in {len(all_files)} scanned files")
    print()

    return True


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_constitutional_pattern_enforcement(project_root)
    exit(0 if success else 1)
