/*
 * Bet Tooltip Modal Component
 * Shows betting system explanation and bet statistics
 * Constitutional pattern: Auto-size tooltip (ADR-008)
 */

#ifndef BET_TOOLTIP_MODAL_H
#define BET_TOOLTIP_MODAL_H

#include "../../common.h"
#include "../../player.h"

// ============================================================================
// CONSTANTS
// ============================================================================

#define BET_TOOLTIP_WIDTH       300
#define BET_TOOLTIP_MIN_HEIGHT  150

// ============================================================================
// BET TOOLTIP MODAL
// ============================================================================

typedef struct BetTooltipModal {
    bool visible;
    int x;
    int y;
} BetTooltipModal_t;

/**
 * CreateBetTooltipModal - Allocate and initialize bet tooltip modal
 *
 * @return BetTooltipModal_t* - Heap-allocated modal (caller must destroy)
 */
BetTooltipModal_t* CreateBetTooltipModal(void);

/**
 * DestroyBetTooltipModal - Free bet tooltip modal
 *
 * @param modal - Pointer to modal pointer (will be set to NULL)
 */
void DestroyBetTooltipModal(BetTooltipModal_t** modal);

/**
 * ShowBetTooltipModal - Make tooltip visible at position
 *
 * @param modal - Bet tooltip modal
 * @param x - X position for tooltip
 * @param y - Y position for tooltip
 */
void ShowBetTooltipModal(BetTooltipModal_t* modal, int x, int y);

/**
 * HideBetTooltipModal - Hide tooltip
 *
 * @param modal - Bet tooltip modal
 */
void HideBetTooltipModal(BetTooltipModal_t* modal);

/**
 * RenderBetTooltipModal - Render tooltip with betting stats
 *
 * Shows:
 * - Title: "Betting System"
 * - Description: How betting works
 * - Current Bet (color-coded)
 * - Highest Bet (with turns ago)
 * - Average Bet
 * - Total Wagered
 *
 * @param modal - Bet tooltip modal
 * @param player - Player to show current bet for
 */
void RenderBetTooltipModal(const BetTooltipModal_t* modal, const Player_t* player);

#endif // BET_TOOLTIP_MODAL_H
