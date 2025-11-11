# Card Fifty-Two: Combat System

## Core Concept

Blackjack is the combat system. Every hand you play is an attack. Every bet you place is damage potential. The casino dealers are enemies with HP bars and special abilities.

---

## Combat Loop

### Round Structure

```
1. Both place bets
   - Player bets chips (1/5/10)
   - Enemy threatens chips based on chip_threat

2. Cards dealt
   - Standard blackjack rules apply

3. Player turn
   - Hit/Stand/Double = combat actions

4. Dealer turn
   - Dealer follows standard rules
   - Abilities may trigger

5. Resolve hand
   - Winner deals damage to loser
   - Damage = (bet amount ÷ 10)

6. Check for defeat
   - Enemy HP reaches 0 = Victory
   - Player runs out of chips = Defeat

7. Next round (if combat continues)
```

---

## Damage Calculation

### Formula: `Damage = (Bet Amount ÷ 10)`

**Examples:**
- Bet 10 chips, win hand → Deal 1 HP damage to enemy
- Bet 50 chips, win hand → Deal 5 HP damage to enemy
- Lose hand → Take damage equal to bet lost ÷ 10

### Special Multipliers

- **Blackjack**: 2x damage multiplier (bet × 2 ÷ 10)
- **Push**: No damage dealt or taken
- **Bust**: Take full damage from bet lost

**Example Combat:**
```
Enemy: "The Broken Dealer" (50 HP, chip_threat 5)

Round 1:
- Player bets 10 chips
- Player wins hand
- Enemy takes 1 damage (HP: 49/50)

Round 2:
- Player bets 10 chips
- Player gets blackjack
- Enemy takes 2 damage (10×2÷10) (HP: 47/50)

Round 3:
- Player bets 10 chips
- Player busts
- Player takes 1 damage (conceptual - chips are HP)
```

---

## Enemy Structure

### Enemy_t Entity

```c
typedef struct Enemy {
    dString_t* name;                // "The Broken Slot Machine"
    int max_hp;                     // Maximum HP
    int current_hp;                 // Current HP
    int chip_threat;                // Chips at risk per round (damage source)
    dArray_t* passive_abilities;    // Always-active abilities
    dArray_t* active_abilities;     // Triggered abilities

    // Portrait system (hybrid for dynamic effects)
    SDL_Surface* portrait_surface;  // Source pixel data (for manipulation)
    SDL_Texture* portrait_texture;  // Cached GPU texture (for rendering)
    bool portrait_dirty;            // Rebuild texture when true

    bool is_defeated;               // HP <= 0
} Enemy_t;
```

### Constitutional Compliance

✅ Uses `dString_t*` for name (not `char[]`)
✅ Uses `dArray_t*` for abilities (not raw arrays)
✅ Heap-allocated entity (like Player_t)
✅ Integrates with existing chip/bet system

---

## Ability System

### Passive Abilities (Always Active)

#### ABILITY_HOUSE_ALWAYS_WINS
- **Effect**: Dealer wins all ties (pushes become losses)
- **Gameplay Impact**: Player must win decisively, no safe pushes
- **Example Enemy**: "The Unfair Odds" (30 HP, attack 5)

#### ABILITY_CARD_COUNTER
- **Effect**: Player's hole card is revealed to enemy AI
- **Gameplay Impact**: Dealer makes optimal decisions against player's hand
- **Example Enemy**: "The Surveillance System" (40 HP, attack 5)

#### ABILITY_LOADED_DECK
- **Effect**: Dealer's face-down card is always 10-value
- **Gameplay Impact**: Dealer starts with guaranteed 10+ hand
- **Example Enemy**: "The Rigged Game" (35 HP, attack 7)

#### ABILITY_CHIP_DRAIN
- **Effect**: Player loses 5 chips per round (before betting)
- **Gameplay Impact**: Time pressure, must win quickly
- **Example Enemy**: "The Slot Machine Demon" (45 HP, attack 5)

#### ABILITY_PRESSURE
- **Effect**: Sanity drains 2x faster during combat
- **Gameplay Impact**: Low sanity runs become even harder
- **Example Enemy**: "The Whispering Cards" (40 HP, attack 6)

---

### Active Abilities (Triggered by Conditions)

#### ABILITY_DOUBLE_OR_NOTHING (Trigger: 50% HP)
- **Effect**: Forces player to double bet or auto-lose hand
- **Gameplay Impact**: High-risk moment, must have enough chips to double
- **Example Enemy**: "The Gambler's Fallacy" (60 HP, attack 8)

#### ABILITY_RESHUFFLE_REALITY (Trigger: Once per combat)
- **Effect**: Replaces player's current hand with random cards
- **Gameplay Impact**: Can turn winning hand into losing hand instantly
- **Example Enemy**: "The Chaos Dealer" (50 HP, attack 6)

#### ABILITY_HOUSE_RULES (Trigger: 25% HP, Phase 2)
- **Effect**: Changes blackjack rules mid-fight
  - Dealer hits on soft 17
  - Blackjack pays 1:1 (not 3:2)
  - Player cannot double down
- **Gameplay Impact**: Removes player advantages, harder to finish fight
- **Example Enemy**: "The Rule Bender" (70 HP, attack 7)

#### ABILITY_ALL_IN (Trigger: 25% HP)
- **Effect**: Both player and enemy bet ALL chips, winner takes all
- **Gameplay Impact**: High-stakes final gambit, can end fight or ruin player
- **Example Enemy**: "The Desperate Machine" (55 HP, attack 10)

#### ABILITY_GLITCH (Trigger: Random, 15% chance/round)
- **Effect**: Dealer's bust becomes 21 (auto-win for dealer)
- **Gameplay Impact**: Unpredictable, undermines player's strategy
- **Example Enemy**: "The Malfunctioning Unit" (40 HP, attack 5)

---

## Ability Trigger System

### Trigger Types

```c
typedef enum {
    TRIGGER_NONE,               // Passive (no trigger)
    TRIGGER_HP_THRESHOLD,       // At X% HP
    TRIGGER_ONCE_PER_COMBAT,    // Only triggers once
    TRIGGER_RANDOM,             // Random chance each round
    TRIGGER_PLAYER_ACTION       // Triggered by player action
} AbilityTrigger_t;
```

### Implementation

- **CheckAbilityTriggers()**: Called at start of each combat round
- **HP Thresholds**: Checked against current HP percentage
- **One-Time Flags**: `has_triggered` prevents repeat triggers
- **Random Rolls**: RNG check each round for TRIGGER_RANDOM

---

## Integration with Game State

### GameContext_t Additions

```c
typedef struct GameContext {
    // ... existing fields ...

    // Combat system
    Enemy_t* current_enemy;      // Current combat enemy (NULL if not in combat)
    bool is_combat_mode;         // true if currently in combat
} GameContext_t;
```

### Combat Flow in State Machine

```
STATE_BETTING:
  - If is_combat_mode:
    - Show enemy HP bar
    - Enemy places bet (chip_threat)
    - CheckAbilityTriggers(enemy)

STATE_DEALING:
  - Deal cards normally

STATE_PLAYER_TURN:
  - Player actions (hit/stand/double)
  - Abilities may modify available actions

STATE_DEALER_TURN:
  - Dealer plays by rules
  - Abilities may modify dealer behavior

STATE_SHOWDOWN:
  - ResolveRound() applies damage
  - TakeDamage(enemy, damage_dealt)
  - Check enemy->is_defeated
  - If defeated: Trigger victory sequence

STATE_ROUND_END:
  - Display combat results
  - If enemy defeated: Exit combat, rewards
  - Else: Continue to next round
```

---

## Example Enemies

### 1. The Broken Dealer (Tutorial Enemy)
- **HP**: 50
- **Attack**: 5 (bets 5 chips/round)
- **Abilities**: None
- **Strategy**: Practice enemy, no tricks

### 2. The Unfair Odds
- **HP**: 30
- **Attack**: 5
- **Abilities**: ABILITY_HOUSE_ALWAYS_WINS (passive)
- **Strategy**: Must beat dealer decisively, no pushes allowed

### 3. The Surveillance System
- **HP**: 40
- **Attack**: 5
- **Abilities**: ABILITY_CARD_COUNTER (passive)
- **Strategy**: Dealer knows your hole card, plays optimally

### 4. The Gambler's Fallacy
- **HP**: 60
- **Attack**: 8
- **Abilities**: ABILITY_DOUBLE_OR_NOTHING (50% HP trigger)
- **Strategy**: Save chips for forced double at 30 HP

### 5. The Rule Bender (Mid-Boss)
- **HP**: 70
- **Attack**: 7
- **Abilities**:
  - ABILITY_CHIP_DRAIN (passive)
  - ABILITY_HOUSE_RULES (25% HP trigger)
- **Strategy**: Chip drain adds time pressure, rule change makes endgame brutal

### 6. The Malfunctioning Unit (Chaos Enemy)
- **HP**: 40
- **Attack**: 5
- **Abilities**: ABILITY_GLITCH (random, 15% chance/round)
- **Strategy**: Unpredictable, can turn sure wins into losses

---

## Player HP System (Future)

Currently, player HP is conceptual (chip count = effective HP). Future implementation options:

### Option 1: Chips ARE HP
- Run out of chips = defeat
- Simple, already functional
- Risk: Low chip count = can't bet = can't deal damage

### Option 2: Separate HP Bar
- Player has HP independent of chips
- Losing hands damages HP, not just chips
- Allows "glass cannon" builds (low HP, high chips)

### Option 3: Sanity as HP
- Sanity meter doubles as HP
- 0 sanity = death
- Ties sanity narrative to survival

**Current Implementation**: Option 1 (chips = HP)

---

## Combat Victory/Defeat

### Victory Conditions
- Enemy HP reaches 0
- Enemy is_defeated = true

### Victory Rewards
- Chips earned (enemy's remaining bet pool?)
- Sanity restored (+10?)
- Story progression unlocked
- Next enemy unlocked

### Defeat Conditions
- Player chips reach 0 (can't place bet)
- Player HP reaches 0 (if separate HP system)

### Defeat Consequences
- Lose chips bet in current round
- Sanity damage (-10)
- Retry combat or return to menu

---

## Design Philosophy

### Why Blackjack as Combat?

1. **Familiar Mechanics**: Players already know how to play
2. **Risk/Reward**: Betting = damage potential = strategic depth
3. **Tension**: Each hand matters, no grinding
4. **Absurdist Horror**: Fighting for your life at a card table

### Damage Scaling

**÷10 scaling** converts chips to HP:
- 10 chips bet = 1 HP damage
- 50 chips bet = 5 HP damage
- 100 chips bet = 10 HP damage

This keeps combat length reasonable:
- Typical enemy: 30-70 HP
- Typical bet: 5-10 chips
- Typical damage: 0.5-1 HP per round
- Combat duration: ~10-20 rounds

### Ability Design Principles

- **Passive abilities** change the rules (always active)
- **Active abilities** create dramatic moments (triggered)
- **HP thresholds** create phase transitions (50%, 25%)
- **Random abilities** add chaos (unpredictable)

---

## Technical Implementation Notes

### Creating an Enemy

```c
// Create enemy
Enemy_t* enemy = CreateEnemy("The Broken Dealer", 50, 5);

// Add passive ability
AddPassiveAbility(enemy, ABILITY_HOUSE_ALWAYS_WINS);

// Add active ability (triggers at 50% HP)
AddActiveAbility(enemy, ABILITY_DOUBLE_OR_NOTHING,
                 TRIGGER_HP_THRESHOLD, 0.5f);

// Start combat
game->current_enemy = enemy;
game->is_combat_mode = true;
```

### Checking Ability Triggers

```c
// Called at start of each round
void CheckAbilityTriggers(Enemy_t* enemy) {
    float hp_percent = GetEnemyHPPercent(enemy);

    for (each active_ability) {
        if (trigger == TRIGGER_HP_THRESHOLD) {
            if (hp_percent <= trigger_value && !has_triggered) {
                TriggerAbility(enemy, ability);
                has_triggered = true;
            }
        }
    }
}
```

### Applying Damage

```c
// In ResolveRound() after determining winner
if (game->is_combat_mode && game->current_enemy) {
    int damage = player->current_bet / 10;  // Damage scaling

    if (player_won) {
        TakeDamage(game->current_enemy, damage);
    }

    if (game->current_enemy->is_defeated) {
        TriggerCombatVictory(game);
    }
}
```

---

## Future Expansions

### Planned Features

1. **Player Special Moves**
   - Consume cards permanently for powerful effects
   - "Burn 3 cards to force dealer re-deal"
   - "Discard pile powers?"

2. **Deck as Resource**
   - 52 cards total, discard pile visible
   - Running out of cards = forced shuffle or penalty
   - Special moves consume deck

3. **Multi-Enemy Encounters**
   - Fight 2-3 weak enemies simultaneously
   - Split bet between targets
   - Defeat all to win

4. **Boss Abilities**
   - The Dragon: Environmental destruction mid-fight
   - The Dogma: Meta-abilities (breaks UI, fourth wall)

5. **Character Class Synergies**
   - The Desperate: Gains damage on reckless bets
   - The Dealer: Can disable enemy abilities
   - The Detective: Reveals enemy's next action
   - The Dreamer: Manipulates probabilities

---

*"The cards are weapons. The bets are ammunition. The table is a battlefield."*
