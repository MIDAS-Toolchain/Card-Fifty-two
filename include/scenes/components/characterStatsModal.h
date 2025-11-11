/*
 * Character Stats Modal Component
 * Shows detailed character info and sanity threshold effects on hover
 * Constitutional pattern: Modal component, heap-allocated
 */

#ifndef CHARACTER_STATS_MODAL_H
#define CHARACTER_STATS_MODAL_H

#include "../../common.h"
#include "../../player.h"

// Modal dimensions
#define STATS_MODAL_WIDTH   550
#define STATS_MODAL_HEIGHT  400
#define STATS_MODAL_PADDING 15
#define STATS_MODAL_GAP     12

typedef struct CharacterStatsModal {
    bool is_visible;
    int x, y, w, h;
    Player_t* player;    // Reference to player (NOT owned)
    FlexBox_t* layout;   // FlexBox for content layout (owned)
} CharacterStatsModal_t;

/**
 * CreateCharacterStatsModal - Initialize character stats modal
 *
 * @param player - Reference to player (modal does NOT own this)
 * @return CharacterStatsModal_t* - Heap-allocated modal (caller must destroy)
 */
CharacterStatsModal_t* CreateCharacterStatsModal(Player_t* player);

/**
 * DestroyCharacterStatsModal - Cleanup modal resources
 *
 * @param modal - Pointer to modal pointer (will be set to NULL)
 */
void DestroyCharacterStatsModal(CharacterStatsModal_t** modal);

/**
 * ShowCharacterStatsModal - Display modal at specified position
 *
 * @param modal - Modal to show
 * @param x - X position for modal
 * @param y - Y position for modal
 */
void ShowCharacterStatsModal(CharacterStatsModal_t* modal, int x, int y);

/**
 * HideCharacterStatsModal - Hide modal
 *
 * @param modal - Modal to hide
 */
void HideCharacterStatsModal(CharacterStatsModal_t* modal);

/**
 * RenderCharacterStatsModal - Render modal if visible
 *
 * @param modal - Modal to render
 */
void RenderCharacterStatsModal(CharacterStatsModal_t* modal);

#endif // CHARACTER_STATS_MODAL_H
