#ifndef GAME_OVER_OVERLAY_H
#define GAME_OVER_OVERLAY_H

#include "common.h"
#include "structs.h"

// ============================================================================
// GAME OVER OVERLAY COMPONENT
// ============================================================================

/**
 * Animation stages for game-over sequence (matches RewardModal pattern)
 */
typedef enum {
    GAME_OVER_ANIM_FADE_IN_OVERLAY,  // 0.5s: Overlay 0→220 alpha
    GAME_OVER_ANIM_FADE_IN_TITLE,    // 0.4s: "DEFEAT" title fade-in
    GAME_OVER_ANIM_FADE_IN_STATS,    // 0.5s: Stats block fade-in
    GAME_OVER_ANIM_FADE_IN_PROMPT,   // 0.3s: "Press SPACE" fade-in
    GAME_OVER_ANIM_COMPLETE          // Flashing prompt, awaiting input
} GameOverAnimStage_t;

/**
 * GameOverOverlay_t - Fullscreen overlay for defeat state
 *
 * Constitutional pattern (matches VictoryOverlay + RewardModal):
 * - Multi-stage animation (4 stages, ~2.2s total)
 * - Displays run statistics from Stats_GetCurrent()
 * - Flashing "Press SPACE" prompt after animation
 * - Uses palette colors (#cf573c title, #a8b5b2 text)
 *
 * Lifecycle:
 * 1. CreateGameOverOverlay() in scene initialization
 * 2. ShowGameOverOverlay() when entering STATE_GAME_OVER
 * 3. UpdateGameOverOverlay() each frame (advances animation)
 * 4. RenderGameOverOverlay() each frame while visible
 * 5. DestroyGameOverOverlay() in scene cleanup
 */
typedef struct GameOverOverlay {
    bool visible;

    // Animation state
    GameOverAnimStage_t anim_stage;
    float overlay_alpha;        // 0.0 → 220.0 (overlay fade-in)
    float title_alpha;          // 0.0 → 1.0 (title fade-in)
    float stats_alpha;          // 0.0 → 1.0 (stats fade-in)
    float prompt_alpha;         // 0.0 → 1.0 (prompt fade-in)
    float prompt_flash_timer;   // For flashing effect (0→1→0 loop)

    // Cached stats (snapshot at game-over)
    uint64_t final_damage;
    uint64_t enemies_defeated;
    uint64_t hands_won;
    uint64_t hands_total;
    float win_rate;
} GameOverOverlay_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateGameOverOverlay - Initialize game-over overlay component
 *
 * Allocates overlay struct and initializes animation state.
 * Does NOT make overlay visible (call ShowGameOverOverlay for that).
 *
 * @return Pointer to new GameOverOverlay_t
 */
GameOverOverlay_t* CreateGameOverOverlay(void);

/**
 * DestroyGameOverOverlay - Free game-over overlay component
 *
 * Follows ADR-015 double-pointer destructor pattern.
 * Frees overlay struct, nulls pointer.
 *
 * @param overlay - Pointer to overlay pointer (nulled after destroy)
 */
void DestroyGameOverOverlay(GameOverOverlay_t** overlay);

// ============================================================================
// DISPLAY
// ============================================================================

/**
 * ShowGameOverOverlay - Display game-over overlay
 *
 * Snapshots run statistics from Stats_GetCurrent() and starts animation.
 * Called when entering STATE_GAME_OVER.
 *
 * @param overlay - Overlay to show
 */
void ShowGameOverOverlay(GameOverOverlay_t* overlay);

/**
 * HideGameOverOverlay - Hide game-over overlay
 *
 * Sets visible=false. Called when transitioning away from STATE_GAME_OVER.
 *
 * @param overlay - Overlay to hide
 */
void HideGameOverOverlay(GameOverOverlay_t* overlay);

// ============================================================================
// UPDATE & RENDERING
// ============================================================================

/**
 * UpdateGameOverOverlay - Advance animation stages and flashing
 *
 * @param overlay - Overlay to update
 * @param dt - Delta time (seconds)
 *
 * Progresses through 4 animation stages:
 * - FADE_IN_OVERLAY (0.5s)
 * - FADE_IN_TITLE (0.4s)
 * - FADE_IN_STATS (0.5s)
 * - FADE_IN_PROMPT (0.3s)
 * - COMPLETE (flashing prompt loop)
 */
void UpdateGameOverOverlay(GameOverOverlay_t* overlay, float dt);

/**
 * RenderGameOverOverlay - Draw the overlay with current animation state
 *
 * Renders (if visible):
 * - Dark fullscreen overlay (0,0,0,overlay_alpha)
 * - "DEFEAT" title (red-orange #cf573c, scale 1.5×)
 * - Stats display:
 *   - Final Damage: X
 *   - Enemies Defeated: X
 *   - Hands Won: X / Y
 *   - Win Rate: X%
 * - "Press SPACE" prompt (gold #e8c170, flashing)
 *
 * @param overlay - Overlay to render
 */
void RenderGameOverOverlay(const GameOverOverlay_t* overlay);

// ============================================================================
// STATE
// ============================================================================

/**
 * IsGameOverOverlayVisible - Check if overlay is currently visible
 *
 * @param overlay - Overlay to check
 * @return true if visible, false otherwise
 */
bool IsGameOverOverlayVisible(const GameOverOverlay_t* overlay);

/**
 * IsGameOverAnimationComplete - Check if animation sequence finished
 *
 * @param overlay - Overlay to check
 * @return true if in ANIM_COMPLETE stage (ready for user input)
 */
bool IsGameOverAnimationComplete(const GameOverOverlay_t* overlay);

/**
 * HandleGameOverOverlayInput - Handle input (SPACE to close)
 *
 * @param overlay - Overlay to check
 * @return bool - true if player pressed SPACE (wants to quit to menu)
 *
 * Only responds to input during COMPLETE stage (after all animations).
 */
bool HandleGameOverOverlayInput(const GameOverOverlay_t* overlay);

#endif // GAME_OVER_OVERLAY_H
