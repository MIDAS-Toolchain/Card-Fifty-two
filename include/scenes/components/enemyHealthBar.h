/*
 * Enemy Health Bar Component
 * Displays enemy HP with filled bar and HP text
 * Constitutional pattern: Reusable component
 */

#ifndef ENEMY_HEALTH_BAR_H
#define ENEMY_HEALTH_BAR_H

#include "../../common.h"
#include "../../enemy.h"

// Health bar dimensions
#define ENEMY_HP_BAR_WIDTH   200
#define ENEMY_HP_BAR_HEIGHT  30

typedef struct EnemyHealthBar {
    int x, y, w, h;
    Enemy_t* enemy;  // Reference to enemy (NOT owned)
} EnemyHealthBar_t;

/**
 * CreateEnemyHealthBar - Initialize enemy health bar component
 *
 * @param x - X position for bar
 * @param y - Y position for bar
 * @param enemy - Reference to enemy (NOT owned)
 * @return EnemyHealthBar_t* - Heap-allocated component (caller must destroy)
 */
EnemyHealthBar_t* CreateEnemyHealthBar(int x, int y, Enemy_t* enemy);

/**
 * DestroyEnemyHealthBar - Cleanup health bar resources
 *
 * @param bar - Pointer to bar pointer (will be set to NULL)
 */
void DestroyEnemyHealthBar(EnemyHealthBar_t** bar);

/**
 * SetEnemyHealthBarPosition - Update bar position
 *
 * @param bar - Health bar component
 * @param x - New X position
 * @param y - New Y position
 */
void SetEnemyHealthBarPosition(EnemyHealthBar_t* bar, int x, int y);

/**
 * SetEnemyHealthBarEnemy - Update enemy reference
 *
 * @param bar - Health bar component
 * @param enemy - New enemy reference (NOT owned)
 */
void SetEnemyHealthBarEnemy(EnemyHealthBar_t* bar, Enemy_t* enemy);

/**
 * RenderEnemyHealthBar - Render health bar with HP text
 *
 * @param bar - Health bar component
 *
 * Renders:
 * - Dark background (#282828)
 * - Red filled portion (#a53030) based on HP percent
 * - Border (#ebede9)
 * - HP text "X/Y" to the right of bar
 */
void RenderEnemyHealthBar(const EnemyHealthBar_t* bar);

#endif // ENEMY_HEALTH_BAR_H
