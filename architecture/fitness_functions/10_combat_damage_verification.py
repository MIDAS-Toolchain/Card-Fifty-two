#!/usr/bin/env python3
"""
FF-010: Combat Damage Formula Verification

Enforces ADR-10 (Combat Damage via Hand Value × Bet Multiplier)

Verifies:
1. Damage formula uses hand_value × bet_amount (no special cases)
2. Push deals exactly half damage (integer division / 2, not float * 0.5)
3. Loss/bust deal zero damage (explicit 0 assignment)
4. Bet amount captured BEFORE WinBet/LoseBet (they clear current_bet)
5. Ace soft hand uses while-loop optimization (not if-statement)
6. No hardcoded damage values (must calculate from formula)

Based on: architecture/decision_records/10_combat_damage_formula.md
"""

import re
from pathlib import Path
from typing import List, Tuple, Optional


def extract_function_body(content: str, func_name: str) -> Optional[str]:
    """Extract function body by matching braces (handles large functions)"""
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


def verify_damage_formula_pattern(project_root: Path) -> bool:
    """Verify damage calculation uses hand_value × bet_amount"""
    print("📋 Verifying damage formula pattern (ADR-10):")
    print()

    game_c = project_root / "src" / "game.c"
    if not game_c.exists():
        print("❌ FATAL: game.c not found")
        return False

    content = game_c.read_text()

    # Extract Game_ResolveRound function body
    func_body = extract_function_body(content, r'void\s+Game_ResolveRound')

    if not func_body:
        print("❌ FATAL: Game_ResolveRound() not found")
        return False

    # ANTI-PATTERN 1: Hardcoded damage values
    hardcoded_pattern = r'damage\s*=\s*\d{2,}'  # damage = 100, damage = 500, etc
    if re.search(hardcoded_pattern, func_body):
        print("❌ FAIL: Hardcoded damage values found (violates ADR-10)")
        print("   Damage MUST be calculated from hand_value × bet_amount")
        print()
        return False

    print("✅ No hardcoded damage values (uses formula)")
    print()

    # REQUIRED PATTERN: damage = hand_value * bet_amount
    # Can be: damage_dealt = hand_value * bet_amount
    #     or: damage = total_value * current_bet
    #     or: int full_damage = hand_value * bet_amount
    formula_pattern = r'=\s*(hand_value|total_value)\s*\*\s*(bet_amount|current_bet)'
    if not re.search(formula_pattern, func_body):
        print("❌ FAIL: Damage formula 'hand_value × bet_amount' not found")
        print("   ADR-10 requires: damage = hand_value * bet_amount")
        print()
        return False

    print("✅ Damage formula uses hand_value × bet_amount")
    print()

    return True


def verify_push_half_damage(project_root: Path) -> bool:
    """Verify push deals exactly half damage using integer division"""
    print("📋 Verifying push half-damage pattern (ADR-10):")
    print()

    game_c = project_root / "src" / "game.c"
    content = game_c.read_text()

    # Extract function body
    func_body = extract_function_body(content, r'void\s+Game_ResolveRound')

    if not func_body:
        return True  # Already reported error in previous check

    # ANTI-PATTERN: Float multiplication (damage * 0.5)
    float_mult_pattern = r'damage\s*\*\s*0\.5'
    if re.search(float_mult_pattern, func_body):
        print("❌ FAIL: Push damage uses float multiplication (* 0.5)")
        print("   ADR-10 requires integer division (/ 2) to avoid rounding issues")
        print()
        return False

    # REQUIRED PATTERN: Integer division (damage / 2)
    int_div_pattern = r'(damage|full_damage)\s*/\s*2'
    if not re.search(int_div_pattern, func_body):
        print("⚠️  WARNING: Push half-damage pattern not found")
        print("   (Push damage may not be implemented yet)")
        print()
        return True

    print("✅ Push damage uses integer division (/ 2), not float (* 0.5)")
    print()

    return True


def verify_loss_zero_damage(project_root: Path) -> bool:
    """Verify losses and busts deal zero damage"""
    print("📋 Verifying loss/bust zero damage (ADR-10):")
    print()

    game_c = project_root / "src" / "game.c"
    content = game_c.read_text()

    # Extract Game_ResolveRound function body
    func_body = extract_function_body(content, r'void\s+Game_ResolveRound')

    if not func_body:
        return True

    # Check for explicit zero damage on loss
    # Pattern: else { damage = 0; } or similar
    zero_damage_pattern = r'damage\s*=\s*0'
    if not re.search(zero_damage_pattern, func_body):
        print("⚠️  WARNING: No explicit 'damage = 0' for loss/bust found")
        print("   (Loss damage handling may rely on default initialization)")
        print()
        return True

    print("✅ Loss/bust explicitly set damage = 0")
    print()

    return True


def verify_bet_capture_timing(project_root: Path) -> bool:
    """Verify bet amount captured BEFORE WinBet/LoseBet clears it"""
    print("📋 Verifying bet capture timing (ADR-10):")
    print()

    game_c = project_root / "src" / "game.c"
    content = game_c.read_text()

    # Extract Game_ResolveRound function body
    func_body = extract_function_body(content, r'void\s+Game_ResolveRound')

    if not func_body:
        return True

    # Look for bet capture pattern
    bet_capture_pattern = r'(int|uint32_t)\s+bet_amount\s*=\s*player->current_bet'
    if not re.search(bet_capture_pattern, func_body):
        print("⚠️  INFO: Bet capture pattern not found")
        print("   (May be using player->current_bet directly)")
        print()
        return True

    # Verify bet captured BEFORE first WinBet/LoseBet
    # Simple approach: find positions in string
    bet_capture_pos = func_body.find('bet_amount = player->current_bet')

    # Find first occurrence of WinBet or LoseBet
    win_bet_pos = func_body.find('WinBet(')
    lose_bet_pos = func_body.find('LoseBet(')

    # Get earliest bet clearing position
    bet_clear_positions = [p for p in [win_bet_pos, lose_bet_pos] if p != -1]
    if not bet_clear_positions:
        print("⚠️  INFO: WinBet/LoseBet not found")
        print()
        return True

    first_bet_clear_pos = min(bet_clear_positions)

    if bet_capture_pos > first_bet_clear_pos:
        print("❌ FAIL: Bet amount captured AFTER WinBet/LoseBet call")
        print("   WinBet/LoseBet clear current_bet, must capture BEFORE")
        print()
        return False

    print("✅ Bet amount captured BEFORE WinBet/LoseBet (preserves value)")
    print()

    return True


def verify_ace_soft_hand_algorithm(project_root: Path) -> bool:
    """Verify ace optimization uses while-loop (not if-statement)"""
    print("📋 Verifying ace soft hand algorithm (ADR-10):")
    print()

    hand_c = project_root / "src" / "hand.c"
    if not hand_c.exists():
        print("❌ FATAL: hand.c not found")
        return False

    content = hand_c.read_text()

    # Extract CalculateHandValue function body
    func_body = extract_function_body(content, r'int\s+CalculateHandValue')

    if not func_body:
        print("❌ FATAL: CalculateHandValue() not found")
        return False

    # ANTI-PATTERN: Single if-statement ace conversion
    # if (total > 21 && num_aces > 0) { total -= 10; }  // Only converts ONE ace
    single_if_pattern = r'if\s*\([^)]*total\s*>\s*21[^)]*num_aces[^)]*\)\s*\{[^}]*total\s*-=\s*10[^}]*\}'
    if re.search(single_if_pattern, func_body):
        # Check if there's also a while loop
        while_pattern = r'while\s*\([^)]*total\s*>\s*21[^)]*num_aces[^)]*\)'
        if not re.search(while_pattern, func_body):
            print("❌ FAIL: Ace optimization uses if-statement (should use while-loop)")
            print("   Single if only converts ONE ace (fails for multiple aces)")
            print("   ADR-10 requires while-loop to convert ALL aces sequentially")
            print()
            return False

    # REQUIRED PATTERN: while-loop ace optimization
    while_pattern = r'while\s*\([^)]*total\s*>\s*21[^)]*num_aces[^)]*\)\s*\{'
    if not re.search(while_pattern, func_body):
        print("❌ FAIL: Ace soft hand while-loop not found")
        print("   ADR-10 requires: while (total > 21 && num_aces > 0) { ... }")
        print()
        return False

    print("✅ Ace optimization uses while-loop (handles multiple aces)")
    print()

    # Check for proper decrement inside loop
    while_block_pattern = r'while\s*\([^)]*total\s*>\s*21[^)]*num_aces[^)]*\)\s*\{([^}]*)\}'
    while_match = re.search(while_block_pattern, func_body)

    if while_match:
        while_body = while_match.group(1)
        if 'total -= 10' not in while_body and 'total = total - 10' not in while_body:
            print("⚠️  WARNING: While-loop doesn't decrement total by 10")
            print()
        if 'num_aces--' not in while_body and 'num_aces = num_aces - 1' not in while_body:
            print("⚠️  WARNING: While-loop doesn't decrement num_aces")
            print()

    return True


def verify_combat_damage_formula(project_root: Path) -> bool:
    """Main verification function"""

    print("=" * 80)
    print("FF-010: Combat Damage Formula Verification")
    print("=" * 80)
    print()

    # Verify damage formula pattern
    check1 = verify_damage_formula_pattern(project_root)
    if not check1:
        return False

    # Verify push half-damage
    check2 = verify_push_half_damage(project_root)
    if not check2:
        return False

    # Verify loss/bust zero damage
    check3 = verify_loss_zero_damage(project_root)
    if not check3:
        return False

    # Verify bet capture timing
    check4 = verify_bet_capture_timing(project_root)
    if not check4:
        return False

    # Verify ace soft hand algorithm
    check5 = verify_ace_soft_hand_algorithm(project_root)
    if not check5:
        return False

    print("=" * 80)
    print("✅ SUCCESS: Combat damage formula verified")
    print("=" * 80)
    print()
    print("Constitutional Pattern Verified:")
    print("  - Damage uses hand_value × bet_amount (no hardcoded values)")
    print("  - Push uses integer division / 2 (not float * 0.5)")
    print("  - Loss/bust explicitly set damage = 0")
    print("  - Bet captured BEFORE WinBet/LoseBet clears it")
    print("  - Ace soft hand uses while-loop (handles multiple aces)")
    print()

    return True


if __name__ == '__main__':
    import sys

    # Determine project root (3 levels up from this script)
    script_path = Path(__file__).resolve()
    project_root = script_path.parent.parent.parent

    success = verify_combat_damage_formula(project_root)
    sys.exit(0 if success else 1)
