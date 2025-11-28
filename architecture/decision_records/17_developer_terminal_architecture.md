# ADR-017: Developer Terminal Architecture

**Status:** Accepted

**Date:** 2025-11-22

**Context:** Live development, debugging, and QA testing infrastructure

---

## Problem Statement

Game development requires rapid iteration and debugging without stopping execution. Common needs include:

1. **Spawning test scenarios** (specific enemies, events, card combinations)
2. **State inspection** (current player stats, enemy abilities, card deck)
3. **Game manipulation** (give chips, damage enemy, force events)
4. **Performance debugging** (toggle FPS, memory stats, render info)
5. **QA regression testing** (reproduce bugs with specific game states)

**Traditional Solutions:**

**External CLI/Debugger:**
- Pros: Powerful, familiar tools (GDB, LLDB)
- Cons: Pauses execution, breaks flow state, can't test real-time behavior
- Cost: Context switching between game and debugger

**In-Game Debug Menu:**
- Pros: Visible during gameplay, no context switch
- Cons: Clutters UI, competes for input, hard to navigate complex commands
- Cost: Development time for menu UI, modal management complexity

**Hardcoded Keybinds:**
- Pros: Fast, direct access
- Cons: Limited scalability (run out of keys), hard to remember, no discoverability
- Cost: Keyboard pollution, conflicts with game inputs

---

## Decision

**Implement Quake-style developer terminal with command registration system.**

**Pattern:** Overlay console that drops down from top of screen, accepts text commands, extensible via function registration.

**Key Design Principles:**

1. **Maintainability** - Clear code organization, self-documenting
2. **Extensibility** - New commands added without modifying core terminal
3. **Consistency** - Follows all constitutional patterns (ADR-003, ADR-006, ADR-015)

**Architecture:**

```
terminal/
├── terminal.c       (Core: lifecycle, rendering, I/O, command execution)
├── commands.c       (Extension: built-in command implementations)
└── terminal.h       (API: public interface, registration system)
```

---

## Core Design Decisions

### 1. Quake-Style Overlay (Not Modal Dialog)

**Choice:** Terminal slides down from top of screen, overlays game, toggled with Ctrl+`

**Alternatives Rejected:**
- Full-screen modal ❌ (blocks game view, kills flow state)
- Bottom-anchored console ❌ (covers gameplay area)
- Side panel ❌ (reduces game area, permanent screen space cost)

**Justification:**
- ✅ Non-intrusive (only visible when needed)
- ✅ Doesn't pause game (can execute commands mid-combat)
- ✅ Familiar to developers (Quake, Source engine, Unreal)
- ✅ Preserves game area when closed

### 2. Command Registration System (Not Hardcoded If-Else)

**Choice:** Commands registered via function pointers, table lookup for execution

**Pattern:**
```c
typedef void (*CommandFunc_t)(Terminal_t* terminal, const char* args);

void RegisterCommand(Terminal_t* terminal,
                     const char* name,
                     CommandFunc_t execute,
                     const char* help_text);
```

**Alternative Rejected:**
```c
// ❌ Hardcoded if-else chain (not extensible)
void ExecuteCommand(Terminal_t* terminal, const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        ShowHelp(terminal);
    } else if (strcmp(cmd, "clear") == 0) {
        ClearTerminal(terminal);
    } else if (...) {
        // Adding new commands requires modifying this function!
    }
}
```

**Justification:**
- ✅ Extensible (add commands without touching terminal.c)
- ✅ Maintainable (each command is isolated function)
- ✅ Testable (commands can be unit tested independently)
- ✅ Self-documenting (help_text required at registration)
- ✅ Modular (commands.c can grow independently)

**Trade-off Accepted:**
- ❌ Slight overhead (table lookup vs direct if-else)
- ✅ Worth it for maintainability and extensibility

### 3. FlexBox Layout (Not Manual Positioning)

**Choice:** Use Archimedes FlexBox for automatic line positioning and scrolling

**Pattern:**
```c
terminal->output_layout = a_FlexBoxCreate(10, 10, width, height);
a_FlexConfigure(terminal->output_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, 8);

// TerminalPrint adds items dynamically
a_FlexAddItem(terminal->output_layout, SCREEN_WIDTH - 20, 24, line);
a_FlexLayout(terminal->output_layout);  // Recalculate positions
```

**Alternative Rejected:**
```c
// ❌ Manual Y position tracking (error-prone)
int current_y = 10;
for (int i = 0; i < output_log->count; i++) {
    RenderLine(lines[i], 10, current_y);
    current_y += line_height + gap;  // Manual math, easy to get wrong
}
```

**Justification:**
- ✅ Automatic layout (no manual Y calculations)
- ✅ Scrolling support (offset applied to FlexBox Y)
- ✅ Consistent with UI patterns (modals use FlexBox)
- ✅ Handles edge cases (content overflow, dynamic resizing)

### 4. Daedalus Types (Not Raw Malloc)

**Choice:** Use dString_t, dArray_t, dTable_t for all dynamic data (ADR-003)

**Pattern:**
```c
// ✅ Constitutional pattern
terminal->input_buffer = d_StringInit();
terminal->command_history = d_ArrayInit(16, sizeof(dString_t*));
terminal->output_log = d_ArrayInit(TERMINAL_MAX_OUTPUT_LINES, sizeof(dString_t*));
terminal->registered_commands = d_TableInit(sizeof(char*), sizeof(CommandHandler_t*), ...);
```

**Alternative Rejected:**
```c
// ❌ Raw malloc (violates ADR-003)
terminal->input_buffer = malloc(256);
terminal->command_history = malloc(16 * sizeof(char*));
```

**Justification:**
- ✅ Consistent with codebase (all UI components use Daedalus)
- ✅ Automatic growth (arrays resize on overflow)
- ✅ Safer cleanup (dString_t/dArray_t handle nested resources)
- ✅ Type safety (d_ArrayInit stores element_size, catches mismatches)

### 5. Double-Pointer Cleanup (Not Single Pointer)

**Choice:** CleanupTerminal uses double-pointer destructor (ADR-015)

**Pattern:**
```c
void CleanupTerminal(Terminal_t** terminal) {
    if (!terminal || !*terminal) return;  // NULL guard

    Terminal_t* t = *terminal;

    // Destroy children (reverse init order)
    for (size_t i = 0; i < t->output_log->count; i++) {
        dString_t** str_ptr = (dString_t**)d_ArrayGet(t->output_log, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }
    d_ArrayDestroy(t->output_log);
    // ... (other cleanup)

    free(*terminal);
    *terminal = NULL;  // Null caller's pointer
}
```

**Alternative Rejected:**
```c
// ❌ Single pointer (can't null caller's variable)
void CleanupTerminal(Terminal_t* terminal) {
    free(terminal);
    // Caller's pointer still points to freed memory!
}
```

**Justification:**
- ✅ Prevents use-after-free (pointer nulled automatically)
- ✅ Prevents double-free (NULL guard catches repeat calls)
- ✅ Consistent with all UI components (33+ components use this pattern)
- ✅ Idempotent (safe to call multiple times)

---

## Memory Management Pattern

### Arrays of Pointers (Not Arrays of Values)

**Pattern:**
```c
// output_log stores POINTERS to dString_t (not dString_t by value)
dArray_t* output_log;  // sizeof(dString_t*)

void TerminalPrint(Terminal_t* terminal, const char* format, ...) {
    dString_t* line = d_StringInit();  // Heap-allocated
    d_StringSet(line, buffer, strlen(buffer));
    d_ArrayAppend(terminal->output_log, &line);  // Store pointer
}

void CleanupTerminal(Terminal_t** terminal) {
    // Free each string individually
    for (size_t i = 0; i < t->output_log->count; i++) {
        dString_t** str_ptr = (dString_t**)d_ArrayGet(t->output_log, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);  // Free string
        }
    }
    d_ArrayDestroy(t->output_log);  // Then free array
}
```

**Why Pointers Not Values:**
- ✅ Flexibility (strings can be different lengths)
- ✅ Ownership (terminal owns the strings, can destroy selectively)
- ✅ Consistency (command_history uses same pattern)

**Trade-off:**
- ❌ Extra indirection (pointer → pointer → string data)
- ✅ Worth it for flexibility and clear ownership

### Command Handler Storage Pattern

**Pattern:**
```c
// registered_commands stores CommandHandler_t* (pointers)
CommandHandler_t* handler = malloc(sizeof(CommandHandler_t));
handler->name = d_StringInit();
handler->execute = function_pointer;
handler->help_text = d_StringInit();

const char* key = d_StringPeek(handler->name);
d_TableSet(terminal->registered_commands, &key, &handler);
```

**Why Heap-Allocated Handlers:**
- ✅ Lifetime (handlers persist until CleanupTerminal)
- ✅ Extensibility (commands.c allocates handlers independently)
- ✅ Cleanup (CleanupTerminal walks table, frees each handler)

---

## Extensibility: Adding New Commands

### Step 1: Implement Command Function (commands.c)

```c
void CommandSpawnEnemy(Terminal_t* terminal, const char* args) {
    // Parse arguments
    char enemy_type[64];
    int hp = 100;
    sscanf(args, "%63s %d", enemy_type, &hp);

    // Execute game logic
    Enemy_t* enemy = CreateEnemy(enemy_type, hp);
    SetCurrentEnemy(enemy);

    // Print confirmation
    TerminalPrint(terminal, "Spawned %s with %d HP", enemy_type, hp);
}
```

### Step 2: Register Command (RegisterBuiltinCommands)

```c
void RegisterBuiltinCommands(Terminal_t* terminal) {
    RegisterCommand(terminal,
                    "spawn_enemy",
                    CommandSpawnEnemy,
                    "Spawn enemy: spawn_enemy <type> <hp>");
    // ... other commands
}
```

**That's it!** No modifications to terminal.c required.

---

## Consistency with Constitutional Patterns

### ADR-003: Daedalus Types

**Evidence:**
```c
// terminal.c:33-52
terminal->input_buffer = d_StringInit();
terminal->command_history = d_ArrayInit(16, sizeof(dString_t*));
terminal->output_log = d_ArrayInit(TERMINAL_MAX_OUTPUT_LINES, sizeof(dString_t*));
```

**Result:** ✅ Zero raw malloc for strings/arrays

### ADR-006: d_ArrayInit Parameter Order

**Evidence:**
```c
// terminal.c:41-42 (with required comment!)
// d_ArrayInit(capacity, element_size) - capacity FIRST!
terminal->command_history = d_ArrayInit(16, sizeof(dString_t*));
```

**Result:** ✅ Capacity first, inline comments present

### ADR-015: Double-Pointer Destructors

**Evidence:**
```c
// terminal.h:80
void CleanupTerminal(Terminal_t** terminal);

// terminal.c:102-156 (all 4 steps present)
void CleanupTerminal(Terminal_t** terminal) {
    if (!terminal || !*terminal) return;  // 1. NULL guard
    // 2. Destroy children
    free(*terminal);                      // 3. Free memory
    *terminal = NULL;                     // 4. Null pointer
}
```

**Result:** ✅ All 4 steps, correct signature

---

## Performance Considerations

### FlexBox Layout Cost

**Pattern:**
```c
a_FlexLayout(terminal->output_layout);  // Called on every TerminalPrint
```

**Cost:**
- Recalculates positions for all output lines
- O(n) where n = number of lines in output log

**Mitigation:**
- Max 100 lines (TERMINAL_MAX_OUTPUT_LINES)
- FlexBox is optimized for UI layouts
- Only recalculates when terminal is visible
- Negligible compared to rendering cost

**Trade-off:**
- ❌ Slight overhead on TerminalPrint
- ✅ Worth it for automatic scrolling and layout

### Command Lookup Cost

**Pattern:**
```c
CommandHandler_t** handler = d_TableGet(registered_commands, &cmd_name);
```

**Cost:**
- Table lookup: O(1) average (hash table)
- Worst case: O(n) on hash collision

**Mitigation:**
- Small number of commands (~10-20)
- d_HashString is fast (Daedalus optimized)
- Commands executed infrequently (human input)

**Trade-off:**
- ❌ Slower than if-else for 2-3 commands
- ✅ Faster than if-else for 10+ commands
- ✅ Extensibility benefit outweighs cost

---

## Visual Design

### Quake-Style Aesthetics

**Colors:**
```c
#define TERMINAL_BG_COLOR          ((aColor_t){20, 20, 20, 230})   // Dark semi-transparent
#define TERMINAL_TEXT_COLOR        ((aColor_t){0, 255, 0, 255})    // Green (classic)
#define TERMINAL_INPUT_COLOR       ((aColor_t){232, 193, 112, 255}) // Gold (input highlight)
#define TERMINAL_CURSOR_COLOR      ((aColor_t){255, 255, 255, 255}) // White (blinking)
```

**Sizing:**
```c
#define TERMINAL_HEIGHT_RATIO      0.6f  // Top 60% of screen
```

**Font:**
```c
.scale=0.8f  // Smaller than game text for density
```

**Justification:**
- ✅ Dark overlay doesn't obscure game too much
- ✅ Green text nostalgic for developers (Quake, Matrix)
- ✅ Gold input stands out (clear what you're typing)
- ✅ 60% height provides space without dominating screen

---

## Integration with Game Systems

### Terminal → Game Communication

**Pattern: Direct Function Calls (Not Event System)**

```c
void CommandDamageEnemy(Terminal_t* terminal, const char* args) {
    int damage = atoi(args);

    // Direct game state manipulation
    if (g_game.current_enemy) {
        DamageEnemy(g_game.current_enemy, damage);
        TerminalPrint(terminal, "Dealt %d damage to enemy", damage);
    }
}
```

**Alternative Rejected:**
```c
// ❌ Event system (over-engineered for debug commands)
QueueEvent(EVENT_DAMAGE_ENEMY, damage);
```

**Justification:**
- ✅ Simple (no event queue needed)
- ✅ Immediate (command executes synchronously)
- ✅ Debuggable (stack trace shows command → game logic)
- ✅ Appropriate (debug commands are synchronous by nature)

**Trade-off:**
- ❌ Commands tightly coupled to game state
- ✅ Acceptable for debug tool (not production API)

---

## Testing Strategy

### Unit Testing Commands

**Pattern:**
```c
// Test CommandSpawnEnemy
Terminal_t* terminal = InitTerminal();
CommandSpawnEnemy(terminal, "Boss 500");
assert(g_game.current_enemy != NULL);
assert(g_game.current_enemy->max_hp == 500);
CleanupTerminal(&terminal);
```

**Benefits:**
- ✅ Each command is isolated function (easy to test)
- ✅ No UI mocking needed (commands just manipulate state)
- ✅ Can test error handling (invalid args, null checks)

### Integration Testing Terminal

**Pattern:**
```c
// Test full command execution flow
Terminal_t* terminal = InitTerminal();
ExecuteCommand(terminal, "spawn_enemy Boss 500");
assert(g_game.current_enemy->max_hp == 500);
CleanupTerminal(&terminal);
```

---

## Security Considerations

### Not Production Code

**Decision:** Terminal is **development-only**, stripped in release builds

**Pattern:**
```c
#ifdef DEBUG_BUILD
    g_terminal = InitTerminal();
#endif
```

**Justification:**
- ✅ No security risk (not shipped to players)
- ✅ No need for input validation (developers are trusted)
- ✅ Simplifies implementation (no sandbox needed)

**Trade-off:**
- ❌ Terminal commands can crash game if misused
- ✅ Acceptable for debug tool (developers understand risk)

---

## Consequences

### Positive

**Maintainability:**
- ✅ Clear code organization (3 files, 1,104 lines total)
- ✅ Each command is ~20-30 lines (easy to understand)
- ✅ Self-documenting (help text required at registration)
- ✅ Follows all constitutional patterns (grep-able, consistent)

**Extensibility:**
- ✅ Add commands without modifying terminal.c
- ✅ Commands isolated in commands.c (grows independently)
- ✅ Registration system makes new commands trivial
- ✅ Function pointer pattern enables external modules to register commands

**Consistency:**
- ✅ Uses Daedalus types (ADR-003)
- ✅ Correct d_ArrayInit order (ADR-006)
- ✅ Double-pointer cleanup (ADR-015)
- ✅ FlexBox layout (matches modal patterns)

**Developer Experience:**
- ✅ Familiar (Quake-style, known UX)
- ✅ Non-intrusive (toggle on/off quickly)
- ✅ Powerful (full game state access)
- ✅ Discoverable (help command lists all commands)

### Negative

**Performance:**
- ❌ FlexBox recalculation on every TerminalPrint (negligible)
- ❌ Table lookup for commands (faster than if-else for 10+ commands)

**Complexity:**
- ❌ More code than simple if-else command parser
- ✅ Trade-off accepted for extensibility

**Testing:**
- ❌ Integration testing requires game state setup
- ✅ Unit testing commands is trivial (isolated functions)

### Accepted Trade-offs

- ✅ Accept FlexBox overhead for automatic layout
- ✅ Accept command registration complexity for extensibility
- ✅ Accept tight coupling to game state (debug tool, not production API)
- ❌ NOT accepting: Single-pointer destructors (violates ADR-015)
- ❌ NOT accepting: Raw malloc for strings (violates ADR-003)

---

## Success Metrics

**Maintainability:**
- ✅ Zero ADR violations (passes all fitness functions)
- ✅ Clear separation of concerns (terminal core vs commands)
- ✅ Self-documenting (help text, inline comments)

**Extensibility:**
- ✅ 10+ commands registered (help, clear, spawn_enemy, give_chips, etc.)
- ✅ Adding new command takes <10 minutes
- ✅ No terminal.c modifications needed for new commands

**Consistency:**
- ✅ Uses dString_t/dArray_t exclusively (ADR-003)
- ✅ Correct d_ArrayInit parameter order (ADR-006)
- ✅ Double-pointer cleanup pattern (ADR-015)

**Usage:**
- ✅ Used daily during development (spawn enemies, test scenarios)
- ✅ Zero crashes from terminal commands
- ✅ Valgrind clean (no memory leaks)

---

## Evidence From Codebase

### Terminal Structure (terminal.h:48-62)

```c
typedef struct Terminal {
    bool is_visible;              // Terminal overlay visible
    dString_t* input_buffer;      // ✅ dString_t (ADR-003)
    dArray_t* command_history;    // ✅ Array of dString_t*
    dArray_t* output_log;         // ✅ Array of dString_t*
    dTable_t* registered_commands; // ✅ Command registry
    int history_index;
    int scroll_offset;
    float cursor_blink_timer;
    bool cursor_visible;
    FlexBox_t* output_layout;     // ✅ FlexBox for layout
    bool scrollbar_dragging;
    int drag_start_y;
    int drag_start_scroll;
} Terminal_t;
```

### Command Registration (terminal.c:534-556)

```c
void RegisterCommand(Terminal_t* terminal,
                     const char* name,
                     CommandFunc_t execute,
                     const char* help_text) {
    CommandHandler_t* handler = malloc(sizeof(CommandHandler_t));
    handler->name = d_StringInit();
    d_StringSet(handler->name, name, strlen(name));
    handler->execute = execute;
    handler->help_text = d_StringInit();
    d_StringSet(handler->help_text, help_text, strlen(help_text));

    const char* key = d_StringPeek(handler->name);
    d_TableSet(terminal->registered_commands, &key, &handler);
}
```

### Cleanup Pattern (terminal.c:102-156)

```c
void CleanupTerminal(Terminal_t** terminal) {
    if (!terminal || !*terminal) return;  // ✅ NULL guard

    Terminal_t* t = *terminal;

    // ✅ Destroy children (reverse init order)
    for (size_t i = 0; i < t->command_history->count; i++) {
        dString_t** str_ptr = (dString_t**)d_ArrayGet(t->command_history, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }
    // ... (output_log cleanup)
    // ... (registered_commands cleanup)

    // ✅ Free heap memory
    free(t);

    // ✅ Null caller's pointer
    *terminal = NULL;
}
```

---

## Related Decisions

- **ADR-003**: Constitutional Patterns (mandates Daedalus types)
- **ADR-006**: d_ArrayInit Parameter Order (verified in terminal code)
- **ADR-015**: Double-Pointer Destructors (CleanupTerminal follows pattern)
- **ADR-008**: Modal Design Consistency (FlexBox usage pattern)

---

## Future Enhancements

### Potential Features

1. **Command History Persistence** - Save/load history across sessions
2. **Tab Completion** - Auto-complete command names
3. **Alias System** - User-defined command shortcuts
4. **Scripting Support** - Load/execute .txt files with commands
5. **Output Filtering** - Grep-like search in output log

**Constraint:** All enhancements MUST follow constitutional patterns (ADR-003, ADR-006, ADR-015)

---

## Summary

**What:** Quake-style developer terminal with command registration system

**Why:**
- **Maintainability** - Clear code organization, constitutional patterns
- **Extensibility** - Add commands without modifying core
- **Consistency** - Follows all ADRs (003, 006, 015)

**How:**
1. Overlay console (Ctrl+` toggle)
2. Command registration (function pointers, table lookup)
3. Daedalus types (dString_t, dArray_t, dTable_t)
4. FlexBox layout (automatic positioning)
5. Double-pointer cleanup (memory safety)

**Impact:**
- ✅ Daily development tool (spawn enemies, test scenarios)
- ✅ Zero architectural violations
- ✅ 10+ commands, extensible design
- ✅ Clean separation of concerns (3 files, 1,104 lines)
