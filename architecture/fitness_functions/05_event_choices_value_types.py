#!/usr/bin/env python3
"""
FF-005: Event Choices as Value Types Enforcement

Ensures EventChoice_t is stored as value types in dArray_t, not as pointers.
Verifies AddEventChoice copies structs into array (per ADR-006).

Based on: ADR-005 (Event Choices as Value Types)

Architecture Characteristic: Memory Safety, Consistency
Domain Knowledge Required: Minimal (EventChoice_t usage pattern)
"""

import re
from pathlib import Path
from typing import List, Dict, Optional


def parse_game_event_struct(header_path: Path) -> Optional[Dict[str, str]]:
    """
    Parse GameEvent_t struct from event.h
    Returns dict of field_name → field_type
    """
    try:
        with open(header_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"⚠ WARNING: Could not read {header_path}: {e}")
        return None

    # Find GameEvent_t struct
    pattern = r'typedef\s+struct\s+GameEvent\s*\{([^}]+)\}\s*GameEvent_t\s*;'
    match = re.search(pattern, content, re.DOTALL)

    if not match:
        return None

    body = match.group(1)
    fields = {}

    # Parse each field
    for line in body.split('\n'):
        line = line.strip()
        if not line or line.startswith('//') or line.startswith('/*'):
            continue

        # Match field: type name;
        field_match = re.match(r'(.+?)\s+(\w+)\s*;', line)
        if field_match:
            field_type = field_match.group(1).strip()
            field_name = field_match.group(2)
            fields[field_name] = field_type

    return fields


def check_choices_array_type(fields: Dict[str, str]) -> List[str]:
    """
    Verify choices field is dArray_t* (not EventChoice_t**)
    """
    violations = []

    if 'choices' not in fields:
        violations.append("GameEvent_t missing 'choices' field")
        return violations

    choices_type = fields['choices']

    # Should be: dArray_t*
    if choices_type != 'dArray_t*':
        violations.append(
            f"choices field is '{choices_type}' (expected 'dArray_t*')\n"
            f"   EventChoice_t should be VALUE TYPE in array, not pointer array"
        )

    return violations


def check_event_choice_allocation(src_path: Path) -> List[str]:
    """
    Verify no malloc/calloc for EventChoice_t in event.c
    (EventChoice_t should be stack-allocated then copied into array)
    """
    violations = []

    try:
        with open(src_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"⚠ WARNING: Could not read {src_path}: {e}")
        return violations

    for line_num, line in enumerate(lines, 1):
        # Skip comments
        stripped = line.strip()
        if stripped.startswith('//') or stripped.startswith('/*'):
            continue

        # Check for malloc(sizeof(EventChoice_t))
        if re.search(r'malloc\s*\(\s*sizeof\s*\(\s*EventChoice_t\s*\)', line):
            violations.append(
                f"event.c:{line_num}\n"
                f"   Code: {line.strip()}\n"
                f"   EventChoice_t should be stack-allocated, not heap-allocated\n"
                f"   Per ADR-006: Use 'EventChoice_t choice = {{0}};' then d_AppendDataToArray"
            )

        # Check for EventChoice_t** (pointer-to-pointer suggests pointer array)
        if re.search(r'EventChoice_t\s*\*\*', line):
            violations.append(
                f"event.c:{line_num}\n"
                f"   Code: {line.strip()}\n"
                f"   EventChoice_t** suggests pointer array (should be value array)"
            )

    return violations


def check_add_event_choice_pattern(src_path: Path) -> List[str]:
    """
    Verify AddEventChoice uses value semantics (&choice passed to append)
    """
    violations = []

    try:
        with open(src_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        print(f"⚠ WARNING: Could not read {src_path}: {e}")
        return violations

    # Find AddEventChoice function start
    func_start_pattern = r'void\s+AddEventChoice\s*\([^)]+\)\s*\{'
    match = re.search(func_start_pattern, content)

    if not match:
        # Function might not exist yet (not implemented)
        return []

    # Extract function body with proper brace matching
    start = match.end()
    brace_count = 1
    pos = start

    while pos < len(content) and brace_count > 0:
        if content[pos] == '{':
            brace_count += 1
        elif content[pos] == '}':
            brace_count -= 1
        pos += 1

    func_body = content[start:pos-1]

    # Check for value initialization: EventChoice_t choice = {0};
    if not re.search(r'EventChoice_t\s+\w+\s*=\s*\{', func_body):
        violations.append(
            "AddEventChoice: Missing 'EventChoice_t choice = {0};' initialization\n"
            "   EventChoice_t should be stack-allocated (value type)"
        )

    # Check for d_AppendDataToArray with address-of operator
    # Look for pattern: d_AppendDataToArray(..., &variable)
    if not re.search(r'd_AppendDataToArray\s*\([^,]+,\s*&\w+\s*\)', func_body):
        violations.append(
            "AddEventChoice: Missing 'd_AppendDataToArray(..., &choice)'\n"
            "   Must pass address of stack variable to copy into array"
        )

    return violations


def verify_event_choices_value_types(project_root: Path) -> bool:
    """
    Main verification function for FF-005.

    Returns: True if event choices properly use value semantics, False otherwise
    """
    print("=" * 80)
    print("FF-005: Event Choices as Value Types Enforcement")
    print("=" * 80)

    header_path = project_root / 'include' / 'event.h'
    src_path = project_root / 'src' / 'event.c'

    # Check if event system exists
    if not header_path.exists():
        print("⚠ INFO: Event system not implemented yet (event.h not found)")
        print("✅ SUCCESS: Skipping verification (feature not present)")
        print()
        return True

    if not src_path.exists():
        print("⚠ INFO: Event system not implemented yet (event.c not found)")
        print("✅ SUCCESS: Skipping verification (feature not present)")
        print()
        return True

    print()
    all_violations = []

    # Check 1: GameEvent_t struct
    print("📋 Checking GameEvent_t struct definition...")
    fields = parse_game_event_struct(header_path)

    if fields is None:
        print("⚠ WARNING: Could not parse GameEvent_t struct")
        print()
    else:
        violations = check_choices_array_type(fields)

        if violations:
            all_violations.extend(violations)
        else:
            print("   ✅ choices field is dArray_t* (value array)")
    print()

    # Check 2: No EventChoice_t malloc in event.c
    print("📋 Checking EventChoice_t allocation patterns...")
    violations = check_event_choice_allocation(src_path)

    if violations:
        all_violations.extend(violations)
    else:
        print("   ✅ No heap allocation of EventChoice_t (value type)")
    print()

    # Check 3: AddEventChoice uses value semantics
    print("📋 Checking AddEventChoice implementation...")
    violations = check_add_event_choice_pattern(src_path)

    if violations:
        all_violations.extend(violations)
    else:
        print("   ✅ AddEventChoice uses value semantics (&choice)")
    print()

    # Report results
    if all_violations:
        print("❌ FAILURE: Event choices value type pattern violated!")
        print()
        for v in all_violations:
            print(f"🚨 {v}")
            print()
        print("💡 Per ADR-006: EventChoice_t must be VALUE TYPE in dArray_t")
        print("💡 Pattern:")
        print("   EventChoice_t choice = {0};          // Stack allocation")
        print("   // ... initialize fields ...")
        print("   d_AppendDataToArray(array, &choice); // Copy into array")
        print()
        return False

    print("✅ SUCCESS: Event choices stored as value types (per ADR-006)")
    print()

    return True


if __name__ == '__main__':
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_event_choices_value_types(project_root)
    exit(0 if success else 1)
