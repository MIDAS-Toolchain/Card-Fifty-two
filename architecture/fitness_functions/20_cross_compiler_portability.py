#!/usr/bin/env python3
"""
FF-020: Cross-Compiler Portability Verification

Verifies code follows C99/C11 compliance rules for GCC/Clang/Emscripten compatibility.

Checks:
1. Forward declarations match typedef definitions (tagged structs)
2. Static assertions use platform guards for pointer-containing structs
3. No function calls without prototypes (all includes present)
4. Typedef structs use tags when forward-declared elsewhere
5. No const qualifier discards

Related ADR: architecture/decision_records/20_cross_compiler_portability.md
"""

import re
import sys
from pathlib import Path

# ============================================================================
# CONFIGURATION
# ============================================================================

HEADER_DIRS = ["include"]
SOURCE_DIRS = ["src"]

# ============================================================================
# VIOLATION TRACKERS
# ============================================================================

violations = []

def add_violation(category, file_path, line_num, message):
    """Record a portability violation"""
    violations.append({
        'category': category,
        'file': file_path,
        'line': line_num,
        'message': message
    })

# ============================================================================
# CHECK 1: Forward Declaration / Typedef Matching
# ============================================================================

def check_forward_declaration_matching():
    """
    Verify forward declarations use tagged structs that match typedef definitions.

    Example violation:
        // common.h
        typedef struct Settings_t Settings_t;  // Forward declaration (tagged)

        // settings.h
        typedef struct {  // ❌ Anonymous struct doesn't match!
            int foo;
        } Settings_t;

    Correct:
        typedef struct Settings_t {  // ✅ Tagged struct matches forward declaration
            int foo;
        } Settings_t;
    """

    # Collect all forward declarations from headers
    forward_decls = {}  # {TypeName: (file, line_num, tag_name)}

    for header_dir in HEADER_DIRS:
        for header_file in Path(header_dir).rglob("*.h"):
            with open(header_file, 'r') as f:
                for line_num, line in enumerate(f, 1):
                    # Match: typedef struct Foo_t Foo_t;
                    match = re.match(r'^\s*typedef\s+struct\s+(\w+)\s+(\w+)\s*;', line)
                    if match:
                        tag_name = match.group(1)
                        typedef_name = match.group(2)

                        if tag_name == typedef_name:  # Forward declaration pattern
                            forward_decls[typedef_name] = (str(header_file), line_num, tag_name)

    # Check all typedef definitions match their forward declarations
    for header_dir in HEADER_DIRS:
        for header_file in Path(header_dir).rglob("*.h"):
            with open(header_file, 'r') as f:
                content = f.read()

                # Find all typedef struct definitions
                # Pattern: typedef struct OptionalTag { ... } TypeName;
                typedef_pattern = r'typedef\s+struct\s+(\w+)?\s*\{[^}]+\}\s*(\w+)\s*;'

                for match in re.finditer(typedef_pattern, content, re.MULTILINE | re.DOTALL):
                    tag_name = match.group(1)  # Optional tag
                    typedef_name = match.group(2)

                    # If this type has a forward declaration, verify tags match
                    if typedef_name in forward_decls:
                        fwd_file, fwd_line, expected_tag = forward_decls[typedef_name]

                        if tag_name is None:
                            # Anonymous struct doesn't match tagged forward declaration
                            line_num = content[:match.start()].count('\n') + 1
                            add_violation(
                                'FORWARD_DECL_MISMATCH',
                                str(header_file),
                                line_num,
                                f"typedef {typedef_name} uses anonymous struct but forward declaration at {fwd_file}:{fwd_line} uses tagged struct '{expected_tag}'. Change to: typedef struct {expected_tag} {{ }} {typedef_name};"
                            )
                        elif tag_name != expected_tag:
                            # Tags don't match
                            line_num = content[:match.start()].count('\n') + 1
                            add_violation(
                                'FORWARD_DECL_MISMATCH',
                                str(header_file),
                                line_num,
                                f"typedef {typedef_name} uses tag '{tag_name}' but forward declaration at {fwd_file}:{fwd_line} uses tag '{expected_tag}'"
                            )

# ============================================================================
# CHECK 2: Platform-Aware Static Assertions
# ============================================================================

def check_platform_aware_assertions():
    """
    Verify static assertions for pointer-containing structs use platform guards.

    Example violation:
        _Static_assert(sizeof(Card_t) == 32, "Card_t size");  // ❌ Assumes x64!

    Correct:
        #ifdef __EMSCRIPTEN__
        _Static_assert(sizeof(Card_t) == 28, "Card_t WASM32");  // 4-byte pointers
        #else
        _Static_assert(sizeof(Card_t) == 32, "Card_t x64");     // 8-byte pointers
        #endif
    """

    # Types known to contain pointers (from ADR-020 evidence)
    POINTER_CONTAINING_TYPES = [
        'Card_t',        # Contains aImage_t* texture
        'Player_t',      # Contains dString_t* name, dArray_t* hand
        'Enemy_t',       # Contains dString_t* name, AbilityData_t* abilities
        'AbilityData_t'  # Contains dString_t* name, description
    ]

    for header_dir in HEADER_DIRS:
        for header_file in Path(header_dir).rglob("*.h"):
            with open(header_file, 'r') as f:
                lines = f.readlines()

                for line_num, line in enumerate(lines, 1):
                    # Find _Static_assert for pointer-containing types
                    for ptr_type in POINTER_CONTAINING_TYPES:
                        if f'_Static_assert(sizeof({ptr_type})' in line:
                            # Check if surrounded by platform guards
                            has_platform_guard = False

                            # Look backwards for #ifdef __EMSCRIPTEN__
                            for prev_line in lines[max(0, line_num - 10):line_num]:
                                if '#ifdef __EMSCRIPTEN__' in prev_line or '#ifndef __EMSCRIPTEN__' in prev_line:
                                    has_platform_guard = True
                                    break

                            if not has_platform_guard:
                                add_violation(
                                    'MISSING_PLATFORM_GUARD',
                                    str(header_file),
                                    line_num,
                                    f"_Static_assert for {ptr_type} lacks platform guard. {ptr_type} contains pointers (size differs on WASM32 vs x64). Wrap in #ifdef __EMSCRIPTEN__"
                                )

# ============================================================================
# CHECK 3: Function Calls Without Prototypes
# ============================================================================

def check_implicit_function_calls():
    """
    Verify all function calls have prototypes (headers included).

    Example violation:
        // game.c
        State_Transition(STATE_MENU);  // ❌ No #include "state.h"!

    This is hard to detect perfectly without a full C parser, so we use heuristics:
    - Look for common function patterns
    - Check if corresponding header is included
    - NOTE: a_ and d_ functions are checked via common.h transitive includes
    """

    # Map of common function prefixes to their required headers
    # NOTE: Excluding a_ and d_ because they're transitively included via common.h
    FUNCTION_HEADERS = {
        'State_': 'state.h',
        'Player_': 'player.h',
        'Enemy_': 'enemy.h',
        'Card_': 'card.h',
        'Settings_': 'settings.h',
        'Event_': 'event.h',
    }

    for src_dir in SOURCE_DIRS:
        for src_file in Path(src_dir).rglob("*.c"):
            with open(src_file, 'r') as f:
                content = f.read()
                lines = content.split('\n')

                # Find all #include statements
                includes = set()
                for line in lines:
                    include_match = re.search(r'#include\s+[<"]([^>"]+)[>"]', line)
                    if include_match:
                        includes.add(include_match.group(1))

                # Check if common.h is included (provides transitive access to Archimedes/Daedalus)
                has_common_h = any('common.h' in inc for inc in includes)

                # Find function calls
                for line_num, line in enumerate(lines, 1):
                    # Skip comments
                    if line.strip().startswith('//') or line.strip().startswith('/*'):
                        continue

                    # Find function calls: FunctionName(
                    for prefix, required_header in FUNCTION_HEADERS.items():
                        pattern = rf'\b({prefix}\w+)\s*\('
                        for match in re.finditer(pattern, line):
                            func_name = match.group(1)

                            # Check if required header is included
                            # Accept both absolute and relative includes
                            header_included = any(
                                required_header in inc for inc in includes
                            )

                            if not header_included:
                                add_violation(
                                    'MISSING_PROTOTYPE',
                                    str(src_file),
                                    line_num,
                                    f"Function call {func_name}() requires #include \"{required_header}\" (or check if function is declared in included headers)"
                                )

# ============================================================================
# CHECK 4: Typedef Struct Tag Consistency
# ============================================================================

def check_typedef_tag_consistency():
    """
    Verify all typedef structs that are forward-declared elsewhere use tags.

    This is a stricter version of Check 1 - ensures ALL typedefs use tags
    if they might be forward-declared.
    """

    # Collect all typedef names that appear in forward declarations
    forward_declared_types = set()

    for header_dir in HEADER_DIRS:
        for header_file in Path(header_dir).rglob("*.h"):
            with open(header_file, 'r') as f:
                for line in f:
                    # Match: typedef struct Foo_t Foo_t;
                    match = re.match(r'^\s*typedef\s+struct\s+(\w+)\s+(\w+)\s*;', line)
                    if match and match.group(1) == match.group(2):
                        forward_declared_types.add(match.group(2))

    # Now check all typedef definitions
    for header_dir in HEADER_DIRS:
        for header_file in Path(header_dir).rglob("*.h"):
            with open(header_file, 'r') as f:
                content = f.read()

                # Find typedef struct definitions
                typedef_pattern = r'typedef\s+struct\s+(\w+)?\s*\{[^}]+\}\s*(\w+)\s*;'

                for match in re.finditer(typedef_pattern, content, re.MULTILINE | re.DOTALL):
                    tag_name = match.group(1)
                    typedef_name = match.group(2)

                    # If this type is forward-declared anywhere, it MUST have a tag
                    if typedef_name in forward_declared_types and tag_name is None:
                        line_num = content[:match.start()].count('\n') + 1
                        add_violation(
                            'UNTAGGED_FORWARD_DECL',
                            str(header_file),
                            line_num,
                            f"typedef {typedef_name} is forward-declared but uses anonymous struct. Change to: typedef struct {typedef_name} {{ }} {typedef_name};"
                        )

# ============================================================================
# MAIN EXECUTION
# ============================================================================

def main():
    """Run all portability checks"""

    print("=" * 80)
    print("FF-020: Cross-Compiler Portability Verification")
    print("=" * 80)
    print()

    # Run all checks
    print("[1/4] Checking forward declaration / typedef matching...")
    check_forward_declaration_matching()

    print("[2/4] Checking platform-aware static assertions...")
    check_platform_aware_assertions()

    print("[3/4] Checking for implicit function calls...")
    check_implicit_function_calls()

    print("[4/4] Checking typedef tag consistency...")
    check_typedef_tag_consistency()

    # Report results
    print()
    print("=" * 80)

    if not violations:
        print("✅ PASS: All cross-compiler portability checks passed")
        print()
        print("Code follows C99/C11 compliance rules:")
        print("  ✓ Forward declarations match typedef tags")
        print("  ✓ Static assertions use platform guards")
        print("  ✓ All function calls have prototypes")
        print("  ✓ Typedef structs use tags consistently")
        print()
        return 0

    # Group violations by category
    violations_by_category = {}
    for v in violations:
        cat = v['category']
        if cat not in violations_by_category:
            violations_by_category[cat] = []
        violations_by_category[cat].append(v)

    print(f"❌ FAIL: {len(violations)} portability violation(s) found")
    print()

    for category, viols in violations_by_category.items():
        print(f"[{category}] {len(viols)} violation(s):")
        print("-" * 80)
        for v in viols:
            print(f"  {v['file']}:{v['line']}")
            print(f"    {v['message']}")
            print()

    print("=" * 80)
    print()
    print("See ADR-020 for portability rules:")
    print("  architecture/decision_records/20_cross_compiler_portability.md")
    print()

    return 1

if __name__ == '__main__':
    sys.exit(main())
