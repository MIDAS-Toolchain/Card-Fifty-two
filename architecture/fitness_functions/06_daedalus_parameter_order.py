#!/usr/bin/env python3
"""
FF-006: d_ArrayInit Parameter Order Verification

Enforces correct parameter order for d_ArrayInit(capacity, element_size).
Prevents catastrophic bugs from swapped parameters that cause misaligned array access.

Based on: ADR-006 (d_ArrayInit Parameter Order Enforcement)

Architecture Characteristic: Safety, Correctness
Domain Knowledge Required: Understanding of Daedalus library API
"""

import re
from pathlib import Path
from typing import List, Dict, Any


# Correct pattern: d_ArrayInit(CAPACITY, sizeof(...))
# Where CAPACITY is a number, constant, or expression WITHOUT sizeof
CORRECT_PATTERN = re.compile(
    r'd_ArrayInit\s*\(\s*'           # Function call
    r'([^,]+?)\s*,\s*'                # First param (capacity) - capture group 1
    r'sizeof\s*\([^)]+\)\s*'          # Second param (element_size) - must be sizeof(...)
    r'\)',
    re.MULTILINE
)

# WRONG pattern: d_ArrayInit(sizeof(...), ...)
# This is the BUG we're preventing - sizeof in first parameter position
WRONG_SIZEOF_FIRST = re.compile(
    r'd_ArrayInit\s*\(\s*'
    r'sizeof\s*\([^)]+\)\s*,',        # sizeof in FIRST parameter (WRONG!)
    re.MULTILINE
)

# WRONG pattern: d_ArrayInit(element_size, capacity) - variable element_size first
# Less common but still wrong
WRONG_VARIABLE_FIRST = re.compile(
    r'd_ArrayInit\s*\(\s*'
    r'(\w*size\w*)\s*,\s*'            # Variable with "size" in name
    r'([^)]+)\)',
    re.MULTILINE
)

# Exception markers
EXCEPTION_MARKERS = [
    '// EXCEPTION:',
    '/* EXCEPTION:',
    '// DAEDALUS_WRAPPER:',  # For legitimate wrappers that flip params
]

# Directories to skip
SKIP_DIRECTORIES = [
    'external',
    'vendor',
    'third_party',
    'test',  # Tests might have intentional violations
]


def is_exception_allowed(line: str, prev_line: str = '', next_line: str = '') -> bool:
    """
    Check if line has an explicit exception marker allowing wrong parameter order.

    Checks current line, previous line, and next line for markers.
    """
    combined = prev_line + ' ' + line + ' ' + next_line
    return any(marker in combined for marker in EXCEPTION_MARKERS)


def is_comment_line(line: str) -> bool:
    """Check if line is a comment"""
    stripped = line.strip()
    return stripped.startswith('//') or stripped.startswith('/*') or stripped.startswith('*')


def has_clarifying_comment(prev_line: str) -> bool:
    """
    Check if previous line has the recommended clarifying comment.

    We recommend adding:
    // d_ArrayInit(capacity, element_size) - capacity FIRST!

    to make the parameter order explicit.
    """
    clarifying_patterns = [
        'capacity first',
        'capacity, element_size',
        'd_ArrayInit(capacity',
    ]
    prev_lower = prev_line.lower()
    return any(pattern in prev_lower for pattern in clarifying_patterns)


def extract_first_param(match_text: str) -> str:
    """
    Extract the first parameter from a d_ArrayInit call.

    Used to provide helpful error messages.
    """
    # Find content between first '(' and first ','
    start = match_text.find('(')
    comma = match_text.find(',', start)
    if start != -1 and comma != -1:
        return match_text[start+1:comma].strip()
    return "???"


def scan_file_for_violations(file_path: Path) -> List[Dict[str, Any]]:
    """
    Scan a single C file for d_ArrayInit parameter order violations.

    Returns: List of violation dictionaries
    """
    violations = []

    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            lines = content.split('\n')
    except Exception as e:
        print(f"⚠ WARNING: Could not read {file_path}: {e}")
        return violations

    # Check for WRONG pattern: sizeof(...) in first parameter
    for match in WRONG_SIZEOF_FIRST.finditer(content):
        line_num = content[:match.start()].count('\n') + 1
        line_text = lines[line_num - 1] if line_num <= len(lines) else ""

        prev_line = lines[line_num - 2] if line_num > 1 else ""
        next_line = lines[line_num] if line_num < len(lines) else ""

        # Skip if exception is allowed
        if is_exception_allowed(line_text, prev_line, next_line):
            continue

        # Skip comment lines
        if is_comment_line(line_text):
            continue

        matched_text = match.group(0)
        first_param = extract_first_param(matched_text)

        violations.append({
            'file': file_path,
            'line': line_num,
            'code': line_text.strip(),
            'issue': f'sizeof(...) in FIRST parameter position',
            'first_param': first_param,
            'suggestion': 'Swap parameters: d_ArrayInit(capacity, sizeof(Type_t))',
            'has_comment': has_clarifying_comment(prev_line)
        })

    # Check for WRONG pattern: variable with "size" in name first
    for match in WRONG_VARIABLE_FIRST.finditer(content):
        line_num = content[:match.start()].count('\n') + 1
        line_text = lines[line_num - 1] if line_num <= len(lines) else ""

        prev_line = lines[line_num - 2] if line_num > 1 else ""
        next_line = lines[line_num] if line_num < len(lines) else ""

        # Skip if exception is allowed
        if is_exception_allowed(line_text, prev_line, next_line):
            continue

        # Skip comment lines
        if is_comment_line(line_text):
            continue

        # Skip if it's actually correct (some variables like max_size could be capacity)
        first_param = match.group(1)
        second_param = match.group(2)

        # Only flag if second param looks like a capacity (number or MAX/COUNT constant)
        if re.match(r'\d+', second_param) or 'MAX' in second_param.upper() or 'COUNT' in second_param.upper():
            violations.append({
                'file': file_path,
                'line': line_num,
                'code': line_text.strip(),
                'issue': f'Suspicious: "{first_param}" (size-like name) in first position',
                'first_param': first_param,
                'suggestion': 'Verify parameter order: d_ArrayInit(capacity, element_size)',
                'has_comment': has_clarifying_comment(prev_line)
            })

    return violations


def should_skip_directory(file_path: Path) -> bool:
    """Check if file is in a directory that should be skipped"""
    parts = file_path.parts
    return any(skip_dir in parts for skip_dir in SKIP_DIRECTORIES)


def verify_daedalus_parameter_order(project_root: Path) -> bool:
    """
    Main verification function for FF-006.

    Returns: True if no violations found, False otherwise
    """
    print("=" * 80)
    print("FF-006: d_ArrayInit Parameter Order Verification")
    print("=" * 80)
    print()
    print("Enforcing: d_ArrayInit(capacity, element_size) - CAPACITY FIRST!")
    print("Based on: ADR-006 (d_ArrayInit Parameter Order Enforcement)")
    print()

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

    # Report results
    if all_violations:
        print("❌ FAILURE: d_ArrayInit parameter order violations detected!")
        print()
        print(f"Found {len(all_violations)} violation(s):")
        print()

        for v in all_violations:
            # Show path relative to project root
            relative_path = v['file'].relative_to(project_root)
            print(f"🚨 {relative_path}:{v['line']}")
            print(f"   Code: {v['code'][:80]}{'...' if len(v['code']) > 80 else ''}")
            print(f"   Issue: {v['issue']}")
            print(f"   First param: {v['first_param']}")
            print(f"   Fix: {v['suggestion']}")

            if not v['has_comment']:
                print(f"   💡 Add comment: // d_ArrayInit(capacity, element_size) - capacity FIRST!")

            print()

        print("=" * 80)
        print("⚠️  CRITICAL BUG PREVENTED")
        print("=" * 80)
        print()
        print("Wrong parameter order causes:")
        print("  • Misaligned array access (bus error)")
        print("  • Silent data corruption")
        print("  • Intermittent crashes (Heisenbugs)")
        print()
        print("Historical evidence: This bug caused 2+ days of debugging.")
        print("See: ADR-006 for full context and examples.")
        print()
        print("=" * 80)
        print()
        return False

    print("✅ SUCCESS: All d_ArrayInit calls have correct parameter order")
    print(f"✅ SUCCESS: Zero violations in {len(all_files)} scanned files")
    print()

    # Bonus: Report files with good clarifying comments
    files_with_comments = sum(1 for f in all_files if check_file_has_clarifying_comments(f))
    if files_with_comments > 0:
        print(f"👍 BONUS: {files_with_comments} file(s) include clarifying comments")

    print()

    return True


def check_file_has_clarifying_comments(file_path: Path) -> bool:
    """
    Check if file has at least one d_ArrayInit with clarifying comment.

    This is a bonus check - not required, but recommended.
    """
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except:
        return False

    for i, line in enumerate(lines):
        if 'd_ArrayInit' in line and i > 0:
            prev_line = lines[i-1]
            if has_clarifying_comment(prev_line):
                return True

    return False


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_daedalus_parameter_order(project_root)
    exit(0 if success else 1)
