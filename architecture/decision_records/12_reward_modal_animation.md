# ADR-12: Reward Modal Animation System

**Status**: Accepted
**Date**: 2025-11-17
**Context**: Post-combat reward flow, card tag system integration
**Depends On**: ADR-04 (Card Metadata), ADR-08 (Modal Design), ADR-09 (Card Tag System)
**Fitness Function**: FF-012 (architecture/fitness_functions/12_reward_modal_animation.py)

---

## Context

After defeating an enemy, players need a rewarding moment to:
1. **Celebrate victory** - Visual feedback for successful combat
2. **Make meaningful choices** - Select permanent card upgrades (tags)
3. **Understand consequences** - See tag effects before committing

**Requirements**:
- Show 3 cards with lowest tag count from player's deck (prioritize untagged)
- Offer random tag for each card
- Animated selection flow (not instant)
- Skip option (player may not want tags)
- Visual consistency with existing modal design

**Prior Art**:
- ADR-08 established modal overlay pattern
- ADR-09 defined tag system (CURSED, VAMPIRIC, LUCKY, BRUTAL, DOUBLED)
- Existing modals use static layouts (character stats, card grid)

---

## Decision

**Modal-based reward flow with multi-stage animation**

### Architecture

**1. Card-First Selection**
```c
typedef struct RewardModal {
    int card_ids[3];        // Parallel array: card IDs
    CardTag_t tags[3];      // Parallel array: offered tags
    int selected_index;     // -1 = none, 0-2 = chosen combo
    int hovered_index;      // Bidirectional hover feedback
} RewardModal_t;
```

**Why parallel arrays**: Constitutional pattern (ADR-003). Cards and tags are conceptually separate - a card can exist without a tag, tags are temporary offers.

**2. Two-Section Layout**
- **Top**: 3 cards (horizontal FlexBox)
- **Bottom**: 3 list items (vertical FlexBox)
  - Tag badge (colored, left side)
  - Card name (white text, middle)
  - Tag description (gray text, below)

**Why list-based**: Previous design (text below cards) caused overlapping. List format provides scannable comparisons.

**3. Multi-Stage Animation**
```c
typedef enum {
    ANIM_NONE,           // Pre-selection state
    ANIM_FADE_OUT,       // 0.4s - Unselected elements → 0% opacity
    ANIM_SCALE_CARD,     // 0.5s - Selected card 1.0x → 1.5x scale
    ANIM_FADE_IN_TAG,    // 0.3s - Tag badge 0% → 100% opacity
    ANIM_COMPLETE        // 0.5s pause → transition to next state
} RewardAnimStage_t;
```

**Why staged**: Creates celebration moment. Instant selection feels mechanical. Animation sequence:
1. Focuses attention (fade out noise)
2. Emphasizes choice (scale up selected card)
3. Reveals consequence (tag badge appears on card)

**4. Tag Badge Visual Language**

Colored rectangles with tag name (black text on colored background):
- **Above cards** (pre-selection): 40% opacity, 100% on hover
- **In list items** (left side): 100% opacity always
- **Final animation** (top-right of card): Fades in during ANIM_FADE_IN_TAG

**Color Palette** (from game palette):
```c
CARD_TAG_CURSED:   #a53030  // Red
CARD_TAG_VAMPIRIC: #cf573c  // Red-orange
CARD_TAG_LUCKY:    #75a743  // Green
CARD_TAG_BRUTAL:   #de9e41  // Orange
CARD_TAG_DOUBLED:  #e8c170  // Gold
```

**Why badges**: Visual consistency across UI. Same pattern used in combat stats, ability displays. Color-coding aids pattern recognition.

---

## Implementation Evidence

### Modal Structure ([include/scenes/components/rewardModal.h](include/scenes/components/rewardModal.h))
```c
typedef struct RewardModal {
    // Layout (FlexBox for automatic positioning)
    FlexBox_t* card_layout;   // Top section: 3 cards horizontal
    FlexBox_t* info_layout;   // Informational text above cards
    FlexBox_t* list_layout;   // Bottom section: 3 list items vertical

    // State
    int card_ids[3];          // Offered cards (parallel array)
    CardTag_t tags[3];        // Offered tags (parallel array)
    int selected_index;       // -1 = none, 0-2 = selected combo
    int hovered_index;        // -1 = none, 0-2 = hovered list item

    // Animation
    RewardAnimStage_t anim_stage;
    float fade_out_alpha;     // 1.0 → 0.0 (ANIM_FADE_OUT)
    float card_scale;         // 1.0 → 1.5 (ANIM_SCALE_CARD)
    float tag_badge_alpha;    // 0.0 → 1.0 (ANIM_FADE_IN_TAG)

    bool reward_taken;
    float result_timer;
} RewardModal_t;
```

### Animation Progression ([src/scenes/components/rewardModal.c:278-316](src/scenes/components/rewardModal.c))
```c
if (modal->reward_taken) {
    switch (modal->anim_stage) {
        case ANIM_FADE_OUT:
            if (modal->fade_out_alpha <= 0.01f) {
                // Stage 1 complete → Start stage 2
                modal->anim_stage = ANIM_SCALE_CARD;
                modal->card_scale = 1.0f;
                TweenFloat(&g_tween_manager, &modal->card_scale, 1.5f,
                          0.5f, TWEEN_EASE_OUT_CUBIC);
            }
            break;
        case ANIM_SCALE_CARD:
            if (modal->card_scale >= 1.49f) {
                // Stage 2 complete → Start stage 3
                modal->anim_stage = ANIM_FADE_IN_TAG;
                modal->tag_badge_alpha = 0.0f;
                TweenFloat(&g_tween_manager, &modal->tag_badge_alpha, 1.0f,
                          0.3f, TWEEN_EASE_IN_QUAD);
            }
            break;
        case ANIM_FADE_IN_TAG:
            if (modal->tag_badge_alpha >= 0.99f) {
                // Stage 3 complete → Brief pause
                modal->anim_stage = ANIM_COMPLETE;
            }
            break;
        case ANIM_COMPLETE:
            modal->result_timer += dt;
            if (modal->result_timer >= 0.5f) {
                // Animation complete → Close modal
                modal->is_open = false;
            }
            break;
    }
}
```

### Tag Badge Rendering ([src/scenes/components/rewardModal.c:421-441](src/scenes/components/rewardModal.c))
```c
// Tag badge above card (pre-selection)
int r, g, b;
GetCardTagColor(modal->tags[i], &r, &g, &b);

int badge_w = 125;
int badge_h = 35;
int badge_x = card_x + (CARD_WIDTH - badge_w) / 2;
int badge_y = card_y - 42;

// 40% opacity by default, 100% when hovered
bool is_card_hovered = (modal->hovered_index == i);
Uint8 badge_alpha = is_card_hovered ? 255 : (Uint8)(255 * 0.4);

a_DrawFilledRect(badge_x, badge_y, badge_w, badge_h, r, g, b, badge_alpha);
a_DrawRect(badge_x, badge_y, badge_w, badge_h, 0, 0, 0, badge_alpha);

a_DrawText((char*)GetCardTagName(modal->tags[i]),
          card_x + CARD_WIDTH / 2, badge_y - 6,
          0, 0, 0,  // Black text
          FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
```

### Integration with Game State ([src/scenes/components/rewardModal.c:233-236](src/scenes/components/rewardModal.c))
```c
// On selection: Apply tag + start animation
modal->selected_index = i;
AddCardTag(modal->card_ids[i], modal->tags[i]);  // ADR-09 integration
modal->reward_taken = true;
modal->anim_stage = ANIM_FADE_OUT;  // Start animation sequence
```

---

## Consequences

### Positive

**1. Celebration Moment**
- 1.7s animation sequence creates satisfying reward feedback
- Scale-up emphasizes player's choice
- Tag badge fade-in reveals consequence clearly

**2. Visual Consistency**
- Reuses FlexBox layout pattern (ADR-08)
- Tag badges match existing UI elements (combat stats, ability displays)
- Modal overlay follows established design

**3. Constitutional Compliance**
- Parallel arrays for card_ids/tags (ADR-003)
- FlexBox for automatic layout (ADR-08)
- Heap allocation for modal struct (conventional pattern)
- dString_t for card names (ADR-003)

**4. Player Agency**
- Skip button allows opt-out (tags may be undesirable)
- Hover preview before selection
- List format enables comparison

### Negative

**1. Animation Delay**
- 1.7s total animation time delays progression
- No skip option for animation (must wait)
- **Accepted**: Celebration moment worth the delay

**2. Complexity**
- 4-stage state machine adds complexity
- Tween system dependency (global `g_tween_manager`)
- **Mitigated**: Clear state transitions, single file implementation

**3. Fixed Constraints**
- Fixed 3-card offer (no scaling)
- Prioritizes lowest-tagged cards (untagged first, then 1-tag, then 2-tag, etc.)
- If all cards equally tagged, randomly selects 3
- **Accepted**: Fair distribution - players upgrade least-upgraded cards first

**4. Hardcoded Tag Pool**
- Tags randomly selected from `all_tags[]` array
- No configurable tag weighting
- Allows duplicate tags in single offer
- **Accepted**: Simple random selection sufficient for current scope

---

## Alternatives Considered

### 1. Full-Screen Scene (Rejected)
**Proposal**: Create `sceneReward.c/h` as dedicated scene

**Rejected Because**:
- Overkill for simple choice
- Modal pattern already established (ADR-08)
- Scene transition overhead unnecessary
- **Evidence**: Initial implementation had `sceneReward.c/h` - deleted as dead code

### 2. Instant Selection (Rejected)
**Proposal**: No animation, immediate modal close

**Rejected Because**:
- Lacks celebration moment
- No visual feedback for consequence
- Feels mechanical, not rewarding

### 3. Text-Below-Cards Layout (Rejected)
**Proposal**: Show tag description below each card

**Rejected Because**:
- Text overlapped between cards (see screenshot evidence from development)
- Hard to compare options
- No space for detailed descriptions

### 4. Externalized Tag Pool (Deferred)
**Proposal**: Load tag offers from config file

**Deferred Because**:
- Current hardcoded pool works fine
- No need for dynamic tag weighting yet
- Can be added later if needed (ADR amendment)

---

## Open Questions

**Q: Should we prevent duplicate tags in single offer?**
- Current: Can offer same tag 3 times (rare but possible)
- Trade-off: Simple code vs better UX
- **Defer**: Wait for playtesting feedback

**Q: Should we scale offer count with deck size?**
- Current: Always 3 cards
- Alternative: Offer scales (1 card/10 deck cards)
- **Defer**: Requires balancing data

**Q: Should animation be skippable?**
- Current: Must wait 1.7s
- Alternative: ESC or click to skip animation
- **Defer**: Celebration moment valuable, revisit if players complain

---

## Verification

**Manual Testing**:
- Hover list item → Card highlights ✓
- Click list item → Animation plays ✓
- Skip button → Modal closes without tag ✓
- All 5 tag types display correctly ✓

**Constitutional Compliance**:
- Uses dString_t for card names ✓
- FlexBox for layout (3 instances) ✓
- Parallel arrays for card_ids/tags ✓
- Heap allocation + proper cleanup ✓

**Fitness Function** (architecture/fitness_functions/12_reward_modal.py):
```python
# Verify reward modal constitutional compliance
- ShowRewardModal() selects cards with lowest tag count (prioritizes untagged)
- Animation stages fire in order: FADE_OUT → SCALE_CARD → FADE_IN_TAG → COMPLETE
- AddCardTag() called exactly once per selection
- FlexBox cleanup on DestroyRewardModal()
- Parallel arrays used for card_ids/tags (ADR-03)
```

---

## Related ADRs

- **ADR-03**: Daedalus Types (parallel arrays, dString_t)
- **ADR-04**: Card Metadata (card_ids storage)
- **ADR-08**: Modal Design Pattern (overlay layout)
- **ADR-09**: Card Tag System (CURSED, VAMPIRIC, etc.)

---

## References

**Files**:
- [include/scenes/components/rewardModal.h](include/scenes/components/rewardModal.h) - Structure definitions
- [src/scenes/components/rewardModal.c](src/scenes/components/rewardModal.c) - Implementation (680 lines)
- [src/cardTags.c](src/cardTags.c) - Tag color palette integration
- [include/tween/tween.h](include/tween/tween.h) - Animation system

**Development Screenshots**:
- List layout before badges (overlapping text issue)
- Badge system iteration (semi-transparent hover states)
- Final animation sequence (4 stages)
