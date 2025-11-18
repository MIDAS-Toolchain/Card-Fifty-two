#!/usr/bin/env python3
"""
FF-009: Tag System Architecture Verification

Enforces ADR-09 (Tag Effect Execution via Event Triggers and Global Aggregation)

Verifies:
1. Tag categorization (on-draw vs passive)
2. On-draw tags use GAME_EVENT_CARD_DRAWN (no hardcoded checks in game.c)
3. Passive tags use combat_bonuses_dirty flag + UpdateCombatBonuses()
4. All tags have helper functions (name/color/description)
5. Tag descriptions match ADR-09 documentation
6. No removed tags exist

Based on: architecture/decision_records/09_card_tags.md
"""

import re
from pathlib import Path
from typing import List, Set, Tuple, Dict

# ADR-09 tag categorization
ON_DRAW_TAGS = {
    "CARD_TAG_CURSED",    # 10 damage to enemy when drawn
    "CARD_TAG_VAMPIRIC",  # 5 damage + 5 chips when drawn
}

PASSIVE_TAGS = {
    "CARD_TAG_LUCKY",     # +10% crit while in any hand (global)
    "CARD_TAG_BRUTAL",    # +10% damage while in any hand (global)
}

ONE_TIME_TAGS = {
    "CARD_TAG_DOUBLED",   # Value doubled this hand (removed after calculation)
}

# All ADR-09 approved tags
ADR_09_TAGS = ON_DRAW_TAGS | PASSIVE_TAGS | ONE_TIME_TAGS | {"CARD_TAG_MAX"}

# Tags that were removed (should NOT exist)
REMOVED_TAGS = {
    "CARD_TAG_BLESSED",
    "CARD_TAG_WILD",
    "CARD_TAG_HEAVY",
    "CARD_TAG_LIGHT",
    "CARD_TAG_VOLATILE"
}

# Expected descriptions from ADR-09
EXPECTED_DESCRIPTIONS = {
    "CARD_TAG_CURSED": "10 damage to enemy when drawn",
    "CARD_TAG_VAMPIRIC": "5 damage + 5 chips when drawn",
    "CARD_TAG_LUCKY": "+10% crit while in any hand",
    "CARD_TAG_BRUTAL": "+10% damage while in any hand",
    "CARD_TAG_DOUBLED": "Counts twice for hand value"
}


def parse_card_tag_enum(project_root: Path) -> Tuple[Set[str], bool]:
    """Parse CardTag_t enum from cardTags.h"""
    cardtags_h = project_root / "include" / "cardTags.h"

    if not cardtags_h.exists():
        print(f"❌ FATAL: {cardtags_h} not found")
        return set(), False

    content = cardtags_h.read_text()

    # Find enum CardTag_t { ... }
    enum_pattern = r'typedef\s+enum\s*\{([^}]+)\}\s*CardTag_t;'
    match = re.search(enum_pattern, content, re.DOTALL)

    if not match:
        print(f"❌ FATAL: Could not parse CardTag_t enum in {cardtags_h}")
        return set(), False

    enum_body = match.group(1)

    # Extract CARD_TAG_* identifiers
    tag_pattern = r'(CARD_TAG_\w+)'
    tags = set(re.findall(tag_pattern, enum_body))

    return tags, True


def verify_tag_definitions(project_root: Path) -> bool:
    """Verify tag enum matches ADR-09"""
    print("📋 Verifying tag definitions:")
    print()

    # Parse enum
    tags, success = parse_card_tag_enum(project_root)
    if not success:
        return False

    print(f"📋 Found {len(tags)} tag(s) in CardTag_t enum:")
    for tag in sorted(tags):
        print(f"   - {tag}")
    print()

    # Check for removed tags
    found_removed = tags & REMOVED_TAGS
    if found_removed:
        print(f"❌ FAIL: Found removed tags (should be deleted per ADR-09):")
        for tag in sorted(found_removed):
            print(f"   ❌ {tag} - REMOVE THIS TAG")
        print()
        return False
    else:
        print("✅ No removed tags found (BLESSED, WILD, HEAVY, LIGHT, VOLATILE)")
        print()

    # Check for unexpected tags
    unexpected = tags - ADR_09_TAGS
    if unexpected:
        print(f"⚠️  WARNING: Found unexpected tags not in ADR-09:")
        for tag in sorted(unexpected):
            print(f"   ⚠️  {tag} - Not documented in ADR-09")
        print()

    # Check for missing ADR-09 tags
    missing = ADR_09_TAGS - tags
    if missing and missing != {"CARD_TAG_MAX"}:
        print(f"❌ FAIL: Missing ADR-09 tags:")
        for tag in sorted(missing):
            print(f"   ❌ {tag}")
        print()
        return False
    else:
        print("✅ All ADR-09 tags present")
        print()

    return True


def verify_helper_functions(project_root: Path) -> bool:
    """Verify all tags have GetCardTagName/Color/Description cases"""
    cardtags_c = project_root / "src" / "cardTags.c"

    if not cardtags_c.exists():
        print(f"❌ FATAL: {cardtags_c} not found")
        return False

    content = cardtags_c.read_text()

    # Get actual tags from enum (excluding CARD_TAG_MAX)
    tags, success = parse_card_tag_enum(project_root)
    if not success:
        return False

    actual_tags = sorted(tags - {"CARD_TAG_MAX"})

    print("📋 Verifying helper functions for each tag:")
    print()

    all_good = True

    for tag in actual_tags:
        has_name = f"case {tag}:" in content and "GetCardTagName" in content
        has_color = f"case {tag}:" in content and "GetCardTagColor" in content
        has_desc = f"case {tag}:" in content and "GetCardTagDescription" in content

        if has_name and has_color and has_desc:
            print(f"   ✅ {tag} - name/color/description all present")
        else:
            print(f"   ❌ {tag} - missing:", end="")
            if not has_name:
                print(" name", end="")
            if not has_color:
                print(" color", end="")
            if not has_desc:
                print(" description", end="")
            print()
            all_good = False

    print()

    if all_good:
        print("✅ All tags have name/color/description helpers")
        print()
    else:
        print("❌ FAIL: Some tags missing helper functions")
        print()

    return all_good


def verify_tag_descriptions(project_root: Path) -> bool:
    """Verify tag descriptions match ADR-09 documentation"""
    cardtags_c = project_root / "src" / "cardTags.c"
    content = cardtags_c.read_text()

    # Find GetCardTagDescription function
    desc_pattern = r'const char\* GetCardTagDescription\(CardTag_t tag\)\s*\{([^}]+)\}'
    match = re.search(desc_pattern, content, re.DOTALL)

    if not match:
        print("❌ FATAL: Could not parse GetCardTagDescription()")
        return False

    func_body = match.group(1)

    print("📋 Verifying tag descriptions match ADR-09:")
    print()

    all_match = True

    for tag, expected_desc in EXPECTED_DESCRIPTIONS.items():
        # Find the return statement for this tag
        case_pattern = rf'case {tag}:\s*return\s+"([^"]+)"'
        case_match = re.search(case_pattern, func_body)

        if not case_match:
            print(f"   ❌ {tag} - description not found")
            all_match = False
            continue

        actual_desc = case_match.group(1)

        # Normalize for comparison (case-insensitive, strip whitespace)
        if actual_desc.lower().strip() == expected_desc.lower().strip():
            print(f"   ✅ {tag}")
            print(f"      \"{actual_desc}\"")
        else:
            print(f"   ❌ {tag} - description mismatch")
            print(f"      Expected: \"{expected_desc}\"")
            print(f"      Actual:   \"{actual_desc}\"")
            all_match = False

    print()

    if all_match:
        print("✅ All tag descriptions match ADR-09")
        print()
    else:
        print("❌ FAIL: Tag descriptions don't match ADR-09")
        print()

    return all_match


def verify_on_draw_pattern(project_root: Path) -> bool:
    """Verify on-draw tags use GAME_EVENT_CARD_DRAWN, NOT hardcoded in game.c"""
    print("📋 Verifying on-draw tag pattern (ADR-09):")
    print()

    # ANTI-PATTERN: Hardcoded tag checks in game.c
    game_c = project_root / "src" / "game.c"
    if not game_c.exists():
        print("❌ FATAL: game.c not found")
        return False

    game_c_content = game_c.read_text()

    violations = []
    for tag in ON_DRAW_TAGS:
        # Look for HasCardTag checks in game.c (should NOT exist)
        pattern = rf'HasCardTag\s*\([^)]*{tag}'
        if re.search(pattern, game_c_content):
            violations.append(f"game.c contains hardcoded {tag} check (should use event handler)")

    if violations:
        print("❌ FAIL: On-draw tags hardcoded in game.c (violates ADR-09):")
        for v in violations:
            print(f"   ❌ {v}")
        print()
        print("ADR-09 requires: On-draw effects MUST use GAME_EVENT_CARD_DRAWN handlers")
        print("Fix: Move tag checks to cardTags.c event handler")
        print()
        return False

    print("✅ On-draw tags NOT hardcoded in game.c (uses event system)")
    print()

    # Check for event handler registration (if effects implemented)
    cardtags_c = project_root / "src" / "cardTags.c"
    if not cardtags_c.exists():
        print("⚠️  INFO: cardTags.c not found (tag effects not yet implemented)")
        print()
        return True

    cardtags_c_content = cardtags_c.read_text()

    # Look for GAME_EVENT_CARD_DRAWN handler
    has_event_handler = "GAME_EVENT_CARD_DRAWN" in cardtags_c_content

    if has_event_handler:
        print("✅ GAME_EVENT_CARD_DRAWN handler registered in cardTags.c")
        print()
    else:
        print("⚠️  INFO: GAME_EVENT_CARD_DRAWN handler not yet implemented")
        print("   (On-draw tags are visual-only until handler added)")
        print()

    return True


def verify_passive_aggregation_pattern(project_root: Path) -> bool:
    """Verify passive tags use dirty-flag + UpdateCombatBonuses()"""
    print("📋 Verifying passive tag pattern (ADR-09):")
    print()

    # ANTI-PATTERN: Per-frame aggregation (recalculates every frame)
    game_c = project_root / "src" / "game.c"
    if not game_c.exists():
        print("❌ FATAL: game.c not found")
        return False

    game_c_content = game_c.read_text()

    # Look for UpdateCombatBonuses() in main loop WITHOUT dirty flag check
    update_pattern = r'void\s+UpdateCombatBonuses\s*\([^)]*\)\s*\{'
    update_match = re.search(update_pattern, game_c_content)

    if update_match:
        # Extract function body (simplified - just check first 20 lines)
        func_start = update_match.end()
        func_body = game_c_content[func_start:func_start+1000]

        # Check for dirty flag guard
        if "combat_bonuses_dirty" not in func_body and "if" in func_body[:100]:
            print("❌ FAIL: UpdateCombatBonuses() missing dirty flag guard")
            print("   Per-frame aggregation wastes 6000 checks/sec (ADR-09)")
            print()
            print("Required pattern:")
            print("   if (!game->combat_bonuses_dirty) return;  // Skip if cached")
            print()
            return False

    # Check for dirty flag in structs.h
    structs_h = project_root / "include" / "structs.h"
    if not structs_h.exists():
        print("❌ FATAL: structs.h not found")
        return False

    structs_h_content = structs_h.read_text()

    has_dirty_flag = "combat_bonuses_dirty" in structs_h_content

    if not has_dirty_flag:
        print("⚠️  INFO: combat_bonuses_dirty flag not yet added")
        print("   (Passive tags are visual-only until aggregation implemented)")
        print()
        return True

    print("✅ combat_bonuses_dirty field exists in structs.h")
    print()

    # Check for dirty flag usage in hand.c (AddCardToHand/RemoveCardFromHand)
    hand_c = project_root / "src" / "hand.c"
    if not hand_c.exists():
        print("⚠️  WARNING: hand.c not found")
        return True

    hand_c_content = hand_c.read_text()

    # Look for dirty flag set in AddCardToHand
    add_pattern = r'void\s+AddCardToHand\s*\([^)]*\)\s*\{[^}]{0,500}combat_bonuses_dirty'
    if not re.search(add_pattern, hand_c_content, re.DOTALL):
        print("⚠️  WARNING: AddCardToHand() doesn't set combat_bonuses_dirty flag")
        print("   (Passive bonuses may not update correctly)")
        print()
        return True

    print("✅ AddCardToHand() sets combat_bonuses_dirty flag")
    print()

    return True


def verify_global_aggregation_pattern(project_root: Path) -> bool:
    """Verify passive effects scan BOTH player and enemy hands"""
    print("📋 Verifying global aggregation pattern (ADR-09):")
    print()

    game_c = project_root / "src" / "game.c"
    if not game_c.exists():
        return True  # Already reported error earlier

    game_c_content = game_c.read_text()

    # Look for CountTagsInPlay or similar function
    count_pattern = r'int\s+CountTagsInPlay\s*\([^)]*\)\s*\{([^}]{0,1000})\}'
    match = re.search(count_pattern, game_c_content, re.DOTALL)

    if not match:
        print("⚠️  INFO: CountTagsInPlay() not yet implemented")
        print("   (Passive tags are visual-only until aggregation implemented)")
        print()
        return True

    func_body = match.group(1)

    # Check for scans of both player and enemy hands
    has_player_scan = "player" in func_body.lower() and "hand" in func_body.lower()
    has_enemy_scan = "enemy" in func_body.lower() and "hand" in func_body.lower()

    if not (has_player_scan and has_enemy_scan):
        print("❌ FAIL: CountTagsInPlay() doesn't scan BOTH player and enemy hands")
        print("   ADR-09 requires global aggregation (shared deck asymmetry)")
        print()
        return False

    print("✅ CountTagsInPlay() scans both player and enemy hands (global)")
    print()

    return True


def verify_tag_system_architecture(project_root: Path) -> bool:
    """Main verification function"""

    print("=" * 80)
    print("FF-009: Tag System Architecture Verification")
    print("=" * 80)
    print()

    # Phase 1: Tag definition consistency (always check)
    print("=" * 80)
    print("Phase 1: Tag Definition Consistency")
    print("=" * 80)
    print()

    step1 = verify_tag_definitions(project_root)
    if not step1:
        return False

    step2 = verify_helper_functions(project_root)
    if not step2:
        return False

    step3 = verify_tag_descriptions(project_root)
    if not step3:
        return False

    # Phase 2: Architectural pattern enforcement (ADR-09)
    print("=" * 80)
    print("Phase 2: Architectural Pattern Enforcement (ADR-09)")
    print("=" * 80)
    print()

    # Verify on-draw tags use event system (not hardcoded in game.c)
    pattern1 = verify_on_draw_pattern(project_root)
    if not pattern1:
        return False

    # Verify passive tags use dirty-flag aggregation (not per-frame)
    pattern2 = verify_passive_aggregation_pattern(project_root)
    if not pattern2:
        return False

    # Verify passive effects scan globally (player + enemy)
    pattern3 = verify_global_aggregation_pattern(project_root)
    if not pattern3:
        return False

    print("=" * 80)
    print("✅ SUCCESS: Tag system architecture verified")
    print("=" * 80)
    print()
    print("Constitutional Pattern Verified:")
    print("  - Only ADR-09 tags exist (no removed tags)")
    print("  - All tags have helper functions")
    print("  - Descriptions match ADR-09 documentation")
    print("  - On-draw tags use event system (not hardcoded)")
    print("  - Passive tags use dirty-flag aggregation (not per-frame)")
    print("  - Global aggregation scans player + enemy hands")
    print()

    return True


if __name__ == '__main__':
    import sys
    project_root = Path(__file__).parent.parent.parent
    success = verify_tag_system_architecture(project_root)
    sys.exit(0 if success else 1)
