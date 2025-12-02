# ADR-09: Tag Effect Execution via Event Triggers and Global Aggregation

## Context

Card tags exist (ADR-04 metadata registry). Need to define how tag EFFECTS execute during gameplay. Two effect categories: on-draw triggers (CURSED, VAMPIRIC) and passive global bonuses (LUCKY, BRUTAL).

**Alternatives Considered:**

1. **Per-frame polling** (every frame, check all cards for active tags)
   - Pros: Simple, no event infrastructure needed
   - Cons: Inefficient (100 cards × 60fps = 6000 checks/sec), can't detect discrete events
   - Cost: Wasted CPU, missed one-time triggers (can't tell WHEN card was drawn)

2. **Event-driven triggers + per-frame aggregation** (trigger on draw, aggregate bonuses every frame)
   - Pros: Efficient triggers, always-current bonuses
   - Cons: Redundant aggregation (recalculates unchanged state 60fps)
   - Cost: ~100 tag checks per frame even when hands don't change

3. **Event-driven triggers + dirty-flag aggregation** (trigger on draw, aggregate only on hand change)
   - Pros: Efficient triggers, cached aggregation, skips redundant work
   - Cons: Must detect hand changes, cache invalidation logic
   - Cost: Hand change detection in draw/discard/round-end paths

4. **Hardcoded effect checks** (in game logic directly)
   - Pros: Zero abstraction, maximum performance
   - Cons: Game.c knows about every tag type, unmaintainable
   - Cost: Adding new tag requires modifying core game loop

## Decision

**Use event-driven triggers (ADR-01 Game_TriggerEvent) for on-draw effects and dirty-flag aggregation for passive global bonuses.**

On-draw tags (CURSED, VAMPIRIC) trigger via GAME_EVENT_CARD_DRAWN. Passive tags (LUCKY, BRUTAL) aggregate when hands change (draw/discard/round-end sets dirty flag).

**Justification:**

1. **Reuses Event Infrastructure**: ADR-01 already has Game_TriggerEvent dispatcher
2. **Discrete Event Detection**: On-draw effects know WHEN card was drawn (not just "is in hand")
3. **Cached Aggregation**: Passive effects recalculate only when hands change (dirty flag)
4. **Global Scope**: Aggregation scans both player AND enemy hands (shared deck asymmetry)
5. **Isolated Effect Logic**: cardTags.c handles all effects, game.c just fires events

## Consequences

**Positive:**
- ✅ Reuses existing event system (ADR-01 dispatcher)
- ✅ Dirty-flag prevents 6000 checks/sec (skips redundant work)
- ✅ On-draw effects fire exactly once per draw (event-driven)
- ✅ Global aggregation leverages shared deck (asymmetric gameplay)
- ✅ Adding new tags requires zero game.c changes

**Negative:**
- ❌ Hand change detection required (must set dirty flag in 5+ places)
- ❌ Global aggregation scans player + enemy hands (O(n) on change)
- ❌ Cache invalidation complexity (must not forget dirty flag)

**Accepted Trade-offs:**
- ✅ Accept dirty-flag maintenance for 60× performance gain
- ✅ Accept O(n) aggregation on hand change (infrequent compared to per-frame)
- ❌ NOT accepting: Per-frame aggregation (wastes 6000 checks/sec)

**Pattern Used:**

On-draw triggers fire via event:
```c
// In game.c when card drawn
Card_t drawn = DrawCardFromDeck(&game->deck);
Game_TriggerEvent(&game, GAME_EVENT_CARD_DRAWN);

// In cardTags.c (event handler)
void OnCardDrawn(int card_id, Player_t* drawer, GameContext_t* game) {
    if (HasCardTag(card_id, CARD_TAG_CURSED)) {
        DamageEnemy(game->current_enemy, 10);
        d_LogInfo("CURSED card! 10 damage to enemy");
    }
}
```

Passive aggregation uses dirty flag:
```c
// In hand.c when card added/removed
void AddCardToHand(Hand_t* hand, Card_t card) {
    d_ArrayAppend(hand->cards, &card);
    g_game.combat_bonuses_dirty = true;  // Invalidate cache
}

// In game.c update loop
void UpdateCombatBonuses(GameContext_t* game) {
    if (!game->combat_bonuses_dirty) return;  // Skip if valid
    
    // Scan all hands for LUCKY/JAGGED tags
    game->global_crit_bonus = CountTagsInPlay(CARD_TAG_LUCKY) * 10;
    game->global_damage_bonus = CountTagsInPlay(CARD_TAG_JAGGED) * 10;
    
    game->combat_bonuses_dirty = false;  // Cache now valid
}
```

**Success Metrics:**
- Profiler shows <1% CPU on combat bonus aggregation
- CURSED/VAMPIRIC trigger exactly once per card draw (logged)
- LUCKY/BRUTAL bonuses update within 1 frame of hand change
- Fitness function verifies all AddCardToHand/RemoveCardFromHand set dirty flag

---

## Evolution: DUF Migration (ADR-021)

**Date:** 2025-11-30

**Change:** Tag metadata (names, colors, descriptions) migrated from hardcoded C to DUF data files.

**Before (Hardcoded):**
```c
const char* GetCardTagName(CardTag_t tag) {
    switch (tag) {
        case CARD_TAG_CURSED: return "Cursed";
        case CARD_TAG_LUCKY: return "Lucky";
        // ... hardcoded for all 5 tags
    }
}
```

**After (DUF-Based):**
```c
// src/cardTags.c
const char* GetCardTagName(CardTag_t tag) {
    return GetTagDisplayName(tag);  // Delegates to DUF loader
}

// data/card_tags/tags.duf
@cursed {
    display_name: "Cursed"
    description: "10 damage to enemy when drawn"
    color_r: 165
    color_g: 48
    color_b: 48
    trigger: { type: "on_draw" }
    effects: [ { type: "damage_enemy", value: 10 } ]
}
```

**Rationale:**
- **Consistency:** Events, trinkets, enemies all use DUF (ADR-019 pattern)
- **Declarative:** Trigger/effect structure matches enemy abilities
- **Flexibility:** Easy to tweak display text/colors without recompiling

**What Stayed in C (ADR-09 Core):**
- `CardTag_t` enum (5 fixed tags: CURSED, VAMPIRIC, LUCKY, BRUTAL, DOUBLED)
- Tag effect execution logic (`ProcessCardTagEffects()`)
- Event-driven triggers (GAME_EVENT_CARD_DRAWN for on-draw tags)
- Dirty-flag aggregation (passive tag bonuses)

**What Moved to DUF (ADR-021):**
- Display names ("Cursed", "Lucky", etc.)
- RGB color values (165,48,48 for CURSED, etc.)
- Effect descriptions ("10 damage to enemy when drawn")
- Trigger metadata (type: "on_draw" vs "passive", scope, duration)

**Architecture Impact:**
- **Loader Created:** `src/loaders/cardTagLoader.c` (400 lines, ADR-019 compliant)
- **Startup Validation:** `ValidateCardTagDatabase()` ensures all 5 tags present
- **Fitness Function Updated:** FF-009 now checks for DUF loader calls (not switch statements)

**See Also:**
- ADR-019 (On-Demand DUF Loader Pattern)
- ADR-021 (Card Tag DUF Migration) - full implementation details
- `data/card_tags/tags.duf` - tag definitions
- `src/loaders/cardTagLoader.c` - loader implementation