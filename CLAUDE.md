# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Standard development
make              # Build debug version
make run          # Build and run game
make clean        # Clean all build artifacts

# Testing
make test         # Build and run unit test suite

# Web build (Emscripten/WebAssembly)
make web          # Compile to WASM (outputs to index/)
make serve        # Start local server at http://localhost:8000

# Memory debugging
make valgrind     # Run with Valgrind
make asan         # Build with AddressSanitizer
```

## Architecture Overview

### Constitutional Patterns (CRITICAL)

This codebase follows strict architectural rules:

1. **NO raw `malloc`/`free`** - Use Daedalus data structures:
   - `dString_t*` for strings
   - `dArray_t*` for arrays
   - `dTable_t*` for hash maps

2. **Value semantics** - Tables store data by value, not pointers:
   ```c
   // ✅ CORRECT
   Player_t player = {0};
   d_TableSet(g_players, &player_id, &player);  // Stores copy
   Player_t* p = (Player_t*)d_TableGet(g_players, &player_id);  // Returns pointer to value inside table

   // ❌ WRONG
   Player_t* player = malloc(sizeof(Player_t));  // Never do this!
   d_TableSet(g_players, &player_id, &player);   // Stores pointer (bad!)
   ```

3. **Parameter order for `d_ArrayInit`**: `capacity` FIRST, `element_size` SECOND:
   ```c
   // ✅ CORRECT
   dArray_t* cards = d_ArrayInit(52, sizeof(Card_t));

   // ❌ WRONG (causes bus errors)
   dArray_t* cards = d_ArrayInit(sizeof(Card_t), 52);
   ```

4. **Double-pointer destructors** for cleanup:
   ```c
   void CleanupEnemy(Enemy_t** enemy);  // Takes &enemy, sets to NULL
   ```

### State Machine Architecture

The game is a **state machine** (see `GameContext_t` in [include/structs.h](include/structs.h)):

```
BETTING → DEALING → PLAYER_TURN → DEALER_TURN → SHOWDOWN → ROUND_END
  ↑                                                              ↓
  └─────────────────(loop if enemy HP > 0)─────────────────────┘
                                 ↓
                    COMBAT_VICTORY → REWARD_SCREEN
```

**Key files**: [src/game.c](src/game.c), [src/state.c](src/state.c), [include/game.h](include/game.h)

### Event-Driven Triggers

Systems communicate via **game events** (not polling):

```c
Game_TriggerEvent(game, GAME_EVENT_CARD_DRAWN);
```

**Listeners**: Enemy abilities, trinket passives, status effects

### Data-Driven Systems

Game content defined in **DUF files** (`data/`): enemies, events, trinkets, affixes, card tags

**DUF Loaders** in [src/loaders/](src/loaders/) parse files at startup and populate global caches.

### Global State

Key globals (see [src/main.c](src/main.c) and [include/common.h](include/common.h)):
- `g_players` - Player registry (table: player_id → Player_t)
- `g_card_metadata` - Card tags/metadata (separate from Card_t)
- `g_trinket_templates` - Trinket definitions from DUF
- `g_enemies_db`, `g_events_db` - DUF parsed databases

### Core Data Structures

Key structs in [include/structs.h](include/structs.h):

**Card_t** (32 bytes, value type):
- Lightweight: texture, card_id, position, suit, rank, face_up
- **Extended metadata** (tags, rarity) stored in `g_card_metadata` global table
- Access via `GetCardMetadata(card_id)`

**Player_t**:
- `Hand_t hand`, `int chips` (HP + currency), `int sanity`
- `dArray_t* trinkets` (TrinketInstance_t values)
- `StatusEffectList_t status_effects`
- `bool combat_stats_dirty` - flag for stat recalculation

**Enemy_t**:
- `dString_t* name`, `int current_hp`, `dArray_t* abilities`

### Trinket System

**Two-phase design**:
1. **Templates** (`Trinket_t`) - immutable definitions from DUF in `g_trinket_templates`
2. **Instances** (`TrinketInstance_t`) - per-player equipped trinkets with stacks/cooldowns

**Affix system**: Trinkets get 1-3 random stat affixes (+% dmg, +% crit, refund) from `data/affixes/`

### UI Architecture

Modular system in [src/scenes/](src/scenes/):
- **Sections**: Persistent regions (topBar, leftSidebar, player, dealer)
- **Components**: Modals/overlays (eventModal, rewardModal, trinketDropModal)
- **Pipeline**: Archimedes → sections → modals → tweens

### Terminal/Debug

Press backtick (\`) to open. Commands: `give_chips N`, `set_sanity N`, `deal_damage N`, `add_tag TAG ID`, `give_trinket TRINKET` 

## Testing

Run `make test` for unit tests. Test files in [test/](test/):
- `test_structs.c`, `test_state.c`, `test_trinkets.c`, `test_stats.c`, etc.

## Game Design Reference

**[GAME_DESIGN_SUMMARY.md](GAME_DESIGN_SUMMARY.md)** - Complete systems reference
**[GAME_DESIGN_BUILD_OPPORTUNITIES.md](GAME_DESIGN_BUILD_OPPORTUNITIES.md)** - Class design, progression

**Quick reference**:
- **Combat**: Blackjack hands damage enemies (bet = base damage)
- **Card Tags**: CURSED (10 dmg), VAMPIRIC (5 dmg + heal), LUCKY (+10% crit), BRUTAL (+10% dmg)
- **Classes**: Degenerate (risk/reward), Dealer (control), Detective (pairs)
- **Resources**: Chips (HP + currency), Sanity (affects betting)

## Common Patterns

**Adding a Card Tag**:
1. Add enum to [include/defs.h](include/defs.h)
2. Add DUF definition to [data/card_tags/tags.duf](data/card_tags/tags.duf)
3. Implement in [src/cardTags.c](src/cardTags.c) `ProcessCardTagEffects()`

**Adding a Trinket**:
1. Add DUF template to [data/trinkets/core_trinkets.duf](data/trinkets/core_trinkets.duf)
2. If custom logic needed, implement in [src/trinket.c](src/trinket.c)

**Adding a Game Event**:
1. Add to `GameEvent_t` in [include/game.h](include/game.h)
2. Add `GameEventToString()` case in [src/game.c](src/game.c)
3. Call `Game_TriggerEvent(game, GAME_EVENT_YOUR_EVENT)` where needed

**Adding an Enemy Ability**:
1. Add DUF to [data/enemies/tutorial_enemies.duf](data/enemies/tutorial_enemies.duf)
2. Implement in [src/ability.c](src/ability.c)

## Important Constraints

- **Card_t must stay 32 bytes** - use `g_card_metadata` for extended data
- **d_ArrayInit order**: `d_ArrayInit(CAPACITY, element_size)` - backwards = bus error
- Event-driven architecture - trigger events, don't poll state
