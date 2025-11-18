/*
 * Card Grid Modal Component
 * Reusable modal for displaying a scrollable grid of cards
 * Used for viewing draw pile (shuffled) and discard pile (ordered)
 */

#ifndef CARD_GRID_MODAL_H
#define CARD_GRID_MODAL_H

#include "../../common.h"
#include "../../deck.h"
#include "cardTooltipModal.h"

// Layout constants (matching reward modal sizing)
#define MODAL_WIDTH              900  // Match reward modal width
#define MODAL_HEIGHT             700  // Match reward modal height
#define MODAL_HEADER_HEIGHT      50
#define CARD_GRID_COLS           6    // 6 columns (back to original)
#define CARD_GRID_PADDING        15
#define CARD_GRID_CARD_WIDTH     75   // Card width in grid
#define CARD_GRID_CARD_HEIGHT    105  // Card height in grid
#define CARD_GRID_SPACING        10   // Space between cards
#define CARD_GRID_TAG_BADGE_W    80   // Tag badge width (smaller than reward modal's 125px)
#define CARD_GRID_TAG_BADGE_H    25   // Tag badge height (smaller than reward modal's 35px)
#define SCROLLBAR_WIDTH          20
#define SCROLLBAR_MIN_HANDLE_HEIGHT  40
#define HOVER_CARD_SCALE         1.5f // Scale factor for hovered card (like hand)

typedef struct CardGridModal {
    char title[256];                // Static modal title (e.g., "Draw Pile (Randomized)")
    dArray_t* cards;                // Pointer to card array (NOT owned - points to deck->cards or deck->discard_pile)
    bool is_visible;                // Whether modal is displayed
    bool should_shuffle_display;    // If true, display cards in random order (for draw pile)
    int* shuffled_indices;          // Array of shuffled indices (NULL if !should_shuffle_display)
    int scroll_offset;              // Vertical scroll offset in pixels
    int max_scroll;                 // Maximum scroll value
    bool dragging_scrollbar;        // Whether user is dragging scrollbar
    int drag_start_y;               // Mouse Y when drag started
    int drag_start_scroll;          // Scroll offset when drag started
    int hovered_card_index;         // -1 = none, 0+ = which card is hovered (enlarges on hover)
    CardTooltipModal_t* tooltip;    // Owned tooltip for showing card info on hover
} CardGridModal_t;

// Lifecycle
CardGridModal_t* CreateCardGridModal(const char* title, dArray_t* cards, bool should_shuffle_display);
void DestroyCardGridModal(CardGridModal_t** modal);

// Visibility
void ShowCardGridModal(CardGridModal_t* modal);
void HideCardGridModal(CardGridModal_t* modal);

// Input handling
bool HandleCardGridModalInput(CardGridModal_t* modal);  // Returns true if modal should close

// Rendering
void RenderCardGridModal(CardGridModal_t* modal);

#endif // CARD_GRID_MODAL_H
