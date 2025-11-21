#!/usr/bin/env python3
"""
FF-011: Targeting Uses Hover State Verification

Enforces ADR-11 (Targeting Selection Uses Hover State)

Verifies:
1. HandleTargetingInput() reads hover_state from sections (no duplicate position calc)
2. Targeting uses hovered_card_index (not manual mouse position iteration)
3. No duplicate z-index iteration logic in targeting code
4. Hover state is single source of truth for "card under mouse"

Based on: architecture/decision_records/11_targeting_uses_hover_state.md
"""

import re
from pathlib import Path
from typing import List, Dict, Any, Optional


def extract_function_body(content: str, func_name: str) -> Optional[str]:
    """Extract function body by matching braces"""
    pattern = rf'{func_name}\s*\([^)]*\)\s*\{{'
    match = re.search(pattern, content)

    if not match:
        return None

    # Find matching closing brace
    func_start = match.end()
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

    return content[func_start:func_end]


def verify_handle_targeting_exists(project_root: Path) -> bool:
    """Verify HandleTargetingInput() function exists"""
    print("📋 Verifying HandleTargetingInput() exists:")
    print()

    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"

    if not scene_blackjack.exists():
        print("❌ FATAL: sceneBlackjack.c not found")
        return False

    content = scene_blackjack.read_text()

    # Look for HandleTargetingInput function
    func_pattern = r'(static\s+)?void\s+HandleTargetingInput\s*\('
    if not re.search(func_pattern, content):
        print("⚠️  WARNING: HandleTargetingInput() not found")
        print("   (Targeting system may not be implemented yet)")
        print()
        return True  # Not a failure if not implemented

    print("✅ HandleTargetingInput() found")
    print()

    return True


def verify_uses_hover_state(project_root: Path) -> bool:
    """
    Verify targeting code reads hover_state from sections.

    Required pattern: section->hover_state.hovered_card_index
    """
    print("📋 Verifying targeting uses hover_state (ADR-11):")
    print()

    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"

    if not scene_blackjack.exists():
        return True

    content = scene_blackjack.read_text()

    # Extract HandleTargetingInput function body
    func_body = extract_function_body(content, r'(static\s+)?void\s+HandleTargetingInput')

    if not func_body:
        print("⚠️  INFO: HandleTargetingInput() not found (targeting not implemented)")
        print()
        return True

    # REQUIRED PATTERN: Reads hover_state from sections
    # Example: g_player_section->hover_state.hovered_card_index
    hover_state_pattern = r'(g_player_section|g_dealer_section)->hover_state\.hovered_card_index'

    if not re.search(hover_state_pattern, func_body):
        print("❌ FAIL: HandleTargetingInput() doesn't read hover_state from sections")
        print("   ADR-11 requires: section->hover_state.hovered_card_index")
        print()
        print("   Targeting MUST use hover state as single source of truth")
        print("   What player sees lifted (hovered) IS what gets selected")
        print()
        return False

    print("✅ Targeting reads hover_state from sections (single source of truth)")
    print()

    return True


def verify_no_duplicate_position_calc(project_root: Path) -> bool:
    """
    Verify targeting doesn't duplicate position calculation logic.

    ANTI-PATTERN: Manual mouse position checking in targeting
    REQUIRED: Use hover_state from sections (already handles position/z-index)
    """
    print("📋 Verifying no duplicate position calculation:")
    print()

    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"

    if not scene_blackjack.exists():
        return True

    content = scene_blackjack.read_text()

    # Extract HandleTargetingInput function body
    func_body = extract_function_body(content, r'(static\s+)?void\s+HandleTargetingInput')

    if not func_body:
        return True

    # ANTI-PATTERNS: Manual position checking (should use hover_state instead)
    anti_patterns = [
        (r'app\.mouse\.(x|y)\s*[<>=]', 'Manual mouse position comparison'),
        (r'IsCardHovered\s*\(', 'IsCardHovered() call (should use hover_state)'),
        (r'CalculateCardFanPosition\s*\(', 'CalculateCardFanPosition() in targeting (duplicates hover logic)'),
        (r'for\s*\([^)]*hand[^)]*\)\s*\{[^}]*mouse', 'Manual card iteration with mouse check'),
    ]

    violations = []

    for pattern, description in anti_patterns:
        if re.search(pattern, func_body, re.IGNORECASE):
            violations.append(description)

    if violations:
        print("❌ FAIL: Targeting contains duplicate position calculation logic")
        print()
        print("   Found anti-patterns:")
        for v in violations:
            print(f"   - {v}")
        print()
        print("   ADR-11: Hover system is authoritative for 'card under mouse'")
        print("   Targeting should ONLY read hover_state, not recalculate positions")
        print()
        return False

    print("✅ No duplicate position calculation (uses hover_state)")
    print()

    return True


def verify_no_manual_iteration(project_root: Path) -> bool:
    """
    Verify targeting doesn't manually iterate cards for z-index priority.

    Hover system already handles reverse iteration for topmost card.
    Targeting should just use the result (hovered_card_index).
    """
    print("📋 Verifying no manual card iteration in targeting:")
    print()

    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"

    if not scene_blackjack.exists():
        return True

    content = scene_blackjack.read_text()

    # Extract HandleTargetingInput function body
    func_body = extract_function_body(content, r'(static\s+)?void\s+HandleTargetingInput')

    if not func_body:
        return True

    # ANTI-PATTERN: Iterating through cards manually
    # for (int i = hand_size - 1; i >= 0; i--)  // Reverse iteration (like hover)
    # for (int i = 0; i < hand_size; i++)       // Forward iteration
    iteration_pattern = r'for\s*\(\s*int\s+i\s*='

    if re.search(iteration_pattern, func_body):
        # Check if it's iterating over cards
        # Look for hand, cards, or similar in the loop
        if re.search(r'for\s*\([^)]*\)\s*\{[^}]{0,200}(hand|card|GetCard)', func_body, re.DOTALL):
            print("❌ FAIL: Targeting manually iterates cards")
            print("   ADR-11: Use hover_state.hovered_card_index (already handles z-index)")
            print()
            print("   Hover system reverse-iterates for topmost card")
            print("   Targeting should just read the result, not duplicate iteration")
            print()
            return False

    print("✅ No manual card iteration (uses hovered_card_index)")
    print()

    return True


def verify_hover_state_coupling_acknowledged(project_root: Path) -> bool:
    """
    Verify that coupling to hover system is acknowledged in comments.

    This is a design decision - we accept the coupling for consistency.
    A comment near HandleTargetingInput should mention ADR-11 or hover_state dependency.
    """
    print("📋 Verifying hover_state dependency is documented:")
    print()

    scene_blackjack = project_root / "src" / "scenes" / "sceneBlackjack.c"

    if not scene_blackjack.exists():
        return True

    content = scene_blackjack.read_text()

    # Look for comments near HandleTargetingInput that mention hover or ADR-11
    # Extract ~20 lines before the function
    lines = content.split('\n')

    func_line = -1
    for i, line in enumerate(lines):
        if 'HandleTargetingInput' in line and 'void' in line:
            func_line = i
            break

    if func_line == -1:
        return True  # Function doesn't exist

    # Check 20 lines before function for documentation
    start = max(0, func_line - 20)
    context = '\n'.join(lines[start:func_line + 1])

    # Look for ADR-11 reference or hover_state documentation
    doc_patterns = [
        r'ADR-11',
        r'hover.*state.*source.*truth',
        r'uses.*hover.*state',
        r'relies.*hover.*system',
    ]

    has_documentation = any(re.search(pattern, context, re.IGNORECASE) for pattern in doc_patterns)

    if not has_documentation:
        print("⚠️  INFO: No ADR-11 reference near HandleTargetingInput()")
        print("   Consider adding comment explaining hover_state dependency")
        print()
        return True  # INFO only, not a failure

    print("✅ Hover_state dependency documented")
    print()

    return True


def verify_targeting_hover_state(project_root: Path) -> bool:
    """Main verification function"""

    print("=" * 80)
    print("FF-011: Targeting Hover State Verification")
    print("=" * 80)
    print()

    # Check 1: HandleTargetingInput exists
    check1 = verify_handle_targeting_exists(project_root)
    if not check1:
        return False

    # Check 2: Uses hover_state from sections
    check2 = verify_uses_hover_state(project_root)
    if not check2:
        return False

    # Check 3: No duplicate position calculation
    check3 = verify_no_duplicate_position_calc(project_root)
    if not check3:
        return False

    # Check 4: No manual card iteration
    check4 = verify_no_manual_iteration(project_root)
    if not check4:
        return False

    # Check 5: Coupling documented (INFO only)
    check5 = verify_hover_state_coupling_acknowledged(project_root)

    print("=" * 80)
    print("✅ SUCCESS: Targeting hover state pattern verified")
    print("=" * 80)
    print()
    print("Constitutional Pattern Verified:")
    print("  - Targeting reads hover_state from sections (single source of truth)")
    print("  - No duplicate position calculation in targeting code")
    print("  - No manual card iteration (uses hovered_card_index)")
    print("  - Hover system is authoritative for 'card under mouse'")
    print()
    print("What player sees lifted (hovered) IS what gets selected!")
    print()

    return True


if __name__ == '__main__':
    import sys

    # Determine project root (3 levels up from this script)
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent.parent

    success = verify_targeting_hover_state(project_root)
    sys.exit(0 if success else 1)
