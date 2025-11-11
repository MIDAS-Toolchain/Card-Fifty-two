/*
 * Card Fifty-Two - Player System Implementation
 * Constitutional pattern: dString_t for name, stored in global g_players table
 */

#include "player.h"

// ============================================================================
// PLAYER LIFECYCLE
// ============================================================================

bool CreatePlayer(const char* name, int id, bool is_dealer) {
    if (!name) {
        d_LogError("CreatePlayer: NULL name passed");
        return false;
    }

    // Constitutional pattern: Player_t stored by value in table, not heap-allocated
    Player_t player = {0};

    // Constitutional pattern: dString_t for name, not char[]
    player.name = d_StringInit();
    if (!player.name) {
        d_LogFatal("CreatePlayer: Failed to initialize name string");
        return false;
    }
    d_StringSet(player.name, name, 0);

    // Initialize hand (value type - no malloc)
    InitHand(&player.hand);

    // Initialize player state
    player.player_id = id;
    player.chips = STARTING_CHIPS;
    player.display_chips = (float)STARTING_CHIPS;  // Start at max (will be tweened on damage)
    player.current_bet = 0;
    player.is_dealer = is_dealer;
    player.is_ai = is_dealer;  // Dealer is always AI
    player.state = PLAYER_STATE_WAITING;

    // Initialize portrait system (will be loaded externally)
    player.portrait_surface = NULL;
    player.portrait_texture = NULL;
    player.portrait_dirty = false;

    // Initialize sanity
    player.sanity = 100;
    player.max_sanity = 100;

    // Register in global players table (store by value)
    d_SetDataInTable(g_players, &player.player_id, &player);

    d_LogInfoF("Created player: %s (ID: %d, Dealer: %s, Chips: %d)",
               d_StringPeek(player.name), id,
               is_dealer ? "yes" : "no", player.chips);

    return true;
}

void DestroyPlayer(int player_id) {
    // Constitutional pattern: Lookup player from table by ID
    Player_t* player = (Player_t*)d_GetDataFromTable(g_players, &player_id);
    if (!player) {
        d_LogError("DestroyPlayer: Player not found in table");
        return;
    }

    // Destroy name string (heap-allocated resource)
    if (player->name) {
        d_StringDestroy(player->name);
        player->name = NULL;
    }

    // Cleanup hand (value type - only cleanup internal resources)
    CleanupHand(&player->hand);

    // Destroy portrait surface and texture (owned by player)
    if (player->portrait_surface) {
        SDL_FreeSurface(player->portrait_surface);
        player->portrait_surface = NULL;
    }

    if (player->portrait_texture) {
        SDL_DestroyTexture(player->portrait_texture);
        player->portrait_texture = NULL;
    }

    // Remove from global players table (Daedalus frees the Player_t value)
    d_RemoveDataFromTable(g_players, &player_id);

    d_LogInfo("Player destroyed");
}

// ============================================================================
// CHIP OPERATIONS
// ============================================================================

bool PlaceBet(Player_t* player, int amount) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!player) {
        d_LogError("PlaceBet: NULL player pointer");
        return false;
    }

    if (amount <= 0) {
        d_LogErrorF("PlaceBet: Invalid bet amount %d", amount);
        return false;
    }

    if (player->chips < amount) {
        d_LogInfoF("PlaceBet: Insufficient chips (%d < %d)",
                   player->chips, amount);
        return false;
    }

    // Deduct bet from chips
    player->chips -= amount;
    player->current_bet = amount;

    d_LogInfoF("%s placed bet: %d (remaining chips: %d)",
               d_StringPeek(player->name), amount, player->chips);

    return true;
}

void WinBet(Player_t* player, float multiplier) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!player) {
        d_LogError("WinBet: NULL player pointer");
        return;
    }

    // Calculate winnings: bet + (bet * multiplier)
    int winnings = (int)(player->current_bet * (1.0f + multiplier));
    player->chips += winnings;

    d_LogInfoF("%s won bet! Bet: %d, Multiplier: %.1fx, Winnings: %d, Total chips: %d",
               d_StringPeek(player->name), player->current_bet,
               multiplier, winnings, player->chips);

    player->current_bet = 0;
}

void LoseBet(Player_t* player) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!player) {
        d_LogError("LoseBet: NULL player pointer");
        return;
    }

    d_LogInfoF("%s lost bet: %d (chips remaining: %d)",
               d_StringPeek(player->name), player->current_bet, player->chips);

    // Bet already deducted in PlaceBet, just reset
    player->current_bet = 0;
}

void ReturnBet(Player_t* player) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!player) {
        d_LogError("ReturnBet: NULL player pointer");
        return;
    }

    // Return bet to chips
    player->chips += player->current_bet;

    d_LogInfoF("%s push - bet returned: %d (chips: %d)",
               d_StringPeek(player->name), player->current_bet, player->chips);

    player->current_bet = 0;
}

// ============================================================================
// PLAYER QUERIES
// ============================================================================

const char* GetPlayerName(const Player_t* player) {
    if (!player || !player->name) {
        return "Unknown";
    }
    return d_StringPeek(player->name);
}

int GetPlayerChips(const Player_t* player) {
    if (!player) {
        return 0;
    }
    return player->chips;
}

bool CanAffordBet(const Player_t* player, int amount) {
    if (!player) {
        return false;
    }
    return player->chips >= amount;
}

// ============================================================================
// PLAYER UTILITIES
// ============================================================================

void PlayerToString(const Player_t* player, dString_t* out) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!player || !out) {
        d_LogError("PlayerToString: NULL pointer passed");
        return;
    }

    // Format: "Alice (Player) | Chips: 1000 | Bet: 50 | Hand: ..."
    d_StringAppend(out, d_StringPeek(player->name),
                     d_StringGetLength(player->name));

    d_StringAppend(out, player->is_dealer ? " (Dealer)" : " (Player)",
                     player->is_dealer ? 9 : 9);

    d_StringAppend(out, " | Chips: ", 10);
    d_StringAppendInt(out, player->chips);

    d_StringAppend(out, " | Bet: ", 8);
    d_StringAppendInt(out, player->current_bet);

    d_StringAppend(out, " | Hand: ", 9);
    HandToString(&player->hand, out);
}

// ============================================================================
// PORTRAIT SYSTEM
// ============================================================================

bool LoadPlayerPortrait(Player_t* player, const char* filename) {
    if (!player) {
        d_LogError("LoadPlayerPortrait: NULL player");
        return false;
    }

    if (!filename) {
        d_LogError("LoadPlayerPortrait: NULL filename");
        return false;
    }

    // Load image as surface using Archimedes
    SDL_Surface* surface = a_Image(filename);
    if (!surface) {
        d_LogError("LoadPlayerPortrait: Failed to load image");
        return false;
    }

    // Free existing surface if any
    if (player->portrait_surface) {
        SDL_FreeSurface(player->portrait_surface);
    }

    // Free existing texture if any
    if (player->portrait_texture) {
        SDL_DestroyTexture(player->portrait_texture);
        player->portrait_texture = NULL;
    }

    // Store surface and mark as dirty
    player->portrait_surface = surface;
    player->portrait_dirty = true;

    d_LogInfoF("Loaded portrait for %s: %s", d_StringPeek(player->name), filename);
    return true;
}

void RefreshPlayerPortraitTexture(Player_t* player) {
    if (!player || !player->portrait_surface) return;

    // Free existing texture
    if (player->portrait_texture) {
        SDL_DestroyTexture(player->portrait_texture);
        player->portrait_texture = NULL;
    }

    // Convert surface to texture using Archimedes (destroy=0, we own the surface)
    player->portrait_texture = a_ToTexture(player->portrait_surface, 0);
    if (!player->portrait_texture) {
        d_LogError("RefreshPlayerPortraitTexture: Failed to create texture");
        return;
    }

    // Clear dirty flag
    player->portrait_dirty = false;

    d_LogInfo("Player portrait texture refreshed");
}

SDL_Texture* GetPlayerPortraitTexture(Player_t* player) {
    if (!player) return NULL;

    // Auto-refresh if dirty
    if (player->portrait_dirty && player->portrait_surface) {
        RefreshPlayerPortraitTexture(player);
    }

    return player->portrait_texture;
}

// ============================================================================
// SANITY SYSTEM
// ============================================================================

void InitializePlayerSanity(Player_t* player, int max_sanity) {
    if (!player) {
        d_LogError("InitializePlayerSanity: NULL player pointer");
        return;
    }

    player->max_sanity = max_sanity;
    player->sanity = max_sanity;

    d_LogInfoF("%s sanity initialized: %d/%d",
               d_StringPeek(player->name), player->sanity, player->max_sanity);
}

void ModifyPlayerSanity(Player_t* player, int amount) {
    if (!player) {
        d_LogError("ModifyPlayerSanity: NULL player pointer");
        return;
    }

    int old_sanity = player->sanity;
    player->sanity += amount;

    // Clamp between 0 and max_sanity
    if (player->sanity < 0) {
        player->sanity = 0;
    }
    if (player->sanity > player->max_sanity) {
        player->sanity = player->max_sanity;
    }

    d_LogInfoF("%s sanity: %d -> %d (change: %+d)",
               d_StringPeek(player->name), old_sanity, player->sanity, amount);
}

float GetPlayerSanityPercent(const Player_t* player) {
    if (!player || player->max_sanity <= 0) {
        return 0.0f;
    }

    return (float)player->sanity / (float)player->max_sanity;
}
