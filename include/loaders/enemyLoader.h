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

/**
 * GetEnemyNameFromDUF - Get enemy name without loading full enemy
 *
 * @param enemies_db - Parsed DUF root containing enemy definitions
 * @param enemy_key - Key name of enemy (e.g., "didact", "daemon")
 *
 * @return const char* - Enemy name, or "Unknown" if not found
 *
 * Lightweight alternative to creating temp enemy just to read name.
 * Does NOT allocate - returns pointer to DUF string (valid until DUF freed).
 */
const char* GetEnemyNameFromDUF(dDUFValue_t* enemies_db, const char* enemy_key);

/**
 * ValidateEnemyDatabase - Validate all enemies in DUF database
 *
 * @param enemies_db - Parsed DUF database to validate
 * @param out_error_msg - Buffer to write error message (if validation fails)
 * @param error_msg_size - Size of error message buffer
 *
 * @return true if all enemies valid, false if any enemy fails validation
 *
 * Attempts to load every enemy in database. If any fail, writes detailed
 * error message to out_error_msg and returns false.
 */
bool ValidateEnemyDatabase(dDUFValue_t* enemies_db, char* out_error_msg, size_t error_msg_size);

#endif // ENEMY_LOADER_H
