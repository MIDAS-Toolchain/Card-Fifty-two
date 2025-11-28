# Card Fifty-Two - Build Opportunity Systems (Designer Reference)

**Last Updated:** 2025-11-28
**Purpose:** Comprehensive game design documentation for build diversity and player power progression
**Companion Document:** [GAME_DESIGN_SUMMARY.md](GAME_DESIGN_SUMMARY.md)

---

## Table of Contents

1. [Philosophy](#philosophy)
2. [Class System](#class-system)
3. [Trinket System](#trinket-system)
4. [Class Trinket Upgrades](#class-trinket-upgrades)
5. [Event System](#event-system)
6. [Card Tag System](#card-tag-system)
7. [Build Archetypes](#build-archetypes)
8. [Progression Pacing](#progression-pacing)
9. [Implementation Status](#implementation-status)

---

## Philosophy

Card Fifty-Two gives players **heavy agency** in shaping their run while maintaining **high RNG variance** to reward skilled play and adaptability.

### Core Tenets

1. **Decision Making > Luck**: The best players adapt to bad RNG with smart choices
2. **Permanent Power**: All upgrades (tags, trinkets, class upgrades) persist throughout the run
3. **Synergistic Depth**: Systems interact (trinkets buff tags, events unlock based on tags, class abilities scale from actives)
4. **Knowledge Rewards**: Experienced players can unlock hidden event choices and build toward specific synergies
5. **Resource Management**: Chips (HP), sanity, and bet sizing create constant tension

### Build Opportunity Touchpoints

Every run provides **5 major systems** for player customization:

| System | Frequency | Permanence | RNG Factor | Skill Factor |
|--------|-----------|------------|------------|--------------|
| **Class Selection** | Once (start of run) | Permanent | None (player choice) | High (class mastery) |
| **Trinkets** | Every combat win | Permanent (6 slots) | High (random affixes) | Medium (keep/sell decisions) |
| **Class Upgrades** | Every boss kill | Permanent | Medium (3 options) | High (build planning) |
| **Events** | Every N encounters | Temporary or permanent | Medium (event pool) | High (choice consequences) |
| **Card Tags** | Every combat win | Permanent | Low (3 tag choices) | High (deck strategy) |

---

## Class System

Players choose their class at the **start of the run**. Each class has:
- **Unique class trinket** (active ability + passive damage dealer)
- **4 sanity threshold effects** (75%, 50%, 25%, 0% sanity)

### Design Intent

Classes provide **identity and playstyle direction** without hard-locking strategies. Each class excels at a playstyle but can pivot based on trinket/tag RNG.

---

### The Degenerate

**Theme:** Gambling addict seeking thrills through high-risk, high-reward plays
**Difficulty:** Beginner-friendly (simple passive, forgiving sanity effects)
**Archetype:** Aggressive damage scaling via risky hits

#### Class Trinket: Degenerate's Gambit

**Active: "Double Down"**
- **Effect:** Select a card in play with value d5, double that card's value for this turn
- **Cooldown:** 3 turns
- **Targeting:** Any card in player/dealer hand
- **Synergies:**
  - Pairs with Aces (1’2 or 11’22 for bust into discard strats)
  - Works with 2-5 value cards to hit 21 from 11-15 hands
- **Side Effect:** Each active use increases passive damage by +5

**Passive: "Reckless Payoff"**
- **Trigger:** When you HIT on hand value 15+
- **Effect:** Deal 10 damage to enemy (+5 damage per active use)
- **Scaling:** Permanent, scales infinitely
- **Synergies:**
  - Rewards aggressive play (hitting on 15-16 instead of standing)
  - Combines with LUCKY tags (crit chance on risky hits)
  - Pairs with status effects that force high-value hands

#### Sanity Thresholds: "Double Down"

**Philosophy:** Sanity loss forces **all-in aggression** by removing safe betting options.

| Sanity % | Effect | Design Intent |
|----------|--------|---------------|
| **100-76%** | No effect | Safe baseline |
| **75-51%** | Lose access to **Minimum Bet** (1 chip) | Push toward medium/high bets |
| **50-26%** | **Maximum bet doubled** (10’20) | Reward high-risk plays with higher damage ceiling |
| **25-1%** | Lose access to **Medium Bet** (5 chips) | Only min or max bets (extreme variance) |
| **0%** | **Maximum bet doubled again** (20’40), "All or nothing" | Ultimate risk/reward endgame |

**Cumulative Effects:** Each threshold **stacks** with previous ones. At 0% sanity:
- MIN bet disabled (from 75% threshold)
- MAX bet = 40 chips (doubled twice)
- MED bet disabled (from 25% threshold)
- **Result:** Only option is 40-chip all-in bets (massive damage or instant loss)

**Strategy Implications:**
- Low sanity = **glass cannon** (huge damage potential, deadly losses)
- Synergizes with chip-refund trinkets (Insurance Policy, Elite Membership)
- Anti-synergy with chip-drain status effects (TILT, CHIP_DRAIN)

---

### The Dealer

**Theme:** Former casino employee, trapped and forgotten
**Difficulty:** Intermediate (control-focused, requires enemy knowledge)
**Archetype:** Information advantage and deck manipulation

#### Class Trinket: Dealer's Badge

**Active: "Mulligan"**
- **Effect:** Discard both player and enemy hands, redraw cards for each
- **Cooldown:** 5 turns
- **Special:** Counts as 1 card draw for ability/trinket counters (triggers CARD_DRAWN events)
- **Synergies:**
  - Escape bad hands (player at 16, dealer showing Ace)
  - Trigger CARD_DRAWN passives (Counting Cards trinket, CURSED tags)
  - Reset deck position mid-combat
- **Side Effect:** Each active use increases passive damage by +10

**Passive: "House Rules"**
- **Trigger:** On deck reshuffle
- **Effect:** Deal 50 damage to enemy (+10 damage per active use)
- **Scaling:** Permanent, scales infinitely
- **Synergies:**
  - Combos with abilities that force reshuffles (Didact's System Override)
  - Rewards long combats (deck naturally reshuffles after ~26 cards)
  - Can be forced via active mulligan spam

#### Sanity Thresholds: "Careful Control"

**Philosophy:** Sanity loss trades **betting freedom** for **information and survivability**.

| Sanity % | Effect | Design Intent |
|----------|--------|---------------|
| **100-76%** | No effect | Baseline |
| **75-51%** | Lose access to **Maximum Bet** (10 chips) | Safer play, lower damage ceiling |
| **50-26%** | **Enemy face-down card always visible** | Massive information advantage (know dealer total) |
| **25-1%** | **Draw 3 cards on turn start; if bust, discard highest card** | Survivability boost (bust protection) |
| **0%** | **Auto-play: Hit on d16, Stand on e17** | Lose control, optimal basic strategy |

**Strategy Implications:**
- Low sanity = **defensive powerhouse** (can't bet high, but hard to lose)
- 50% sanity breakpoint is **game-changing** (perfect information on dealer hand)
- 0% sanity removes player agency but plays optimally (blackjack basic strategy)
- Synergizes with chip-gain trinkets (Golden Horseshoe, Loaded Dice)

---

### The Detective

**Theme:** Obsessive investigator seeking casino's secrets
**Difficulty:** Advanced (pattern-recognition, requires hand analysis)
**Archetype:** Combo/synergy focus via pairs and pattern matching

#### Class Trinket: Detective's Files

**Active: "Case Notes"**
- **Effect:** Target two cards in play. Second card becomes a copy of first card for this turn.
- **Cooldown:** 5 turns
- **Targeting:** Any 2 cards in player/dealer hands
- **Synergies:**
  - Create pairs on demand (copy Ace’Ace’Ace for pair damage)
  - Force specific hand values (copy 10’10’20 total)
  - Combo with passive (more pairs = more damage)
- **Side Effect:** Each active use increases passive damage by +5

**Passive: "Pattern Recognition"**
- **Trigger:** Passive (always active)
- **Effect:** Deal 5 damage for each **pair** in play (player hand + dealer hand combined)
- **Scaling:** +5 damage per active use
- **Synergies:**
  - Rewards natural pairs (deck has 4 of each rank)
  - Combos with active to force pairs
  - Scales in long hands (more cards = more pair chances)

**Example:** Player has [7`, 7e, Kc], Dealer has [10f, 10`]
- Player has 1 pair (7s), Dealer has 1 pair (10s) = **2 pairs total**
- Damage: 5 × 2 = **10 damage** (before scaling)

#### Sanity Thresholds: "Pattern Recognition"

**Philosophy:** Sanity loss creates **forced patterns** that constrain but reward analysis.

| Sanity % | Effect | Design Intent |
|----------|--------|---------------|
| **100-76%** | No effect | Baseline |
| **75-51%** | **Can only do Medium bets** (5 chips) | Consistent damage, predictable economy |
| **50-26%** | **Medium bet = enemy's score from last round** | Dynamic betting (scales with enemy performance) |
| **25-1%** | **Must HIT if your total < enemy's visible card value** | Forced aggression based on information |
| **0%** | **Pairs in hand count double for damage calculation** | Massive damage spike (2 pairs ’ 4 pairs worth) |

**Strategy Implications:**
- 50% threshold creates **adaptive betting** (bet scales with enemy threat)
- 25% threshold rewards **information play** (visible card = bet decision)
- 0% threshold is **explosive damage** (pairs deal 10 base × 2 = 20 per pair)
- Synergizes with pair-generating effects (Detective's Files active)

---

### The Dreamer (Planned)

**Theme:** Reality-bender, luck manipulation
**Status:** Not yet implemented
**Archetype:** Chaos/RNG manipulation

**Placeholder Abilities:**
- Active: Reroll hand or deck shuffle
- Passive: Bonus damage on specific card combinations (e.g., suited pairs)
- Sanity: Randomized bet amounts, card effects trigger twice, etc.

---

## Trinket System

Trinkets are **equipment with passive effects** that drop after **every combat victory**. Players can equip up to **6 trinkets** (plus 1 class trinket).

### Drop System

| Enemy Type | Common % | Uncommon % | Rare % | Legendary % |
|------------|----------|------------|--------|-------------|
| **Normal** | 80% | 20% | 0% | 0% |
| **Elite** | 30% | 50% | 20% | 0% |
| **Boss** | 0% | 30% | 50% | 20% |

**Pity System:**
- Normal enemies: Every 5 kills without Uncommon+ ’ **force Uncommon**
- Elite enemies: Every 5 kills without Legendary ’ **force Legendary**

### Affix System (DUF-Based Trinkets)

**Implementation Status:**  **Fully Designed** (see [TRINKET_DUF_INVESTIGATION.md](TRINKET_DUF_INVESTIGATION.md))

Each trinket has:
1. **Base Passive** (from DUF template, e.g., "Lucky Chip" ’ gain LUCKY_STREAK on win)
2. **Affixes** (1-3 random stat bonuses based on Act tier)

#### Example: Trinket Drop in Act 2

**Rolled Trinket:** Lucky Chip (Common rarity)
- **Base Passive:** On win: +1 LUCKY_STREAK (status effect), On loss: clear LUCKY_STREAK
- **Tier:** 2 (Act 2 = 2 affixes)
- **Affix 1:** Violent (+12% damage dealt)
- **Affix 2:** Deadly (+18% crit damage)
- **Sell Value:** 40 chips (base 30 + tier scaling)

**Result:** Player gets both the LUCKY_STREAK buildup **and** flat damage/crit scaling.

#### Affix Pool (from [combat_affixes.duf](data/affixes/combat_affixes.duf))

| Affix | Stat | Value Range | Weight | Category |
|-------|------|-------------|--------|----------|
| **Violent** | +X% damage dealt | 5-20% | 100 | Offensive |
| **Precise** | +X% crit chance | 3-9% | 100 | Offensive |
| **Deadly** | +X% crit damage | 10-25% | 90 | Offensive |
| **Brutal** | +X flat damage | 5-20 | 80 | Offensive |
| **Savior** | X% chance to discard highest card on bust | 3-9% | 60 | Defensive |
| **Insured** | Refund X% chips on loss | 10-30% | 50 | Defensive |
| **Greedy** | +X% chips on win | 10-30% | 50 | Economic |
| **Cushioned** | +X flat chips on loss | 2-6 | 30 | Economic |
| **Prosperous** | +X flat chips on win | 3-9 | 25 | Economic |

**Effective Weight Calculation:**
```c
effective_weight = base_weight + rarity_bonus
// Common: +0, Uncommon: +5, Rare: +10, Legendary: +15
```

**Example:** "Violent" affix (weight 100):
- On Common trinket: 100 weight
- On Legendary trinket: 115 weight (15% more likely than Common)

---

### Trinket Stacking System

Some trinkets have **persistent stacks** that carry across combats:

#### Example: Broken Watch
- **Passive:** On combat start: +1 stack
- **Stack Effect:** Each stack = +2% damage
- **Stack Max:** 12 stacks = **24% damage bonus**
- **Persistence:** Stacks never decay (permanent power growth)

**Design Intent:** Rewards long runs (more combats = more stacks = exponential scaling).

#### Example: Iron Knuckles (Uncommon)
- **Passive:** On combat start: +1 stack
- **Stack Effect:** Each stack = +5 flat damage
- **Stack Max:** 10 stacks = **50 flat damage**
- **Persistence:** Stacks never decay

**Comparison:**
- Broken Watch = **percentage scaling** (better for high-bet builds)
- Iron Knuckles = **flat scaling** (better for low-bet, high-frequency builds)

---

### Tag Buff System

Some trinkets **buff specific card tags**:

#### Example: Cursed Skull (Legendary)
- **On Equip:** Add CURSED tag to 4 random cards
- **On Equip:** CURSED cards deal **+5 damage** (15 total instead of 10)
- **Persistence:** Tag damage buff lasts until trinket is sold

**Design Intent:**
- Creates **tag-focused archetypes** (cursed-damage build)
- Rewards deck knowledge (knowing which cards are CURSED)
- Synergizes with events that add more CURSED tags

---

### Example Trinkets (Combat Drops)

#### Lucky Chip (Common)
**Passive:**
- **On Win:** Gain 1 stack of LUCKY_STREAK (+5% crit chance per stack, max 10)
- **On Loss:** Clear all LUCKY_STREAK stacks

**Design Intent:** High-variance streakiness (win streaks = massive crit, loss = reset)

---

#### Loaded Dice (Common)
**Passive:**
- **On Win:** +5 chips (flat bonus)

**Design Intent:** Economic consistency (slow chip buildup)

---

#### Gambler's Coin (Common)
**Passive:**
- **On Blackjack:** +10 chips

**Design Intent:** Rewards natural 21s (encourages hitting vs standing at 20)

---

#### Tarnished Medal (Common)
**Passive:**
- **On Bust:** Refund 15% of bet

**Design Intent:** Bust protection (risk mitigation for aggressive play)

---

#### Rabbit's Foot (Uncommon)
**Passive:**
- **On Bust:** Refund 25% of bet

**Design Intent:** Better Tarnished Medal (risk mitigation upgrade)

---

#### Counting Cards (Uncommon)
**Passive:**
- **On Card Drawn:** +2 flat damage (per card)

**Design Intent:** Rewards long hands (hit spam = damage spam)

---

#### Lucky Seven (Uncommon)
**Passive:**
- **On Blackjack:** +15 chips

**Design Intent:** Better Gambler's Coin (blackjack reward upgrade)

---

#### Insurance Policy (Uncommon)
**Passive:**
- **On Loss:** Refund 30% of bet

**Design Intent:** Loss mitigation (encourages high bets with safety net)

---

#### Blood Ante (Rare)
**Passive:**
- **On Combat Start:** Lose 3 chips, gain BLOOD_RAGE (+8% damage for 1 round)

**Design Intent:** Self-damage for power (high-risk aggression)

---

#### Dealer's Tell (Rare)
**Passive:**
- **On Push (Tie):** Deal 50% damage anyway

**Design Intent:** Turns neutral outcomes into wins (reduces variance)

---

#### High Roller's Chip (Rare)
**Passive:**
- **On Win (if bet e20):** +20 chips bonus

**Design Intent:** Rewards high-bet aggression (synergy with Degenerate low-sanity)

---

#### Cold Deck (Rare)
**Passive:**
- **On Combat Start:** +3 stacks (each stack = +3% damage, max 15 stacks = 45%)

**Design Intent:** Faster stacking than Broken Watch (front-loaded power)

---

#### Ace's Sleeve (Legendary)
**Passive:**
- **On Blackjack:** Deal **double damage** (200% = 2×)

**Design Intent:** Massive burst damage on natural 21s

---

#### Devil's Bargain (Legendary)
**Passive:**
- **On Combat Start:** Lose 5 chips, +10 flat damage for this round

**Design Intent:** Self-damage for huge power spike (risky but explosive)

---

#### Golden Horseshoe (Legendary)
**Passive:**
- **On Win:** +30 chips

**Design Intent:** Economic powerhouse (huge chip snowball)

---

### Event Trinkets (Event-Only Rewards)

#### Elite Membership (Event)
**Passive:**
- **On Win:** +20% chip bonus
- **On Loss:** Refund 20% of bet

**Design Intent:** VIP rewards (both sides of economy buffed)

**Event Source:** "House Odds" event (tutorial act)

---

#### Stack Trace (Event)
**Passive:**
- **On Combat Start:** +1 stack (each stack = +3% damage, max 20 stacks = 60%)

**Design Intent:** Slow but infinite scaling (event reward = better than combat trinkets)

**Event Source:** "System Maintenance" event (tutorial act)

---

## Class Trinket Upgrades

**Status:**   **Designed but NOT Implemented**

After defeating each **boss enemy**, players are offered **3 class trinket upgrades** (instead of a trinket drop). Each upgrade:
1. **Modifies active OR passive** (or both) of the class trinket
2. **Comes paired with a random affix** (from combat affix pool)

### Design Intent

- Provides **permanent power growth** tied to class identity
- Creates **build divergence** (same class, different upgrade paths)
- Rewards **boss mastery** (better players see more upgrades)

---

### Degenerate's Gambit Upgrade Pool (Examples)

#### Upgrade: "Greedy Gamble"
**Active Change:** Can now target cards with value d**7** (was d5)
**Affix:** Violent (+15% damage dealt)

**Design Intent:** More flexible active (can double 6s and 7s)

---

#### Upgrade: "Reckless Abandon"
**Passive Change:** Trigger threshold lowered to **14** (was 15)
**Affix:** Precise (+6% crit chance)

**Design Intent:** Triggers more often (hitting on 14 is riskier but more frequent)

---

#### Upgrade: "All In"
**Active Change:** Cooldown reduced to **2 turns** (was 3)
**Affix:** Deadly (+20% crit damage)

**Design Intent:** Spam the active more (faster passive scaling)

---

#### Upgrade: "High Stakes"
**Passive Change:** Base damage increased to **15** (was 10)
**Affix:** Brutal (+10 flat damage)

**Design Intent:** Front-loaded damage boost

---

#### Upgrade: "House Edge"
**Active Change:** Can now target **face cards (10-value)** to turn them into **11s**
**Affix:** Greedy (+25% chips on win)

**Design Intent:** Exotic active option (turn 10’11 for precise hand values)

---

#### Upgrade: "Exponential Greed"
**Passive Change:** Scaling increased to **+10 per active use** (was +5)
**Affix:** Insured (Refund 20% chips on loss)

**Design Intent:** Faster exponential scaling (endgame power spike)

---

### Upgrade Selection UI (Mockup)

```
                                                                 
                     BOSS DEFEATED!                              
                                                                 
  Choose one upgrade for Degenerate's Gambit:                   
                                                                 
                                                         
   Greedy Gamble     Reckless          All In            
                     Abandon                             
   Target d7         Trigger on 14     Cooldown: 2       
   (was d5)          (was 15)          (was 3)           
                                                         
   + Violent         + Precise         + Deadly          
   (+15% damage)     (+6% crit)        (+20% crit dmg)   
                                                         
                                                                 
  [SELECT]            [SELECT]            [SELECT]              
                                                                 
```

---

### Upgrade Pool Expansion

Each class should have **~10-15 possible upgrades** in its pool:
- **5 Active Upgrades** (cooldown, targeting, side effects)
- **5 Passive Upgrades** (damage, trigger, scaling)
- **3 Hybrid Upgrades** (modify both active + passive)

**Boss Count per Run:** 3 bosses (Acts 1-3) = **3 upgrades per run**

**Build Divergence:** 10 upgrades choose 3 = **120 unique upgrade combinations**

---

## Event System

Events are **Slay the Spire-style encounters** with narrative choices and mechanical consequences.

### Event Structure

Each event has:
- **Title + Description** (narrative text)
- **2-3 Choices** (button options)
- **Unlockable 3rd Choice** (requires tags/HP/sanity/trinkets)

### Event Frequency

**Act Structure (Tutorial):**
```
COMBAT (Didact) ’ EVENT ’ COMBAT (Daemon) ’ EVENT ’ Victory
```

**Act Structure (Future):**
```
COMBAT ’ EVENT ’ COMBAT ’ EVENT ’ ELITE ’ EVENT ’ BOSS
```

**Event Density:** ~40% of encounters are events (high decision density)

---

### Choice Consequence Types

| Consequence Type | Example | Permanence |
|------------------|---------|------------|
| **Chip Gain/Loss** | +50 chips, -20 chips | Permanent |
| **Sanity Gain/Loss** | +30 sanity, -10 sanity | Permanent |
| **Card Tag Application** | Add CURSED to 3 random cards | Permanent |
| **Card Tag Removal** | Remove all LUCKY tags | Permanent |
| **Enemy HP Modifier** | Next enemy starts at 75% HP | One combat |
| **Trinket Reward** | Gain Elite Membership | Permanent |
| **Status Effect** | Gain CHIP_DRAIN for 3 rounds | Temporary |

---

### Unlockable Choices (Conditional)

**Mechanic:** 3rd choice is **grayed out** until requirements are met.

#### Requirement Types

| Type | Example | UI Display |
|------|---------|------------|
| **Tag Count** | Requires 3+ CURSED cards | "= Requires 3 CURSED cards" |
| **HP Threshold** | Requires e50 HP | "= Requires 50+ HP" |
| **Sanity Threshold** | Requires e25 sanity | "= Requires 25+ sanity" |
| **Chips Threshold** | Requires e100 chips | "= Requires 100+ chips" |
| **Trinket Possession** | Requires Lucky Coin | "= Requires Lucky Coin trinket" |

**Design Intent:**
- Rewards **build planning** (get CURSED cards ’ unlock powerful choice)
- Encourages **resource management** (maintain high HP to access healing)
- Creates **knowledge checks** (experienced players know what unlocks what)

---

### Tutorial Events

#### System Maintenance
**Theme:** Corrupted casino with sabotage option

**Choices:**
- **A: Investigate the Panel**
  - Result: "You find corrupted code. The system notices."
  - Effect: -10 sanity, gain Stack Trace trinket
- **B: Walk Away**
  - Result: "Chips spill from the machine as it glitches."
  - Effect: +20 chips, add CURSED to 3 random cards
- **C: Sabotage (LOCKED)**
  - Requirement: 1+ CURSED card
  - Result: "The system crashes. The Daemon is weakened."
  - Effect: Daemon starts at 75% HP, -20 sanity

**Tutorial Purpose:**
- Teaches locked choice mechanic
- Shows enemy HP modification
- Demonstrates tag-based unlocks

---

#### House Odds
**Theme:** VIP casino membership with scaling difficulty

**Choices:**
- **A: Accept Upgrade**
  - Result: "You're now Elite. The house expects more."
  - Effect: Daemon HP +50%, gain Elite Membership trinket
- **B: Refuse**
  - Result: "The casino's luck turns against you."
  - Effect: -15 sanity, add LUCKY to all 4 Aces
- **C: Negotiate (LOCKED)**
  - Requirement: 1+ LUCKY card
  - Result: "Your luck speaks for itself. The house relents."
  - Effect: Normal Daemon HP, +30 chips, add BRUTAL to all face cards

**Tutorial Purpose:**
- Teaches tag targeting strategies (RANK_ACES, RANK_FACE_CARDS)
- Shows enemy HP scaling trade-off
- Demonstrates tag synergy (B unlocks C)

---

### Tag Targeting Strategies (Event Design Tool)

| Strategy | Target | Example Event Use |
|----------|--------|-------------------|
| `TAG_TARGET_RANDOM_CARD` | 1 random card (0-51) | "The dealer curses a card" |
| `TAG_TARGET_HIGHEST_UNTAGGED` | Highest rank without tag | "Bless the highest pure card" |
| `TAG_TARGET_LOWEST_UNTAGGED` | Lowest rank without tag | "Curse the weakest card" |
| `TAG_TARGET_SUIT_HEARTS` | All 13 hearts | "Hearts become vampiric" |
| `TAG_TARGET_SUIT_DIAMONDS` | All 13 diamonds | "Diamonds sparkle with luck" |
| `TAG_TARGET_SUIT_CLUBS` | All 13 clubs | "Clubs turn brutal" |
| `TAG_TARGET_SUIT_SPADES` | All 13 spades | "Spades are cursed" |
| `TAG_TARGET_RANK_ACES` | All 4 aces | "Aces gain LUCKY" (House Odds) |
| `TAG_TARGET_RANK_FACE_CARDS` | All J/Q/K (12 cards) | "Face cards gain BRUTAL" (House Odds) |
| `TAG_TARGET_ALL_CARDS` | All 52 cards | "Global corruption spreads" |

**Design Intent:**
- Provides **granular control** for event designers
- Creates **memorable consequences** (all hearts = vampiric deck)
- Enables **build-defining moments** (all face cards = aggro build)

---

### Event-Only Trinkets

Some trinkets **ONLY** drop from events (not combat):

| Trinket | Event Source | Rarity | Effect |
|---------|--------------|--------|--------|
| **Elite Membership** | House Odds | Event | +20% win bonus, 20% loss refund |
| **Stack Trace** | System Maintenance | Event | +3% damage/stack (max 20 stacks = 60%) |
| **Cursed Dealer's Coin** | (Planned) | Event | On blackjack: Deal 2× damage BUT lose 10 sanity |
| **Broken Clock** | (Planned) | Event | Rewind last hand (once per combat) |

**Design Intent:**
- Events provide **unique build-warping effects**
- Encourages **event risk-taking** (best rewards come from events)
- Creates **run-defining moments** ("I got Elite Membership at 10% HP!")

---

## Card Tag System

Card tags are **permanent upgrades** to individual cards in the 52-card deck.

### Tag Philosophy

- **52 cards always exist** (no deckbuilding, no card removal)
- **Tags stack** (a single card can have CURSED + VAMPIRIC + LUCKY)
- **Tags tell a story** ("This Ace of Spades saved my run 3 times")

---

### Tag Categories

#### On-Draw Tags (Trigger when card is drawn/flipped face-up)

| Tag | Effect | Damage Type | Visual |
|-----|--------|-------------|--------|
| **CURSED** | Deal 10 damage to enemy | Immediate trigger | Purple glow + skull icon |
| **VAMPIRIC** | Deal 5 damage to enemy + heal 5 chips | Immediate trigger | Red glow + fangs icon |

**Design Intent:**
- **CURSED** = pure aggro (10 damage is ~10% of normal enemy HP)
- **VAMPIRIC** = sustain hybrid (damage + healing)
- **Trigger Timing:** On draw (not on hand calculation) = rewards drawing many cards

---

#### Passive Tags (Active while card is in ANY hand)

| Tag | Effect | Scope | Stacking |
|-----|--------|-------|----------|
| **LUCKY** | +10% crit chance | Global (player + dealer hands) | Yes (+10% per card) |
| **BRUTAL** | +10% damage dealt | Global (player + dealer hands) | Yes (+10% per card) |

**Design Intent:**
- **LUCKY** = offense boost (10% crit = ~5% average damage increase at 1.5× crit)
- **BRUTAL** = consistent damage (10% flat = always useful)
- **Global Scope:** Works even if card is in dealer's hand (asymmetric power)

**Example:** Player has 3 LUCKY cards in hand, dealer has 1 LUCKY card
- Total crit chance: **+40%** (4 cards × 10% = 40%)
- Player benefits even from dealer's LUCKY card!

---

#### Temporary Tags (One-Time Use)

| Tag | Effect | Duration | Source |
|-----|--------|----------|--------|
| **DOUBLED** | Card value ×2 for hand calculation | This hand only | Degenerate's Gambit active |

**Design Intent:**
- **DOUBLED** = tactical active ability tag (not permanent)
- Removed after hand resolves (doesn't persist)

---

### Tag Reward Frequency

**Combat Victory:** Choose **1 card** to tag (from reward options)

**Example Reward:**
```
                                     
   COMBAT VICTORY!                   
                                     
   Choose one card to upgrade:       
                                     
   [View All 52 Cards]               
                                     
   Tag to Apply: CURSED              
   Effect: Deal 10 damage on draw    
                                     
```

**Event Choices:** Can apply tags to **multiple cards** (3-13 cards)

**Example:**
- House Odds Choice B: "Add LUCKY to all 4 Aces"
- System Maintenance Choice B: "Add CURSED to 3 random cards"

---

### Tag Synergies

#### CURSED + Cursed Skull Trinket
- Base CURSED damage: 10
- With Cursed Skull: **15 damage** (+5 from trinket buff)
- **Result:** 50% damage increase on CURSED triggers

---

#### LUCKY + Lucky Chip Trinket
- Base LUCKY: +10% crit per card
- With Lucky Chip: +5% crit per LUCKY_STREAK stack (max 10 stacks = +50%)
- **Result:** Crit stacking build (80%+ crit chance possible)

---

#### BRUTAL + Detective's Files Passive
- Base BRUTAL: +10% damage per card
- Detective passive: +5 damage per pair
- **Result:** More cards in hand = more pairs = more BRUTAL procs

---

### Tag Build Archetypes

#### "Cursed Aggro"
- **Strategy:** Stack CURSED tags, equip Cursed Skull trinket
- **Gameplay:** Draw spam (hit spam to trigger CURSED procs)
- **Synergies:** Counting Cards trinket (+2 damage per draw)
- **Weakness:** No sustain (pure aggro, risky)

---

#### "Vampiric Sustain"
- **Strategy:** Stack VAMPIRIC tags, high-draw strategy
- **Gameplay:** Hit aggressively, heal chip losses via VAMPIRIC procs
- **Synergies:** Tarnished Medal (bust refund), Insurance Policy (loss refund)
- **Weakness:** Lower damage ceiling than CURSED

---

#### "Crit Lucky"
- **Strategy:** Stack LUCKY tags, equip Lucky Chip trinket
- **Gameplay:** Maintain win streaks for LUCKY_STREAK status effect
- **Synergies:** High-bet builds (crit on 40-chip bet = massive burst)
- **Weakness:** Loss streaks reset LUCKY_STREAK (high variance)

---

#### "Brutal Pair Detective"
- **Strategy:** BRUTAL tags + Detective class + pair-focused trinkets
- **Gameplay:** Long hands to create pairs, BRUTAL scaling from many cards
- **Synergies:** Detective's Files active (create pairs on demand)
- **Weakness:** Requires specific class (not universal)

---

## Build Archetypes

Combining all systems creates **distinct build archetypes**:

### 1. "Glass Cannon Degenerate"

**Core:**
- Class: Degenerate (low sanity ’ 40-chip bets)
- Class Upgrades: "Exponential Greed" (+10 scaling per active)
- Trinkets: Blood Ante, Devil's Bargain, Ace's Sleeve
- Tags: BRUTAL on face cards, CURSED on low cards

**Gameplay:**
- Bet 40 chips every hand (0% sanity)
- Hit on 15+ for Reckless Payoff passive (40+ damage per trigger)
- Use active on 5s to hit 21 from 16
- One-shot enemies with CURSED procs + active damage

**Strengths:** Highest damage ceiling, fastest kills
**Weaknesses:** Dies in 3-4 losses, requires perfect play

---

### 2. "Immortal Dealer"

**Core:**
- Class: Dealer (low sanity ’ face-down visibility, bust protection)
- Class Upgrades: Cooldown reduction, reshuffle damage scaling
- Trinkets: Insurance Policy, Elite Membership, Golden Horseshoe
- Tags: VAMPIRIC on Aces, LUCKY on face cards

**Gameplay:**
- See enemy's face-down card (50% sanity threshold)
- Play optimally with perfect information
- Mulligan active to force deck reshuffles (50 damage passive)
- Heal chip losses via VAMPIRIC + chip-refund trinkets

**Strengths:** Unkillable sustain, consistent wins
**Weaknesses:** Slow kills, long combats

---

### 3. "Pair Detective Combo"

**Core:**
- Class: Detective (pair damage scaling)
- Class Upgrades: Passive damage increase, active cooldown reduction
- Trinkets: Counting Cards, Cursed Skull, Stack Trace
- Tags: CURSED on all same-rank cards (e.g., all 7s)

**Gameplay:**
- Hit aggressively to create long hands (more pair chances)
- Use active to copy cards into pairs (7’7’pair)
- CURSED tags on same-rank cards trigger multiple times
- Stack damage via Counting Cards (+2 per draw) + Stack Trace (stacking %)

**Strengths:** Explosive combo turns, scales infinitely
**Weaknesses:** Requires setup (slow start), RNG-dependent pairs

---

### 4. "Lucky Crit Streaker"

**Core:**
- Any class (universal strategy)
- Class Upgrades: Affix-focused (Precise, Deadly affixes)
- Trinkets: Lucky Chip, Rabbit's Foot, High Roller's Chip
- Tags: LUCKY on all Aces + face cards (16 cards = +160% crit!)

**Gameplay:**
- Win first hand to start LUCKY_STREAK buildup
- Each win: +5% crit (stacks to +50% at 10 stacks)
- With 16 LUCKY tags: **160% + 50% = 210% crit chance** (capped at 100% but overkill)
- Bet high (20+ chips) for High Roller's Chip bonus

**Strengths:** Win-more snowball, massive burst
**Weaknesses:** Loses LUCKY_STREAK on single loss (reset to 0%)

---

## Progression Pacing

### Early Game (Act 1, Encounters 1-2)

**Player Power:**
- Class trinket (baseline)
- 0-2 combat trinkets (random affixes)
- 2-4 card tags (1-2 per combat win)
- 100 chips, 100 sanity

**Enemy Power:**
- Normal enemy: 500 HP (5-10 hands to kill)
- Elite enemy: 5000 HP (50-100 hands to kill)

**Design Goal:** Teach systems, low RNG variance

---

### Mid Game (Act 2, Encounters 3-5)

**Player Power:**
- Class trinket + 1 upgrade
- 4-6 combat trinkets (tier 2 = 2 affixes each)
- 6-10 card tags (synergies emerging)
- 80-120 chips (variance from events/losses)

**Enemy Power:**
- Normal enemy: 1000 HP (scaled via player power)
- Elite enemy: 10000 HP (requires optimized damage)

**Design Goal:** Build identity solidified, synergies online

---

### Late Game (Act 3, Encounters 6-7)

**Player Power:**
- Class trinket + 2 upgrades
- 6 combat trinkets (tier 3 = 3 affixes each)
- 12-20 card tags (full build expression)
- 60-150 chips (high variance, risky)

**Enemy Power:**
- Boss enemy: 15000+ HP (marathon fight)
- Abilities scale with player power (adaptive difficulty)

**Design Goal:** Full build expression, skill check

---

## Implementation Status

###  Fully Implemented

- [x] Class system (Degenerate, Dealer, Detective enums)
- [x] Sanity threshold system (betting modifiers)
- [x] Class trinket (Degenerate's Gambit)
- [x] Card tag system (CURSED, VAMPIRIC, LUCKY, BRUTAL, DOUBLED)
- [x] Tag effects (on-draw damage, passive bonuses)
- [x] Event system (choices, consequences, requirements)
- [x] Event unlocks (tag count, HP, sanity, chips)
- [x] Tutorial events (System Maintenance, House Odds)
- [x] Combat trinket drops (rarity, pity system)

###  Designed (DUF Files Ready)

- [x] Trinket affix system (DUF templates + loader architecture)
- [x] Affix pool (9 affixes with weights)
- [x] Trinket templates (20+ combat trinkets, 2 event trinkets)
- [x] Trinket stacking (Broken Watch, Iron Knuckles)
- [x] Tag buff system (Cursed Skull)

###   Designed but NOT Implemented

- [ ] Class trinket upgrade system (boss rewards)
- [ ] Dealer class trinket (active + passive)
- [ ] Detective class trinket (active + passive)
- [ ] Class trinket upgrade pools (~10 upgrades per class)
- [ ] Event-only trinkets (beyond Elite Membership, Stack Trace)
- [ ] Tag removal events (anti-corruption mechanics)
- [ ] Shop events (buy/sell tags/trinkets)
- [ ] Rest events (heal, sanity restore)

### =. Future Design Work

- [ ] Dreamer class (full design)
- [ ] Act 2-3 event pools
- [ ] Act 2-3 enemy pools
- [ ] Advanced trinket effects (hand manipulation, deck ordering)
- [ ] Tag combos (CURSED + VAMPIRIC = special effect?)
- [ ] Meta-progression (unlock classes, unlock events)

---

## Design Notes for Future Expansion

### Class Trinket Upgrade Guidelines

Each upgrade should:
1. **Modify only 1-2 stats** (avoid overwhelming complexity)
2. **Enable new strategies** (not just +10% damage)
3. **Pair with relevant affixes** (offensive upgrade ’ offensive affix)
4. **Cost nothing** (always beneficial, no downsides)

**Bad Example:** "Active cooldown -1 turn, passive damage -5" (downside = feels bad)
**Good Example:** "Active cooldown -1 turn, passive scales 2× faster" (pure upside)

---

### Event Design Guidelines

Each event should:
1. **Have clear theme** (corrupted AI, casino greed, etc.)
2. **Offer 2 easy choices + 1 locked choice** (knowledge reward)
3. **Create memorable moments** ("I got all Aces as LUCKY!")
4. **Avoid pure downsides** (every choice should have upside)

**Bad Example:** "Choice A: Lose 50 chips. Choice B: Lose 30 chips." (feels bad)
**Good Example:** "Choice A: Lose 30 chips, gain CURSED cards. Choice B: Gain 30 chips, lose sanity."

---

### Tag Design Guidelines

New tags should:
1. **Have clear trigger** (on draw, while in hand, on discard, etc.)
2. **Synergize with 2+ systems** (trinkets, class abilities, events)
3. **Feel powerful** (10 damage is ~10% of enemy HP = impactful)
4. **Stack multiplicatively** (multiple tags = exponential power)

**Future Tag Ideas:**
- **ETHEREAL:** Card is removed from deck after use (once-per-combat)
- **WILD:** Card can be used as any rank (1-13)
- **POISONED:** Dealer takes 2 damage per round if this card is in their hand

---

**End of Document**

This document should serve as a **comprehensive reference** for designing future content, balancing systems, and understanding player build opportunities. Pair with [GAME_DESIGN_SUMMARY.md](GAME_DESIGN_SUMMARY.md) for technical implementation details.
