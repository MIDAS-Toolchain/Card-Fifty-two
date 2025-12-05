# Card Fifty-Two

**A Blackjack roguelike RPG with survival horror and absurd surrealism.**

## What is this?

Card Fifty-Two is a blackjack roguelike where **blackjack is your weapon**. Fight corrupted AI enemies in a glitching casino by playing blackjack hands—each hand you win damages the enemy, each hand you lose costs you chips (your health).

Built in C using:
- **[Archimedes](https://github.com/McCoy1701/Archimedes)**: SDL2 game framework
- **[Daedalus](https://github.com/mcCoy1701/daedalus)**: C data structures library

**Core Twist**: You always have all 52 cards. Instead of building a deck, you **upgrade cards with tags** that grant special powers (SHARP cards boost damage, VAMPIRIC cards heal you, LUCKY cards boost crit chance).

## How to build

### Prerequisites
- GCC (C11 support)
- SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
- Archimedes and Daedalus libraries installed
- make

### Native Build
```bash
# Build the game
make

# Build and run
make run

# Debug build
make debug

# Run tests
make test

# Verify architecture compliance
make verify
```

### Web Build (Emscripten)
```bash
# Build for web (requires Emscripten SDK)
make web

# Serve locally at http://localhost:8000
make serve
```

See [Makefile](Makefile) for all available targets.

## Architecture

**Tech Stack**: C11, SDL2, Archimedes framework, Daedalus data structures

**Project Structure**:
```
card-fifty-two/
├── src/           # ~10,500 lines of modular C code
│   ├── scenes/    # UI sections and components
│   ├── loaders/   # DUF file parsers (enemies, events, trinkets)
│   ├── terminal/  # Debug command system
│   └── trinkets/  # Class-specific trinket implementations
├── include/       # Header files
├── data/          # DUF data files (enemies, events, affixes, trinkets)
├── resources/     # Textures, audio, fonts
├── test/          # Unit test suite
└── Makefile
```

**Constitutional Patterns**:
- No raw `malloc`—use `dString_t`, `dArray_t`, `dTable_t` from Daedalus
- Value semantics (tables store data by value, not pointers)
- Fitness functions enforce architectural rules (`make verify`)
- Data-driven design with DUF format files

**Key Files**:
- [src/main.c](src/main.c) - Entry point, initialization
- [src/game.c](src/game.c) - Core game state machine
- [include/structs.h](include/structs.h) - Core data structures
- [Makefile](Makefile) - Build system with native + web targets

## Design

See comprehensive design documentation:
- **[GAME_DESIGN_SUMMARY.md](GAME_DESIGN_SUMMARY.md)** - Complete game systems reference
- **[GAME_DESIGN_BUILD_OPPORTUNITIES.md](GAME_DESIGN_BUILD_OPPORTUNITIES.md)** - Class system, trinkets, progression

**Key Systems**:
- **Combat**: Blackjack hands are attacks (win = damage enemy, lose = lose chips)
- **Card Tags**: Permanent upgrades (CURSED, VAMPIRIC, LUCKY, BRUTAL, DOUBLED)
- **Trinkets**: Equipment with passive effects + random affixes (27 unique trinkets)
- **Classes**: 3 playable classes with unique abilities and sanity thresholds
- **Events**: Slay the Spire-style narrative encounters with unlockable choices
- **Status Effects**: Enemy debuffs that manipulate betting and resources

## Gameplay

### Core Loop
1. **Place Bet** (10-100 chips) - higher bet = more damage if you win
2. **Play Blackjack** - hit, stand, double down against dealer AI
3. **Deal Damage** - winning hands damage the enemy (based on bet amount)
4. **Repeat** until enemy HP reaches 0 or you run out of chips

### Resources
- **Chips**: Your health AND betting currency (lose if you hit 0)
- **Sanity**: Mental stability (affects betting options and class abilities)

### Classes
- **Degenerate**: Risk/reward gambler (rewards hitting on 15+, scales with aggressive play)
- **Dealer**: Control specialist (see enemy's face-down card at low sanity, mulligan mechanic)
- **Detective**: Pattern recognition (deals damage based on pairs in play)

### Card Tags (Permanent Upgrades)
- **CURSED**: Deal 10 damage to enemy when drawn
- **VAMPIRIC**: Deal 5 damage + heal 5 chips when drawn
- **LUCKY**: +10% crit chance while in any hand
- **BRUTAL**: +10% damage while in any hand

### Progression
- Win combat → choose card to upgrade with tag
- Encounter events → make narrative choices (gain chips, sanity, tags, trinkets)
- Defeat bosses → unlock class trinket upgrades (planned)
- Collect trinkets → equipment with passive effects + random stat affixes
