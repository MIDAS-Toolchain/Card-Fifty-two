#ifndef REWARD_MODAL_H
#define REWARD_MODAL_H

#include "common.h"
#include "structs.h"
#include "cardTags.h"

// ============================================================================
// REWARD MODAL COMPONENT
// ============================================================================

// Layout constants (matching cardGridModal design pattern)
#define REWARD_MODAL_WIDTH          900
#define REWARD_MODAL_HEIGHT         700
#define REWARD_MODAL_HEADER_HEIGHT  50
#define REWARD_MODAL_PADDING        30
#define REWARD_CARD_SPACING         40
#define REWARD_LIST_ITEM_HEIGHT     90
#define REWARD_LIST_ITEM_SPACING    10

/**
 * RewardAnimStage_t - Animation stages for reward selection
 */
typedef enum {
    ANIM_NONE,           // No animation
    ANIM_FADE_OUT,       // Stage 1: Fade out non-selected
    ANIM_SCALE_CARD,     // Stage 2: Scale up selected card
    ANIM_FADE_IN_TAG,    // Stage 3: Fade in tag badge
    ANIM_COMPLETE        // Ready to transition
} RewardAnimStage_t;

/**
 * RewardModal_t - Modal for card tag rewards (overlays on blackjack scene)
 *
 * NEW FLOW (v2):
 * 1. Pick 3 random untagged cards from deck
 * 2. Assign each card a random tag
 * 3. Show all 3 card+tag combos with visual preview
 * 4. Player clicks one card to apply that tag to that card
 * 5. Animate selection: fade out → scale up → show tag badge
 * 6. Auto-hide modal after animation
 *
 * Constitutional pattern:
 * - Modal component (like cardGridModal, matches design palette)
 * - Renders on top of blackjack scene
 * - card_ids[] and tags[] are parallel arrays (3 offers)
 * - Uses FlexBox for card layout
 * - Uses tween system for smooth animations
 */
typedef struct RewardModal {
    bool is_visible;              // true = shown, false = hidden
    int card_ids[3];              // 3 different cards being offered
    CardTag_t tags[3];            // Tag for each card (parallel array)
    int selected_index;           // -1 = none, 0-2 = which combo picked
    int hovered_index;            // -1 = none, 0-2 = which item is hovered
    int key_held_index;           // -1 = none, 0-2 = which key is held (1/2/3)
    bool reward_taken;            // true = confirmed, ready to exit
    float result_timer;           // Timer for final pause before close
    FlexBox_t* card_layout;       // Horizontal layout for 3 cards
    FlexBox_t* info_layout;       // Vertical layout for header content
    FlexBox_t* list_layout;       // Vertical layout for tag list items

    // Animation state
    RewardAnimStage_t anim_stage; // Current animation stage
    float fade_out_alpha;         // 1.0 → 0.0 for unselected elements
    float card_scale;             // 1.0 → 1.5 for selected card
    float tag_badge_alpha;        // 0.0 → 1.0 for tag badge
} RewardModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateRewardModal - Initialize reward modal
 *
 * @return RewardModal_t* - Heap-allocated modal, or NULL on failure
 *
 * Modal starts hidden (is_visible = false).
 * Call ShowRewardModal() to display it.
 */
RewardModal_t* CreateRewardModal(void);

/**
 * DestroyRewardModal - Free reward modal
 *
 * @param modal - Pointer to modal pointer (double pointer for nulling)
 */
void DestroyRewardModal(RewardModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowRewardModal - Display the modal and generate reward
 *
 * @param modal - Modal to show
 *
 * Picks 3 random untagged cards, assigns each a random tag.
 * If fewer than 3 untagged cards exist, returns false (don't show).
 *
 * @return bool - true if shown, false if skipped (not enough cards)
 */
bool ShowRewardModal(RewardModal_t* modal);

/**
 * HideRewardModal - Hide the modal
 *
 * @param modal - Modal to hide
 */
void HideRewardModal(RewardModal_t* modal);

/**
 * IsRewardModalVisible - Check if modal is visible
 *
 * @param modal - Modal to check
 * @return bool - true if visible
 */
bool IsRewardModalVisible(const RewardModal_t* modal);

// ============================================================================
// INPUT & UPDATE
// ============================================================================

/**
 * HandleRewardModalInput - Process input for modal
 *
 * @param modal - Modal to update
 * @param dt - Delta time
 * @return bool - true if modal wants to close (reward taken/skipped)
 *
 * Handles tag selection, Skip All button, and result timer.
 * Returns true when ready to transition to next state.
 */
bool HandleRewardModalInput(RewardModal_t* modal, float dt);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderRewardModal - Draw the modal overlay
 *
 * @param modal - Modal to render
 *
 * Draws dark overlay, target card, tag buttons, and result.
 * Only renders if is_visible = true.
 */
void RenderRewardModal(const RewardModal_t* modal);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * GetSelectedTag - Get the tag that was applied
 *
 * @param modal - Modal to query
 * @return CardTag_t - Selected tag, or CARD_TAG_MAX if none/skipped
 */
CardTag_t GetSelectedTag(const RewardModal_t* modal);

/**
 * GetTargetCardID - Get the card that received the tag
 *
 * @param modal - Modal to query
 * @return int - Card ID (0-51), or -1 if none/skipped
 */
int GetTargetCardID(const RewardModal_t* modal);

#endif // REWARD_MODAL_H
