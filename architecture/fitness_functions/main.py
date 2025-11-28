#!/usr/bin/env python3
"""
Fitness Function Runner

Executes all architecture fitness functions and reports results.
Exits with non-zero code if any fitness function fails.

Usage:
    python architecture/fitness_functions/main.py
    make fitness  # (if added to Makefile)
"""

import sys
from pathlib import Path
from typing import List, Tuple

# Import fitness functions directly
import importlib.util

# FF-001: Event Coverage
spec1 = importlib.util.spec_from_file_location(
    "event_coverage_verification",
    Path(__file__).parent / "01_event_coverage_verification.py"
)
event_coverage_verification = importlib.util.module_from_spec(spec1)
spec1.loader.exec_module(event_coverage_verification)

# FF-002: Status Effect Modifier Coverage
spec2 = importlib.util.spec_from_file_location(
    "status_effect_modifier_coverage",
    Path(__file__).parent / "02_status_effect_modifier_coverage.py"
)
status_effect_modifier_coverage = importlib.util.module_from_spec(spec2)
spec2.loader.exec_module(status_effect_modifier_coverage)

# FF-003: Constitutional Pattern Enforcement
spec3 = importlib.util.spec_from_file_location(
    "constitutional_pattern_enforcement",
    Path(__file__).parent / "03_constitutional_pattern_enforcement.py"
)
constitutional_pattern_enforcement = importlib.util.module_from_spec(spec3)
spec3.loader.exec_module(constitutional_pattern_enforcement)

# FF-004: Card Metadata Separation
spec4 = importlib.util.spec_from_file_location(
    "card_metadata_separation",
    Path(__file__).parent / "04_card_metadata_separation.py"
)
card_metadata_separation = importlib.util.module_from_spec(spec4)
spec4.loader.exec_module(card_metadata_separation)

# FF-005: Event Choices as Value Types
spec5 = importlib.util.spec_from_file_location(
    "event_choices_value_types",
    Path(__file__).parent / "05_event_choices_value_types.py"
)
event_choices_value_types = importlib.util.module_from_spec(spec5)
spec5.loader.exec_module(event_choices_value_types)

# FF-006: d_ArrayInit Parameter Order Verification
spec6 = importlib.util.spec_from_file_location(
    "daedalus_parameter_order",
    Path(__file__).parent / "06_daedalus_parameter_order.py"
)
daedalus_parameter_order = importlib.util.module_from_spec(spec6)
spec6.loader.exec_module(daedalus_parameter_order)

# FF-007: Texture Cleanup Verification
spec7 = importlib.util.spec_from_file_location(
    "texture_cleanup_verification",
    Path(__file__).parent / "07_texture_cleanup_verification.py"
)
texture_cleanup_verification = importlib.util.module_from_spec(spec7)
spec7.loader.exec_module(texture_cleanup_verification)

# FF-008: Modal Design Consistency
spec8 = importlib.util.spec_from_file_location(
    "modal_design_consistency",
    Path(__file__).parent / "08_modal_design_consistency.py"
)
modal_design_consistency = importlib.util.module_from_spec(spec8)
spec8.loader.exec_module(modal_design_consistency)

# FF-009: Tag System Architecture
spec9 = importlib.util.spec_from_file_location(
    "tag_system_architecture",
    Path(__file__).parent / "09_tag_system_architecture.py"
)
tag_system_architecture = importlib.util.module_from_spec(spec9)
spec9.loader.exec_module(tag_system_architecture)

# FF-010: Combat Damage Formula
spec10 = importlib.util.spec_from_file_location(
    "combat_damage_verification",
    Path(__file__).parent / "10_combat_damage_verification.py"
)
combat_damage_verification = importlib.util.module_from_spec(spec10)
spec10.loader.exec_module(combat_damage_verification)

# FF-011: Targeting Hover State
spec11 = importlib.util.spec_from_file_location(
    "targeting_hover_state",
    Path(__file__).parent / "11_targeting_hover_state.py"
)
targeting_hover_state = importlib.util.module_from_spec(spec11)
spec11.loader.exec_module(targeting_hover_state)

# FF-012: Reward Modal Animation
spec12 = importlib.util.spec_from_file_location(
    "reward_modal_animation",
    Path(__file__).parent / "12_reward_modal_animation.py"
)
reward_modal_animation = importlib.util.module_from_spec(spec12)
spec12.loader.exec_module(reward_modal_animation)

# FF-013: Universal Damage Modifiers
spec13 = importlib.util.spec_from_file_location(
    "universal_damage_modifiers",
    Path(__file__).parent / "13_universal_damage_modifiers.py"
)
universal_damage_modifiers = importlib.util.module_from_spec(spec13)
spec13.loader.exec_module(universal_damage_modifiers)

# FF-014: Reverse-Order Cleanup
spec14 = importlib.util.spec_from_file_location(
    "reverse_order_cleanup",
    Path(__file__).parent / "14_reverse_order_cleanup.py"
)
reverse_order_cleanup = importlib.util.module_from_spec(spec14)
spec14.loader.exec_module(reverse_order_cleanup)

# FF-015: Double-Pointer Destructor
spec15 = importlib.util.spec_from_file_location(
    "double_pointer_destructor",
    Path(__file__).parent / "15_double_pointer_destructor.py"
)
double_pointer_destructor = importlib.util.module_from_spec(spec15)
spec15.loader.exec_module(double_pointer_destructor)

# FF-016: Trinket Value Semantics
spec16 = importlib.util.spec_from_file_location(
    "trinket_value_semantics",
    Path(__file__).parent / "16_trinket_value_semantics.py"
)
trinket_value_semantics = importlib.util.module_from_spec(spec16)
spec16.loader.exec_module(trinket_value_semantics)

# FF-017: Terminal Architecture
spec17 = importlib.util.spec_from_file_location(
    "terminal_architecture",
    Path(__file__).parent / "17_terminal_architecture.py"
)
terminal_architecture = importlib.util.module_from_spec(spec17)
spec17.loader.exec_module(terminal_architecture)

def run_fitness_functions() -> List[Tuple[str, bool]]:
    """Run all fitness functions and collect results"""
    results = []

    print("=" * 70)
    print("🏋️  ARCHITECTURE FITNESS FUNCTIONS")
    print("=" * 70)
    print()

    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    # FF-001: Event Coverage
    try:
        success = event_coverage_verification.verify_event_coverage(project_root)
        results.append(("FF-001: Event Coverage Verification", success))
    except Exception as e:
        print(f"❌ FF-001 crashed: {e}")
        results.append(("FF-001: Event Coverage Verification", False))

    print()

    # FF-002: Status Effect Modifier Coverage
    try:
        success = status_effect_modifier_coverage.verify_status_effect_modifier_coverage(project_root)
        results.append(("FF-002: Status Effect Modifier Coverage", success))
    except Exception as e:
        print(f"❌ FF-002 crashed: {e}")
        results.append(("FF-002: Status Effect Modifier Coverage", False))

    print()

    # FF-003: Constitutional Pattern Enforcement
    try:
        success = constitutional_pattern_enforcement.verify_constitutional_pattern_enforcement(project_root)
        results.append(("FF-003: Constitutional Pattern Enforcement", success))
    except Exception as e:
        print(f"❌ FF-003 crashed: {e}")
        results.append(("FF-003: Constitutional Pattern Enforcement", False))

    print()

    # FF-004: Card Metadata Separation
    try:
        success = card_metadata_separation.verify_card_metadata_separation(project_root)
        results.append(("FF-004: Card Metadata Separation", success))
    except Exception as e:
        print(f"❌ FF-004 crashed: {e}")
        results.append(("FF-004: Card Metadata Separation", False))

    print()

    # FF-005: Event Choices as Value Types
    try:
        success = event_choices_value_types.verify_event_choices_value_types(project_root)
        results.append(("FF-005: Event Choices as Value Types", success))
    except Exception as e:
        print(f"❌ FF-005 crashed: {e}")
        results.append(("FF-005: Event Choices as Value Types", False))

    print()

    # FF-006: d_ArrayInit Parameter Order Verification
    try:
        success = daedalus_parameter_order.verify_daedalus_parameter_order(project_root)
        results.append(("FF-006: d_ArrayInit Parameter Order Verification", success))
    except Exception as e:
        print(f"❌ FF-006 crashed: {e}")
        results.append(("FF-006: d_ArrayInit Parameter Order Verification", False))

    print()

    # FF-007: Texture Cleanup Verification
    try:
        success = texture_cleanup_verification.verify_texture_cleanup(project_root)
        results.append(("FF-007: Texture Cleanup Verification", success))
    except Exception as e:
        print(f"❌ FF-007 crashed: {e}")
        results.append(("FF-007: Texture Cleanup Verification", False))

    print()

    # FF-008: Modal Design Consistency
    try:
        success = modal_design_consistency.verify_modal_consistency(project_root)
        results.append(("FF-008: Modal Design Consistency", success))
    except Exception as e:
        print(f"❌ FF-008 crashed: {e}")
        results.append(("FF-008: Modal Design Consistency", False))

    print()

    # FF-009: Tag System Architecture
    try:
        success = tag_system_architecture.verify_tag_system_architecture(project_root)
        results.append(("FF-009: Tag System Architecture", success))
    except Exception as e:
        print(f"❌ FF-009 crashed: {e}")
        results.append(("FF-009: Tag System Architecture", False))

    print()

    # FF-010: Combat Damage Formula
    try:
        success = combat_damage_verification.verify_combat_damage_formula(project_root)
        results.append(("FF-010: Combat Damage Formula", success))
    except Exception as e:
        print(f"❌ FF-010 crashed: {e}")
        results.append(("FF-010: Combat Damage Formula", False))

    print()

    # FF-011: Targeting Hover State
    try:
        success = targeting_hover_state.verify_targeting_hover_state(project_root)
        results.append(("FF-011: Targeting Hover State", success))
    except Exception as e:
        print(f"❌ FF-011 crashed: {e}")
        results.append(("FF-011: Targeting Hover State", False))

    print()

    # FF-012: Reward Modal Animation
    try:
        success = reward_modal_animation.verify_reward_modal_animation(project_root)
        results.append(("FF-012: Reward Modal Animation", success))
    except Exception as e:
        print(f"❌ FF-012 crashed: {e}")
        results.append(("FF-012: Reward Modal Animation", False))

    print()

    # FF-013: Universal Damage Modifiers
    try:
        success = universal_damage_modifiers.verify_universal_damage_modifiers(project_root)
        results.append(("FF-013: Universal Damage Modifiers", success))
    except Exception as e:
        print(f"❌ FF-013 crashed: {e}")
        results.append(("FF-013: Universal Damage Modifiers", False))

    print()

    # FF-014: Reverse-Order Cleanup
    try:
        success = reverse_order_cleanup.verify_reverse_order_cleanup(project_root)
        results.append(("FF-014: Reverse-Order Cleanup", success))
    except Exception as e:
        print(f"❌ FF-014 crashed: {e}")
        results.append(("FF-014: Reverse-Order Cleanup", False))

    print()

    # FF-015: Double-Pointer Destructor
    try:
        success = double_pointer_destructor.verify_double_pointer_destructors(project_root)
        results.append(("FF-015: Double-Pointer Destructor", success))
    except Exception as e:
        print(f"❌ FF-015 crashed: {e}")
        results.append(("FF-015: Double-Pointer Destructor", False))

    print()

    # FF-016: Trinket Value Semantics
    try:
        success = trinket_value_semantics.verify_trinket_value_semantics(project_root)
        results.append(("FF-016: Trinket Value Semantics", success))
    except Exception as e:
        print(f"❌ FF-016 crashed: {e}")
        results.append(("FF-016: Trinket Value Semantics", False))

    print()

    # FF-017: Terminal Architecture
    try:
        success = terminal_architecture.verify_terminal_architecture(project_root)
        results.append(("FF-017: Terminal Architecture", success))
    except Exception as e:
        print(f"❌ FF-017 crashed: {e}")
        results.append(("FF-017: Terminal Architecture", False))

    print()

    return results

def print_summary(results: List[Tuple[str, bool]]) -> bool:
    """Print summary and return overall pass/fail"""
    print("=" * 70)
    print("📊 SUMMARY")
    print("=" * 70)
    print()

    passed = sum(1 for _, success in results if success)
    failed = len(results) - passed

    for name, success in results:
        status = "✅ PASS" if success else "❌ FAIL"
        print(f"{status}: {name}")

    print()
    print(f"Total: {passed}/{len(results)} passed, {failed}/{len(results)} failed")
    print()

    if failed == 0:
        print("🎉 All fitness functions passed!")
        return True
    else:
        print("🚨 Some fitness functions failed - see details above")
        return False

if __name__ == '__main__':
    results = run_fitness_functions()
    overall_success = print_summary(results)
    sys.exit(0 if overall_success else 1)
