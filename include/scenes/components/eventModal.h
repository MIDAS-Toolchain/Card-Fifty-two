#ifndef EVENT_MODAL_H
#define EVENT_MODAL_H

#include "common.h"
#include "structs.h"
#include "event.h"

// ============================================================================
// EVENT MODAL COMPONENT
// ============================================================================

// Layout constants (matching rewardModal design pattern)
#define EVENT_MODAL_WIDTH          900
#define EVENT_MODAL_HEIGHT         700
#define EVENT_MODAL_HEADER_HEIGHT  50
#define EVENT_MODAL_PADDING        30
#define EVENT_CHOICE_HEIGHT        90
#define EVENT_CHOICE_SPACING       10

/**
 * EventModal_t - Modal for displaying event encounters (overlays on blackjack scene)
 *
 * Design Pattern: Matches RewardModal architecture
 * - Full-screen dark overlay
 * - Centered panel with header/body separation
 * - FlexBox layout for choice positioning
 * - Hover feedback on choice items
 * - Fade-in animation
 *
 * Constitutional pattern:
 * - Modal component (matches color palette from rewardModal)
 * - Renders on top of blackjack scene
 * - References EventEncounter_t (does NOT own it)
 * - Uses FlexBox for automatic layout
 */
typedef struct EventModal {
    bool is_visible;                    // true = shown, false = hidden
    EventEncounter_t* current_event;    // Reference to event (NOT owned by modal)

    // Layout system
    FlexBox_t* header_layout;           // Vertical layout for header content
    FlexBox_t* choice_layout;           // Vertical layout for choice buttons

    // Interaction state
    int hovered_choice;                 // -1 = none, 0-N = hovered choice index
    int selected_choice;                // -1 = none, 0-N = confirmed selection

    // Animation state
    float fade_in_alpha;                // 0.0 → 1.0 on show
} EventModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateEventModal - Initialize event modal
 *
 * @return EventModal_t* - Heap-allocated modal, or NULL on failure
 *
 * Modal starts hidden (is_visible = false).
 * Call ShowEventModal() to display it with an event.
 */
EventModal_t* CreateEventModal(void);

/**
 * DestroyEventModal - Free event modal
 *
 * @param modal - Pointer to modal pointer (double pointer for nulling)
 *
 * Does NOT destroy the event (modal doesn't own it).
 */
void DestroyEventModal(EventModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowEventModal - Display the modal with an event
 *
 * @param modal - Modal to show
 * @param event - Event to display (modal references this, doesn't own it)
 *
 * Sets is_visible = true and starts fade-in animation.
 * Modal will render event title, description, and choices.
 */
void ShowEventModal(EventModal_t* modal, EventEncounter_t* event);

/**
 * HideEventModal - Hide the modal
 *
 * @param modal - Modal to hide
 *
 * Sets is_visible = false. Does NOT destroy the event.
 */
void HideEventModal(EventModal_t* modal);

/**
 * IsEventModalVisible - Check if modal is visible
 *
 * @param modal - Modal to check
 * @return bool - true if visible
 */
bool IsEventModalVisible(const EventModal_t* modal);

// ============================================================================
// INPUT & UPDATE
// ============================================================================

/**
 * HandleEventModalInput - Process input for modal
 *
 * @param modal - Modal to update
 * @param player - Player to check requirements against (for locked choice detection)
 * @param dt - Delta time
 * @return bool - true if choice was selected (ready to close)
 *
 * Handles:
 * - Mouse hover over choices (skips locked choices)
 * - Mouse click to select choice (blocks locked choices)
 * - Keyboard hotkeys (blocks locked choices)
 * - Fade-in animation updates
 *
 * When a choice is selected, returns true and sets modal->selected_choice.
 * Caller should apply consequences and hide modal.
 */
bool HandleEventModalInput(EventModal_t* modal, const struct Player* player, float dt);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderEventModal - Draw the modal overlay
 *
 * @param modal - Modal to render
 * @param player - Player to check requirements against (for locked choice detection)
 *
 * Draws (if is_visible = true):
 * - Full-screen dark overlay
 * - Centered panel with header/body separation
 * - Event title (gold text in header)
 * - Event description (wrapped text in body)
 * - Choice list (vertical items with hover highlight)
 * - Locked choices (grayed out with lock icon instead of number)
 * - Requirement tooltips (on hover over locked choices)
 * - Chips/sanity deltas for each choice (color-coded)
 *
 * Uses same color palette as RewardModal for consistency.
 */
void RenderEventModal(const EventModal_t* modal, const struct Player* player);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * GetSelectedChoiceIndex - Get the index of selected choice
 *
 * @param modal - Modal to query
 * @return int - Selected choice index, or -1 if none selected
 */
int GetSelectedChoiceIndex(const EventModal_t* modal);

#endif // EVENT_MODAL_H
