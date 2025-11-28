#ifndef VICTORY_OVERLAY_H
#define VICTORY_OVERLAY_H

#include "common.h"
#include "structs.h"

// ============================================================================
// VICTORY OVERLAY COMPONENT
// ============================================================================

/**
 * VictoryOverlay_t - Fullscreen overlay displayed when enemy is defeated
 *
 * Constitutional pattern (ADR-008):
 * - Component manages victory presentation (dark overlay, title, defeat message)
 * - Follows modal lifecycle: Create/Show/Render/Destroy
 * - Integrates with resultScreen.c for chip effects ("Win", "Cleansed!")
 *
 * Responsibilities:
 * - Visual presentation (VICTORY! title, "You defeated X!" message)
 * - Enemy name formatting and display
 * - Fullscreen dark overlay (200 alpha black rect)
 *
 * NOT responsible for:
 * - Game state transitions (handled by state.c)
 * - Result screen effects (handled by resultScreen.c)
 * - Audio playback (handled by state.c)
 *
 * Lifecycle:
 * 1. CreateVictoryOverlay() in scene initialization
 * 2. ShowVictoryOverlay() when entering STATE_COMBAT_VICTORY
 * 3. RenderVictoryOverlay() each frame while visible
 * 4. DestroyVictoryOverlay() in scene cleanup
 */
typedef struct VictoryOverlay {
    bool visible;            // Show/hide state
    dString_t* enemy_name;   // Defeated enemy name (cached to avoid re-lookup)
} VictoryOverlay_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateVictoryOverlay - Initialize victory overlay component
 *
 * Allocates overlay struct and initializes enemy_name string.
 * Does NOT make overlay visible (call ShowVictoryOverlay for that).
 *
 * @return Pointer to new VictoryOverlay_t
 */
VictoryOverlay_t* CreateVictoryOverlay(void);

/**
 * DestroyVictoryOverlay - Free victory overlay component
 *
 * Follows ADR-015 double-pointer destructor pattern.
 * Frees enemy_name string and overlay struct, nulls pointer.
 *
 * @param overlay - Pointer to overlay pointer (nulled after destroy)
 */
void DestroyVictoryOverlay(VictoryOverlay_t** overlay);

// ============================================================================
// DISPLAY
// ============================================================================

/**
 * ShowVictoryOverlay - Display victory overlay for defeated enemy
 *
 * Caches enemy name and makes overlay visible.
 * Called when entering STATE_COMBAT_VICTORY.
 *
 * @param overlay - Overlay to show
 * @param defeated_enemy - Enemy that was defeated (for name display)
 */
void ShowVictoryOverlay(VictoryOverlay_t* overlay, Enemy_t* defeated_enemy);

/**
 * HideVictoryOverlay - Hide victory overlay
 *
 * Sets visible=false. Called when transitioning to next state.
 *
 * @param overlay - Overlay to hide
 */
void HideVictoryOverlay(VictoryOverlay_t* overlay);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderVictoryOverlay - Draw victory celebration overlay
 *
 * Renders (if visible):
 * - "VICTORY!" title (gold, centered horizontally, 128px above center)
 * - "You defeated [Enemy]!" message (off-white, centered)
 *
 * Note: No dark overlay - leaves screen visible for center content.
 * Does NOT render result screen effects (that's resultScreen.c's job).
 *
 * @param overlay - Overlay to render
 */
void RenderVictoryOverlay(const VictoryOverlay_t* overlay);

// ============================================================================
// STATE
// ============================================================================

/**
 * IsVictoryOverlayVisible - Check if overlay is currently visible
 *
 * @param overlay - Overlay to check
 * @return true if visible, false otherwise
 */
bool IsVictoryOverlayVisible(const VictoryOverlay_t* overlay);

#endif // VICTORY_OVERLAY_H
