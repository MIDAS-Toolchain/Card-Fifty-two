# ADR-11: Targeting Selection Uses Hover State

## Context

Trinket targeting mode requires player to select a card from overlapping fan layout. Two separate systems exist for detecting "which card is under mouse": hover system (visual feedback) and targeting click system (selection logic). This duplication causes selection mismatch when cards overlap.

**Alternatives Considered:**

1. **Non-overlapping card layout** (spread cards so no overlap)
   - Pros: Click always matches visual, no z-index ambiguity
   - Cons: Wastes screen space, ruins card game aesthetics, feels wrong
   - Cost: Redesign entire card layout system, still need hover for other features

2. **Duplicate z-index iteration in targeting** (reverse iteration like hover)
   - Pros: Matches hover priority (topmost card), no visual system dependency
   - Cons: Duplicates logic, ignores animations, two sources of truth
   - Cost: Must keep two systems in sync, animations still broken

3. **Explicit "lock selection" action** (hover to preview, keypress to lock, click to confirm)
   - Pros: Unambiguous, player controls selection timing
   - Cons: Too many steps (move → preview → lock → confirm), awkward UX
   - Cost: Adds complexity without benefit, slows down gameplay

4. **Hover state as targeting source** (click confirms hovered card)
   - Pros: Single system, respects animations, natural UX, eliminates duplication
   - Cons: Couples targeting to hover system (hover broken = targeting broken)
   - Cost: Must access section hover state from targeting handler

## Decision

**Targeting selection MUST use hover state as single source of truth. Click confirms selection of currently hovered card.**

During `STATE_TARGETING`, the hover system continues to detect which card is topmost (reverse iteration, animation-aware). Clicking triggers ability on whatever card is currently hovered. No separate position calculation or iteration logic in targeting handler.

**Justification:**

1. **Visual Consistency**: What player sees lifted (hovered card) IS what gets selected
2. **Eliminates Duplication**: One system for "card under mouse", not two
3. **Animation-Aware**: Hover system respects `CardTransition_t` (draw animations, tweens)
4. **Z-Index Correct**: Reverse iteration ensures topmost card in overlap wins
5. **Natural UX**: Matches player expectation from card games (hover = selection preview)

## Consequences

**Positive:**
- ✅ Click selects hovered/lifted card (matches visual feedback)
- ✅ Works correctly during card animations (draw, transition, hover lift)
- ✅ Single source of truth (hover state authoritative for "card under mouse")
- ✅ Less code (removes manual position calculation from targeting)
- ✅ Player can "scan" targets by hovering before clicking (UX improvement)

**Negative:**
- ❌ Requires hover to select (can't click without hovering first)
  - Mitigation: Natural behavior - player must aim mouse anyway
- ❌ Coupling to hover system (hover broken = targeting broken)
  - Mitigation: Hover already critical for normal gameplay, acceptable dependency
- ❌ Section hover state must be accessible from targeting handler
  - Current status: `g_player_section` and `g_dealer_section` already global ✅

**Accepted Trade-offs:**
- ✅ Accept hover coupling for consistency (visual feedback MUST match click)
- ✅ Accept global section access for simplicity (already exists, no new globals)
- ❌ NOT accepting: Duplicate iteration logic (two systems = inevitable divergence)

**Pattern Used:**

Targeting handler reads hover state from sections:
```c
// In HandleTargetingInput() - sceneBlackjack.c
if (app.mouse.pressed) {
    // Check player section hover state
    int player_hovered = g_player_section->hover_state.hovered_card_index;
    if (player_hovered >= 0) {
        const Card_t* card = GetCardFromHand(&player->hand, player_hovered);
        if (IsCardValidTarget(card, trinket_slot)) {
            ExecuteTrinketAbility(card);
            State_Transition(&g_game, STATE_PLAYER_TURN);
            return;
        }
    }

    // Check dealer section hover state
    int dealer_hovered = g_dealer_section->hover_state.hovered_card_index;
    if (dealer_hovered >= 0) {
        const Card_t* card = GetCardFromHand(&dealer->hand, dealer_hovered);
        if (IsCardValidTarget(card, trinket_slot)) {
            ExecuteTrinketAbility(card);
            State_Transition(&g_game, STATE_PLAYER_TURN);
            return;
        }
    }
}
```

Hover state populated by sections (unchanged):
```c
// In RenderPlayerSection() - playerSection.c:110-129
// Detect which card is hovered (reverse order for z-index priority)
for (int i = (int)hand_size - 1; i >= 0; i--) {
    // Get position (respects CardTransition_t animations)
    int x, y;
    CardTransition_t* trans = GetCardTransition(anim_mgr, hand, i);
    if (trans && trans->active) {
        x = (int)trans->current_x;
        y = (int)trans->current_y;
    } else {
        CalculateCardFanPosition(i, hand_size, cards_y, &x, &y);
    }

    if (IsCardHovered(x, y)) {
        new_hovered_index = i;  // Topmost card wins
        break;
    }
}
```

Visual feedback during targeting:
```c
// In hovered card rendering - playerSection.c (during targeting mode)
if (g_game.current_state == STATE_TARGETING) {
    bool is_valid = IsCardValidTarget(card, trinket_slot);
    if (is_valid) {
        // Bright green border (hovered + valid target)
        a_DrawRect(scaled_x - 2, scaled_y - 2,
                  scaled_width + 4, scaled_height + 4,
                  0, 255, 0, 255);
    } else {
        // Red border (hovered + invalid target)
        a_DrawRect(scaled_x - 2, scaled_y - 2,
                  scaled_width + 4, scaled_height + 4,
                  255, 50, 50, 255);
    }
}
```

**Why This Matters:**

Card overlap is intentional (`CARD_SPACING = 32` < `CARD_WIDTH = 85`). Hover system already solves z-index priority for visual feedback (reverse iteration, topmost card lifts). Duplicating this logic in targeting creates inevitable divergence:

- **Before**: Hover lifts card #3, click selects card #2 underneath (forward iteration)
- **After**: Hover lifts card #3, click selects card #3 (uses hover state)

Player expects "what I see is what I click" - this ADR enforces that expectation at architecture level.

**Visual Hierarchy During Targeting:**
- Hovered + Valid: Bright green border + lift animation
- Hovered + Invalid: Red border + lift animation
- Not Hovered + Valid: Faint green overlay
- Not Hovered + Invalid: Gray dim overlay

Player mental model: "The card that pops up when I hover is the one I'll select" - matches decades of digital card game conventions (Hearthstone, Slay the Spire, MTG Arena).
