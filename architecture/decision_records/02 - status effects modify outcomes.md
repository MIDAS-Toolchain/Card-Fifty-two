# ADR-002: Status Effects Modify Outcomes, Not Prevent Actions

## Context

Token manipulation debuffs (chip drain, greed, tilt) need to affect gameplay. Could implement as preventive validators (block actions), outcome modifiers (change results), or passive observers (log only).

**Alternatives Considered:**

1. **Outcome modifiers in round resolution** (modify values after calculation)
   - Pros: Clear audit trail (base → modified), effects see complete state
   - Cons: Can't prevent invalid actions, round resolution grows large
   - Cost: 300+ line function, all modification logic centralized

2. **Preventive validators in input layer** (block actions before execution)
   - Pros: Early rejection, clear error messages, prevents wasted computation
   - Cons: Effects scattered across multiple validators, complex state checks
   - Cost: Validation logic duplicated in UI + game logic

3. **Passive observers** (log effects, don't modify gameplay)
   - Pros: Zero gameplay impact, pure telemetry
   - Cons: Status effects are cosmetic only, no actual debuffs
   - Cost: Misleading UX (effects shown but don't do anything)

4. **Active resource spending** (player chooses when to apply effects)
   - Pros: Player agency, strategic depth
   - Cons: Adds complexity, dilutes blackjack focus
   - Cost: New UI, new mechanics, tutorial overhead

## Decision

**Status effects are outcome modifiers applied in `Game_ResolveRound()` and `Game_ProcessBettingInput()` after base values are calculated.**

Effects modify numbers AFTER decisions are made, not before. Example: Calculate base winnings → GREED halves it → Award modified amount.

**Justification:**

1. **Transparency**: Log shows base value, effect applied, final value (clear cause/effect)
2. **Single Modification Point**: All payout logic in one function, not scattered
3. **Effect Ordering**: Modifiers apply in predictable sequence (no race conditions)
4. **Testable**: Mock game context, verify modifier math in isolation
5. **Composition**: Multiple effects stack predictably (GREED + TILT both modify)

## Consequences

**Positive:**
- ✅ Clear modification flow (base → effect → result logged)
- ✅ Centralized in round resolution (grep "ModifyWinnings" finds all uses)
- ✅ Status effect module isolated (no coupling to game rules)
- ✅ Future effects slot into existing hooks (no refactor needed)
- ✅ UI shows transparency (can display "base 100, -50% GREED, final 50")

**Negative:**
- ❌ Can't prevent actions (only modify outcomes)
- ❌ Round resolution function large (~400 lines with effects)
- ❌ Effect timing not obvious from effect definition alone

**Accepted Trade-offs:**
- ✅ Accept large function for clarity over distributed checks
- ✅ Accept outcome-only effects for simplicity (no preventive logic yet)
- ❌ NOT accepting: Effects modifying bet amounts directly - must use ModifyBetWithEffects()

**Pattern Used:**

Status effects are VALUE MODIFIERS not ACTION VALIDATORS.
```c
// Validation: Happens in input processors
if (!CanAffordBet(player, amount)) return false;

// Modification: Happens in resolution
int base_winnings = bet * 1.0f;
int modified = ModifyWinnings(status_effects, base_winnings);
player->chips += modified;
```

**Success Metrics:**
- All win branches call ModifyWinnings() before awarding chips
- All loss branches call ModifyLosses() before deducting chips
- Status effect tooltips show modification formula
- Combat log transparency: "Won 100 chips (-50% GREED) → 50 chips"