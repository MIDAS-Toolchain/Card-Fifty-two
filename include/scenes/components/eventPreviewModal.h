#ifndef EVENT_PREVIEW_MODAL_H
#define EVENT_PREVIEW_MODAL_H

#include "common.h"
#include "structs.h"
#include "button.h"

// ============================================================================
// EVENT PREVIEW MODAL COMPONENT
// ============================================================================

/**
 * EventPreviewModal_t - Modal for event preview with reroll system
 *
 * Flow:
 * 1. Generate random event (CreatePostCombatEvent)
 * 2. Show title with 3-second countdown timer
 * 3. Player can reroll (costs chips, doubles each use: 50→100→200→400)
 * 4. Player can continue (skip timer, proceed immediately)
 * 5. Auto-proceed when timer hits 0.0
 *
 * Constitutional pattern:
 * - Modal component (matches design palette)
 * - Renders on top of blackjack scene
 * - Uses tween system for fade animations
 * - Static char buffer for event title (no dString_t needed)
 */
typedef struct EventPreviewModal {
    bool is_visible;              // true = shown, false = hidden
    char event_title[128];        // Event title text (static buffer)
    float title_alpha;            // 0.0 → 1.0 fade animation
    Button_t* reroll_button;      // "Reroll (50 chips)" button
    Button_t* continue_button;    // "Continue" button
} EventPreviewModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateEventPreviewModal - Initialize event preview modal
 *
 * @param game - Game context (for reroll cost tracking)
 * @param event_title - Title of event to preview
 * @return EventPreviewModal_t* - Heap-allocated modal, or NULL on failure
 *
 * Modal starts hidden (is_visible = false).
 * Call ShowEventPreviewModal() to display it.
 */
EventPreviewModal_t* CreateEventPreviewModal(GameContext_t* game, const char* event_title);

/**
 * DestroyEventPreviewModal - Free event preview modal
 *
 * @param modal - Pointer to modal pointer (double pointer for nulling)
 */
void DestroyEventPreviewModal(EventPreviewModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowEventPreviewModal - Display the modal
 *
 * @param modal - Modal to show
 */
void ShowEventPreviewModal(EventPreviewModal_t* modal);

/**
 * HideEventPreviewModal - Hide the modal
 *
 * @param modal - Modal to hide
 */
void HideEventPreviewModal(EventPreviewModal_t* modal);

/**
 * IsEventPreviewModalVisible - Check if modal is visible
 *
 * @param modal - Modal to check
 * @return bool - true if visible
 */
bool IsEventPreviewModalVisible(const EventPreviewModal_t* modal);

// ============================================================================
// INPUT & UPDATE
// ============================================================================

/**
 * UpdateEventPreviewModal - Update fade animations
 *
 * @param modal - Modal to update
 * @param dt - Delta time
 *
 * Handles title fade-in animation.
 */
void UpdateEventPreviewModal(EventPreviewModal_t* modal, float dt);

/**
 * UpdateEventPreviewModalCost - Update reroll button label with current cost
 *
 * @param modal - Modal to update
 * @param current_cost - Current reroll cost (50, 100, 200, etc.)
 *
 * Updates the reroll button text to show the current cost.
 * Call this after doubling the cost.
 */
void UpdateEventPreviewModalCost(EventPreviewModal_t* modal, int current_cost);

/**
 * UpdateEventPreviewContent - Update modal content for new event (avoids recreate)
 *
 * @param modal - Modal to update
 * @param new_title - New event title
 * @param new_cost - New reroll cost
 *
 * Updates the modal's event_title and reroll button cost without destroying/recreating.
 * Use this on reroll to avoid allocation churn.
 * Resets title fade-in animation.
 */
void UpdateEventPreviewContent(EventPreviewModal_t* modal, const char* new_title, int new_cost);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderEventPreviewModal - Draw the modal overlay
 *
 * @param modal - Modal to render
 * @param game - Game context (for timer display)
 *
 * Draws dark overlay, centered title with fade, timer bar, and buttons.
 * Only renders if is_visible = true.
 */
void RenderEventPreviewModal(const EventPreviewModal_t* modal, const GameContext_t* game);

#endif // EVENT_PREVIEW_MODAL_H
