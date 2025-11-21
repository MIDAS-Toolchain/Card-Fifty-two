# ADR-13: Universal Damage Modifier Application

## Context

Combat stats system (BRUTAL tag for +damage%, LUCKY tag for +crit%) was implemented in ADR-09 (Dirty-flag Aggregation). However, damage modifier application was **inconsistent across damage sources**:

**Before This ADR:**
- ✅ Turn damage (Win/Push/Blackjack): Applied damage_percent → damage_flat → crit
- ❌ Tag damage (CURSED/VAMPIRIC): Only applied crit (skipped damage_percent!)
- ❌ Trinket damage (Degenerate's Gambit): Only applied crit (skipped damage_percent!)

**User Report (2025-01-19):**
> "brutal the damage increase seems to work for winning, but it doesnt seem to work for tag damage or for trinket damage"

**Root Cause:**
- Turn damage manually duplicated modifier logic in game.c:713-738
- Tag/trinket damage only called `RollForCrit()` helper (no damage_percent/damage_flat)
- No single source of truth for damage modification order

**Alternatives Considered:**

1. **Copy modifier logic to each damage source**
   - Pros: No new functions, explicit control per source
   - Cons: Code duplication, inconsistency risk, manual sync required
   - Cost: 5 locations to maintain identical logic (game.c, cardTags.c×2, trinkets/×N)

2. **Make RollForCrit() apply all modifiers**
   - Pros: Reuses existing function, minimal refactor
   - Cons: Misleading name (does more than crit), unclear responsibility
   - Cost: Function does 3 things (damage%, flat, crit) but named for 1

3. **Create ApplyPlayerDamageModifiers() universal function**
   - Pros: Single source of truth, clear name/responsibility, enforces consistency
   - Cons: One more function in codebase
   - Cost: Minimal (5 lines to wrap existing logic)

## Decision

**Create `ApplyPlayerDamageModifiers()` as universal damage modifier function. ALL damage sources MUST use it.**

**Function Signature:**
```c
// player.h:215-232
int ApplyPlayerDamageModifiers(Player_t* player, int base_damage, bool* out_is_crit);
```

**Modifier Order (Non-Negotiable):**
1. **Percentage damage increase** (damage_percent) - BRUTAL tag (+10% per card)
2. **Flat damage bonus** (damage_flat) - Reserved for future trinkets/abilities
3. **Crit roll** (crit_chance) - LUCKY tag (+10% crit per card)

**Implementation Pattern:**
```c
// Example: CURSED tag dealing 10 damage
int base_damage = 10;
bool is_crit = false;
int final_damage = ApplyPlayerDamageModifiers(drawer, base_damage, &is_crit);
TakeDamage(enemy, final_damage);

// With 2 BRUTAL cards: 10 → 12 (+20%) → maybe crit → 18 (if +50% crit)
// Without BRUTAL: 10 → 10 → maybe crit → 15 (if +50% crit)
```

**Justification:**

1. **Consistency**: All damage sources (turns, tags, trinkets, abilities) use SAME modifiers
2. **Single Source of Truth**: Modifier logic in ONE place (player.c:543-582)
3. **Maintainability**: Future stat changes (new tags, trinkets) automatically apply everywhere
4. **Clarity**: Function name clearly states purpose ("apply damage modifiers")
5. **Correctness**: Enforces order (% before flat before crit) to prevent calculation errors

## Consequences

**Positive:**
- ✅ BRUTAL tag now increases ALL damage (tags, trinkets, turns)
- ✅ Future damage stats (e.g., PIERCING tag) automatically work everywhere
- ✅ Crit calculation always happens AFTER other modifiers (correct order)
- ✅ Easy to verify: grep for `TakeDamage()` → all preceded by `ApplyPlayerDamageModifiers()`

**Negative:**
- ❌ One more function to understand when reading code
- ❌ Slightly more verbose callsite (3 lines vs 1 line for old `RollForCrit()`)

**Accepted Trade-offs:**
- ✅ Accept extra function for consistency (better than duplicate logic)
- ✅ Accept verbosity for clarity (explicit better than implicit)
- ❌ NOT accepting: Per-source damage modifier "flavors" (all sources use same stats)

**Pattern Used:**

All damage sources refactored to use universal function:

**Turn Damage (game.c:713-717):**
```c
// Apply combat stat modifiers (ADR-010: Universal damage modifier)
bool is_crit = false;
if (damage_dealt > 0 && game->is_combat_mode) {
    damage_dealt = ApplyPlayerDamageModifiers(player, damage_dealt, &is_crit);
}
```

**CURSED Tag (cardTags.c:354-358):**
```c
int base_damage = 10;
bool is_crit = false;
int damage = ApplyPlayerDamageModifiers(drawer, base_damage, &is_crit);
TakeDamage(game->current_enemy, damage);
```

**VAMPIRIC Tag (cardTags.c:393-398):**
```c
int base_damage = 5;
bool is_crit = false;
int damage = ApplyPlayerDamageModifiers(drawer, base_damage, &is_crit);
TakeDamage(game->current_enemy, damage);
```

**Degenerate's Gambit Trinket (degenerateGambit.c:56-60):**
```c
int base_damage = 10 + self->passive_damage_bonus;
bool is_crit = false;
int damage = ApplyPlayerDamageModifiers(player, base_damage, &is_crit);
TakeDamage(game->current_enemy, damage);
```

**Implementation Details:**

Function automatically handles:
- NULL checks (returns base_damage if player is NULL)
- Zero/negative damage (returns unchanged, no modifiers)
- Dirty flag refresh (calls `CalculatePlayerCombatStats()` if needed)
- Logging per step (shows % bonus, flat bonus, crit result)

```c
// player.c:543-582
int ApplyPlayerDamageModifiers(Player_t* player, int base_damage, bool* out_is_crit) {
    if (!player || base_damage <= 0) {
        if (out_is_crit) *out_is_crit = false;
        return base_damage;
    }

    // Recalculate stats if dirty (ADR-09)
    if (player->combat_stats_dirty) {
        CalculatePlayerCombatStats(player);
    }

    int modified_damage = base_damage;

    // Step 1: Apply percentage damage increase (BRUTAL tag)
    if (player->damage_percent > 0) {
        int bonus_damage = (modified_damage * player->damage_percent) / 100;
        modified_damage += bonus_damage;
        d_LogInfoF("  [Damage Modifier] +%d%% damage: %d → %d (+%d)",
                   player->damage_percent, base_damage, modified_damage, bonus_damage);
    }

    // Step 2: Apply flat damage bonus (currently unused)
    if (player->damage_flat > 0) {
        modified_damage += player->damage_flat;
        d_LogInfoF("  [Damage Modifier] +%d flat damage: %d → %d",
                   player->damage_flat, modified_damage - player->damage_flat, modified_damage);
    }

    // Step 3: Roll for crit (LUCKY tag)
    bool is_crit = RollForCrit(player, modified_damage, &modified_damage);
    if (out_is_crit) *out_is_crit = is_crit;

    d_LogInfoF("  [Damage Modifier] Final: %d → %d%s",
               base_damage, modified_damage, is_crit ? " (CRIT!)" : "");

    return modified_damage;
}
```

**Modifier Order Rationale:**

1. **Percentage first**: Scales base damage proportionally (10 + 20% = 12)
2. **Flat second**: Adds fixed bonus after scaling (12 + 5 = 17)
3. **Crit last**: Multiplies final pre-crit damage (17 × 1.5 = 25.5 → 25)

Order matters! Example with 10 base damage, 20% damage_percent, +5 damage_flat, 50% crit:
- **Correct**: 10 → 12 (+20%) → 17 (+5) → 25 (crit +50%) = **25 damage**
- **Wrong (flat first)**: 10 → 15 (+5) → 18 (+20%) → 27 (crit +50%) = **27 damage** (overpowered!)

**RollForCrit() Remains Separate:**

The existing `RollForCrit()` function (player.c:502-541) remains unchanged for backward compatibility and clarity:
- Used internally by `ApplyPlayerDamageModifiers()`
- Can still be called directly if only crit (no other modifiers) is needed
- Name accurately describes what it does (rolls crit, not all modifiers)

**Future Damage Modifiers:**

When adding new damage modifiers (e.g., PIERCING tag = ignore 10% of enemy armor):
1. Add field to Player_t struct (e.g., `int armor_penetration`)
2. Add calculation to `CalculatePlayerCombatStats()` (scan for PIERCING tag)
3. Add modifier step to `ApplyPlayerDamageModifiers()` (in correct order)
4. ALL damage sources automatically benefit (no per-source changes needed)

**Example Scenarios:**

| Base Dmg | BRUTAL Cards | Flat Bonus | LUCKY Cards | Crit Roll | Final Dmg | Calculation |
|----------|--------------|------------|-------------|-----------|-----------|-------------|
| 10 (CURSED) | 0 | 0 | 0 | No | 10 | Base only |
| 10 (CURSED) | 2 (+20%) | 0 | 0 | No | 12 | 10 + 20% = 12 |
| 10 (CURSED) | 2 (+20%) | 0 | 3 (+30% crit) | Yes (+50%) | 18 | 10 → 12 (+20%) → 18 (crit) |
| 5 (VAMPIRIC) | 3 (+30%) | 5 | 0 | No | 11.5 → 11 | 5 → 6.5 (+30%) → 11.5 (+5) → 11 (floor) |
| 2100 (Turn) | 1 (+10%) | 0 | 2 (+20% crit) | Yes (+50%) | 3465 | 2100 → 2310 (+10%) → 3465 (crit) |

**Verification:**

To verify all damage sources use universal modifier:
```bash
# All TakeDamage() calls should be preceded by ApplyPlayerDamageModifiers()
grep -B5 "TakeDamage" src/**/*.c | grep -E "(ApplyPlayerDamageModifiers|TakeDamage)"
```

Expected: Every `TakeDamage()` has `ApplyPlayerDamageModifiers()` within 5 lines before it.

**Success Metrics:**
- BRUTAL tag increases damage from ALL sources (turns, tags, trinkets)
- Damage modifier order consistent (% → flat → crit) everywhere
- No duplicate modifier logic in codebase
- Future damage stats automatically apply to all sources
- Logs show step-by-step modifier application for debugging

## Related ADRs

- **ADR-09** (Card Tags): Defines BRUTAL/LUCKY tags and dirty-flag aggregation pattern
- **ADR-10** (Combat Damage Formula): Defines base damage calculation (hand_value × bet_amount)
- **This ADR (13)** extends damage system to apply modifiers consistently across ALL sources

**Dependencies:**
- Requires ADR-09 combat stats fields (damage_percent, damage_flat, crit_chance)
- Builds on ADR-10 base damage formula (modifiers applied AFTER base calculation)

**Status:** ✅ Implemented (2025-01-19)
- Committed in: [commit hash will be added by user]
- Files changed: player.h, player.c, game.c, cardTags.c, degenerateGambit.c
