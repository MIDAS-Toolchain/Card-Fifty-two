# Card Fifty-Two - Game Design Summary

**A Blackjack Roguelike with Corrupted Casino Aesthetics**

---

## Table of Contents

1. [Quick Start](#quick-start)
2. [Core Concept](#core-concept)
3. [Core Game Loop](#core-game-loop)
4. [Combat System](#combat-system)
5. [Progression System](#progression-system)
6. [Enemy System](#enemy-system)
7. [Card System](#card-system)
8. [Status Effects System](#status-effects-system)
9. [Trinket System](#trinket-system)
10. [Tutorial System](#tutorial-system)
11. [Event System](#event-system)
12. [UI/UX Flow](#uiux-flow)
13. [Theming & Narrative](#theming--narrative)

---

## Quick Start

### For Players
- **Genre:** Roguelike Deckbuilder meets Blackjack
- **Goal:** Defeat enemies by winning blackjack hands while managing chips (health) and sanity
- **Win Condition:** Defeat the boss enemy at the end of each Act
- **Loss Condition:** Run out of chips (0 HP)
- **Starting Resources:** 100 chips, 100 sanity

### For Developers
- **Architecture:** C with SDL2 via Archimedes framework
- **Data Structures:** Daedalus library (no raw malloc!)
- **Build:** `make run` to play, `make verify` to check architecture compliance
- **Key Files:**
  - `/include/structs.h` - Core data structures
  - `/src/game.c` - Game state machine
  - `/architecture/decision_records/` - Design rationale
  - `CLAUDE.md` - AI development guide

---

## Core Concept

**Card Fifty-Two** is a roguelike where blackjack is your weapon. You fight corrupted AI enemies in a glitching casino by playing blackjack hands. Each hand you win damages the enemy; each hand you lose costs you chips (your health).

### Core Twist
Unlike traditional deckbuilders where you build a custom deck, **you always have all 52 cards**. Instead of adding/removing cards, you **upgrade cards with tags** that grant special powers:
- **CURSED** cards deal 10 damage to enemies when drawn
- **VAMPIRIC** cards deal 5 damage and heal you for 5 chips
- **LUCKY** cards grant +10% critical hit chance while in hand
- **BRUTAL** cards grant +10% damage while in hand

---

## Core Game Loop

### Tutorial Act Structure
```
1. COMBAT: The Didact (500 HP)
   ↓
2. EVENT: Random event with choices
   ↓
3. COMBAT: The Daemon (5000 HP) [ELITE]
   ↓
4. EVENT: Final event
   ↓
   Victory Screen
```

### Single Combat Round Flow
```
STATE_BETTING
  ↓ (Place bet: 10-100 chips)
STATE_DEALING
  ↓ (Deal 2 cards to player, 2 to dealer)
STATE_PLAYER_TURN
  ↓ (HIT/STAND/DOUBLE actions)
STATE_DEALER_TURN
  ↓ (Dealer hits on 16, stands on 17)
STATE_SHOWDOWN
  ↓ (Compare hands, resolve outcome)
STATE_ROUND_END
  ↓ (Display results, trigger abilities)
  ↓
  ↓ (If enemy HP > 0: Return to BETTING)
  ↓ (If enemy HP ≤ 0: Continue)
STATE_COMBAT_VICTORY
  ↓
STATE_REWARD_SCREEN
  ↓ (Choose card upgrades)
  ↓
Next encounter...
```

### Blackjack Rules
- **Goal:** Get as close to 21 as possible without going over
- **Bust:** Hand value > 21 (you lose your bet)
- **Blackjack:** Ace + 10-value card = automatic 21
- **Dealer Rules:** Hits on 16, stands on 17
- **Win:** Beat dealer's hand → Gain chips equal to bet
- **Loss:** Lose to dealer → Lose your bet
- **Push:** Tie with dealer → Get bet back (no gain/loss)

### Resource Management
- **Chips:** Your health AND betting currency (start: 100, lose if 0)
- **Sanity:** Mental stability (start: 100, affects betting options)
- **Bet Amount:** 10-100 chips per hand (higher bets = more risk/reward)

---

## Combat System

### How Combat Works

Each combat encounter is a **boss fight** against a single enemy with high HP (500-5000+). You damage the enemy by **winning blackjack hands**.

#### Damage Formula
```c
Base Damage = Bet Amount (10-100)
Modified Damage = Base × (1 + damage_percent/100) + damage_flat
Final Damage = Modified × (1.5 if CRIT, else 1.0)
```

**Example:**
- Bet 50 chips
- You have 2 BRUTAL tags (+20% damage)
- Critical hit triggers (1.5× multiplier)
- Final damage: 50 × 1.2 × 1.5 = **90 damage**

#### Combat Stats
Players accumulate combat stats from tags and trinkets:
- **damage_flat**: Flat damage bonus (added to base)
- **damage_percent**: Percentage damage increase (e.g., 20 = +20%)
- **crit_chance**: Chance to deal 1.5× damage (0-100%)
- **crit_bonus**: Additional crit multiplier (currently fixed at 1.5×)

#### Combat Outcomes
- **Win Hand:** Deal damage to enemy = your bet amount (modified by stats)
- **Lose Hand:** Lose chips = your bet amount
- **Push (Tie):** No damage, no chip loss
- **Bust:** Lose chips, deal no damage

---

## Progression System

### Act Structure

**Tutorial Act:** 2 combats, 2 events
```
NORMAL: The Didact (500 HP)
EVENT:  Random event pool
ELITE:  The Daemon (5000 HP)
EVENT:  Random event pool
```

**Future Acts:** 7 encounters total
```
NORMAL → EVENT → NORMAL → EVENT → ELITE → EVENT → BOSS
```

### Encounter Types

| Type | Description | Difficulty | Rewards |
|------|-------------|-----------|---------|
| **NORMAL** | Standard enemy | Base HP | Standard rewards |
| **ELITE** | Harder enemy | 10× HP | Better rewards |
| **BOSS** | Act finale | High HP | Act completion |
| **EVENT** | Story/choices | No combat | Tags, chips, sanity |

### Reward System

#### Combat Victory Rewards
After defeating an enemy, you choose **one card to upgrade**:
1. **View all 52 cards** in a grid
2. **Select one card** to apply a tag
3. **Tag choice** depends on enemy/difficulty

Example reward: "Apply CURSED tag to one card"

#### Event Rewards
Events offer **multiple choices** with different rewards:
- Chip gains/losses
- Sanity gains/losses
- Card tag applications (single card or multiple)
- Trinket rewards
- Enemy HP modifications (for next combat)

---

## Enemy System

### Enemy Structure
Enemies have:
- **Name** (e.g., "The Didact")
- **HP** (500-5000+)
- **Portrait** (PNG image with corruption effects)
- **Abilities** (passive and active)

### Tutorial Enemies

#### The Didact (Tutorial Enemy)
- **HP:** 500
- **Role:** Teaches basic mechanics
- **Abilities:**
  1. **The House Remembers** (Passive)
     - Trigger: Player gets blackjack
     - Effect: Apply STATUS_GREED (2 rounds) - "The casino is alive"
  2. **Irregularity Detected** (Counter)
     - Trigger: Every 5 cards drawn
     - Effect: Apply STATUS_CHIP_DRAIN (5 chips, 3 rounds)
  3. **System Override** (HP Threshold)
     - Trigger: Below 30% HP (once)
     - Effect: Heal 50 HP + force deck shuffle

#### The Daemon (Elite Enemy)
- **HP:** 5000
- **Role:** Final challenge of tutorial act
- **Abilities:** Same as Didact (scaled up)

### Ability System

Abilities are **event-driven** (ADR-001). They trigger based on:
- **Game Events:** COMBAT_START, HAND_END, PLAYER_WIN, CARD_DRAWN, etc.
- **Triggers:** HP thresholds, counters, random chance

#### Trigger Types
- `TRIGGER_NONE`: Passive (always active)
- `TRIGGER_HP_THRESHOLD`: Fires at X% HP
- `TRIGGER_ONCE_PER_COMBAT`: Fires only once
- `TRIGGER_ON_EVENT`: Fires on specific game event
- `TRIGGER_COUNTER`: Fires after N occurrences

#### Example Ability Definitions (from DUF file)
```
@didact {
    name: "The Didact"
    hp: 500
    image_path: "resources/enemies/didact.png"
    abilities: [
        "ABILITY_THE_HOUSE_REMEMBERS",
        "ABILITY_IRREGULARITY_DETECTED",
        "ABILITY_SYSTEM_OVERRIDE"
    ]
}
```

---

## Card System

### The 52-Card Deck

**Core Philosophy:** You always have all 52 cards (standard playing deck). You **don't add or remove cards**—instead, you **upgrade them with tags**.

#### Card Structure (32 bytes, value type)
```c
typedef struct Card {
    SDL_Surface* texture;  // Cached surface pointer
    int card_id;           // 0-51 unique ID
    int x, y;              // Screen position
    CardSuit_t suit;       // Hearts/Diamonds/Clubs/Spades
    CardRank_t rank;       // Ace through King
    bool face_up;          // Visibility flag
} Card_t;
```

### Card Metadata Registry

Each card (0-51) has extended metadata stored globally:
- **Tags:** Array of active tags (CURSED, VAMPIRIC, LUCKY, BRUTAL, DOUBLED)
- **Rarity:** Common/Uncommon/Rare/Legendary
- **Flavor Text:** Description of upgrades

**Global Registry:** `g_card_metadata` (dTable_t: card_id → CardMetadata_t*)

### Card Tags

Tags modify card behavior. **Multiple tags can stack** on a single card.

#### On-Draw Tags (Trigger when drawn)
| Tag | Effect | Trigger |
|-----|--------|---------|
| **CURSED** | Deal 10 damage to enemy | On card drawn |
| **VAMPIRIC** | Deal 5 damage + heal 5 chips | On card drawn |

#### Passive Tags (Active while in hand)
| Tag | Effect | Duration |
|-----|--------|----------|
| **LUCKY** | +10% crit chance | While in any hand (global) |
| **BRUTAL** | +10% damage | While in any hand (global) |

#### Temporary Tags (One-time use)
| Tag | Effect | Duration |
|-----|--------|----------|
| **DOUBLED** | Card value ×2 for hand calculation | This hand only |

### Tag Application Strategies

Events and rewards can apply tags to cards using **targeting strategies**:

| Strategy | Target | Example |
|----------|--------|---------|
| `TAG_TARGET_RANDOM_CARD` | 1 random card | Add CURSED to random card |
| `TAG_TARGET_HIGHEST_UNTAGGED` | Highest rank without tag | Tag King without CURSED |
| `TAG_TARGET_LOWEST_UNTAGGED` | Lowest rank without tag | Tag 2 without LUCKY |
| `TAG_TARGET_SUIT_HEARTS` | All 13 hearts | All hearts → VAMPIRIC |
| `TAG_TARGET_RANK_ACES` | All 4 aces | All aces → LUCKY |
| `TAG_TARGET_RANK_FACE_CARDS` | All J/Q/K (12 cards) | All face cards → BRUTAL |
| `TAG_TARGET_ALL_CARDS` | All 52 cards | Global curse |

### Card Rarity System

Cards gain rarity as they're upgraded:
- **Common** (white): No tags
- **Uncommon** (green): 1 tag
- **Rare** (blue): 2+ tags
- **Legendary** (gold): 3+ tags (rare)

---

## Status Effects System

**Status effects** are debuffs applied by enemies that manipulate your resources and betting options.

### Token Drain Effects

| Effect | Value | Duration | Description |
|--------|-------|----------|-------------|
| **CHIP_DRAIN** | X chips | N rounds | Lose X chips per round |
| **TILT** | 2× | N rounds | Lose 2× chips on loss |
| **GREED** | 0.5× | N rounds | Win only 50% chips on victory |

### Betting Restriction Effects

| Effect | Value | Duration | Description |
|--------|-------|----------|-------------|
| **FORCED_ALL_IN** | - | 1 round | Must bet all chips next round |
| **ESCALATION** | - | N rounds | Must increase bet each round |
| **NO_ADJUST** | - | N rounds | Can't change bet |
| **MINIMUM_BET** | X chips | N rounds | Min bet increased to X |
| **MADNESS** | - | N rounds | Bet randomized (10-100) |

### Status Effect Lifecycle
1. **Application:** Enemy ability applies effect
2. **Round Start:** CHIP_DRAIN triggers
3. **Betting Phase:** Bet restrictions apply
4. **Round End:** TILT/GREED modify outcomes
5. **Tick Duration:** Decrease by 1 round
6. **Expiration:** Remove when duration = 0

### Visual Feedback
Status effects display as **colored icons** with:
- 2-letter abbreviation (e.g., "Cd" for Chip Drain)
- Duration counter
- Value indicator (e.g., "-5" for 5-chip drain)
- Shake/flash animations on trigger

---

## Trinket System

**Trinkets** are equipment items with passive and active effects. Players can equip up to **6 trinkets** plus **1 class trinket**.

### Trinket Structure

Each trinket has:
- **Passive Effect:** Triggers on game events (CARD_DRAWN, PLAYER_WIN, etc.)
- **Active Effect:** Manually activated with targeting (3-5 turn cooldown)
- **Rarity:** Common/Uncommon/Rare/Legendary/Class
- **Stats:** Per-instance tracking (damage dealt, bonuses granted, etc.)

### Rarity Tiers

| Rarity | Color | Description |
|--------|-------|-------------|
| **Common** | White | Basic effects |
| **Uncommon** | Green | Moderate bonuses |
| **Rare** | Blue | Powerful passives |
| **Legendary** | Gold | Game-changing effects |
| **Class** | Purple | Unique to character class |

### Example Trinkets

#### Degenerate's Gambit (Class - Degenerate)
**Passive:** "Reckless Payoff"
- **Trigger:** When you HIT on 15+
- **Effect:** Deal 10 damage (+5 per active use)
- **Theme:** Rewards risky high-value hits

**Active:** "Double Down"
- **Target:** Card with rank ≤5
- **Effect:** Double card's value this hand
- **Cooldown:** 3 turns
- **Side Effect:** +5 permanent passive damage
- **Theme:** Risky active that scales passive

#### Elite Membership (Rare)
**Passive:** "VIP Rewards"
- **Win Bonus:** +30% chips on wins
- **Loss Refund:** +30% chips back on losses
- **Theme:** Casino loyalty program

**No Active Effect**

#### Stack Trace (Uncommon - Not Yet Implemented)
**Passive:** "Memory Leak"
- **Trigger:** When you STAND
- **Effect:** Track last 3 hands, repeat oldest on command

**Active:** "Rollback"
- **Target:** Self
- **Effect:** Replay oldest tracked hand

### Class Trinkets

Each player class has a unique class trinket:
- **Degenerate:** Degenerate's Gambit (risk/reward scaling)
- **Dealer:** (TBD - control/manipulation)
- **Detective:** (TBD - information/prediction)
- **Dreamer:** (TBD - luck/chaos)

### Trinket Modifiers

Trinkets can modify win/loss outcomes (like status effects):

```c
// Win modifiers
int ModifyWinningsWithTrinkets(Player_t* player, int base_winnings, int bet_amount);

// Loss modifiers (refunds)
int ModifyLossesWithTrinkets(Player_t* player, int base_loss, int bet_amount);
```

**Elite Membership Example:**
- Win 50 chips → +30% → **65 chips**
- Lose 50 chips → +30% refund → **-35 chips** (15 refunded)

---

## Tutorial System

The tutorial is an **event-driven dialogue system** that teaches mechanics through:
- **Dialogue Modals:** Text boxes with continue arrows
- **Spotlight Highlights:** Arrows pointing to UI elements
- **State-Based Progression:** Waits for player actions

### Tutorial Flow

```
Step 1: "Welcome to the casino..."
  ↓ (Click to continue)
Step 2: "Place your bet" → (Arrow points to bet slider)
  ↓ (Wait for STATE_BETTING)
Step 3: "You've placed 50 chips..." → (Arrow points to chip display)
  ↓ (Wait for player action)
Step 4: "The dealer draws..." → (Explain dealer rules)
  ↓ (Wait for STATE_DEALER_TURN)
...
Final Step: "You've won your first hand!" → (Finish button)
```

### Tutorial Features

#### Dialogue Modals
- **Width:** 600px, **Height:** 200px
- **Position:** Centered or offset (configurable per step)
- **Continue Arrow:** Pulsing → symbol (bottom-right)
- **Skip Confirmation:** ESC → "Skip tutorial?" modal

#### Pointing Arrows
- **From:** Dialogue box edge
- **To:** Target UI element (bet slider, chip count, etc.)
- **Style:** Curved arrow with arrowhead

#### State Gating
Tutorial steps can wait for:
- **Game State:** Wait for STATE_BETTING, STATE_PLAYER_TURN, etc.
- **Button Click:** Wait for specific button
- **Key Press:** Wait for specific key
- **Hover:** Wait for mouse hover over area

---

## Event System

**Events** are Slay the Spire-style encounters with narrative choices and consequences.

### Event Structure

```c
typedef struct EventEncounter {
    dString_t* title;           // "The Broken Dealer"
    dString_t* description;     // Main narrative text
    EventType_t type;           // DIALOGUE/CHOICE/BLESSING/CURSE
    dArray_t* choices;          // Array of EventChoice_t (2-3 options)
    int selected_choice;        // Player's selection (-1 = none)
} EventEncounter_t;
```

### Event Types

| Type | Description | Example |
|------|-------------|---------|
| **DIALOGUE** | Pure story/lore | "The casino speaks..." |
| **CHOICE** | Branching consequences | "Take the deal or refuse?" |
| **BLESSING** | Pure positive | "You found chips!" |
| **CURSE** | Pure negative | "The house takes its due" |

### Event Choices

Each choice has:
- **Text:** Button label ("Accept", "Refuse", "Investigate")
- **Result Text:** Outcome description (shown after selection)
- **Chips Delta:** +/- chips (e.g., +50, -20)
- **Sanity Delta:** +/- sanity (e.g., -10, +30)
- **Tag Grants:** Apply tags to cards (with targeting strategy)
- **Tag Removals:** Remove tags from cards
- **Requirements:** Unlock conditions (tag count, HP, sanity, chips)
- **Enemy HP Multiplier:** Modify next enemy HP (0.75 = 75%, 1.5 = 150%)
- **Trinket Reward:** Grant trinket on selection

### Choice Requirements

Choices can be **locked** until requirements are met:

```c
ChoiceRequirement_t req = {
    .type = REQUIREMENT_TAG_COUNT,
    .data.tag_req = {
        .required_tag = CARD_TAG_CURSED,
        .min_count = 3
    }
};
```

**Requirement Types:**
- `REQUIREMENT_TAG_COUNT`: Requires N cards with tag
- `REQUIREMENT_HP_THRESHOLD`: Requires HP ≥ X
- `REQUIREMENT_SANITY_THRESHOLD`: Requires sanity ≥ X
- `REQUIREMENT_CHIPS_THRESHOLD`: Requires chips ≥ X
- `REQUIREMENT_TRINKET`: Requires specific trinket (future)

**UI Behavior:**
- **Locked:** Grayed out, shows lock icon, hover shows requirement
- **Unlocked:** Normal, selectable

### Tutorial Events

#### System Maintenance Event
**Theme:** System corruption with sabotage option

**Choices:**
- **A:** Investigate panel (-10 sanity, trinket TODO)
- **B:** Walk away (+20 chips, 3 random cards → CURSED)
- **C [Locked]:** Sabotage (Daemon starts at 75% HP, -20 sanity)
  - **Requirement:** 1+ CURSED card

**Tutorial Purpose:**
- Teaches locked choice mechanic
- Shows enemy HP modification
- Demonstrates consequence of ignoring corruption

#### House Odds Event
**Theme:** Casino VIP membership with scaling difficulty

**Choices:**
- **A:** Accept upgrade (Daemon +50% HP, trinket TODO)
- **B:** Refuse (-15 sanity, all Aces → LUCKY)
- **C [Locked]:** Negotiate (Normal HP, +30 chips, face cards → BRUTAL)
  - **Requirement:** 1+ LUCKY card

**Tutorial Purpose:**
- Teaches tag targeting (RANK_ACES, RANK_FACE_CARDS)
- Shows enemy HP scaling
- Demonstrates tag synergy (Choice B unlocks Choice C)

### Event Reroll System

During `STATE_EVENT_PREVIEW`, you can **reroll** the event for chips:
- **Base Cost:** 50 chips
- **Cost Scaling:** Doubles each reroll (50 → 100 → 200 → 400)
- **Countdown Timer:** 3 seconds (auto-proceed if no reroll)
- **Reset:** Cost resets to 50 at next event

**UI:**
- "Reroll Event (50 chips)" button
- Timer display (3.0 → 0.0)
- Cost updates after each reroll

---

## UI/UX Flow

### Game States

The game is a **state machine** with 14 states:

| State | Description | Duration |
|-------|-------------|----------|
| **MENU** | Main menu | User input |
| **INTRO_NARRATIVE** | Tutorial intro story | Auto-advance |
| **BETTING** | Place bet (10-100 chips) | User input |
| **DEALING** | Deal initial hands | 0.5s animation |
| **PLAYER_TURN** | HIT/STAND/DOUBLE actions | User input |
| **DEALER_TURN** | Dealer plays by rules | 0.5s per action |
| **SHOWDOWN** | Compare hands, resolve | Instant |
| **ROUND_END** | Display results | 2s delay |
| **COMBAT_VICTORY** | Enemy defeated celebration | User input |
| **REWARD_SCREEN** | Choose card upgrades | User input |
| **COMBAT_PREVIEW** | Elite/boss preview (no reroll) | 3s countdown |
| **EVENT_PREVIEW** | Event preview (with reroll) | 3s countdown |
| **EVENT** | Event encounter | User input |
| **TARGETING** | Trinket active targeting | User input |

### State Transitions (Combat Round)

```
BETTING → (Bet placed)
  ↓
DEALING → (Cards dealt, 0.5s)
  ↓
PLAYER_TURN → (HIT/STAND/BUST)
  ↓
DEALER_TURN → (Dealer logic, 0.5s per action)
  ↓
SHOWDOWN → (Resolve outcome)
  ↓
ROUND_END → (Display results, 2s)
  ↓
  ↓ (If enemy HP > 0: Loop to BETTING)
  ↓ (If enemy HP ≤ 0: Victory)
COMBAT_VICTORY → (Victory celebration)
  ↓
REWARD_SCREEN → (Choose upgrade)
  ↓
Next encounter...
```

### UI Sections (Blackjack Scene)

#### Title Section (Top)
- Act progress (Encounter X/Y)
- Enemy name + portrait
- Enemy HP bar (animated drain)

#### Left Sidebar
- Player portrait
- Chip count (with tweened drain)
- Sanity meter
- Status effect icons
- Trinket slots (6 + class)

#### Dealer Section (Top Center)
- Dealer hand (cards)
- Hand value (hidden until reveal)
- Dealer actions (animations)

#### Player Section (Bottom Center)
- Player hand (cards with hover effects)
- Hand value (live calculation)
- Action buttons (HIT/STAND/DOUBLE)

#### Betting Section (Center)
- Bet slider (10-100 chips)
- Current bet display
- "Place Bet" button

#### Modals (Overlay)
- **Event Modal:** Event choices (600×400px)
- **Reward Modal:** Card grid (52 cards, 13×4 layout)
- **Tutorial Modal:** Dialogue + arrows
- **Combat Preview Modal:** Enemy stats + countdown
- **Event Preview Modal:** Event summary + reroll button

---

## Theming & Narrative

### Core Aesthetic: **"Corrupted Casino AI"**

The game takes place in a **glitching, sentient casino** run by malfunctioning AI entities.

#### Visual Style
- **Dark Palette:** Near-black backgrounds (#090a14)
- **Glitch Effects:** Scanlines, chromatic aberration, pixel corruption
- **Neon Accents:** Cyan (#73bed3), light gray (#a8b5b2)
- **Corrupted Portraits:** Enemy portraits with digital artifacts

#### Narrative Themes

##### 1. The House is Alive
The casino is a **sentient entity** that watches and reacts to your actions.

**Evidence:**
- Enemy ability: "The House Remembers" (STATUS_GREED on blackjack)
- Event: "System Maintenance" (corruption spreading)
- Flavor text: "The casino is alive, and it's learning"

##### 2. System Degradation
The world is **actively corrupting** as you play.

**Mechanics:**
- Events introduce CURSED cards (corruption spreading to deck)
- Locked choices require corruption to unlock
- Enemy abilities apply status effects (system malfunction)

##### 3. Dread-Themed Names
Enemies and abilities use **unsettling, clinical language**:
- "The Didact" (teacher, but corrupted)
- "The Daemon" (background process gone rogue)
- "Irregularity Detected" (you've been noticed)
- "System Override" (AI self-repair)

##### 4. Risk vs. Corruption
Core player choice: **Power requires corruption**

**Examples:**
- Elite Membership (+30% wins, but are you complicit?)
- Degenerate's Gambit (high-risk plays scale damage)
- Event Choice B: "Accept chips, but your deck is cursed"

### Character Classes (Planned)

Each class represents a **different relationship with the casino**:

| Class | Theme | Starting Trinket | Philosophy |
|-------|-------|------------------|------------|
| **Degenerate** | Risk/reward gambler | Degenerate's Gambit | Embrace chaos |
| **Dealer** | Control/manipulation | TBD | Work within the system |
| **Detective** | Information/prediction | TBD | Outsmart the house |
| **Dreamer** | Luck/chaos | TBD | Break the rules |

### Sanity System (Thematic)

**Sanity** represents your **mental stability** in the corrupted casino.

**Effects of Low Sanity:**
- Restricts betting options (can't bet high)
- Visual distortions (glitch effects intensify)
- Narrative flavor (dialogue changes tone)

**How to Lose Sanity:**
- Event choices (investigate corruption, sabotage)
- Enemy abilities (psychological pressure)
- Prolonged combat (casino pressure effect)

**How to Gain Sanity:**
- Event choices (refuse temptation, walk away)
- Rest events (future)
- Victory (temporary relief)

---

## Design Philosophy

### Core Pillars

#### 1. Blackjack is the Combat
- No separate combat system—blackjack IS the weapon
- Every hand matters (chip economy is tight)
- Risk/reward in every decision (bet high = more damage, more risk)

#### 2. 52-Card Persistence
- You always have all 52 cards (no deckbuilding dilution)
- Upgrades are **permanent** and **cumulative**
- Each run tells a story (this Ace of Spades has CURSED + VAMPIRIC)

#### 3. Tag Synergies
- Tags stack in interesting ways
- Multiple strategies viable (offensive tags, defensive tags, hybrid)
- Deck "remembers" your choices (permanent upgrades)

#### 4. Event-Driven Systems
- Abilities trigger on gameplay events (not timers)
- Encourages reactive counterplay
- Emergent combos (trinket + tag + enemy ability)

#### 5. Constitutional Architecture
- No raw malloc (Daedalus library)
- Value semantics for safety
- Fitness functions enforce patterns
- Read ADRs before changing core systems

---

## Technical Architecture

### Data Flow

```
main.c
  ↓ (Initialize global tables)
g_players (dTable_t: player_id → Player_t)
g_card_metadata (dTable_t: card_id → CardMetadata_t*)
g_trinket_templates (dTable_t: trinket_id → Trinket_t)
g_enemies_db (DUF file: enemy definitions)
  ↓
GameContext_t (state machine)
  ↓
sceneBlackjack.c (UI + rendering)
  ↓
game.c (state transitions + event triggers)
  ↓
event.c, enemy.c, trinket.c (system implementations)
```

### Constitutional Patterns (ADR-003)

**Golden Rules:**
1. **No raw malloc** for strings/arrays—use dString_t, dArray_t
2. **Tables store by value** (not pointers)
3. **d_ArrayInit(capacity, element_size)** - capacity FIRST
4. **Double-pointer destructors** for cleanup (e.g., `DestroyEnemy(Enemy_t** enemy)`)

**Example (Correct):**
```c
// ✅ CORRECT: Value semantics
Player_t player = {0};
player.name = d_StringInit();
d_StringSet(player.name, "Alice", 0);
d_TableSet(g_players, &player.player_id, &player);  // Store by value

// Lookup returns pointer to value INSIDE table
Player_t* p = (Player_t*)d_TableGet(g_players, &player_id);
p->chips -= 10;  // Modify in-place
```

**Anti-Example (Wrong):**
```c
// ❌ WRONG: Raw malloc
Player_t* player = malloc(sizeof(Player_t));  // NO!

// ❌ WRONG: Double pointer dereference
Player_t** player_ptr = (Player_t**)d_TableGet(g_players, &id);
Player_t* player = *player_ptr;  // CRASHES!

// ❌ WRONG: Backwards parameters
d_ArrayInit(sizeof(Card_t), 16);  // BUS ERROR!
```

### Fitness Functions

Located in `/architecture/fitness_functions/*.py`

**Purpose:** Automated architecture compliance checks

**Run:** `make verify`

**What They Check:**
- FF-001: All events have trigger callsites
- FF-002: Win/loss code calls status effect modifiers
- FF-003: No raw malloc for strings/arrays + correct table usage
- FF-004: Card_t stays lightweight (no metadata bloat)
- FF-005: Event choices stored as value types
- FF-006: d_ArrayInit parameter order
- FF-016: Trinket value semantics
- FF-017: Terminal architecture compliance

### Decision Records

Located in `/architecture/decision_records/*.md`

**Must Read Before:**
- Changing event system (ADR-001)
- Modifying status effects (ADR-002)
- Adding new tables (ADR-003)
- Changing card metadata (ADR-004, ADR-009)
- Modifying trinkets (ADR-016)

---

## Future Systems (Planned)

### Shop Events
- Buy/sell card upgrades
- Trade trinkets
- Remove tags (corruption cleansing)

### Rest Events
- Heal chips
- Restore sanity
- Upgrade existing tags

### Additional Enemies
- More abilities (HOUSE_RULES, GLITCH, RESHUFFLE_REALITY)
- Boss variations
- Act 2+ enemies

### Character Classes
- Dealer (control/manipulation)
- Detective (information/prediction)
- Dreamer (luck/chaos)

### Advanced Trinkets
- More class trinkets
- Trinket combos
- Legendary-tier effects

---

## Credits & Contact

**Developer:** Mathew (GitHub: mathew-kurian)

**Frameworks:**
- **Archimedes:** SDL2 game framework
- **Daedalus:** C data structures library

**Architecture:**
- See `CLAUDE.md` for AI development guidelines
- See `/architecture/decision_records/` for design rationale

**Build:**
```bash
make              # Build debug version
make run          # Build and run
make test         # Run unit tests
make verify       # Run architecture fitness functions
```

---

**Last Updated:** 2025-11-25
**Version:** Tutorial Act Implementation
**Status:** Playable (2 enemies, 2 events, trinket system, card tags)
