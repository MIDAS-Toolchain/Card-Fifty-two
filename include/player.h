#ifndef PLAYER_H
#define PLAYER_H

#include "common.h"
#include "hand.h"

// ============================================================================
// PLAYER LIFECYCLE
// ============================================================================

/**
 * CreatePlayer - Initialize a new player
 *
 * @param name Player name (will be copied into dString_t)
 * @param id Unique player ID (0 = dealer, 1+ = players)
 * @param is_dealer true if this is the dealer
 * @return true on success, false on failure
 *
 * Constitutional pattern:
 * - Player_t is value type stored directly in g_players table
 * - name copied into dString_t (not char[])
 * - Caller must call DestroyPlayer(player_id) when done
 * - Automatically registers in global g_players table
 */
bool CreatePlayer(const char* name, int id, bool is_dealer);

/**
 * DestroyPlayer - Free player resources and remove from table
 *
 * @param player_id Player ID to destroy
 *
 * Constitutional pattern: Lookup by ID, cleanup internal resources, remove from table
 * Automatically removes from global g_players table
 */
void DestroyPlayer(int player_id);

// ============================================================================
// CHIP OPERATIONS
// ============================================================================

/**
 * PlaceBet - Place a bet (deduct from chips)
 *
 * @param player Player placing bet
 * @param amount Bet amount
 * @return true if bet placed successfully, false if insufficient chips
 *
 * Updates player->current_bet and deducts from player->chips
 */
bool PlaceBet(Player_t* player, int amount);

/**
 * WinBet - Player wins bet (return bet + winnings)
 *
 * @param player Player who won
 * @param multiplier Payout multiplier (1.0 = even money, 1.5 = blackjack)
 *
 * Returns current_bet * (1 + multiplier) to chips
 * Resets current_bet to 0
 */
void WinBet(Player_t* player, float multiplier);

/**
 * LoseBet - Player loses bet
 *
 * @param player Player who lost
 *
 * Bet was already deducted, just reset current_bet to 0
 */
void LoseBet(Player_t* player);

/**
 * ReturnBet - Push/tie, return bet to player
 *
 * @param player Player with push
 *
 * Returns current_bet to chips, resets current_bet to 0
 */
void ReturnBet(Player_t* player);

// ============================================================================
// PLAYER QUERIES
// ============================================================================

/**
 * GetPlayerName - Get player name as const char*
 *
 * @param player Player to query
 * @return Player name, or "Unknown" if player is NULL
 */
const char* GetPlayerName(const Player_t* player);

/**
 * GetPlayerChips - Get player's chip count
 *
 * @param player Player to query
 * @return Chip count, or 0 if player is NULL
 */
int GetPlayerChips(const Player_t* player);

/**
 * CanAffordBet - Check if player can afford a bet
 *
 * @param player Player to check
 * @param amount Bet amount to check
 * @return true if player->chips >= amount, false otherwise
 */
bool CanAffordBet(const Player_t* player, int amount);

// ============================================================================
// PORTRAIT SYSTEM
// ============================================================================

/**
 * LoadPlayerPortrait - Load player portrait from file as surface
 *
 * @param player - Player to load portrait for
 * @param filename - Path to portrait image file
 * @return bool - true on success, false on failure
 *
 * Loads portrait as SDL_Surface for pixel manipulation
 * Sets portrait_dirty = true to trigger texture generation
 */
bool LoadPlayerPortrait(Player_t* player, const char* filename);

/**
 * RefreshPlayerPortraitTexture - Convert surface to texture
 *
 * @param player - Player to refresh portrait for
 *
 * Converts portrait_surface → portrait_texture
 * Only call when portrait_dirty == true
 * Automatically sets portrait_dirty = false after conversion
 */
void RefreshPlayerPortraitTexture(Player_t* player);

/**
 * GetPlayerPortraitTexture - Get current portrait texture for rendering
 *
 * @param player - Player to query
 * @return SDL_Texture* - Portrait texture, or NULL if not loaded
 *
 * Automatically refreshes texture if portrait_dirty == true
 */
SDL_Texture* GetPlayerPortraitTexture(Player_t* player);

// ============================================================================
// SANITY SYSTEM
// ============================================================================

/**
 * InitializePlayerSanity - Initialize sanity values for player
 *
 * @param player Player to initialize
 * @param max_sanity Maximum sanity value (default: 100)
 *
 * Sets player->sanity = max_sanity and player->max_sanity = max_sanity
 */
void InitializePlayerSanity(Player_t* player, int max_sanity);

/**
 * ModifyPlayerSanity - Change player's sanity value
 *
 * @param player Player to modify
 * @param amount Amount to add (positive) or subtract (negative)
 *
 * Clamps result between 0 and player->max_sanity
 */
void ModifyPlayerSanity(Player_t* player, int amount);

/**
 * GetPlayerSanityPercent - Get sanity as percentage
 *
 * @param player Player to query
 * @return float - Sanity percentage (0.0 - 1.0)
 */
float GetPlayerSanityPercent(const Player_t* player);

// ============================================================================
// PLAYER UTILITIES
// ============================================================================

/**
 * PlayerToString - Convert player to human-readable string
 *
 * @param player Player to convert
 * @param out Output dString_t (caller must create and destroy)
 *
 * Format: "Alice (Player) | Chips: 1000 | Bet: 50 | Hand: ..."
 *
 * Constitutional pattern: Uses dString_t, not char[] buffer
 */
void PlayerToString(const Player_t* player, dString_t* out);

#endif // PLAYER_H
