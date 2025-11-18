/*
 * Character Stats Modal Component
 * Shows detailed character info and sanity threshold effects on hover
 * Constitutional pattern: Modal component, heap-allocated
 */

#ifndef CHARACTER_STATS_MODAL_H
#define CHARACTER_STATS_MODAL_H

#include "../../common.h"
#include "../../player.h"

// Modal dimensions (modern tooltip style - dynamic height)
#define STATS_MODAL_WIDTH   280

typedef struct CharacterStatsModal {
    bool is_visible;
    int x, y;
    Player_t* player;    // Reference to player (NOT owned)
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
