# ADR-10: Combat Damage via Hand Value × Bet Multiplier

## Context

Combat system needs damage formula that scales with both hand quality and bet risk. Player wins deal damage to enemies, losses deal zero. Push (tie) scenarios unclear - should they deal reduced damage or none?

**Alternatives Considered:**

1. **Flat damage per win** (win = 100 damage regardless of bet/hand)
   - Pros: Simple, predictable, no math complexity
   - Cons: Ignores bet risk, no strategic depth, removes blackjack scoring relevance
   - Cost: Degenerate optimal play (min bet every hand)

2. **Bet amount only** (damage = bet × 1)
   - Pros: Encourages high bets, simple multiplication
   - Cons: Hand value irrelevant (21 = 15 damage), blackjack not special
   - Cost: Removes blackjack skill ceiling, pure gambling

3. **Hand value only** (damage = hand value × 10)
   - Pros: Rewards optimal play (21 = 210 damage)
   - Cons: No risk/reward (betting 1 chip = same as 100), bet mechanic pointless
   - Cost: Removes strategic betting decisions

4. **Hand value × bet amount** (damage = value × bet)
   - Pros: Scales with both skill AND risk, high bets on strong hands = maximum damage
   - Cons: Simple multiplication (no complexity), push scenarios ambiguous
   - Cost: Must define push behavior explicitly

## Decision

**Use `damage = hand_value × bet_amount` for wins. Push (tie) deals half damage. Losses deal zero damage.**

Blackjack payout remains 1.5× bet (3:2 odds) for chips, but damage treats blackjack as regular 21-value win.

**Justification:**

1. **Risk/Reward Balance**: High bets on weak hands (15×100 = 1500) vs low bets on blackjack (21×10 = 210)
2. **Strategic Depth**: Doubling down increases both chip gain AND damage potential
3. **Push Fairness**: Tie still rewards player (half damage) for reaching same value as dealer
4. **Simplicity**: Single multiplication, no lookup tables or conditional logic
5. **Blackjack Relevance**: 21 naturally deals more damage than 18, incentivizes optimal play

## Consequences

**Positive:**
- ✅ Encourages strategic betting (high bets on strong hands = double benefit)
- ✅ Blackjack scoring matters (21 > 20 > 19 in damage scaling)
- ✅ Push not punishing (player still makes progress even on tie)
- ✅ Simple formula (easy to understand, predict, and test)
- ✅ Doubling down meaningful (doubles both chips and damage potential)

**Negative:**
- ❌ Blackjack payout asymmetry (1.5× chips but same damage as regular 21)
- ❌ Push half-damage arbitrary (why 50%? why not 25% or 75%?)
- ❌ No bonus for "perfect play" (blackjack = regular 21 for damage)

**Accepted Trade-offs:**
- ✅ Accept blackjack chip/damage asymmetry for consistency (all wins use same formula)
- ✅ Accept arbitrary 50% push damage for simplicity (explicit is better than none)
- ❌ NOT accepting: Different damage formula per win type (blackjack vs dealer bust vs regular) - too complex

**Pattern Used:**

Damage calculated AFTER chip resolution, uses SAME bet value before modifications:
```c
// In Game_ResolveRound() - game.c:483-610
int bet_amount = player->current_bet;  // Before WinBet/LoseBet clears it
int hand_value = player->hand.total_value;
int damage_dealt = 0;

if (player_wins) {
    damage_dealt = hand_value * bet_amount;  // Full damage
} else if (player_push) {
    int full_damage = hand_value * bet_amount;
    damage_dealt = full_damage / 2;  // Half damage (explicit two-step to avoid rounding issues)
} else {
    damage_dealt = 0;  // Loss/bust = no damage
}

TakeDamage(enemy, damage_dealt);
```

**Ace Handling:**

Aces use **soft hand optimization** (blackjack standard rules):
- Initial value: 11
- Auto-converts to 1 when total > 21 (prevents bust)
- Multiple aces optimize sequentially (Ace+Ace = 12, not 22)
- Optimization algorithm in `CalculateHandValue()` (hand.c:133-203):

```c
int total = 0;
int num_aces = 0;

// Sum all cards (Aces = 11)
for (each card) {
    if (card->rank == RANK_ACE) {
        num_aces++;
        value = 11;
    }
    total += value;
}

// Optimize aces to prevent bust
while (total > 21 && num_aces > 0) {
    total -= 10;  // Convert one ace from 11 to 1
    num_aces--;
}
```

**Visual Feedback (Added 2025-01-17):**
- Aces glowing **green** = counting as 11 (soft hand)
- Aces glowing **blue** = counting as 1 (hard hand)
- Blue `(n)` text shows alternate score if ace was 11 (only if ≤ 21)
- Example: Hand shows "12 (22)" = current 12 (Ace=1), could be 22 if Ace=11 (but would bust)

**Push Damage Rationale:**

Push represents EQUAL skill (player matched dealer's hand). Zero damage would punish ties harshly. Full damage would reward luck over skill. Half damage balances:
- Player still makes progress toward enemy defeat
- Damage correctly reflects "partial success" (tied, not won)
- Integer division floors (15×100/2 = 750, not 750.5)

**Example Scenarios:**

| Hand | Bet | Outcome | Damage | Reasoning |
|------|-----|---------|--------|-----------|
| 21 (BJ) | 100 | Win | 2100 | 21 × 100 (chip payout: 150) |
| 18 | 50 | Win | 900 | 18 × 50 (chip payout: 50) |
| 20 | 200 | Push | 2000 | (20 × 200) / 2 (chip payout: 0, bet returned) |
| 15 | 100 | Bust | 0 | Bust = zero damage |
| 19 | 100 | Loss | 0 | Dealer 20 beats 19, zero damage |

**Success Metrics:**
- Damage formula produces intuitive results (high hand + high bet = high damage)
- Push damage always rounds down (integer division)
- Zero damage on bust/loss (logged correctly)
- Blackjack deals same damage as regular 21 win (consistency verified)
- Stats tracking shows damage breakdown by source (turn_win vs turn_push)
