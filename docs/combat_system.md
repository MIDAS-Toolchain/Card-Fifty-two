# Combat System Design

## Philosophy

Combat in Card Fifty-Two is **purely economic** - damage is dealt through betting and winning hands, not through a separate health/damage system for the player.

## Core Mechanics

### Damage Formula

**Player deals damage to enemy:**
```
damage = hand_value × bet_amount
```

**Examples:**
- Bet 10, score 21 → Deal 210 damage
- Bet 5, score 19 → Deal 95 damage
- Bet 1, score 18 → Deal 18 damage

**Rationale:** Higher bets = more risk = more reward (damage)

### Win Conditions

**Enemy Defeat:**
- Enemy HP reaches 0
- Player wins combat encounter
- Receives rewards (chips, items, progression)

**Player Defeat (Game Over):**
- Player chips reach 0
- Cannot place minimum bet
- Game over state triggered

### Player "Health" System

**Chips = Health:**
- Player chips represent economic health
- Losing hands costs bet amount (already deducted via `PlaceBet()`)
- **NO additional combat damage taken by player**
- Running out of chips = game over (can't bet anymore)

### Combat Flow

```
┌─────────────────────────────────────────────────────────────┐
│ BETTING PHASE                                               │
│ - Player places bet (chips deducted immediately)            │
│ - Bet amount determines potential damage if player wins     │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ PLAYING PHASE                                               │
│ - Standard blackjack gameplay                               │
│ - Player makes decisions (Hit/Stand/Double/etc)             │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ RESOLUTION PHASE                                            │
│                                                             │
│ ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐ │
│ │ PLAYER WINS     │  │ PLAYER LOSES    │  │ PUSH        │ │
│ ├─────────────────┤  ├─────────────────┤  ├─────────────┤ │
│ │ • Payout chips  │  │ • Bet lost      │  │ • Bet back  │ │
│ │ • Deal damage   │  │ • NO damage     │  │ • No damage │ │
│ │   to enemy      │  │   taken         │  │             │ │
│ │ • damage =      │  │ • Chip economy  │  │             │ │
│ │   hand × bet    │  │   punishment    │  │             │ │
│ └─────────────────┘  └─────────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│ CHECK WIN/LOSE CONDITIONS                                   │
│ - Enemy HP ≤ 0? → Combat victory                            │
│ - Player chips ≤ 0? → Game over                             │
│ - Otherwise → Next round                                    │
└─────────────────────────────────────────────────────────────┘
```

## Implementation Details

### Damage Calculation (src/game.c)

```c
// Win conditions - deal damage to enemy
if (player_wins) {
    int hand_value = player->hand.total_value;
    int bet_amount = player->current_bet;
    int damage_dealt = hand_value * bet_amount;  // NO division!

    if (game->is_combat_mode && game->current_enemy) {
        TakeDamage(game->current_enemy, damage_dealt);
        TweenEnemyHP(game->current_enemy);
        SpawnDamageNumber(damage_dealt, x, y, false);
    }
}

// Lose condition - just lose bet (already deducted in PlaceBet)
if (player_loses) {
    LoseBet(player);  // Resets current_bet to 0
    // NO combat damage taken by player!
}
```

### Chip Economy

**Chip Sources (gaining chips):**
- Winning hands (1:1 payout)
- Blackjack (3:2 payout)
- Dealer bust
- Combat rewards (defeating enemies)

**Chip Sinks (losing chips):**
- Losing hands (bet amount lost)
- Busting (bet amount lost)
- Dealer wins (bet amount lost)

**Critical Threshold:**
- When chips < minimum bet → Cannot continue
- Game over state triggered

## Balance Considerations

### Risk vs Reward

**High Bets:**
- ✅ Deal massive damage on wins
- ❌ Lose significant chips on losses
- ❌ Risk game over if chips run low

**Low Bets:**
- ✅ Safe, sustainable chip economy
- ❌ Deal minimal damage
- ❌ Combat takes many rounds

**Strategy:**
- Bet high when confident (good hand, dealer showing weak card)
- Bet low when uncertain (risky hand, dealer showing Ace/face card)
- Manage chip economy to avoid game over

### Enemy Scaling

**Enemy HP determines encounter length:**
- Low HP (100-200): 2-5 rounds at moderate bets
- Medium HP (300-500): 5-10 rounds
- High HP (1000+): Boss encounter, requires strategic betting

**Chip threat determines minimum bet:**
```c
// Enemy forces minimum bet = chip_threat
int min_bet = enemy->chip_threat;
```

## Forbidden Anti-Patterns

### ❌ DON'T: Player Takes Combat Damage

```c
// WRONG - Player should never take combat damage
if (player_loses) {
    int damage_taken = bet_amount / 10;
    player->chips -= damage_taken;  // NO!
}
```

**Why wrong:**
- Bet already deducted via `PlaceBet()`
- Double punishment for losing
- Violates economic combat design

### ❌ DON'T: Divide Damage by 10

```c
// WRONG - Damage should be full hand × bet
damage_dealt = (hand_value * bet_amount) / 10;  // NO!
```

**Why wrong:**
- Makes damage trivial (bet 10, score 21 = only 21 damage)
- Breaks risk/reward balance
- Combat takes too long

### ✅ DO: Pure Economic Combat

```c
// CORRECT - Full damage, no player combat damage
damage_dealt = hand_value * bet_amount;  // Full damage!

if (player_loses) {
    LoseBet(player);  // Bet already lost in PlaceBet()
    // NO additional chip deduction!
}
```

## Future Enhancements

**Possible additions (not yet implemented):**
- Critical hits (blackjack deals 2× damage)
- Combo multipliers (consecutive wins increase damage)
- Enemy abilities (force minimum bet, reduce payout, etc.)
- Chip recovery items (heal chips between rounds)

---

*Last Updated: 2025-10-21*
*Part of Card Fifty-Two Design Documentation*
