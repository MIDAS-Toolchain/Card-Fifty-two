#ifndef ENEMY_LOADER_H
#define ENEMY_LOADER_H

#include "../enemy.h"
#include <Daedalus.h>

/**
 * LoadEnemyFromDUF - Load enemy definition from DUF database
 *
 * @param enemies_db - Parsed DUF root containing enemy definitions
 * @param enemy_key - Key name of enemy to load (e.g., "didact", "daemon")
 *
 * @return Enemy_t* on success, NULL on failure
 *
 * Example:
 *   dDUFValue_t* enemies = NULL;
 *   d_DUFParseFile("data/enemies/tutorial_enemies.duf", &enemies);
 *   Enemy_t* didact = LoadEnemyFromDUF(enemies, "didact");
 */
Enemy_t* LoadEnemyFromDUF(dDUFValue_t* enemies_db, const char* enemy_key);

#endif // ENEMY_LOADER_H
