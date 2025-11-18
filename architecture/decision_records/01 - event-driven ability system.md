# ADR-001: Event-Driven Ability System Over Observer Pattern

## Context

Enemies need dynamic abilities that trigger on gameplay outcomes (win, loss, blackjack, card drawn). The system must support multiple abilities per enemy, various trigger conditions, and future expansion to player abilities.

**Alternatives Considered:**

1. **Direct event hooks** (`Game_TriggerEvent()` called explicitly in game logic)
   - Pros: Simple to trace, no framework overhead, explicit control flow
   - Cons: Coupling between game.c and event system, manual hook insertion
   - Cost: Must remember to fire events at decision points

2. **Observer pattern with event bus** (publish/subscribe registry)
   - Pros: Fully decoupled, automatic event propagation, multiple listeners per event
   - Cons: Complex implementation, harder to debug, indirection overhead
   - Cost: Event bus infrastructure, registration/unregistration logic

3. **Polling system** (abilities check game state each frame)
   - Pros: No event infrastructure needed
   - Cons: Inefficient, abilities don't know WHEN things happened
   - Cost: Every ability checks every frame, missed one-time events

4. **Hardcoded ability checks** (in game logic directly)
   - Pros: Zero abstraction, maximum performance
   - Cons: Game logic knows about every ability, unmaintainable
   - Cost: Game.c becomes 5000+ lines, every new ability modifies core loop

## Decision

**Use direct event hooks with explicit `Game_TriggerEvent()` calls at key gameplay moments.**

Events defined as enum in game.h. Game logic fires events after state changes. Single dispatcher function checks enemy abilities if in combat mode.

**Justification:**

1. **Traceability**: Grep for "Game_TriggerEvent" shows all event sources
2. **Simplicity**: One function, ten callsites, zero magic
3. **Extensibility**: Adding new event = one enum value + one callsite
4. **Isolation**: Enemy system doesn't know about game logic, game logic doesn't know about abilities
5. **Future-Proof**: Same hook supports player abilities without refactor

## Consequences

**Positive:**
- ✅ Explicit control flow (no hidden event propagation)
- ✅ Easy to debug (breakpoint in dispatcher catches all ability triggers)
- ✅ Zero framework overhead (direct function call)
- ✅ Future player abilities use same system (symmetry)
- ✅ Tutorial-friendly (events visible in logs)

**Negative:**
- ❌ Game.c coupled to event system (must import game.h event enum)
- ❌ Manual hook insertion required (developer must remember)
- ❌ No compile-time verification of event coverage

**Accepted Trade-offs:**
- ✅ Accept coupling for simplicity over observer pattern complexity
- ✅ Accept manual hooks over automatic detection (explicit > implicit)
- ❌ NOT accepting: Missing events = runtime bugs - fitness function will verify

**Pattern Used:**
Events fire AFTER state changes, not before. Example: bust is detected, LoseBet() called, THEN event fires. This ensures abilities see final state.

**Success Metrics:**
- Fitness function verifies all GameEvent_t values have at least one callsite
- Combat log shows ability triggers in order
- Adding new ability doesn't require touching game.c