/*
 * Card Animation System Implementation
 */

#include "../include/cardAnimation.h"
#include "../include/hand.h"
#include <Daedalus.h>
#include <string.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

void InitCardTransitionManager(CardTransitionManager_t* manager) {
    if (!manager) return;

    memset(manager, 0, sizeof(CardTransitionManager_t));
    manager->active_count = 0;

    // Mark all slots as inactive
    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        manager->transitions[i].active = false;
    }
}

void CleanupCardTransitionManager(CardTransitionManager_t* manager) {
    if (!manager) return;

    // Mark all transitions as inactive
    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        manager->transitions[i].active = false;
    }
    manager->active_count = 0;
}

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * FindFreeTransitionSlot - Find an available transition slot
 *
 * @param manager - Transition manager
 * @return Index of free slot, or -1 if pool full
 */
static int FindFreeTransitionSlot(CardTransitionManager_t* manager) {
    if (!manager) return -1;

    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        if (!manager->transitions[i].active) {
            return i;
        }
    }

    return -1;  // Pool full
}

/**
 * OnTransitionComplete - Callback when position tween finishes
 *
 * @param user_data - Pointer to CardTransition_t
 */
static void OnTransitionComplete(void* user_data) {
    CardTransition_t* trans = (CardTransition_t*)user_data;
    if (trans) {
        trans->active = false;
    }
}

// ============================================================================
// TRANSITION CREATION
// ============================================================================

bool StartCardDealAnimation(CardTransitionManager_t* manager,
                             TweenManager_t* tween_manager,
                             Hand_t* hand,
                             size_t card_index,
                             float start_x, float start_y,
                             float target_x, float target_y,
                             float duration,
                             bool flip_face_up) {
    if (!manager || !tween_manager || !hand) {
        d_LogError("StartCardDealAnimation: NULL pointer passed");
        return false;
    }

    if (card_index >= hand->cards->count) {
        d_LogError("StartCardDealAnimation: Invalid card index");
        return false;
    }

    // Find free slot
    int slot = FindFreeTransitionSlot(manager);
    if (slot == -1) {
        d_LogError("StartCardDealAnimation: Transition pool full");
        return false;
    }

    // Initialize transition
    CardTransition_t* trans = &manager->transitions[slot];
    trans->active = true;
    trans->owner_hand = hand;
    trans->card_index = card_index;
    trans->current_x = start_x;
    trans->current_y = start_y;
    trans->target_x = target_x;
    trans->target_y = target_y;
    trans->flip_face_up = flip_face_up;
    trans->flip_progress = 0.0f;
    trans->duration = duration;
    trans->elapsed = 0.0f;

    manager->active_count++;

    // Tween X position (with completion callback to cleanup transition)
    TweenFloatWithCallback(tween_manager, &trans->current_x, target_x,
                            duration, TWEEN_EASE_OUT_CUBIC,
                            OnTransitionComplete, trans);

    // Tween Y position (no callback - X callback handles cleanup)
    TweenFloat(tween_manager, &trans->current_y, target_y,
               duration, TWEEN_EASE_OUT_CUBIC);

    return true;
}

bool StartCardDiscardAnimation(CardTransitionManager_t* manager,
                                TweenManager_t* tween_manager,
                                Hand_t* hand,
                                size_t card_index,
                                float start_x, float start_y,
                                float discard_x, float discard_y,
                                float duration) {
    if (!manager || !tween_manager || !hand) {
        d_LogError("StartCardDiscardAnimation: NULL pointer passed");
        return false;
    }

    if (card_index >= hand->cards->count) {
        d_LogError("StartCardDiscardAnimation: Invalid card index");
        return false;
    }

    // Find free slot
    int slot = FindFreeTransitionSlot(manager);
    if (slot == -1) {
        d_LogError("StartCardDiscardAnimation: Transition pool full");
        return false;
    }

    // Initialize transition
    CardTransition_t* trans = &manager->transitions[slot];
    trans->active = true;
    trans->owner_hand = hand;
    trans->card_index = card_index;
    trans->current_x = start_x;
    trans->current_y = start_y;
    trans->target_x = discard_x;
    trans->target_y = discard_y;
    trans->flip_face_up = false;  // Discard doesn't flip
    trans->flip_progress = 0.0f;
    trans->duration = duration;
    trans->elapsed = 0.0f;

    manager->active_count++;

    // Tween positions (faster, different easing for discard)
    TweenFloatWithCallback(tween_manager, &trans->current_x, discard_x,
                            duration, TWEEN_EASE_IN_QUAD,
                            OnTransitionComplete, trans);

    TweenFloat(tween_manager, &trans->current_y, discard_y,
               duration, TWEEN_EASE_IN_QUAD);

    return true;
}

// ============================================================================
// UPDATE
// ============================================================================

void UpdateCardTransitions(CardTransitionManager_t* manager, float dt) {
    if (!manager || dt <= 0.0f) return;

    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        CardTransition_t* trans = &manager->transitions[i];

        if (!trans->active) continue;

        // Update elapsed time
        trans->elapsed += dt;

        // Calculate flip progress (0.0 to 1.0)
        if (trans->duration > 0.0f) {
            trans->flip_progress = trans->elapsed / trans->duration;
            if (trans->flip_progress > 1.0f) {
                trans->flip_progress = 1.0f;
            }
        }

        // Flip card face-up at 50% progress
        if (trans->flip_face_up && trans->flip_progress >= 0.5f) {
            // Get card from hand and flip it
            if (trans->owner_hand && trans->card_index < trans->owner_hand->cards->count) {
                Card_t* card = (Card_t*)d_IndexDataFromArray(trans->owner_hand->cards, trans->card_index);
                if (card && !card->face_up) {
                    card->face_up = true;
                    // Note: Card texture should already be loaded
                }
            }
            // Only flip once
            trans->flip_face_up = false;
        }

        // Cleanup happens via tween completion callback (OnTransitionComplete)
        // If transition is still active but tween completed, decrement count
        if (!trans->active && manager->active_count > 0) {
            manager->active_count--;
        }
    }
}

// ============================================================================
// QUERY
// ============================================================================

CardTransition_t* GetCardTransition(CardTransitionManager_t* manager,
                                     const Hand_t* hand,
                                     size_t card_index) {
    if (!manager || !hand) return NULL;

    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        CardTransition_t* trans = &manager->transitions[i];

        if (trans->active &&
            trans->owner_hand == hand &&
            trans->card_index == card_index) {
            return trans;
        }
    }

    return NULL;
}

bool IsHandAnimating(CardTransitionManager_t* manager, const Hand_t* hand) {
    if (!manager || !hand) return false;

    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        const CardTransition_t* trans = &manager->transitions[i];

        if (trans->active && trans->owner_hand == hand) {
            return true;
        }
    }

    return false;
}

int GetActiveTransitionCount(const CardTransitionManager_t* manager) {
    if (!manager) return 0;
    return manager->active_count;
}

int StopTransitionsForHand(CardTransitionManager_t* manager, const Hand_t* hand) {
    if (!manager || !hand) return 0;

    int stopped = 0;
    for (int i = 0; i < MAX_CARD_TRANSITIONS; i++) {
        CardTransition_t* trans = &manager->transitions[i];

        if (trans->active && trans->owner_hand == hand) {
            trans->active = false;
            stopped++;
        }
    }

    if (stopped > 0) {
        manager->active_count -= stopped;
    }

    return stopped;
}
