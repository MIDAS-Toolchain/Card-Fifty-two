#!/usr/bin/env python3
"""
FF-002: Status Effect Modifier Coverage Verification

Verifies that ModifyWinnings() and ModifyLosses() are called in ALL payout branches
of Game_ResolveRound().

Based on: ADR-002 (Status Effects as Outcome Modifiers)

Architecture Characteristic: Consistency
Domain Knowledge Required: None (purely structural analysis)
"""

import re
from pathlib import Path
from typing import List, Tuple, Optional


def extract_function_body(file_path: Path, function_name: str) -> Optional[Tuple[str, int]]:
    """
    Extract the body of a C function from a file.

    Returns: (function_body_text, start_line_number) or None if not found
    """
    with open(file_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    # Find function start: return_type function_name(
    func_pattern = re.compile(rf'\b{re.escape(function_name)}\s*\(')

    func_start_line = None
    for i, line in enumerate(lines):
        if func_pattern.search(line):
            func_start_line = i
            break

    if func_start_line is None:
        return None

    # Find opening brace (might be on next line)
    brace_count = 0
    body_start = None

    for i in range(func_start_line, len(lines)):
        for char in lines[i]:
            if char == '{':
                if brace_count == 0:
                    body_start = i
                brace_count += 1
            elif char == '}':
                brace_count -= 1
                if brace_count == 0:
                    # Found matching closing brace
                    body_lines = lines[body_start:i+1]
                    return (''.join(body_lines), body_start + 1)  # +1 for 1-indexed

    return None


def find_payout_branches(function_body: str, start_line: int) -> Tuple[List[int], List[int]]:
    """
    Find all branches that call WinBet or LoseBet.

    Returns: (win_branch_lines, loss_branch_lines)
    """
    lines = function_body.split('\n')

    win_branches = []
    loss_branches = []

    for i, line in enumerate(lines):
        actual_line = start_line + i

        # Look for direct chip modifications (the actual pattern used in this codebase)
        # Pattern: player->chips += ... (win)
        # Pattern: player->chips -= ... OR chip deduction (loss)

        # Check for WinBet calls (might be present)
        if re.search(r'\bWinBet\s*\(', line):
            win_branches.append(actual_line)

        # Check for LoseBet calls (might be present)
        if re.search(r'\bLoseBet\s*\(', line):
            loss_branches.append(actual_line)

        # Check for direct chip additions (player->chips +=)
        if re.search(r'player\s*->\s*chips\s*\+=', line):
            # Only count as win if it's in payout context (has 'bet' or 'modified' or 'winnings')
            if re.search(r'\b(bet|modified|winnings)\b', line):
                win_branches.append(actual_line)

        # Check for direct chip subtractions (player->chips -=)
        if re.search(r'player\s*->\s*chips\s*-=', line):
            loss_branches.append(actual_line)

    return (win_branches, loss_branches)


def check_modifier_near_payout(
    function_body: str,
    start_line: int,
    payout_line: int,
    modifier_name: str,
    search_window: int = 30
) -> bool:
    """
    Check if modifier_name is called within search_window lines before OR after payout_line.

    This accounts for patterns like:
    - ModifyWinnings() BEFORE player->chips +=
    - LoseBet() FOLLOWED BY ModifyLosses()
    """
    lines = function_body.split('\n')

    # Convert absolute line to relative line within function body
    relative_line = payout_line - start_line

    if relative_line < 0 or relative_line >= len(lines):
        return False

    # Search both backward and forward from payout line
    search_start = max(0, relative_line - search_window)
    search_end = min(len(lines), relative_line + search_window + 1)
    search_region = lines[search_start:search_end]

    # Look for modifier call
    modifier_pattern = re.compile(rf'\b{re.escape(modifier_name)}\s*\(')

    for line in search_region:
        if modifier_pattern.search(line):
            return True

    return False


def verify_status_effect_modifier_coverage(project_root: Path) -> bool:
    """
    Main verification function for FF-002.

    Returns: True if all payout branches have appropriate modifiers, False otherwise
    """
    print("=" * 80)
    print("FF-002: Status Effect Modifier Coverage Verification")
    print("=" * 80)

    game_c_path = project_root / "src" / "game.c"

    if not game_c_path.exists():
        print(f"❌ FAILURE: Could not find {game_c_path}")
        return False

    # Extract Game_ResolveRound function
    result = extract_function_body(game_c_path, "Game_ResolveRound")

    if result is None:
        print(f"❌ FAILURE: Could not find Game_ResolveRound() in {game_c_path}")
        return False

    function_body, start_line = result
    print(f"✓ Found Game_ResolveRound() at line {start_line}")

    # Find all payout branches
    win_branches, loss_branches = find_payout_branches(function_body, start_line)

    print(f"✓ Found {len(win_branches)} win branches (chip additions)")
    print(f"✓ Found {len(loss_branches)} loss branches (chip deductions)")

    if len(win_branches) == 0 and len(loss_branches) == 0:
        print("⚠ WARNING: No payout branches found. Is Game_ResolveRound() implemented?")
        return False

    failures = []

    # Check each win branch has ModifyWinnings
    for branch_line in win_branches:
        has_modifier = check_modifier_near_payout(
            function_body,
            start_line,
            branch_line,
            "ModifyWinnings",
            search_window=30
        )

        if not has_modifier:
            failures.append(f"Win branch at game.c:{branch_line} missing ModifyWinnings()")

    # Check each loss branch has ModifyLosses
    for branch_line in loss_branches:
        has_modifier = check_modifier_near_payout(
            function_body,
            start_line,
            branch_line,
            "ModifyLosses",
            search_window=30
        )

        if not has_modifier:
            failures.append(f"Loss branch at game.c:{branch_line} missing ModifyLosses()")

    # Report results
    print()
    if failures:
        print("❌ FAILURE: Missing status effect modifiers!")
        for failure in failures:
            print(f"   🚨 {failure}")
        print()
        print("💡 Fix: Ensure all chip additions call ModifyWinnings() first")
        print("💡 Fix: Ensure all chip deductions call ModifyLosses() first")
        return False

    print(f"✅ SUCCESS: All {len(win_branches)} win branches have ModifyWinnings()")
    print(f"✅ SUCCESS: All {len(loss_branches)} loss branches have ModifyLosses()")
    print()

    return True


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_status_effect_modifier_coverage(project_root)
    exit(0 if success else 1)
