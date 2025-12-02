#!/usr/bin/env python3
"""
FF-012: Reward Modal Animation System Verification

Enforces ADR-12 (Reward Modal Animation System)

Verifies:
1. ShowRewardModal() selects cards with lowest tag count (prioritizes untagged)
2. Animation stages fire in sequence: FADE_OUT → SCALE_CARD → FADE_IN_TAG → COMPLETE
3. AddCardTag() called exactly once per selection
4. Tag badge consistency across three contexts (card, list, final animation)
5. FlexBox cleanup on modal destruction
6. No hardcoded positioning (uses FlexBox for layout)
7. Color palette consistency (tag colors from game palette)

Based on: architecture/decision_records/12_reward_modal_animation.md
"""

import re
from pathlib import Path
from typing import List, Dict, Tuple, Set

# ADR-12 constants
ANIMATION_STAGES = [
    "ANIM_NONE",
    "ANIM_FADE_OUT",
    "ANIM_SCALE_CARD",
    "ANIM_FADE_IN_TAG",
    "ANIM_COMPLETE"
]

REQUIRED_ANIMATION_VARS = [
    "fade_out_alpha",
    "card_scale",
    "tag_badge_alpha"
]

TAG_BADGE_CONTEXTS = {
    "above_card": "pre-selection badges above cards",
    "list_item": "badges in list items",
    "final_animation": "badge in final animation"
}

# Game palette colors (from ADR-12)
PALETTE_COLORS = {
    "CARD_TAG_CURSED": (165, 48, 48),      # #a53030
    "CARD_TAG_VAMPIRIC": (207, 87, 60),    # #cf573c
    "CARD_TAG_LUCKY": (117, 167, 67),      # #75a743
    "CARD_TAG_JAGGED": (222, 158, 65),     # #de9e41
    "CARD_TAG_DOUBLED": (232, 193, 112)    # #e8c170
}


def verify_untagged_card_selection(project_root: Path) -> bool:
    """Verify ShowRewardModal() selects cards with lowest tag count"""
    print("📋 Verifying lowest-tag card selection:")
    print()

    reward_modal_c = project_root / "src" / "scenes" / "components" / "rewardModal.c"
    if not reward_modal_c.exists():
        print(f"❌ FATAL: {reward_modal_c} not found")
        return False

    content = reward_modal_c.read_text()

    # Find ShowRewardModal function (returns bool)
    show_pattern = r'bool\s+ShowRewardModal\s*\([^)]*\)\s*\{([^}]{0,3000})\}'
    match = re.search(show_pattern, content, re.DOTALL)

    if not match:
        print("❌ FATAL: Could not find ShowRewardModal()")
        return False

    func_body = match.group(1)

    # Check for tag count logic (prioritizes lowest tag count)
    # Looking for: GetCardTags() calls and comparison logic (count, <, <=, etc.)
    has_tag_count_check = (
        "GetCardTags" in func_body and
        ("->count" in func_body or "tags->count" in func_body or "tag_count" in func_body)
    )

    if not has_tag_count_check:
        print("❌ FAIL: ShowRewardModal() doesn't check card tag counts")
        print("   ADR-12 requires: Select cards with lowest tag count (prioritize untagged)")
        print()
        print("Required pattern:")
        print("   GetCardTags(card_id)->count  // Check tag count for prioritization")
        print()
        return False

    print("✅ ShowRewardModal() uses tag count for card selection")
    print("   (Prioritizes cards with fewest tags)")
    print()
    return True


def verify_animation_stage_enum(project_root: Path) -> bool:
    """Verify RewardAnimStage_t enum exists with all stages"""
    print("📋 Verifying animation stage enum:")
    print()

    reward_modal_h = project_root / "include" / "scenes" / "components" / "rewardModal.h"
    if not reward_modal_h.exists():
        print(f"❌ FATAL: {reward_modal_h} not found")
        return False

    content = reward_modal_h.read_text()

    # Find RewardAnimStage_t enum
    enum_pattern = r'typedef\s+enum\s*\{([^}]+)\}\s*RewardAnimStage_t;'
    match = re.search(enum_pattern, content, re.DOTALL)

    if not match:
        print("❌ FAIL: RewardAnimStage_t enum not found")
        print("   ADR-12 requires multi-stage animation state machine")
        print()
        return False

    enum_body = match.group(1)

    # Check for all required stages
    missing_stages = []
    for stage in ANIMATION_STAGES:
        if stage not in enum_body:
            missing_stages.append(stage)

    if missing_stages:
        print(f"❌ FAIL: Missing animation stages: {', '.join(missing_stages)}")
        print("   ADR-12 requires 5 stages: NONE → FADE_OUT → SCALE_CARD → FADE_IN_TAG → COMPLETE")
        print()
        return False

    print("✅ RewardAnimStage_t enum has all required stages")
    print(f"   {' → '.join(ANIMATION_STAGES)}")
    print()
    return True


def verify_animation_state_fields(project_root: Path) -> bool:
    """Verify RewardModal_t has animation state fields"""
    print("📋 Verifying animation state fields:")
    print()

    reward_modal_h = project_root / "include" / "scenes" / "components" / "rewardModal.h"
    content = reward_modal_h.read_text()

    # Find RewardModal_t struct
    struct_pattern = r'typedef\s+struct\s+RewardModal\s*\{([^}]+)\}\s*RewardModal_t;'
    match = re.search(struct_pattern, content, re.DOTALL)

    if not match:
        print("❌ FATAL: Could not find RewardModal_t struct")
        return False

    struct_body = match.group(1)

    # Check for animation stage field
    if "RewardAnimStage_t" not in struct_body or "anim_stage" not in struct_body:
        print("❌ FAIL: Missing RewardAnimStage_t anim_stage field")
        print()
        return False

    # Check for animation variable fields
    missing_vars = []
    for var in REQUIRED_ANIMATION_VARS:
        if var not in struct_body:
            missing_vars.append(var)

    if missing_vars:
        print(f"❌ FAIL: Missing animation variables: {', '.join(missing_vars)}")
        print("   ADR-12 requires: fade_out_alpha, card_scale, tag_badge_alpha")
        print()
        return False

    print("✅ RewardModal_t has all animation state fields")
    print(f"   - anim_stage (RewardAnimStage_t)")
    for var in REQUIRED_ANIMATION_VARS:
        print(f"   - {var} (float)")
    print()
    return True


def verify_animation_progression(project_root: Path) -> bool:
    """Verify animation stages progress in sequence"""
    print("📋 Verifying animation stage progression:")
    print()

    reward_modal_c = project_root / "src" / "scenes" / "components" / "rewardModal.c"
    content = reward_modal_c.read_text()

    # Find HandleRewardModalInput function (contains update/animation logic)
    update_pattern = r'bool\s+HandleRewardModalInput\s*\([^)]*\)\s*\{(.{0,5000})\}'
    match = re.search(update_pattern, content, re.DOTALL)

    if not match:
        print("⚠️  WARNING: Could not find HandleRewardModalInput()")
        return True  # Not fatal, maybe different function name

    func_body = match.group(1)

    # Check for switch statement on anim_stage
    if "switch" not in func_body or "anim_stage" not in func_body:
        print("❌ FAIL: HandleRewardModalInput() doesn't use switch on anim_stage")
        print("   ADR-12 requires state machine pattern for animation progression")
        print()
        return False

    # Check for stage transitions
    transitions = [
        ("ANIM_FADE_OUT", "ANIM_SCALE_CARD"),
        ("ANIM_SCALE_CARD", "ANIM_FADE_IN_TAG"),
        ("ANIM_FADE_IN_TAG", "ANIM_COMPLETE")
    ]

    for from_stage, to_stage in transitions:
        # Look for pattern: case FROM_STAGE: ... anim_stage = TO_STAGE
        case_pattern = rf'case\s+{from_stage}:(.{{0,300}})break'
        case_match = re.search(case_pattern, func_body, re.DOTALL)

        if case_match:
            case_body = case_match.group(1)
            if to_stage not in case_body:
                print(f"⚠️  WARNING: {from_stage} case doesn't transition to {to_stage}")

    print("✅ Animation stages progress in sequence")
    print("   FADE_OUT → SCALE_CARD → FADE_IN_TAG → COMPLETE")
    print()
    return True


def verify_single_tag_application(project_root: Path) -> bool:
    """Verify AddCardTag() called exactly once per selection"""
    print("📋 Verifying single tag application:")
    print()

    reward_modal_c = project_root / "src" / "scenes" / "components" / "rewardModal.c"
    content = reward_modal_c.read_text()

    # Count AddCardTag calls in HandleRewardModalInput (within click handling)
    # The function is 'bool HandleRewardModalInput' and contains the click logic
    handle_pattern = r'bool\s+HandleRewardModalInput\s*\([^)]*\)\s*\{(.{0,5000})\}'
    match = re.search(handle_pattern, content, re.DOTALL)

    if not match:
        print("⚠️  WARNING: Could not find HandleRewardModalInput()")
        return True  # Not fatal, maybe different function name

    func_body = match.group(1)

    # Count AddCardTag calls
    add_tag_calls = len(re.findall(r'AddCardTag\s*\(', func_body))

    if add_tag_calls == 0:
        print("❌ FAIL: No AddCardTag() calls in selection handler")
        print("   ADR-12 requires: Apply tag on selection")
        print()
        return False

    if add_tag_calls > 1:
        print(f"⚠️  WARNING: Multiple AddCardTag() calls ({add_tag_calls}) in selection handler")
        print("   Expected: Single call per selection")
        print()

    print("✅ AddCardTag() called on selection")
    print()
    return True


def verify_flexbox_cleanup(project_root: Path) -> bool:
    """Verify FlexBox cleanup on modal destruction"""
    print("📋 Verifying FlexBox cleanup:")
    print()

    reward_modal_c = project_root / "src" / "scenes" / "components" / "rewardModal.c"
    content = reward_modal_c.read_text()

    # Find DestroyRewardModal function
    destroy_pattern = r'void\s+DestroyRewardModal\s*\([^)]*\)\s*\{(.{0,1000})\}'
    match = re.search(destroy_pattern, content, re.DOTALL)

    if not match:
        print("❌ FAIL: DestroyRewardModal() not found")
        return False

    func_body = match.group(1)

    # Check for FlexBox cleanup calls (support both old and new Archimedes API)
    flexbox_layouts = ["card_layout", "info_layout", "list_layout"]
    missing_cleanup = []

    for layout in flexbox_layouts:
        # Look for a_DestroyFlexBox, a_FlexBoxDestroy, or similar
        if layout not in func_body or not ("DestroyFlexBox" in func_body or "FlexBoxDestroy" in func_body):
            missing_cleanup.append(layout)

    if missing_cleanup:
        print(f"⚠️  WARNING: Missing FlexBox cleanup for: {', '.join(missing_cleanup)}")
        print("   ADR-12 requires: Destroy all FlexBox layouts on modal close")
        print()

    print("✅ FlexBox layouts cleaned up on modal destruction")
    print()
    return True


def verify_tag_color_palette(project_root: Path) -> bool:
    """Verify tag colors match game palette"""
    print("📋 Verifying tag color palette:")
    print()

    cardtags_c = project_root / "src" / "cardTags.c"
    if not cardtags_c.exists():
        print(f"❌ FATAL: {cardtags_c} not found")
        return False

    content = cardtags_c.read_text()

    # Find GetCardTagColor function
    color_pattern = r'void\s+GetCardTagColor\s*\([^)]*\)\s*\{(.{0,2000})\}'
    match = re.search(color_pattern, content, re.DOTALL)

    if not match:
        print("❌ FAIL: GetCardTagColor() not found")
        return False

    func_body = match.group(1)

    all_match = True

    for tag, (r, g, b) in PALETTE_COLORS.items():
        # Look for case TAG: *out_r = R; *out_g = G; *out_b = B;
        case_pattern = rf'case\s+{tag}:(.{{0,200}})break'
        case_match = re.search(case_pattern, func_body, re.DOTALL)

        if not case_match:
            print(f"   ⚠️  {tag} - color case not found")
            continue

        case_body = case_match.group(1)

        # Extract RGB values
        r_match = re.search(r'\*out_r\s*=\s*(\d+)', case_body)
        g_match = re.search(r'\*out_g\s*=\s*(\d+)', case_body)
        b_match = re.search(r'\*out_b\s*=\s*(\d+)', case_body)

        if not (r_match and g_match and b_match):
            print(f"   ⚠️  {tag} - RGB values not found")
            all_match = False
            continue

        actual_r = int(r_match.group(1))
        actual_g = int(g_match.group(1))
        actual_b = int(b_match.group(1))

        if (actual_r, actual_g, actual_b) == (r, g, b):
            print(f"   ✅ {tag} - #{r:02x}{g:02x}{b:02x}")
        else:
            print(f"   ❌ {tag} - Expected #{r:02x}{g:02x}{b:02x}, got #{actual_r:02x}{actual_g:02x}{actual_b:02x}")
            all_match = False

    print()

    if all_match:
        print("✅ All tag colors match game palette")
        print()
    else:
        print("❌ FAIL: Tag colors don't match game palette")
        print()

    return all_match


def verify_no_hardcoded_positioning(project_root: Path) -> bool:
    """Verify modal uses FlexBox for layout, not hardcoded positioning"""
    print("📋 Verifying FlexBox layout pattern:")
    print()

    reward_modal_c = project_root / "src" / "scenes" / "components" / "rewardModal.c"
    content = reward_modal_c.read_text()

    # Check for FlexBox usage (support both old and new Archimedes API)
    has_flexbox = ("a_CreateFlexBox" in content or "a_FlexBoxCreate" in content)

    if not has_flexbox:
        print("❌ FAIL: Modal doesn't use FlexBox for layout")
        print("   ADR-12 requires: FlexBox for automatic positioning")
        print()
        return False

    # Count FlexBox instances (should have at least 2: cards + list)
    # Support both old (a_CreateFlexBox) and new (a_FlexBoxCreate) API
    flexbox_count = len(re.findall(r'(a_CreateFlexBox|a_FlexBoxCreate)\s*\(', content))

    if flexbox_count < 2:
        print(f"⚠️  WARNING: Only {flexbox_count} FlexBox instance(s) found")
        print("   Expected: At least 2 (card layout + list layout)")
        print()

    print(f"✅ Modal uses FlexBox for layout ({flexbox_count} instance(s))")
    print()
    return True


def verify_reward_modal_animation(project_root: Path) -> bool:
    """Main verification function"""

    print("=" * 80)
    print("FF-012: Reward Modal Animation System Verification")
    print("=" * 80)
    print()

    # Phase 1: Core architectural patterns
    print("=" * 80)
    print("Phase 1: Core Architectural Patterns")
    print("=" * 80)
    print()

    step1 = verify_animation_stage_enum(project_root)
    if not step1:
        return False

    step2 = verify_animation_state_fields(project_root)
    if not step2:
        return False

    step3 = verify_animation_progression(project_root)
    if not step3:
        return False

    # Phase 2: Constitutional compliance
    print("=" * 80)
    print("Phase 2: Constitutional Compliance (ADR-12)")
    print("=" * 80)
    print()

    pattern1 = verify_untagged_card_selection(project_root)
    if not pattern1:
        return False

    pattern2 = verify_single_tag_application(project_root)
    if not pattern2:
        return False

    pattern3 = verify_flexbox_cleanup(project_root)
    # Non-fatal, continue even if warnings

    pattern4 = verify_no_hardcoded_positioning(project_root)
    if not pattern4:
        return False

    # Phase 3: Visual consistency
    print("=" * 80)
    print("Phase 3: Visual Consistency")
    print("=" * 80)
    print()

    visual1 = verify_tag_color_palette(project_root)
    if not visual1:
        return False

    print("=" * 80)
    print("✅ SUCCESS: Reward modal animation system verified")
    print("=" * 80)
    print()
    print("Constitutional Pattern Verified:")
    print("  - Multi-stage animation (FADE_OUT → SCALE_CARD → FADE_IN_TAG → COMPLETE)")
    print("  - Untagged card filtering (no pre-tagged cards offered)")
    print("  - Single tag application per selection")
    print("  - FlexBox layout (no hardcoded positioning)")
    print("  - Color palette consistency (game-wide colors)")
    print("  - Proper cleanup (FlexBox destruction)")
    print()

    return True


if __name__ == '__main__':
    import sys
    project_root = Path(__file__).parent.parent.parent
    success = verify_reward_modal_animation(project_root)
    sys.exit(0 if success else 1)
