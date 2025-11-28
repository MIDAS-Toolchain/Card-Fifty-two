#!/usr/bin/env python3
"""
FF-017: Terminal Architecture Verification

Ensures terminal implementation follows ADR-017 patterns:
1. Daedalus types (ADR-003)
2. Correct d_ArrayInit parameter order (ADR-006)
3. Double-pointer destructor (ADR-015)
4. Command registration system (ADR-017)
5. File organization (terminal.c, commands.c, terminal.h)
6. Memory management patterns

Based on: ADR-017 (Developer Terminal Architecture)

Architecture Characteristics: Maintainability, Extensibility, Consistency
"""

import re
from pathlib import Path
from typing import List, Dict, Any, Tuple


# Terminal files that must exist
REQUIRED_FILES = {
    'terminal.c': 'src/terminal/terminal.c',
    'commands.c': 'src/terminal/commands.c',
    'terminal.h': 'include/terminal/terminal.h',
}

# Required functions in terminal.h
REQUIRED_FUNCTIONS = [
    'InitTerminal',
    'CleanupTerminal',
    'RegisterCommand',
    'ExecuteCommand',
    'TerminalPrint',
]

# Required patterns in Terminal_t struct
REQUIRED_STRUCT_FIELDS = [
    ('dString_t*', 'input_buffer'),
    ('dArray_t*', 'command_history'),
    ('dArray_t*', 'output_log'),
    ('dTable_t*', 'registered_commands'),
]


def check_required_files_exist(project_root: Path) -> Tuple[bool, List[str]]:
    """
    Verify all required terminal files exist.

    Returns: (success, missing_files)
    """
    missing = []

    for name, rel_path in REQUIRED_FILES.items():
        full_path = project_root / rel_path
        if not full_path.exists():
            missing.append(rel_path)

    return (len(missing) == 0, missing)


def check_daedalus_types_in_struct(project_root: Path) -> Tuple[bool, List[str]]:
    """
    Verify Terminal_t struct uses Daedalus types (ADR-003).

    Returns: (success, missing_fields)
    """
    terminal_h = project_root / REQUIRED_FILES['terminal.h']

    if not terminal_h.exists():
        return (False, ['terminal.h not found'])

    content = terminal_h.read_text()

    # Find Terminal_t struct
    struct_pattern = r'typedef\s+struct\s+Terminal\s*\{([^}]+)\}\s*Terminal_t;'
    match = re.search(struct_pattern, content, re.MULTILINE | re.DOTALL)

    if not match:
        return (False, ['Terminal_t struct not found'])

    struct_body = match.group(1)
    missing = []

    for field_type, field_name in REQUIRED_STRUCT_FIELDS:
        # Check if field exists with correct type
        pattern = rf'{re.escape(field_type)}\s+{field_name}\s*;'
        if not re.search(pattern, struct_body):
            missing.append(f'{field_type} {field_name}')

    return (len(missing) == 0, missing)


def check_d_init_array_parameter_order(project_root: Path) -> Tuple[bool, List[Dict[str, Any]]]:
    """
    Verify all d_ArrayInit calls have correct parameter order (ADR-006).

    Returns: (success, violations)
    """
    violations = []

    terminal_c = project_root / REQUIRED_FILES['terminal.c']

    if not terminal_c.exists():
        return (False, [{'error': 'terminal.c not found'}])

    content = terminal_c.read_text()
    lines = content.split('\n')

    # WRONG pattern: sizeof(...) in first parameter
    wrong_pattern = re.compile(r'd_ArrayInit\s*\(\s*sizeof\s*\([^)]+\)\s*,')

    for match in wrong_pattern.finditer(content):
        line_num = content[:match.start()].count('\n') + 1
        line_text = lines[line_num - 1] if line_num <= len(lines) else ""

        violations.append({
            'file': 'terminal.c',
            'line': line_num,
            'code': line_text.strip(),
            'issue': 'sizeof(...) in FIRST parameter (should be capacity first)',
        })

    return (len(violations) == 0, violations)


def check_cleanup_terminal_signature(project_root: Path) -> Tuple[bool, List[str]]:
    """
    Verify CleanupTerminal uses double-pointer pattern (ADR-015).

    Returns: (success, issues)
    """
    issues = []

    # Check header signature
    terminal_h = project_root / REQUIRED_FILES['terminal.h']

    if not terminal_h.exists():
        return (False, ['terminal.h not found'])

    h_content = terminal_h.read_text()

    # Pattern: void CleanupTerminal(Terminal_t** terminal);
    double_ptr_pattern = r'void\s+CleanupTerminal\s*\(\s*Terminal_t\s*\*\*\s*\w+\s*\)\s*;'

    if not re.search(double_ptr_pattern, h_content):
        issues.append('CleanupTerminal signature not double-pointer (Terminal_t**)')

    # Check implementation
    terminal_c = project_root / REQUIRED_FILES['terminal.c']

    if not terminal_c.exists():
        return (False, ['terminal.c not found'])

    c_content = terminal_c.read_text()

    # Find CleanupTerminal implementation (look for the whole function, handle nested braces)
    # Find start of function
    start_pattern = r'void\s+CleanupTerminal\s*\([^)]+\)\s*\{'
    start_match = re.search(start_pattern, c_content)

    if not start_match:
        issues.append('CleanupTerminal implementation not found')
        return (False, issues)

    # Extract function body by counting braces
    start_pos = start_match.end()
    brace_count = 1
    pos = start_pos

    while pos < len(c_content) and brace_count > 0:
        if c_content[pos] == '{':
            brace_count += 1
        elif c_content[pos] == '}':
            brace_count -= 1
        pos += 1

    impl_body = c_content[start_pos:pos-1]

    # Check for NULL guard
    null_guard_pattern = r'if\s*\(\s*!\s*terminal\s*\|\|\s*!\s*\*terminal\s*\)\s*return\s*;'
    if not re.search(null_guard_pattern, impl_body):
        issues.append('CleanupTerminal missing NULL guard: if (!terminal || !*terminal) return;')

    # Check for pointer nulling (either *terminal = NULL or *ptr = NULL)
    null_ptr_pattern = r'\*\w+\s*=\s*NULL\s*;'
    if not re.search(null_ptr_pattern, impl_body):
        issues.append('CleanupTerminal missing pointer nulling: *terminal = NULL;')

    # Check for free() call (free(t) or free(*terminal))
    free_pattern = r'free\s*\('
    if not re.search(free_pattern, impl_body):
        issues.append('CleanupTerminal missing free() call')

    return (len(issues) == 0, issues)


def check_command_registration_pattern(project_root: Path) -> Tuple[bool, List[str]]:
    """
    Verify command registration system exists (ADR-017).

    Returns: (success, issues)
    """
    issues = []

    terminal_h = project_root / REQUIRED_FILES['terminal.h']

    if not terminal_h.exists():
        return (False, ['terminal.h not found'])

    h_content = terminal_h.read_text()

    # Check for CommandFunc_t typedef
    if 'CommandFunc_t' not in h_content:
        issues.append('CommandFunc_t typedef not found')

    # Check for RegisterCommand function
    if 'RegisterCommand' not in h_content:
        issues.append('RegisterCommand function not declared')

    # Check for ExecuteCommand function
    if 'ExecuteCommand' not in h_content:
        issues.append('ExecuteCommand function not declared')

    # Check implementation in terminal.c
    terminal_c = project_root / REQUIRED_FILES['terminal.c']

    if terminal_c.exists():
        c_content = terminal_c.read_text()

        # Check RegisterCommand implementation
        if 'void RegisterCommand' not in c_content:
            issues.append('RegisterCommand implementation not found in terminal.c')

        # Check ExecuteCommand implementation
        if 'void ExecuteCommand' not in c_content:
            issues.append('ExecuteCommand implementation not found in terminal.c')

        # Check d_TableSet usage for command storage
        if 'd_TableSet' not in c_content or 'registered_commands' not in c_content:
            issues.append('Commands not stored in dTable_t (expected d_TableSet usage)')

    return (len(issues) == 0, issues)


def check_file_organization(project_root: Path) -> Tuple[bool, List[str]]:
    """
    Verify terminal code is organized across terminal.c, commands.c, terminal.h.

    Returns: (success, issues)
    """
    issues = []

    # Check commands.c exists
    commands_c = project_root / REQUIRED_FILES['commands.c']

    if not commands_c.exists():
        issues.append('commands.c not found (command implementations should be separate)')
        return (False, issues)

    c_content = commands_c.read_text()

    # Check for RegisterBuiltinCommands
    if 'RegisterBuiltinCommands' not in c_content:
        issues.append('RegisterBuiltinCommands not found in commands.c')

    # Check for at least some command implementations (CMD_* or Command* pattern)
    command_pattern = r'void\s+(CMD_\w+|Command\w+)\s*\(\s*Terminal_t\s*\*\s*terminal'
    command_matches = re.findall(command_pattern, c_content)

    if len(command_matches) < 3:
        issues.append(f'Only {len(command_matches)} command implementations found (expected at least 3)')

    # Check terminal.c doesn't contain command implementations (should be in commands.c)
    terminal_c = project_root / REQUIRED_FILES['terminal.c']

    if terminal_c.exists():
        t_content = terminal_c.read_text()
        terminal_command_matches = re.findall(command_pattern, t_content)

        if len(terminal_command_matches) > 0:
            issues.append(f'Found {len(terminal_command_matches)} command implementations in terminal.c (should be in commands.c)')

    return (len(issues) == 0, issues)


def check_memory_management_patterns(project_root: Path) -> Tuple[bool, List[str]]:
    """
    Verify proper memory management in terminal code.

    Returns: (success, issues)
    """
    issues = []

    terminal_c = project_root / REQUIRED_FILES['terminal.c']

    if not terminal_c.exists():
        return (False, ['terminal.c not found'])

    content = terminal_c.read_text()

    # Check TerminalPrint creates heap-allocated strings
    if 'TerminalPrint' in content:
        # Find TerminalPrint implementation
        print_pattern = r'void\s+TerminalPrint\s*\([^)]+\)\s*\{([^}]+(?:\{[^}]*\}[^}]*)*)\}'
        print_match = re.search(print_pattern, content, re.MULTILINE | re.DOTALL)

        if print_match:
            print_body = print_match.group(1)

            # Check for d_StringInit (heap allocation)
            if 'd_StringInit' not in print_body:
                issues.append('TerminalPrint should use d_StringInit for heap-allocated strings')

            # Check for d_ArrayAppend (storing pointer)
            if 'd_ArrayAppend' not in print_body or 'output_log' not in print_body:
                issues.append('TerminalPrint should append to output_log array')

    # Check CleanupTerminal destroys nested resources
    # Find start of CleanupTerminal function
    cleanup_start_pattern = r'void\s+CleanupTerminal\s*\([^)]+\)\s*\{'
    cleanup_start_match = re.search(cleanup_start_pattern, content)

    if cleanup_start_match:
        # Extract function body by counting braces
        start_pos = cleanup_start_match.end()
        brace_count = 1
        pos = start_pos

        while pos < len(content) and brace_count > 0:
            if content[pos] == '{':
                brace_count += 1
            elif content[pos] == '}':
                brace_count -= 1
            pos += 1

        cleanup_body = content[start_pos:pos-1]

        # Check for string destruction in loops
        if 'd_StringDestroy' not in cleanup_body:
            issues.append('CleanupTerminal should destroy nested dString_t objects')

        # Check for array destruction (case-insensitive for variations)
        if not re.search(r'd_ArrayDestroy', cleanup_body, re.IGNORECASE):
            issues.append('CleanupTerminal should destroy dArray_t objects')

        # Check for table destruction (case-insensitive for variations)
        if not re.search(r'd_TableDestroy', cleanup_body, re.IGNORECASE):
            issues.append('CleanupTerminal should destroy dTable_t objects')

    return (len(issues) == 0, issues)


def check_no_raw_malloc_in_terminal(project_root: Path) -> Tuple[bool, List[Dict[str, Any]]]:
    """
    Verify terminal files don't use raw malloc for strings/arrays (ADR-003).

    Returns: (success, violations)
    """
    violations = []

    # Forbidden patterns (raw malloc for strings/arrays)
    forbidden_patterns = [
        (r'malloc\s*\(\s*.*\*\s*sizeof\s*\(\s*char\s*\)', 'malloc(n * sizeof(char)) - use dString_t'),
        (r'char\s*\*\s*\w+\s*=\s*malloc', 'char* var = malloc(...) - use dString_t'),
        (r'malloc\s*\(\s*strlen', 'malloc(strlen(...)) - use dString_t'),
    ]

    for file_key in ['terminal.c', 'commands.c']:
        file_path = project_root / REQUIRED_FILES[file_key]

        if not file_path.exists():
            continue

        content = file_path.read_text()
        lines = content.split('\n')

        for pattern, reason in forbidden_patterns:
            for match in re.finditer(pattern, content):
                line_num = content[:match.start()].count('\n') + 1
                line_text = lines[line_num - 1] if line_num <= len(lines) else ""

                # Skip if EXCEPTION marker present
                prev_line = lines[line_num - 2] if line_num > 1 else ""
                if 'EXCEPTION:' in prev_line or 'EXCEPTION:' in line_text:
                    continue

                violations.append({
                    'file': file_key,
                    'line': line_num,
                    'code': line_text.strip(),
                    'issue': reason,
                })

    return (len(violations) == 0, violations)


def verify_terminal_architecture(project_root: Path) -> bool:
    """
    Main verification function for FF-017.

    Returns: True if all checks pass, False otherwise
    """
    print("=" * 80)
    print("FF-017: Terminal Architecture Verification")
    print("=" * 80)
    print()
    print("Based on: ADR-017 (Developer Terminal Architecture)")
    print()

    all_passed = True

    # Check 1: Required files exist
    print("📁 Checking file organization...")
    files_ok, missing_files = check_required_files_exist(project_root)

    if not files_ok:
        print(f"❌ FAILURE: Missing required files:")
        for f in missing_files:
            print(f"   - {f}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: All required files present (terminal.c, commands.c, terminal.h)")
        print()

    # Check 2: Daedalus types in Terminal_t struct
    print("🏗️  Checking Daedalus types usage (ADR-003)...")
    types_ok, missing_fields = check_daedalus_types_in_struct(project_root)

    if not types_ok:
        print(f"❌ FAILURE: Terminal_t struct issues:")
        for f in missing_fields:
            print(f"   - Missing or incorrect: {f}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: Terminal_t uses dString_t, dArray_t, dTable_t")
        print()

    # Check 3: d_ArrayInit parameter order
    print("🔢 Checking d_ArrayInit parameter order (ADR-006)...")
    order_ok, violations = check_d_init_array_parameter_order(project_root)

    if not order_ok:
        print(f"❌ FAILURE: d_ArrayInit parameter order violations:")
        for v in violations:
            if 'error' in v:
                print(f"   - {v['error']}")
            else:
                print(f"   - {v['file']}:{v['line']}: {v['issue']}")
                print(f"     Code: {v['code']}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: All d_ArrayInit calls use correct parameter order (capacity first)")
        print()

    # Check 4: CleanupTerminal signature and implementation
    print("🧹 Checking CleanupTerminal pattern (ADR-015)...")
    cleanup_ok, issues = check_cleanup_terminal_signature(project_root)

    if not cleanup_ok:
        print(f"❌ FAILURE: CleanupTerminal issues:")
        for issue in issues:
            print(f"   - {issue}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: CleanupTerminal follows double-pointer destructor pattern")
        print()

    # Check 5: Command registration system
    print("📝 Checking command registration system (ADR-017)...")
    cmd_ok, cmd_issues = check_command_registration_pattern(project_root)

    if not cmd_ok:
        print(f"❌ FAILURE: Command registration issues:")
        for issue in cmd_issues:
            print(f"   - {issue}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: Command registration system present (RegisterCommand, ExecuteCommand)")
        print()

    # Check 6: File organization (separation of concerns)
    print("📂 Checking file organization (ADR-017)...")
    org_ok, org_issues = check_file_organization(project_root)

    if not org_ok:
        print(f"❌ FAILURE: File organization issues:")
        for issue in org_issues:
            print(f"   - {issue}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: Clean separation (terminal.c core, commands.c implementations)")
        print()

    # Check 7: Memory management patterns
    print("💾 Checking memory management patterns (ADR-017)...")
    mem_ok, mem_issues = check_memory_management_patterns(project_root)

    if not mem_ok:
        print(f"❌ FAILURE: Memory management issues:")
        for issue in mem_issues:
            print(f"   - {issue}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: Proper memory management (heap strings, recursive cleanup)")
        print()

    # Check 8: No raw malloc for strings/arrays
    print("🚫 Checking for raw malloc violations (ADR-003)...")
    malloc_ok, malloc_violations = check_no_raw_malloc_in_terminal(project_root)

    if not malloc_ok:
        print(f"❌ FAILURE: Raw malloc violations in terminal code:")
        for v in malloc_violations:
            print(f"   - {v['file']}:{v['line']}: {v['issue']}")
            print(f"     Code: {v['code']}")
        print()
        all_passed = False
    else:
        print(f"✅ SUCCESS: No raw malloc (uses dString_t/dArray_t exclusively)")
        print()

    # Final summary
    print("=" * 80)

    if all_passed:
        print("✅ ALL CHECKS PASSED")
        print()
        print("Terminal architecture follows all constitutional patterns:")
        print("  • Daedalus types (ADR-003)")
        print("  • Correct d_ArrayInit order (ADR-006)")
        print("  • Double-pointer cleanup (ADR-015)")
        print("  • Command registration system (ADR-017)")
        print("  • Clean file organization (ADR-017)")
        print("  • Proper memory management (ADR-017)")
        print()
        return True
    else:
        print("❌ SOME CHECKS FAILED")
        print()
        print("Terminal architecture violations detected - see details above.")
        print()
        return False


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_terminal_architecture(project_root)
    exit(0 if success else 1)
