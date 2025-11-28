#!/usr/bin/env python3
"""
FF-018: Interactive Button UX Pattern Verification

Enforces ADR-018 (Interactive Button UX Pattern - Hold-and-Release)

Verifies:
1. Modal buttons use key_held_* field (hold-and-release pattern)
2. Button scale animations use lerp (smooth transitions, not instant)
3. ADR-008 color constants used (COLOR_BUTTON_BG, COLOR_BUTTON_HOVER, etc.)
4. Font scale >= 1.0f for all button labels (readability)
5. Hotkey labels shown in format "(E)", "(S)", "[BACKSPACE]"
6. Back buttons use BACKSPACE hotkey (not ESC)
7. No instant activation without hold (anti-pattern check)
8. Mutex input: mouse blocked when key held (prevents double trigger)

Based on: architecture/decision_records/18_interactive_button_ux.md
"""

import re
import sys
from pathlib import Path
from typing import List, Dict, Tuple, Set

# ============================================================================
# CONSTANTS (from ADR-018)
# ============================================================================

# Required state fields for hold-and-release pattern
HOLD_RELEASE_FIELDS = [
    "key_held",  # Must track which key is held
    "button_scale",  # Must have scale for animation
]

# ADR-008/ADR-018 color constants
REQUIRED_COLOR_CONSTANTS = [
    "COLOR_BUTTON_BG",
    "COLOR_BUTTON_HOVER",
    "COLOR_BUTTON_PRESSED",
    "COLOR_EQUIP_ACCENT",
    "COLOR_SELL_ACCENT",
]

# Anti-patterns (instant activation without hold)
ANTI_PATTERNS = [
    r'if\s*\(app\.keyboard\[SDL_SCANCODE_\w+\]\)\s*\{[^}]*Perform\w+Action',  # Instant action on keypress
    r'if\s*\(.*_pressed\)\s*\{[^}]*return\s+true',  # Return true immediately on press (no release check)
]

# Modal and button file patterns
MODAL_PATTERNS = ['*Modal.c', '*Button.c', 'resultScreen.c', 'victoryOverlay.c']

# Minimum font scale (ADR-008 requirement)
MIN_FONT_SCALE = 1.0

# ============================================================================
# HELPER FUNCTIONS
# ============================================================================

def find_button_files(project_root: Path) -> List[Path]:
    """Find all files with interactive buttons"""
    files = []
    src_dir = project_root / 'src'

    for pattern in MODAL_PATTERNS:
        files.extend(src_dir.rglob(pattern))

    return sorted(set(files))

def has_interactive_buttons(content: str) -> bool:
    """Check if file has interactive buttons (not just display-only UI)"""
    # Look for button click handlers or hotkey checks
    has_click = 'mouse.pressed' in content or 'clicked' in content
    has_hotkey = 'app.keyboard[SDL_SCANCODE_' in content
    has_button_logic = 'button' in content.lower() and ('equip' in content.lower() or 'sell' in content.lower() or 'confirm' in content.lower())

    return (has_click or has_hotkey) and has_button_logic

# ============================================================================
# VERIFICATION FUNCTIONS
# ============================================================================

def check_hold_release_pattern(file_path: Path, content: str) -> List[str]:
    """Verify file uses hold-and-release pattern (key_held field)"""
    violations = []

    # Skip if file doesn't have interactive buttons
    if not has_interactive_buttons(content):
        return violations

    # Check for key_held field in struct or variables
    has_key_held = 'key_held' in content
    has_prev_key = 'prev_key' in content

    if not has_key_held:
        violations.append(
            "  ❌ Missing 'key_held' field (hold-and-release pattern required)"
        )

    if has_key_held and not has_prev_key:
        violations.append(
            "  ⚠️  Has 'key_held' but missing 'prev_key' comparison (release detection incomplete)"
        )

    # Check for release detection pattern (prev != current or prev == X && current != X)
    if has_key_held and has_prev_key:
        # Allow both patterns:
        # 1. prev != current
        # 2. prev == 0 && current != 0 (more specific release check)
        release_pattern1 = r'prev_key.*!=.*key_held|key_held.*!=.*prev'
        release_pattern2 = r'prev.*==\s*\d+.*&&.*key_held.*!=\s*\d+'
        if not re.search(release_pattern1, content) and not re.search(release_pattern2, content):
            violations.append(
                "  ⚠️  Has key_held/prev_key but no release detection (prev != current)"
            )

    return violations

def check_button_scale_lerp(file_path: Path, content: str) -> List[str]:
    """Verify button scale uses lerp (smooth animation, not instant snap)"""
    violations = []

    if not has_interactive_buttons(content):
        return violations

    has_button_scale = 'button_scale' in content or '_scale' in content

    if has_button_scale:
        # Check for lerp pattern (scale += (target - scale) * speed * dt)
        lerp_pattern = r'scale\s*\+\=\s*\([^)]+\)\s*\*\s*\w+\s*\*\s*dt'
        has_lerp = re.search(lerp_pattern, content)

        # Check for instant snap (scale = 1.0f without interpolation)
        instant_snap_pattern = r'scale\s*=\s*[\d\.]+f?\s*;'
        has_instant = re.search(instant_snap_pattern, content)

        if not has_lerp and has_instant:
            violations.append(
                "  ❌ Button scale uses instant snap (scale = value) instead of lerp"
            )
            violations.append(
                "     Required pattern: button_scale += (target_scale - button_scale) * 10.0f * dt"
            )

    return violations

def check_color_constants(file_path: Path, content: str) -> List[str]:
    """Verify ADR-008 color constants used"""
    violations = []

    if not has_interactive_buttons(content):
        return violations

    # Check for hardcoded colors in button rendering
    hardcoded_colors = re.findall(r'a_DrawFilledRect\([^)]*\(\(aColor_t\)\s*\{\s*\d+\s*,\s*\d+\s*,\s*\d+', content)

    if hardcoded_colors:
        # Check if COLOR_BUTTON_* constants are defined
        has_constants = any(const in content for const in REQUIRED_COLOR_CONSTANTS)

        if not has_constants:
            violations.append(
                "  ❌ Hardcoded button colors found - should use ADR-008 color constants"
            )
            violations.append(
                "     Required: #define COLOR_BUTTON_BG, COLOR_BUTTON_HOVER, etc."
            )

    return violations

def check_font_scale(file_path: Path, content: str) -> List[str]:
    """Verify font scale >= 1.0f (ADR-008 readability requirement)"""
    violations = []

    # Find button text rendering (a_DrawText for buttons only)
    # Look for button-related text (EQUIP, SELL, BACK, etc.) with font scale
    button_text_pattern = r'a_DrawText\s*\([^)]*(?:EQUIP|SELL|BACK|CONFIRM|SKIP|CANCEL)[^)]*\)'

    for match in re.finditer(button_text_pattern, content, re.IGNORECASE):
        button_text_block = match.group(0)

        # Look backwards to find the font config for this text
        text_pos = match.start()
        context = content[max(0, text_pos-500):text_pos]  # 500 chars before

        # Find scale in context
        scale_match = re.findall(r'\.scale\s*=\s*(\d+\.\d+)f?', context)
        if scale_match:
            scale_value = float(scale_match[-1])  # Last scale before this text

            if scale_value < MIN_FONT_SCALE:
                line_num = content[:match.start()].count('\n') + 1
                violations.append(
                    f"  ❌ Line {line_num}: Button font scale {scale_value}f < {MIN_FONT_SCALE}f (too small)"
                )

    return violations

def check_hotkey_labels(file_path: Path, content: str) -> List[str]:
    """Verify hotkeys shown in button labels in standard format"""
    violations = []

    if not has_interactive_buttons(content):
        return violations

    # Find button text rendering (a_DrawText calls for buttons)
    draw_text_pattern = r'a_DrawText\s*\(\s*"([^"]+)"'

    button_texts = re.findall(draw_text_pattern, content)

    # Check for buttons with obvious actions but no hotkey label
    for text in button_texts:
        text_lower = text.lower()

        # Skip non-button text
        if len(text) < 3 or len(text) > 50:
            continue

        # Check if it's a button label (equip, sell, back, confirm, etc.)
        is_button = any(keyword in text_lower for keyword in ['equip', 'sell', 'back', 'confirm', 'skip', 'cancel'])

        if is_button:
            # Check if it has hotkey format
            has_hotkey = re.search(r'\([A-Z]\)|\[[\w]+\]', text)

            if not has_hotkey:
                violations.append(
                    f"  ⚠️  Button label '{text}' missing hotkey hint (e.g., 'EQUIP (E)')"
                )

    return violations

def check_backspace_for_back(file_path: Path, content: str) -> List[str]:
    """Verify back buttons use BACKSPACE (not ESC)"""
    violations = []

    if not has_interactive_buttons(content):
        return violations

    # Check for "back" functionality
    has_back_button = 'back' in content.lower() and 'button' in content.lower()

    if has_back_button:
        # Check if ESC is used for back (anti-pattern)
        esc_for_back_pattern = r'SDL_SCANCODE_ESCAPE.*back|back.*SDL_SCANCODE_ESCAPE'
        if re.search(esc_for_back_pattern, content, re.IGNORECASE):
            violations.append(
                "  ❌ Back button uses ESC hotkey (conflicts with menu)"
            )
            violations.append(
                "     Required: Use SDL_SCANCODE_BACKSPACE for back action"
            )

        # Check if BACKSPACE is properly used
        has_backspace = 'SDL_SCANCODE_BACKSPACE' in content
        if not has_backspace:
            violations.append(
                "  ⚠️  Back button found but no SDL_SCANCODE_BACKSPACE (should use BACKSPACE key)"
            )

    return violations

def check_anti_patterns(file_path: Path, content: str) -> List[str]:
    """Check for instant activation anti-patterns"""
    violations = []

    if not has_interactive_buttons(content):
        return violations

    for pattern in ANTI_PATTERNS:
        if re.search(pattern, content):
            violations.append(
                "  ⚠️  Possible instant activation pattern detected (should use hold-and-release)"
            )
            violations.append(
                "     Pattern: Immediate action on keypress without release check"
            )
            break  # Only report once per file

    return violations

def check_mutex_input(file_path: Path, content: str) -> List[str]:
    """Verify mouse clicks blocked when key held (prevents double trigger)"""
    violations = []

    if not has_interactive_buttons(content):
        return violations

    # Check if file has both mouse and keyboard input
    has_mouse = 'clicked' in content or 'mouse.pressed' in content
    has_keyboard = 'app.keyboard[' in content
    has_key_held = 'key_held' in content

    if has_mouse and has_keyboard and has_key_held:
        # Look for mutex pattern: key_held < 0 check before mouse action
        # Allow: key_held_button < 0, key_held < 0, key_held_index < 0
        mutex_pattern = r'key_held[a-z_]*\s*<\s*0'

        if not re.search(mutex_pattern, content):
            violations.append(
                "  ⚠️  Both mouse and keyboard input found but no mutex check"
            )
            violations.append(
                "     Required: Block mouse clicks when key held (key_held < 0)"
            )

    return violations

# ============================================================================
# MAIN VERIFICATION LOGIC
# ============================================================================

def verify_button_ux_consistency(project_root: Path) -> bool:
    """Main verification logic"""

    print("=" * 70)
    print("FF-018: Interactive Button UX Pattern Verification")
    print("=" * 70)
    print()

    # Find all button files
    button_files = find_button_files(project_root)

    # Filter to only files with interactive buttons
    interactive_files = [f for f in button_files if has_interactive_buttons(f.read_text())]

    if not interactive_files:
        print("⚠️  WARNING: No interactive button files found")
        return False

    print(f"📋 Found {len(interactive_files)} file(s) with interactive buttons:")
    for file in interactive_files:
        print(f"   - {file.relative_to(project_root)}")
    print()

    print("🔍 Verifying ADR-018 patterns:")
    print("   ✓ Hold-and-release pattern (key_held field + prev_key comparison)")
    print("   ✓ Button scale lerp (smooth animation, not instant snap)")
    print("   ✓ ADR-008 color constants (COLOR_BUTTON_BG, etc.)")
    print("   ✓ Font scale >= 1.0f (readability)")
    print("   ✓ Hotkey labels in standard format")
    print("   ✓ BACKSPACE for back (not ESC)")
    print("   ✓ No instant activation anti-patterns")
    print("   ✓ Mutex input (mouse blocked when key held)")
    print()

    all_violations = {}
    total_violations = 0

    for file_path in interactive_files:
        content = file_path.read_text()
        violations = []

        # Run all checks
        violations.extend(check_hold_release_pattern(file_path, content))
        violations.extend(check_button_scale_lerp(file_path, content))
        violations.extend(check_color_constants(file_path, content))
        violations.extend(check_font_scale(file_path, content))
        violations.extend(check_hotkey_labels(file_path, content))
        violations.extend(check_backspace_for_back(file_path, content))
        violations.extend(check_anti_patterns(file_path, content))
        violations.extend(check_mutex_input(file_path, content))

        if violations:
            all_violations[file_path.relative_to(project_root)] = violations
            total_violations += len(violations)

    # Report results
    if total_violations == 0:
        print("✅ SUCCESS: All interactive buttons follow ADR-018 pattern")
        print()
        print("Constitutional Pattern Verified:")
        print("  - Hold-and-release interaction (hold = highlight, release = activate)")
        print("  - Smooth button scale animation (lerp interpolation)")
        print("  - ADR-008 color palette (consistent blue/green/red)")
        print("  - Font scale >= 1.0f (readable labels)")
        print("  - Standard hotkey format (E, S, BACKSPACE)")
        print("  - Mutex input (keyboard OR mouse, never both)")
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
        print("1. Add hold-and-release pattern:")
        print("   int prev_key_held = modal->key_held_button;")
        print("   modal->key_held_button = -1;")
        print("   if (app.keyboard[SDL_SCANCODE_E]) modal->key_held_button = 0;")
        print()
        print("   if (prev_key_held == 0 && modal->key_held_button != 0) {")
        print("       PerformAction();  // Trigger on release")
        print("   }")
        print()
        print("2. Add button scale lerp:")
        print("   float target_scale = pressed ? 0.95f : (hovered ? 1.05f : 1.0f);")
        print("   modal->button_scale += (target_scale - modal->button_scale) * 10.0f * dt;")
        print()
        print("3. Use ADR-008 color constants:")
        print("   #define COLOR_BUTTON_BG ((aColor_t){37, 58, 94, 255})")
        print("   #define COLOR_BUTTON_HOVER ((aColor_t){60, 94, 139, 255})")
        print()
        print("4. Add mutex input check:")
        print("   if (clicked && modal->key_held_button < 0) {  // Only allow mouse if no key held")
        print("       PerformAction();")
        print("   }")
        print()

        return False

if __name__ == '__main__':
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_button_ux_consistency(project_root)
    sys.exit(0 if success else 1)
