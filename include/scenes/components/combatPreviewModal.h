#ifndef COMBAT_PREVIEW_MODAL_H
#define COMBAT_PREVIEW_MODAL_H

#include "common.h"
#include "structs.h"
#include "button.h"

// ============================================================================
// COMBAT PREVIEW MODAL COMPONENT
// ============================================================================

/**
 * CombatPreviewModal_t - Modal for combat preview (elite/boss encounters)
 *
 * Flow:
 * 1. Get next encounter (elite/boss)
 * 2. Show enemy name + type with 3-second countdown timer
 * 3. Reroll button shown but DISABLED (grayed out, "Reroll Unavailable")
 * 4. Player can continue (skip timer, proceed immediately)
 * 5. Auto-proceed when timer hits 0.0
 *
 * Differences from EventPreviewModal:
 * - No reroll functionality (button disabled)
 * - Shows "{Type} - {Name}" format (e.g., "Elite Combat - The Daemon")
 * - Only for ELITE and BOSS encounter types (NORMAL spawns directly)
 *
 * Constitutional pattern:
 * - Modal component (matches design palette)
 * - Renders on top of blackjack scene
 * - Uses tween system for fade animations
 * - Static char buffers (no dString_t needed)
 */
typedef struct CombatPreviewModal {
    bool is_visible;              // true = shown, false = hidden
    char enemy_name[64];          // Enemy name (e.g., "The Daemon")
    char encounter_type[32];      // "Elite Combat", "Boss Fight", "Normal Combat"
    float title_alpha;            // 0.0 → 1.0 fade animation
    Button_t* continue_button;    // "Continue" button (active)
} CombatPreviewModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateCombatPreviewModal - Initialize combat preview modal
 *
 * @param game - Game context (for timer tracking)
 * @param enemy_name - Name of enemy to preview
 * @param encounter_type - "Elite Combat", "Boss Fight", etc.
 * @return CombatPreviewModal_t* - Heap-allocated modal, or NULL on failure
 *
 * Modal starts hidden (is_visible = false).
 * Call ShowCombatPreviewModal() to display it.
 */
CombatPreviewModal_t* CreateCombatPreviewModal(GameContext_t* game,
                                                const char* enemy_name,
                                                const char* encounter_type);

/**
 * DestroyCombatPreviewModal - Free combat preview modal
 *
 * @param modal - Pointer to modal pointer (double pointer for nulling)
 */
void DestroyCombatPreviewModal(CombatPreviewModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowCombatPreviewModal - Display the modal
 *
 * @param modal - Modal to show
 */
void ShowCombatPreviewModal(CombatPreviewModal_t* modal);

/**
 * HideCombatPreviewModal - Hide the modal
 *
 * @param modal - Modal to hide
 */
void HideCombatPreviewModal(CombatPreviewModal_t* modal);

/**
 * IsCombatPreviewModalVisible - Check if modal is visible
 *
 * @param modal - Modal to check
 * @return bool - true if visible
 */
bool IsCombatPreviewModalVisible(const CombatPreviewModal_t* modal);

// ============================================================================
// INPUT & UPDATE
// ============================================================================

/**
 * UpdateCombatPreviewModal - Update fade animations
 *
 * @param modal - Modal to update
 * @param dt - Delta time
 *
 * Handles title fade-in animation.
 */
void UpdateCombatPreviewModal(CombatPreviewModal_t* modal, float dt);

/**
 * UpdateCombatPreviewContent - Update modal content for new combat
 *
 * @param modal - Modal to update
 * @param enemy_name - New enemy name
 * @param encounter_type - New encounter type
 *
 * Updates the modal's enemy_name and encounter_type without destroying/recreating.
 * Resets title fade-in animation.
 */
void UpdateCombatPreviewContent(CombatPreviewModal_t* modal,
                                 const char* enemy_name,
                                 const char* encounter_type);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderCombatPreviewModal - Draw the modal overlay
 *
 * @param modal - Modal to render
 * @param game - Game context (for timer display)
 *
 * Draws dark overlay, centered title with fade, timer bar, and buttons.
 * Only renders if is_visible = true.
 */
void RenderCombatPreviewModal(const CombatPreviewModal_t* modal, const GameContext_t* game);

#endif // COMBAT_PREVIEW_MODAL_H
