/*
 * Enemy Portrait Renderer Component
 * Encapsulates enemy portrait rendering with effects (shake, red flash, defeat fade)
 * Constitutional pattern: Component with lifecycle, references enemy (not owned)
 */

#ifndef ENEMY_PORTRAIT_RENDERER_H
#define ENEMY_PORTRAIT_RENDERER_H

#include "../../common.h"
#include "../../enemy.h"

// ============================================================================
// ENEMY PORTRAIT RENDERER COMPONENT
// ============================================================================

typedef struct EnemyPortraitRenderer {
    int x, y;              // Position (center point before offsets)
    int target_w;          // Target width (e.g., 400 for 0.8x scale)
    int target_h;          // Target height (e.g., 400 for 0.8x scale)
    Enemy_t* enemy;        // Reference only, not owned (can be NULL)
} EnemyPortraitRenderer_t;

/**
 * CreateEnemyPortraitRenderer - Initialize portrait renderer component
 *
 * @param x - X position (center point before offsets)
 * @param y - Y position (center point before offsets)
 * @param target_w - Target width for rendering (e.g., 400 for 0.8x scale)
 * @param target_h - Target height for rendering (e.g., 400 for 0.8x scale)
 * @param enemy - Enemy to render portrait for (NULL if not in combat)
 * @return EnemyPortraitRenderer_t* - Heap-allocated renderer (caller must destroy)
 */
EnemyPortraitRenderer_t* CreateEnemyPortraitRenderer(int x, int y, int target_w, int target_h, Enemy_t* enemy);

/**
 * DestroyEnemyPortraitRenderer - Cleanup portrait renderer resources
 *
 * @param renderer - Pointer to renderer pointer (will be set to NULL)
 */
void DestroyEnemyPortraitRenderer(EnemyPortraitRenderer_t** renderer);

/**
 * SetEnemyPortraitEnemy - Update enemy reference
 *
 * @param renderer - Portrait renderer component
 * @param enemy - New enemy to render (NULL to clear)
 */
void SetEnemyPortraitEnemy(EnemyPortraitRenderer_t* renderer, Enemy_t* enemy);

/**
 * RenderEnemyPortrait - Render enemy portrait with effects
 *
 * @param renderer - Portrait renderer component
 *
 * Handles:
 * - Texture scaling to fit target size
 * - Shake offset (damage feedback)
 * - Red flash effect (damage feedback)
 * - Defeat fade and scale animation
 * - Automatic texture color/alpha mod reset
 *
 * Does nothing if renderer or enemy is NULL, or if portrait texture is not loaded.
 */
void RenderEnemyPortrait(const EnemyPortraitRenderer_t* renderer);

#endif // ENEMY_PORTRAIT_RENDERER_H
