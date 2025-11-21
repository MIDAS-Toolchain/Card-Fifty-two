#ifndef INTRO_NARRATIVE_MODAL_H
#define INTRO_NARRATIVE_MODAL_H

#include "common.h"
#include "structs.h"
#include "button.h"

// ============================================================================
// INTRO NARRATIVE MODAL COMPONENT
// ============================================================================

/**
 * IntroNarrativeModal_t - Modal for tutorial intro story
 *
 * Design:
 * - Displays narrative text with character portrait
 * - Single "Continue" button (no choices)
 * - Similar visual style to EventModal for consistency
 * - Used only at the start of the tutorial
 *
 * Flow:
 * 1. Show modal with story about the Degenerate confronting The Didact
 * 2. Player reads narrative
 * 3. Player clicks Continue → transition to first combat
 *
 * Constitutional pattern:
 * - Modal component (matches EventModal design palette)
 * - Renders on top of blackjack scene
 * - Uses button system for input
 * - Static char buffers for text storage
 */
#define MAX_NARRATIVE_LINES 16
#define MAX_LINE_TEXT_LENGTH 2048  // Increased to hold full narrative text

typedef struct IntroNarrativeModal {
    bool is_visible;              // true = shown, false = hidden
    char title[128];              // Story title (e.g., "Prologue: The Degenerate")
    char narrative_lines[MAX_NARRATIVE_LINES][MAX_LINE_TEXT_LENGTH];  // Story split into lines
    int line_count;               // Number of lines in narrative
    int current_block;            // Current narrative block being displayed (0-indexed)
    int total_blocks;             // Total number of narrative blocks
    float line_fade_alpha[MAX_NARRATIVE_LINES];  // Fade-in alpha per line
    char portrait_path[128];      // Path to character portrait PNG
    SDL_Texture* portrait;        // Loaded portrait texture (or NULL)
    Button_t* continue_button;    // "Continue" button
    float fade_in_alpha;          // 0.0 → 1.0 global fade animation
    FlexBox_t* layout;            // FlexBox for responsive layout
} IntroNarrativeModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateIntroNarrativeModal - Initialize intro narrative modal
 *
 * @return IntroNarrativeModal_t* - Heap-allocated modal, or NULL on failure
 *
 * Modal starts hidden (is_visible = false).
 * Call ShowIntroNarrativeModal() to display it with content.
 */
IntroNarrativeModal_t* CreateIntroNarrativeModal(void);

/**
 * DestroyIntroNarrativeModal - Free intro narrative modal
 *
 * @param modal - Pointer to modal pointer (double pointer for nulling)
 */
void DestroyIntroNarrativeModal(IntroNarrativeModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowIntroNarrativeModal - Display the modal with story content
 *
 * @param modal - Modal to show
 * @param title - Story title (e.g., "The Casino")
 * @param narrative_blocks - Array of narrative text blocks (NULL-terminated)
 * @param block_count - Number of narrative blocks
 * @param portrait_path - Path to character portrait (or NULL for no portrait)
 *
 * Sets is_visible = true and starts fade-in animation.
 * Loads portrait texture if path is provided.
 * Displays all narrative blocks vertically with dynamic spacing.
 */
void ShowIntroNarrativeModal(IntroNarrativeModal_t* modal,
                             const char* title,
                             const char** narrative_blocks,
                             int block_count,
                             const char* portrait_path);

/**
 * HideIntroNarrativeModal - Hide the modal
 *
 * @param modal - Modal to hide
 *
 * Sets is_visible = false.
 */
void HideIntroNarrativeModal(IntroNarrativeModal_t* modal);

/**
 * IsIntroNarrativeModalVisible - Check if modal is visible
 *
 * @param modal - Modal to check
 * @return bool - true if visible
 */
bool IsIntroNarrativeModalVisible(const IntroNarrativeModal_t* modal);

// ============================================================================
// INPUT & UPDATE
// ============================================================================

/**
 * HandleIntroNarrativeModalInput - Process input for modal
 *
 * @param modal - Modal to update
 * @param dt - Delta time
 * @return bool - true if Continue button was clicked (ready to close)
 *
 * Handles:
 * - Continue button click
 * - Fade-in animation updates
 *
 * When Continue is clicked, returns true.
 * Caller should hide modal and proceed to first combat.
 */
bool HandleIntroNarrativeModalInput(IntroNarrativeModal_t* modal, float dt);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderIntroNarrativeModal - Draw the modal overlay
 *
 * @param modal - Modal to render
 *
 * Draws (if is_visible = true):
 * - Full-screen dark overlay
 * - Centered panel with header/body separation
 * - Story title (gold text in header)
 * - Character portrait (left side, 1/3 width)
 * - Narrative text (right side, 2/3 width, wrapped)
 * - Continue button (centered at bottom)
 *
 * Uses same color palette as EventModal for consistency.
 */
void RenderIntroNarrativeModal(const IntroNarrativeModal_t* modal);

#endif // INTRO_NARRATIVE_MODAL_H
