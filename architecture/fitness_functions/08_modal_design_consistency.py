#!/usr/bin/env python3
"""
FF-008: Modal Design Consistency Verification

Enforces ADR-008 modal design patterns:
1. Description text MUST use scale = 1.1f (readability)
2. Full-height panels MUST use SCREEN_HEIGHT - TOP_BAR_HEIGHT
3. Full-height panels MUST anchor to SIDEBAR_WIDTH (not hover trigger)
4. Auto-size tooltips MUST calculate height dynamically
5. Color scheme consistency (dark background, gray border)

Constitutional Pattern: Mixed approach (tooltips auto-size, panels full-height)

Based on: ADR-008 - Consistent Modal Design and Positioning
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Set

# ============================================================================
# CONSTANTS (from ADR-008)
# ============================================================================

# Font scale requirements
TITLE_SCALE = 1.0
DESCRIPTION_SCALE = 1.1  # Enforced for readability

# Full-height panel requirements
FULL_HEIGHT_PATTERN = r'SCREEN_HEIGHT\s*-\s*TOP_BAR_HEIGHT'
SIDEBAR_ANCHOR_PATTERN = r'SIDEBAR_WIDTH\s*\+\s*\d+'

# Color scheme (modern dark)
BG_COLOR = "{20, 20, 30, 230}"
BORDER_COLOR = "{100, 100, 100, 200}"

# Modal classification
FULL_HEIGHT_MODALS = {
    'characterStatsModal.c',
    'combatStatsModal.c'
}

AUTO_SIZE_MODALS = {
    'abilityTooltipModal.c',
    'statusEffectTooltipModal.c',
    'chipsTooltipModal.c'
}

# ============================================================================
# VERIFICATION FUNCTIONS
# ============================================================================

def find_modal_files(project_root: Path) -> List[Path]:
    """Find all modal implementation files"""
    modals = []

    # Search in components and sections
    src_dir = project_root / 'src'
    modals.extend(src_dir.rglob('*Modal.c'))  # Match all files ending in Modal.c

    # Filter to only include actual modal components
    modals = [m for m in modals if 'modal' in m.name.lower()]

    return sorted(modals)

def check_font_scale_consistency(file_path: Path, content: str) -> List[str]:
    """Check that description text uses scale = 1.1f"""
    violations = []

    # Find all aFontConfig_t definitions with scale field
    # Pattern: aFontConfig_t name = { ... .scale = X.Xf ... };
    config_pattern = r'aFontConfig_t\s+(\w+)\s*=\s*\{([^}]+)\}'

    for match in re.finditer(config_pattern, content, re.DOTALL):
        config_name = match.group(1)
        config_body = match.group(2)

        # Skip title configs (they use 1.0f)
        if 'title' in config_name.lower():
            continue

        # Find scale value
        scale_match = re.search(r'\.scale\s*=\s*(\d+\.\d+)f?', config_body)
        if scale_match:
            scale = float(scale_match.group(1))

            # Description/effect/value configs MUST use 1.1f
            if config_name in ['desc_config', 'effect_config', 'trigger_config',
                              'state_config', 'duration_config', 'value_config',
                              'highest_config', 'lowest_config', 'effect_config',
                              'range_config', 'label_config']:
                if scale != DESCRIPTION_SCALE:
                    # Find line number
                    line_num = content[:match.start()].count('\n') + 1
                    violations.append(
                        f"  ❌ Line {line_num}: {config_name} uses scale = {scale}f "
                        f"(expected {DESCRIPTION_SCALE}f)"
                    )

    return violations

def check_full_height_pattern(file_path: Path, content: str) -> List[str]:
    """Check that full-height modals use SCREEN_HEIGHT - TOP_BAR_HEIGHT"""
    violations = []

    # Only check files designated as full-height modals
    if file_path.name not in FULL_HEIGHT_MODALS:
        return violations

    # Check for modal_height calculation
    if not re.search(FULL_HEIGHT_PATTERN, content):
        violations.append(
            f"  ❌ Missing full-height pattern: 'modal_height = SCREEN_HEIGHT - TOP_BAR_HEIGHT'"
        )

    return violations

def check_sidebar_anchor(file_path: Path, content: str) -> List[str]:
    """Check that full-height modals anchor to SIDEBAR_WIDTH, not hover trigger"""
    violations = []

    # Only check files designated as full-height modals
    if file_path.name not in FULL_HEIGHT_MODALS:
        return violations

    # Full-height modals should NOT position based on hover coords in Show function
    # They should be positioned in the section that calls them
    # So we check they DON'T have positioning logic in ShowXModal()

    show_func_pattern = rf'void\s+Show{file_path.stem.replace("Modal", "").title()}Modal\s*\([^)]+\)'
    show_match = re.search(show_func_pattern, content)

    if show_match:
        # Extract function body
        func_start = show_match.end()
        brace_count = 0
        func_body_start = content.find('{', func_start)
        if func_body_start == -1:
            return violations

        pos = func_body_start
        brace_count = 1
        while pos < len(content) and brace_count > 0:
            pos += 1
            if content[pos] == '{':
                brace_count += 1
            elif content[pos] == '}':
                brace_count -= 1

        func_body = content[func_body_start:pos]

        # Check that modal->x and modal->y are just assigned from params (no calculation)
        if 'modal->x' in func_body or 'modal->y' in func_body:
            # Simple assignment is OK: modal->x = x;
            # Calculation is WRONG: modal->x = card_x + CARD_WIDTH + 10;
            assignment_pattern = r'modal->(x|y)\s*=\s*([^;]+);'
            for assign_match in re.finditer(assignment_pattern, func_body):
                coord = assign_match.group(1)
                value = assign_match.group(2).strip()

                # If value contains arithmetic or function calls (not just param name)
                if any(op in value for op in ['+', '-', '*', '/', '(', ')']):
                    # Check if it's not the simple case of passing through params
                    if value != coord:
                        violations.append(
                            f"  ⚠️  Full-height modal positions in Show function "
                            f"(should anchor to SIDEBAR_WIDTH in section code)"
                        )
                        break

    return violations

def check_auto_size_pattern(file_path: Path, content: str) -> List[str]:
    """Check that auto-size tooltips calculate height dynamically"""
    violations = []

    # Only check files designated as auto-size modals
    if file_path.name not in AUTO_SIZE_MODALS:
        return violations

    # Check for height calculation logic using a_GetWrappedTextHeight
    if not re.search(r'a_GetWrappedTextHeight', content):
        violations.append(
            f"  ❌ Missing dynamic height calculation (a_GetWrappedTextHeight)"
        )

    # Check for modal_height variable
    if not re.search(r'(int|float)\s+modal_height\s*=', content):
        violations.append(
            f"  ❌ Missing modal_height variable (dynamic sizing)"
        )

    return violations

def check_color_scheme(file_path: Path, content: str) -> List[str]:
    """Check that modals use consistent color scheme"""
    violations = []

    # Check for dark background color definition
    bg_pattern = r'#define\s+COLOR_BG\s+\(\(aColor_t\)\s*{([^}]+)}\)'
    bg_match = re.search(bg_pattern, content)

    if bg_match:
        bg_value = '{' + bg_match.group(1).strip() + '}'
        if bg_value != BG_COLOR:
            violations.append(
                f"  ⚠️  Non-standard background color: {bg_value} "
                f"(recommended: {BG_COLOR})"
            )

    # Check for border color definition
    border_pattern = r'#define\s+COLOR_BORDER\s+\(\(aColor_t\)\s*{([^}]+)}\)'
    border_match = re.search(border_pattern, content)

    if border_match:
        border_value = '{' + border_match.group(1).strip() + '}'
        if border_value != BORDER_COLOR:
            violations.append(
                f"  ⚠️  Non-standard border color: {border_value} "
                f"(recommended: {BORDER_COLOR})"
            )

    return violations

# ============================================================================
# MAIN VERIFICATION LOGIC
# ============================================================================

def verify_modal_consistency(project_root: Path) -> bool:
    """Main verification logic"""

    print("=" * 70)
    print("FF-008: Modal Design Consistency Verification")
    print("=" * 70)
    print()

    # Find all modal files
    modal_files = find_modal_files(project_root)

    if not modal_files:
        print("⚠️  WARNING: No modal files found")
        return False

    print(f"📋 Found {len(modal_files)} modal file(s):")
    for modal in modal_files:
        print(f"   - {modal.name}")
    print()

    print("🔍 Verifying ADR-008 patterns:")
    print("   ✓ Font scale = 1.1f for descriptions")
    print("   ✓ Full-height panels use SCREEN_HEIGHT - TOP_BAR_HEIGHT")
    print("   ✓ Full-height panels anchor to SIDEBAR_WIDTH")
    print("   ✓ Auto-size tooltips calculate height dynamically")
    print("   ✓ Consistent color scheme (dark background, gray border)")
    print()

    all_violations = {}
    total_violations = 0

    for modal_file in modal_files:
        with open(modal_file, 'r') as f:
            content = f.read()

        violations = []

        # Run all checks
        violations.extend(check_font_scale_consistency(modal_file, content))
        violations.extend(check_full_height_pattern(modal_file, content))
        violations.extend(check_sidebar_anchor(modal_file, content))
        violations.extend(check_auto_size_pattern(modal_file, content))
        violations.extend(check_color_scheme(modal_file, content))

        if violations:
            all_violations[modal_file.name] = violations
            total_violations += len(violations)

    # Report results
    if total_violations == 0:
        print("✅ SUCCESS: All modals follow ADR-008 patterns")
        print()
        print("Constitutional Pattern Verified:")
        print("  - Description text uses scale = 1.1f (readability)")
        print("  - Full-height panels extend to SCREEN_HEIGHT - TOP_BAR_HEIGHT")
        print("  - Full-height panels anchor to SIDEBAR_WIDTH (no overlap)")
        print("  - Auto-size tooltips calculate height dynamically")
        print("  - Consistent color scheme across all modals")
        return True
    else:
        print(f"❌ FAILURE: Found {total_violations} violation(s) in {len(all_violations)} file(s)")
        print()

        for filename, violations in sorted(all_violations.items()):
            print(f"📄 {filename}:")
            for violation in violations:
                print(violation)
            print()

        print("Fix Guide:")
        print()
        print("1. Font Scale Violations:")
        print("   Change: .scale = 0.9f")
        print("   To:     .scale = 1.1f  // ADR-008: Consistent readability")
        print()
        print("2. Full-Height Pattern:")
        print("   int modal_height = SCREEN_HEIGHT - TOP_BAR_HEIGHT;")
        print()
        print("3. Sidebar Anchor (in section code, NOT modal Show function):")
        print("   int modal_x = SIDEBAR_WIDTH + 10;  // Prevent overlap")
        print("   int modal_y = TOP_BAR_HEIGHT;")
        print()
        print("4. Auto-Size Pattern:")
        print("   int title_height = a_GetWrappedTextHeight(title, font, width);")
        print("   int modal_height = padding + title_height + ... + padding;")
        print()

        return False

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_modal_consistency(project_root)
    sys.exit(0 if success else 1)
