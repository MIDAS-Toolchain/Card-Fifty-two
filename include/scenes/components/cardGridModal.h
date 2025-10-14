/*
 * Card Grid Modal Component
 * Reusable modal for displaying a scrollable grid of cards
 * Used for viewing draw pile (shuffled) and discard pile (ordered)
 */

#ifndef CARD_GRID_MODAL_H
#define CARD_GRID_MODAL_H

#include "../../common.h"
#include "../../deck.h"

// Layout constants
#define MODAL_WIDTH              600
#define MODAL_HEIGHT             500
#define MODAL_HEADER_HEIGHT      50
#define CARD_GRID_COLS           5
#define CARD_GRID_PADDING        10
#define CARD_GRID_CARD_WIDTH     80   // Scaled down from 100
#define CARD_GRID_CARD_HEIGHT    112  // Scaled down from 140
#define CARD_GRID_SPACING        10   // Space between cards
#define SCROLLBAR_WIDTH          20
#define SCROLLBAR_MIN_HANDLE_HEIGHT  40

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
