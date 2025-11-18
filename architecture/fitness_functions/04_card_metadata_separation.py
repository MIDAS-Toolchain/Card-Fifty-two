#!/usr/bin/env python3
"""
FF-004: Card Metadata Separation Enforcement

Ensures Card_t struct contains NO metadata fields (tags, rarity, flavor).
Enforces metadata lives in separate CardMetadata_t registry.

Based on: ADR-004 (Card Metadata as Separate Registry Over Embedded Fields)

Architecture Characteristic: Separation of Concerns
Domain Knowledge Required: Minimal (Card_t struct layout)
"""

import re
from pathlib import Path
from typing import List, Dict, Any, Optional


class StructField:
    """Represents a single field in a C struct"""
    def __init__(self, field_type: str, name: str, declaration: str, line_number: int):
        self.type = field_type
        self.name = name
        self.declaration = declaration
        self.line_number = line_number


class StructInfo:
    """Represents a parsed C struct"""
    def __init__(self, name: str, fields: List[StructField]):
        self.name = name
        self.fields = fields


def parse_c_struct(header_path: Path, struct_name: str) -> Optional[StructInfo]:
    """
    Parse C struct definition from header file.

    Returns: StructInfo object or None if not found
    """
    try:
        with open(header_path, 'r', encoding='utf-8') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"⚠ WARNING: Could not read {header_path}: {e}")
        return None

    # Find struct definition: typedef struct Name { ... } Name_t;
    content = ''.join(lines)
    pattern = rf'typedef\s+struct\s+{struct_name}\s*\{{([^}}]+)\}}\s*{struct_name}_t\s*;'
    match = re.search(pattern, content, re.DOTALL)

    if not match:
        return None

    body = match.group(1)
    struct_start_line = content[:match.start()].count('\n') + 1

    # Parse fields
    fields = []
    in_multiline_comment = False

    for rel_line_num, line in enumerate(body.split('\n'), 1):
        stripped = line.strip()

        # Handle multi-line comments
        if '/*' in stripped:
            in_multiline_comment = True
        if '*/' in stripped:
            in_multiline_comment = False
            continue
        if in_multiline_comment:
            continue

        # Skip single-line comments and empty lines
        if not stripped or stripped.startswith('//'):
            continue

        # Parse field: type name;
        # Handle various patterns:
        # - Simple: int x;
        # - Pointer: SDL_Texture* texture;
        # - Array: int coords[2];
        field_match = re.match(r'(.+?)\s+(\w+)\s*(\[[^\]]*\])?\s*;', stripped)
        if field_match:
            field_type = field_match.group(1).strip()
            field_name = field_match.group(2)
            array_suffix = field_match.group(3) or ''

            fields.append(StructField(
                field_type=field_type,
                name=field_name,
                declaration=stripped,
                line_number=struct_start_line + rel_line_num
            ))

    return StructInfo(name=struct_name, fields=fields)


def get_struct_size_assertion(header_path: Path, struct_name: str) -> Optional[int]:
    """
    Get sizeof(struct) from _Static_assert if present.

    Returns: size in bytes or None if not found
    """
    try:
        with open(header_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception:
        return None

    # Look for _Static_assert(sizeof(Card_t) == 32, ...)
    pattern = rf'_Static_assert\s*\(\s*sizeof\s*\(\s*{struct_name}_t\s*\)\s*==\s*(\d+)'
    match = re.search(pattern, content)

    if match:
        return int(match.group(1))

    return None


def file_contains(file_path: Path, text: str) -> bool:
    """Check if file contains exact text"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return text in f.read()
    except Exception:
        return False


def verify_card_metadata_separation(project_root: Path) -> bool:
    """
    Main verification function for FF-004.

    Returns: True if Card_t properly separated from metadata, False otherwise
    """
    print("=" * 80)
    print("FF-004: Card Metadata Separation Enforcement")
    print("=" * 80)

    structs_h_path = project_root / "include" / "structs.h"

    if not structs_h_path.exists():
        print(f"❌ FAILURE: Could not find {structs_h_path}")
        return False

    # Parse Card_t struct
    card_struct = parse_c_struct(structs_h_path, "Card")

    if not card_struct:
        print(f"❌ FAILURE: Could not parse Card struct from {structs_h_path}")
        return False

    print(f"✓ Found Card_t struct with {len(card_struct.fields)} fields")

    # Forbidden field patterns in Card_t (metadata belongs in CardMetadata_t)
    forbidden_patterns = [
        (r'dArray_t\s*\*.*tags', 'tags array (use CardMetadata_t)'),
        (r'CardTag_t', 'tag type (use CardMetadata_t)'),
        (r'CardRarity_t', 'rarity field (use CardMetadata_t)'),
        (r'dString_t\s*\*.*flavor', 'flavor text (use CardMetadata_t)'),
        (r'CardMetadata_t', 'metadata struct reference (use separate registry)'),
        (r'dArray_t\s*\*.*effect', 'effect array (use CardMetadata_t)'),
    ]

    violations = []

    # Check each field in Card_t for forbidden patterns
    for field in card_struct.fields:
        for pattern, description in forbidden_patterns:
            if re.search(pattern, field.declaration, re.IGNORECASE):
                violations.append({
                    'type': 'FORBIDDEN_FIELD',
                    'field': field.name,
                    'field_type': field.type,
                    'line': field.line_number,
                    'description': description
                })

    # Verify Card_t size hasn't bloated (if _Static_assert present)
    expected_size = get_struct_size_assertion(structs_h_path, "Card")

    if expected_size:
        print(f"✓ Found size assertion: sizeof(Card_t) == {expected_size} bytes")

        # ADR-005 specifies Card_t should be 32 bytes
        if expected_size > 48:  # Allow some flexibility, but 32 is ideal
            violations.append({
                'type': 'SIZE_BLOAT',
                'field': 'STRUCT_SIZE',
                'actual_size': expected_size,
                'description': f'Card_t should be lightweight (~32 bytes), found {expected_size}'
            })
    else:
        print("⚠ WARNING: No _Static_assert found for Card_t size")

    # Report results
    print()
    if violations:
        print("❌ FAILURE: Card metadata separation violated!")
        print()

        for v in violations:
            if v['type'] == 'SIZE_BLOAT':
                print(f"🚨 Card_t size bloated: {v['actual_size']} bytes")
                print(f"   {v['description']}")
                print(f"   Fix: Move metadata fields to separate CardMetadata_t registry")
            elif v['type'] == 'FORBIDDEN_FIELD':
                relative_path = structs_h_path.relative_to(project_root)
                print(f"🚨 {relative_path}:{v['line']}")
                print(f"   Field: {v['field']} ({v['field_type']})")
                print(f"   Issue: {v['description']}")
                print(f"   Fix: Remove from Card_t, store in g_card_metadata table")
            print()

        return False

    # Success - verify metadata registry exists (if cardTags.h exists)
    # Note: This is optional - might not be implemented yet
    card_tags_h = project_root / "include" / "cardTags.h"
    if card_tags_h.exists():
        has_registry = file_contains(card_tags_h, "g_card_metadata")
        if has_registry:
            print("✅ SUCCESS: Card_t is lightweight (no metadata bloat)")
            print("✅ SUCCESS: Metadata registry exists (g_card_metadata)")
        else:
            print("✅ SUCCESS: Card_t is lightweight (no metadata bloat)")
            print("⚠ INFO: g_card_metadata not found (metadata system not implemented yet)")
    else:
        print("✅ SUCCESS: Card_t is lightweight (no metadata bloat)")
        print("⚠ INFO: cardTags.h not found (metadata system not implemented yet)")

    print()

    return True


if __name__ == "__main__":
    # When run directly, verify from script directory
    script_dir = Path(__file__).parent
    project_root = script_dir.parent.parent

    success = verify_card_metadata_separation(project_root)
    exit(0 if success else 1)
