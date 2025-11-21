# Card Fifty-Two - AI Development Guide

## What This Is

A blackjack roguelike built with:
- **Archimedes** (SDL2 game framework)
- **Daedalus** (C data structures: dString_t, dArray_t, dTable_t)

Philosophy: **Everything's a table or array. No raw pointers, no manual memory chaos.**

---

## Critical Rule: Read Architecture Docs First

**BEFORE suggesting ANY code changes, read the relevant ADR (Architecture Decision Record).**

Location: `architecture/decision_records/*.md`

These docs explain WHY things work the way they do. Violating them causes crashes and multi-day debugging sessions.

run `make verify` in the shell to run all of our fitness functions and ensure the project is stable.

---

## Core Patterns (Non-Negotiable)

### 1. Use Daedalus Types, Not Raw Malloc

**DO:**
```c
dArray_t* cards = d_InitArray(16, sizeof(Card_t));  // capacity FIRST!
d_AppendDataToArray(cards, &card);
d_DestroyArray(&cards);
```

**DON'T:**
```c
Card_t* cards = malloc(52 * sizeof(Card_t));  // ❌ NO!
```

**Why:** Manual memory management = segfaults. Daedalus handles it. See `ADR-003`.

---

### 2. Tables Store Structs By Value

**Critical:** `dTable_t` stores data **by value** (memcpy), not by pointer.

**Example (g_players):**
```c
// CreatePlayer - stores Player_t BY VALUE
Player_t player = {0};
player.name = d_StringInit();
// ... initialize player ...
d_SetDataInTable(g_players, &player.player_id, &player);  // Copies struct

// Lookup - returns pointer to struct INSIDE table
Player_t* player = (Player_t*)d_GetDataFromTable(g_players, &player_id);
if (!player) return;
player->chips -= 10;  // Modify in-place
```

**Common Bug:**
```c
// ❌ WRONG - double pointer dereference
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
Player_t* player = *player_ptr;  // CRASHES! Reads garbage as pointer
```

**Why:** See `ADR-003` - tables store by value, not by pointer.

---

### 3. Parameter Order: d_InitArray(capacity, element_size)

**CAPACITY FIRST, element_size second** (backwards from calloc!)

```c
// ✅ CORRECT
d_InitArray(16, sizeof(Card_t));  // 16 capacity, Card_t elements

// ❌ WRONG (causes bus errors)
d_InitArray(sizeof(Card_t), 16);  // Creates misaligned array
```

**Why:** See `ADR-006` - wrong order causes memory misalignment crashes.

---

### 4. Archimedes Delegate Pattern

Scenes control game flow via function pointers:

```c
void InitBlackjackScene(void) {
    app.delegate.logic = BlackjackLogic;  // Update function
    app.delegate.draw = BlackjackDraw;    // Render function
}

// Main loop calls delegates
void MainLoop(void) {
    a_PrepareScene();
    app.delegate.logic(a_GetDeltaTime());
    app.delegate.draw(a_GetDeltaTime());
    a_PresentScene();
}
```

**DON'T** hardcode scene logic in main loop. Use delegates.

---

## Architecture Enforcement

### Fitness Functions (Architecture Tests)

Location: `architecture/fitness_functions/*.py`

Run all: `./verify_architecture.sh` or `make verify`

**What They Check:**
- `FF-001`: All events have trigger callsites
- `FF-002`: Win/loss code calls status effect modifiers
- `FF-003`: No raw malloc for strings/arrays + correct table usage
- `FF-004`: Card_t stays lightweight (no metadata bloat)
- `FF-005`: Event choices stored as value types
- `FF-006`: d_InitArray parameter order

**When They Run:**
- Manually: `make verify`
- Before commits (recommended)
- In CI/CD (future)

### Decision Records (ADRs)

Location: `architecture/decision_records/*.md`

Each ADR documents:
- **Context**: What problem we faced
- **Decision**: What we chose and why
- **Consequences**: Trade-offs accepted
- **Evidence**: Code examples from codebase

**Read these BEFORE making architectural changes!**

---

## Testing

### Unit Tests
```bash
make test          # Run all tests
./bin/test_runner  # Direct execution
```

Tests use stub/mock patterns for game systems.

### Integration Testing
```bash
make run           # Manual playtesting
make valgrind      # Memory leak detection
make asan          # Address sanitizer (detects use-after-free)
```

### Architecture Verification
```bash
make verify        # Run all fitness functions
```

---

## Common Mistakes (That Cost Us Days)

### ❌ Wrong: Double Pointer from g_players
```c
Player_t** player_ptr = (Player_t**)d_GetDataFromTable(g_players, &id);
Player_t* player = *player_ptr;  // SEGFAULT!
```
**Fix:** `Player_t* player = (Player_t*)d_GetDataFromTable(g_players, &id);`

### ❌ Wrong: Backwards d_InitArray Parameters
```c
d_InitArray(sizeof(Card_t), 16);  // BUS ERROR!
```
**Fix:** `d_InitArray(16, sizeof(Card_t));  // capacity FIRST`

### ❌ Wrong: Manual String Management
```c
char buffer[128];
snprintf(buffer, sizeof(buffer), "HP: %d", hp);
```
**Fix:** Use `dString_t*` with `d_StringFormat()`

### ❌ Wrong: Uninitialized Struct Fields
```c
AbilityData_t ability = {
    .ability_id = ABILITY_FOO,
    .trigger = TRIGGER_PASSIVE
    // Missing: animation fields!
};
```
**Fix:** Initialize ALL fields, especially floats (they become NaN garbage)

---

## Project Structure

```
card-fifty-two/
├── architecture/
│   ├── decision_records/    # ADRs explaining WHY
│   └── fitness_functions/   # Automated architecture checks
├── include/                 # Header files
├── src/                     # Implementation
│   ├── scenes/             # Game screens (menu, blackjack, etc)
│   ├── terminal/           # Debug console
│   └── tween/              # Animation system
├── tests/                   # Unit tests
├── CLAUDE.md               # This file
├── Makefile                # Build system
└── verify_architecture.sh  # Run all fitness functions
```

---

## AI Workflow (Follow This!)

When asked to modify code:

1. **Read relevant ADR** from `architecture/decision_records/`
2. **Check existing patterns** - grep for similar code
3. **Suggest changes** that follow constitutional patterns
4. **Run fitness functions** - `make verify`
5. **Run tests** - `make test`
6. **If tests fail**, fix them or update them

When in doubt:
- Check `architecture/decision_records/` for precedent
- Ask human before violating constitutional patterns
- Prefer editing existing files over creating new ones

---

## Human Approval Required For

- Adding new global tables
- Changing core data structures (Player_t, Card_t, etc)
- Breaking delegate pattern
- Adding external dependencies
- Creating new ADRs or fitness functions

---

## Build Commands Reference

```bash
make              # Build debug version
make run          # Build and run
make test         # Run unit tests
make verify       # Run architecture fitness functions
make clean        # Clean build artifacts
make valgrind     # Check for memory leaks
make asan         # Address sanitizer build
```

---

## Key Files Reference

**Global State:**
- `include/defs.h` - Global tables (g_players, g_card_textures)
- `src/main.c` - Initialization and cleanup

**Core Game Logic:**
- `src/game.c` - Blackjack rules, hand resolution
- `src/player.c` - Player management
- `src/enemy.c` - Enemy abilities and AI

**Architecture:**
- `architecture/decision_records/` - WHY things work this way
- `architecture/fitness_functions/` - Automated checks

**Structs:**
- `include/structs.h` - All major data structures

---

## Version History

- **2025-11-14**: Major rewrite - fixed contradictions, added fitness function docs
- **2025-10-05**: Initial constitutional baseline

---

**Remember:** The architecture docs (`architecture/`) are the source of truth. This file is a quick reference. When they conflict, trust the ADRs.
