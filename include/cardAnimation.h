/*
 * Card Animation System
 * Constitutional pattern: Fixed-pool transition states separate from value-type cards
 *
 * Design philosophy:
 * - Cards are value types (copied in dArray_t), so we can't tween their positions directly
 * - Instead, we create ephemeral "transition" objects that track animations
 * - Transitions reference cards by (hand pointer, index), not by card pointer
 * - Fixed pool of transitions (no malloc during gameplay)
 * - Integrates with existing tween system for smooth interpolation
 */

#ifndef CARD_ANIMATION_H
#define CARD_ANIMATION_H

#include "structs.h"
#include "tween/tween.h"
#include <stdbool.h>

// ============================================================================
// CONFIGURATION
// ============================================================================

#define MAX_CARD_TRANSITIONS 16  // Maximum simultaneous card animations

// ============================================================================
// CARD TRANSITION STRUCTURE
// ============================================================================

/**
 * CardTransition_t - Tracks animation state for a card being dealt/discarded
 *
 * Constitutional pattern: Separate from Card_t to avoid value-copy issues
 * References cards by hand + index, not by pointer (safe with value types)
 */
typedef struct CardTransition {
    bool active;                // Is this slot in use?

    // Card identification (safe with value-type cards)
    Hand_t* owner_hand;         // Which hand contains the card
    size_t card_index;          // Index in hand->cards array

    // Animation state (tweened by TweenManager)
    float current_x;            // Current X position (tweened)
    float current_y;            // Current Y position (tweened)
    float target_x;             // Final X position
    float target_y;             // Final Y position

    // Animation effects
    bool flip_face_up;          // Should card flip face-up during animation?
    float flip_progress;        // 0.0-1.0, triggers flip at 0.5

    // Lifetime tracking
    float duration;             // Total animation duration
    float elapsed;              // Time elapsed (for flip timing)
} CardTransition_t;

// ============================================================================
// TRANSITION MANAGER (Fixed Pool)
// ============================================================================

/**
 * CardTransitionManager_t - Manages fixed pool of card transitions
 *
 * Constitutional pattern: Stack-allocated, fixed-size array (no malloc)
 */
typedef struct CardTransitionManager {
    CardTransition_t transitions[MAX_CARD_TRANSITIONS];  // Fixed pool
    int active_count;                                     // Number of active transitions
} CardTransitionManager_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * InitCardTransitionManager - Initialize transition manager
 *
 * @param manager - Pointer to stack-allocated manager
 *
 * Example:
 *   CardTransitionManager_t card_anim_mgr;
 *   InitCardTransitionManager(&card_anim_mgr);
 */
void InitCardTransitionManager(CardTransitionManager_t* manager);

/**
 * CleanupCardTransitionManager - Clear all active transitions
 *
 * @param manager - Pointer to transition manager
 */
void CleanupCardTransitionManager(CardTransitionManager_t* manager);

// ============================================================================
// TRANSITION CREATION
// ============================================================================

/**
 * StartCardDealAnimation - Animate card from deck to hand position
 *
 * @param manager - Transition manager
 * @param tween_manager - Tween manager (for position interpolation)
 * @param hand - Hand containing the card
 * @param card_index - Index of card in hand->cards
 * @param start_x - Starting X position (deck position)
 * @param start_y - Starting Y position (deck position)
 * @param target_x - Final X position (hand position)
 * @param target_y - Final Y position (hand position)
 * @param duration - Animation duration in seconds
 * @param flip_face_up - Should card flip face-up halfway through?
 * @return true if animation started, false if pool full
 *
 * Example:
 *   // Card just dealt to player, now animate it
 *   StartCardDealAnimation(&card_anim_mgr, &tween_mgr, &player->hand,
 *                          hand->cards->count - 1, 850, 300, final_x, final_y,
 *                          0.4f, true);
 */
bool StartCardDealAnimation(CardTransitionManager_t* manager,
                             TweenManager_t* tween_manager,
                             Hand_t* hand,
                             size_t card_index,
                             float start_x, float start_y,
                             float target_x, float target_y,
                             float duration,
                             bool flip_face_up);

/**
 * StartCardDiscardAnimation - Animate card from hand to discard pile
 *
 * @param manager - Transition manager
 * @param tween_manager - Tween manager
 * @param hand - Hand containing the card
 * @param card_index - Index of card in hand->cards
 * @param start_x - Starting X position (hand position)
 * @param start_y - Starting Y position (hand position)
 * @param discard_x - Discard pile X position
 * @param discard_y - Discard pile Y position
 * @param duration - Animation duration in seconds
 * @return true if animation started, false if pool full
 */
bool StartCardDiscardAnimation(CardTransitionManager_t* manager,
                                TweenManager_t* tween_manager,
                                Hand_t* hand,
                                size_t card_index,
                                float start_x, float start_y,
                                float discard_x, float discard_y,
                                float duration);

// ============================================================================
// UPDATE
// ============================================================================

/**
 * UpdateCardTransitions - Update all active card transitions
 *
 * @param manager - Transition manager
 * @param dt - Delta time in seconds
 *
 * Handles flip timing and cleanup of completed transitions.
 * Call this every frame AFTER UpdateTweens().
 *
 * Example (in game loop):
 *   UpdateTweens(&tween_mgr, dt);
 *   UpdateCardTransitions(&card_anim_mgr, dt);
 */
void UpdateCardTransitions(CardTransitionManager_t* manager, float dt);

// ============================================================================
// QUERY
// ============================================================================

/**
 * GetCardTransition - Get active transition for a card
 *
 * @param manager - Transition manager
 * @param hand - Hand to search
 * @param card_index - Card index in hand
 * @return Pointer to transition if active, NULL otherwise
 *
 * Example:
 *   CardTransition_t* trans = GetCardTransition(&mgr, &player->hand, 2);
 *   if (trans) {
 *       // Card at index 2 is currently animating
 *       int x = (int)trans->current_x;
 *       int y = (int)trans->current_y;
 *   }
 */
CardTransition_t* GetCardTransition(CardTransitionManager_t* manager,
                                     const Hand_t* hand,
                                     size_t card_index);

/**
 * IsHandAnimating - Check if any cards in hand are animating
 *
 * @param manager - Transition manager
 * @param hand - Hand to check
 * @return true if at least one card in hand has active transition
 */
bool IsHandAnimating(CardTransitionManager_t* manager, const Hand_t* hand);

/**
 * GetActiveTransitionCount - Get number of active transitions
 *
 * @param manager - Transition manager
 * @return Number of active card transitions
 */
int GetActiveTransitionCount(const CardTransitionManager_t* manager);

/**
 * StopTransitionsForHand - Stop all animations for cards in a hand
 *
 * @param manager - Transition manager
 * @param hand - Hand whose transitions should be stopped
 * @return Number of transitions stopped
 *
 * Use when clearing a hand mid-animation (round end, player busts, etc.)
 */
int StopTransitionsForHand(CardTransitionManager_t* manager, const Hand_t* hand);

#endif // CARD_ANIMATION_H
