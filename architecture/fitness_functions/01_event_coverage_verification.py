#!/usr/bin/env python3
"""
FF-001: Event Coverage Verification

Ensures every GameEvent_t enum value has at least one Game_TriggerEvent() callsite.
Prevents orphaned events (defined but never fired).

Based on: ADR-001 (Event-Driven Ability System)
"""

import re
import sys
from pathlib import Path
from typing import Set, Dict, List

def parse_game_events(header_path: Path) -> Set[str]:
    """Extract all GameEvent_t enum values from game.h"""
    events = set()

    with open(header_path, 'r') as f:
        content = f.read()

    # Find GameEvent_t enum block
    enum_pattern = r'typedef enum \{([^}]+)\} GameEvent_t;'
    match = re.search(enum_pattern, content, re.DOTALL)

    if not match:
        raise ValueError(f"Could not find GameEvent_t enum in {header_path}")

    enum_body = match.group(1)

    # Extract event names (ignore comments and whitespace)
    event_pattern = r'(GAME_EVENT_\w+)'
    for match in re.finditer(event_pattern, enum_body):
        events.add(match.group(1))

    return events

def find_triggered_events(src_dir: Path) -> Dict[str, List[str]]:
    """Find all Game_TriggerEvent() callsites in source files"""
    triggered = {}

    # Search all .c files recursively
    for c_file in src_dir.rglob('*.c'):
        with open(c_file, 'r') as f:
            for line_num, line in enumerate(f, 1):
                # Match: Game_TriggerEvent(game, GAME_EVENT_XXX);
                match = re.search(r'Game_TriggerEvent\s*\([^,]+,\s*(GAME_EVENT_\w+)\)', line)
                if match:
                    event = match.group(1)
                    location = f"{c_file.relative_to(src_dir.parent)}:{line_num}"

                    if event not in triggered:
                        triggered[event] = []
                    triggered[event].append(location)

    return triggered

def verify_event_coverage(project_root: Path) -> bool:
    """Main verification logic"""
    header_path = project_root / 'include' / 'game.h'
    src_dir = project_root / 'src'

    print("=" * 60)
    print("FF-001: Event Coverage Verification")
    print("=" * 60)
    print()

    # Parse events
    defined_events = parse_game_events(header_path)
    print(f"📋 Defined events: {len(defined_events)}")
    for event in sorted(defined_events):
        print(f"   - {event}")
    print()

    # Find triggered events
    triggered_events = find_triggered_events(src_dir)
    print(f"✅ Triggered events: {len(triggered_events)}")
    for event in sorted(triggered_events.keys()):
        locations = triggered_events[event]
        print(f"   - {event} ({len(locations)} callsite{'s' if len(locations) > 1 else ''})")
        for loc in locations[:3]:  # Show first 3 locations
            print(f"     → {loc}")
        if len(locations) > 3:
            print(f"     → ... and {len(locations) - 3} more")
    print()

    # Find orphaned events
    orphaned = defined_events - triggered_events.keys()

    if orphaned:
        print(f"❌ FAILURE: {len(orphaned)} orphaned event(s) detected!")
        print()
        for event in sorted(orphaned):
            print(f"   🚨 {event} - NEVER TRIGGERED")
        print()
        print("Fix: Add Game_TriggerEvent(game, {EVENT}) callsite, or remove unused event from enum")
        return False
    else:
        print("✅ SUCCESS: All events have at least one trigger callsite")
        return True

if __name__ == '__main__':
    # Assume script is in architecture/fitness_functions/
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_event_coverage(project_root)
    sys.exit(0 if success else 1)
