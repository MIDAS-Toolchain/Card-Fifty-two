#ifndef RESULT_SCREEN_H
#define RESULT_SCREEN_H

#include "common.h"
#include "structs.h"
#include "tween/tween.h"

// ============================================================================
// RESULT SCREEN COMPONENT
// ============================================================================

/**
 * ResultScreen_t - Animated result overlay for win/loss/push states
 *
 * Constitutional pattern:
 * - Component manages result screen animations (slot machine chip counter, damage text)
 * - Separated win/loss animation (immediate) from status drain animation (delayed)
 * - Uses tween system for smooth fades and counters
 * - Collision detection prevents text overlap
 *
 * Lifecycle:
 * 1. CreateResultScreen() in scene initialization
 * 2. ShowResultScreen() when entering STATE_ROUND_END or STATE_COMBAT_VICTORY
 * 3. UpdateResultScreen() each frame (increments timer, triggers animations)
 * 4. RenderResultScreen() each frame (draws overlay + animated text)
 * 5. DestroyResultScreen() in scene cleanup
 */
typedef struct ResultScreen {
    // Chip animation state
    float display_chips;      // Tweened chip counter (slot machine effect)
    int old_chips;            // Chips before round (for slot animation)
    int chip_delta;           // Win/loss amount (bet outcome only)
    int status_drain;         // Status effect chip drain (CHIP_DRAIN only)

    // Timing
    float timer;              // Delay timer for status drain reveal

    // Win/loss animation (immediate, 0-0.5s)
    bool winloss_started;
    float winloss_alpha;
    float winloss_offset_x;
    float winloss_offset_y;

    // Status drain animation (delayed, 0.5s+)
    bool bleed_started;
    float bleed_alpha;
    float bleed_offset_x;
    float bleed_offset_y;
} ResultScreen_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateResultScreen - Initialize result screen component
 *
 * @return ResultScreen_t* - Heap-allocated component, or NULL on failure
 */
ResultScreen_t* CreateResultScreen(void);

/**
 * DestroyResultScreen - Free result screen component
 *
 * @param screen - Pointer to screen pointer (double pointer for nulling)
 */
void DestroyResultScreen(ResultScreen_t** screen);

// ============================================================================
// DISPLAY
// ============================================================================

/**
 * ShowResultScreen - Start result screen animations
 *
 * @param screen - Screen to update
 * @param old_chips - Player chips before round (for slot animation)
 * @param chip_delta - Bet outcome delta (win/loss amount)
 * @param status_drain - Status effect drain amount (0 if none)
 *
 * Call this when entering STATE_ROUND_END or STATE_COMBAT_VICTORY.
 * Resets timers and animation flags, generates random offsets.
 */
void ShowResultScreen(ResultScreen_t* screen, int old_chips, int chip_delta, int status_drain);

// ============================================================================
// UPDATE & RENDERING
// ============================================================================

/**
 * UpdateResultScreen - Advance result screen animations
 *
 * @param screen - Screen to update
 * @param dt - Delta time
 *
 * Increments timer, triggers win/loss animation (immediate) and status drain
 * animation (delayed at 0.5s).
 */
void UpdateResultScreen(ResultScreen_t* screen, float dt, TweenManager_t* tween_mgr);

/**
 * RenderResultScreen - Draw result overlay
 *
 * @param screen - Screen to render
 * @param player - Player to display state for (win/loss/push)
 * @param state - Current game state (determines result message)
 *
 * Draws:
 * - Semi-transparent overlay
 * - Result message (YOU WIN/YOU LOSE/PUSH)
 * - Animated chip counter (slot machine effect)
 * - Win/loss delta text (immediate, 0-0.5s)
 * - Status drain text (delayed, 0.5s+)
 * - Collision detection prevents text overlap
 */
void RenderResultScreen(ResultScreen_t* screen, Player_t* player, GameState_t state);

// ============================================================================
// EXTERNAL CALLBACKS (for statusEffects.c)
// ============================================================================

/**
 * SetGlobalResultScreen - Register result screen for external callbacks
 *
 * @param screen - Result screen to register (or NULL to unregister)
 *
 * MUST be called during scene initialization so statusEffects.c can track drain.
 */
void SetGlobalResultScreen(ResultScreen_t* screen);

/**
 * SetStatusEffectDrainAmount - Track chip drain from status effects
 *
 * @param drain_amount - Total chips drained
 *
 * Called by statusEffects.c to record CHIP_DRAIN effect.
 * Allows result screen to separate bet outcome from status drain.
 */
void SetStatusEffectDrainAmount(int drain_amount);

/**
 * TriggerSidebarBetAnimation - Trigger sidebar bet damage animation
 *
 * @param bet_amount - Bet amount to display
 *
 * Called when bet chips are deducted (before round starts).
 * Shows bet amount flying off sidebar.
 */
void TriggerSidebarBetAnimation(int bet_amount);

#endif // RESULT_SCREEN_H
