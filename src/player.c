/*
 * Card Fifty-Two - Player System Implementation
 * Constitutional pattern: dString_t for name, stored in global g_players table
 */

#include "player.h"
#include "trinket.h"
#include "trinketStats.h"
#include "statusEffects.h"
#include "stats.h"
#include "scenes/sceneBlackjack.h"
#include "cardTags.h"  // For HasCardTag, CARD_TAG_LUCKY, CARD_TAG_JAGGED

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

    // Initialize status effects manager (heap-allocated)
    player.status_effects = CreateStatusEffectManager();
    if (!player.status_effects) {
        d_LogError("CreatePlayer: Failed to create status effect manager");
        d_StringDestroy(player.name);
        CleanupHand(&player.hand);
        return false;
    }
    player.debuff_blocks_remaining = 0;           // No debuff blocks by default (Warded Charm adds blocks)
    player.enemy_heal_punishes_remaining = 0;    // No heal punishes by default (Bleeding Heart adds punishes)

    // Initialize class system (Constitutional: no class by default)
    player.class = PLAYER_CLASS_NONE;
    player.has_class_trinket = false;  // No class trinket until class is set
    memset(&player.class_trinket, 0, sizeof(Trinket_t));  // Zero-initialize embedded value

    // Initialize trinket slots (Constitutional: fixed array of TrinketInstance_t, 6 slots)
    // Zero-initialize all slots and mark as unoccupied
    memset(player.trinket_slots, 0, sizeof(player.trinket_slots));
    for (int i = 0; i < 6; i++) {
        player.trinket_slot_occupied[i] = false;
    }

    d_LogInfo("Initialized 6 trinket instance slots for player");

    // Initialize combat stats (ADR-09: Dirty-flag aggregation)
    player.damage_flat = 0;
    player.damage_percent = 0;
    player.crit_chance = 0;
    player.crit_bonus = 0;
    player.combat_stats_dirty = true;  // Will be calculated on first update

    // Register in global players table (store by value)
    d_TableSet(g_players, &player.player_id, &player);

    d_LogInfoF("Created player: %s (ID: %d, Dealer: %s, Chips: %d)",
               d_StringPeek(player.name), id,
               is_dealer ? "yes" : "no", player.chips);

    return true;
}

void DestroyPlayer(int player_id) {
    // Constitutional pattern: Lookup player from table by ID
    d_LogInfoF("DestroyPlayer: Starting destruction of player ID %d", player_id);

    if (!g_players) {
        d_LogError("DestroyPlayer: g_players table is NULL!");
        return;
    }

    Player_t* player = (Player_t*)d_TableGet(g_players, &player_id);
    if (!player) {
        d_LogErrorF("DestroyPlayer: Player ID %d not found in table", player_id);
        return;
    }

    // CRITICAL: Copy player data to stack BEFORE removing from table
    // This prevents double-free when d_TableRemove() frees the table entry
    Player_t player_copy = *player;

    // Remove from global players table FIRST (Daedalus frees the Player_t value memory)
    d_LogDebug("DestroyPlayer: Removing from g_players table");
    d_TableRemove(g_players, &player_id);

    // NOW cleanup internal heap resources using stack copy
    // (player pointer is now INVALID - use player_copy)

    // Destroy name string (heap-allocated resource)
    d_LogDebug("DestroyPlayer: Destroying player name");
    if (player_copy.name) {
        d_StringDestroy(player_copy.name);
    }

    // Cleanup hand (value type - only cleanup internal resources)
    d_LogDebug("DestroyPlayer: Cleaning up hand");
    CleanupHand(&player_copy.hand);

    // Destroy portrait surface and texture (owned by player)
    d_LogDebug("DestroyPlayer: Destroying portrait");
    if (player_copy.portrait_surface) {
        SDL_FreeSurface(player_copy.portrait_surface);
    }

    if (player_copy.portrait_texture) {
        SDL_DestroyTexture(player_copy.portrait_texture);
    }

    // Destroy status effects manager (heap-allocated)
    d_LogDebug("DestroyPlayer: Destroying status effects");
    if (player_copy.status_effects) {
        DestroyStatusEffectManager(&player_copy.status_effects);
    }

    // Cleanup class trinket (VALUE semantics - only cleanup internal dStrings, not struct)
    d_LogDebug("DestroyPlayer: Cleaning up class trinket");
    if (player_copy.has_class_trinket) {
        CleanupTrinketValue(&player_copy.class_trinket);
    }

    // Cleanup trinket instance slots (Constitutional: cleanup each instance's dStrings)
    // TrinketInstance_t contains dString_t* fields that need cleanup
    d_LogDebug("DestroyPlayer: Cleaning up trinket instance slots");
    for (int i = 0; i < 6; i++) {
        if (player_copy.trinket_slot_occupied[i]) {
            TrinketInstance_t* instance = &player_copy.trinket_slots[i];
            // Free internal dString_t fields
            if (instance->base_trinket_key) {
                d_StringDestroy(instance->base_trinket_key);
            }
            if (instance->trinket_stack_stat) {
                d_StringDestroy(instance->trinket_stack_stat);
            }
            // Free affix stat keys
            for (int j = 0; j < instance->affix_count; j++) {
                if (instance->affixes[j].stat_key) {
                    d_StringDestroy(instance->affixes[j].stat_key);
                }
            }
        }
    }

    d_LogInfoF("Player ID %d destroyed successfully", player_id);
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

    // Check if this is a new bet (current_bet should be 0) or a double-down
    bool is_double_down = (player->current_bet > 0);

    if (is_double_down) {
        // Double-down: ADD to existing bet
        int old_bet = player->current_bet;
        player->current_bet += amount;

        // Update peak with TOTAL bet (chips_bet will be recorded on round resolution)
        Stats_UpdateBetPeak(player->current_bet);

        d_LogInfoF("%s doubled bet: %d → %d (current chips: %d)",
                   d_StringPeek(player->name), old_bet, player->current_bet, player->chips);
    } else {
        // New bet: SET current_bet (should be 0 already, but just in case)
        player->current_bet = amount;

        // Update peak (chips_bet will be recorded on round resolution)
        Stats_UpdateBetPeak(amount);

        d_LogInfoF("%s placed bet: %d (current chips: %d)",
                   d_StringPeek(player->name), amount, player->chips);
    }

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
    Stats_UpdateChipsPeak(player->chips);

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

    // Deduct bet from chips (happens at round resolution, not at bet time)
    player->chips -= player->current_bet;

    // CLAMP TO 0 (prevent negative chips)
    if (player->chips < 0) {
        player->chips = 0;
        d_LogWarning("Player chips clamped to 0 (would have gone negative)");
    }

    Stats_UpdateChipsPeak(player->chips);

    d_LogInfoF("%s lost bet: %d (chips remaining: %d)",
               d_StringPeek(player->name), player->current_bet, player->chips);

    player->current_bet = 0;
}

void ReturnBet(Player_t* player) {
    // Constitutional pattern: Check NULL before dereferencing
    if (!player) {
        d_LogError("ReturnBet: NULL player pointer");
        return;
    }

    // Push - no change to chips (bet was never deducted)
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

    // Load image using Archimedes (returns aImage_t with both surface and texture)
    aImage_t* img = a_ImageLoad(filename);
    if (!img || !img->surface) {
        d_LogError("LoadPlayerPortrait: Failed to load image");
        return false;
    }

    // Free existing surface if any
    if (player->portrait_surface) {
        SDL_FreeSurface(player->portrait_surface);
    }

    // Store surface (make a copy since Archimedes owns the original)
    player->portrait_surface = SDL_ConvertSurface(img->surface, img->surface->format, 0);

    // Create texture from surface
    if (player->portrait_texture) {
        SDL_DestroyTexture(player->portrait_texture);
        player->portrait_texture = NULL;
    }

    player->portrait_texture = SDL_CreateTextureFromSurface(app.renderer, player->portrait_surface);
    if (!player->portrait_texture) {
        d_LogErrorF("Failed to create portrait texture: %s", SDL_GetError());
        player->portrait_dirty = false;
        return false;
    }

    player->portrait_dirty = false;
    d_LogInfoF("Loaded portrait for %s: %s", d_StringPeek(player->name), filename);
    return true;
}

void RefreshPlayerPortraitTexture(Player_t* player) {
    if (!player || !player->portrait_surface) {
        return;
    }

    // Recreate texture from surface
    if (player->portrait_texture) {
        SDL_DestroyTexture(player->portrait_texture);
        player->portrait_texture = NULL;
    }

    player->portrait_texture = SDL_CreateTextureFromSurface(app.renderer, player->portrait_surface);
    if (!player->portrait_texture) {
        d_LogErrorF("Failed to create portrait texture: %s", SDL_GetError());
    }

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

// ============================================================================
// COMBAT STATS SYSTEM (ADR-09: Dirty-flag aggregation)
// ============================================================================

void CalculatePlayerCombatStats(Player_t* player) {
    if (!player) {
        d_LogError("CalculatePlayerCombatStats: NULL player");
        return;
    }

    // Reset stats to base values
    player->damage_flat = 0;
    player->damage_percent = 0;
    player->crit_chance = 0;
    player->crit_bonus = 0;  // Default crit bonus (no cards give this yet)

    // Reset defensive stats
    player->win_bonus_percent = 0;
    player->loss_refund_percent = 0;
    player->push_damage_percent = 0;
    player->flat_chips_on_win = 0;

    int lucky_count = 0;
    int brutal_count = 0;

    // ADR-09: LUCKY/BRUTAL are GLOBAL bonuses - scan ALL hands in play (player + dealer)
    // Iterate through all players in g_players (player ID 0 = dealer, ID 1 = human)
    extern dTable_t* g_players;
    if (!g_players) {
        d_LogWarning("CalculatePlayerCombatStats: g_players table is NULL");
        player->combat_stats_dirty = false;
        return;
    }

    // Scan player ID 0 (dealer) and player ID 1 (human player)
    for (int player_id = 0; player_id <= 1; player_id++) {
        Player_t* p = (Player_t*)d_TableGet(g_players, &player_id);
        if (!p || !p->hand.cards) continue;

        // Scan all FACE-UP cards in this player's hand (hidden cards don't count!)
        for (size_t i = 0; i < p->hand.cards->count; i++) {
            const Card_t* card = (const Card_t*)d_ArrayGet(p->hand.cards, i);
            if (!card || !card->face_up) continue;  // Skip face-down cards!

            // Check for LUCKY tag (+10% crit per card)
            if (HasCardTag(card->card_id, CARD_TAG_LUCKY)) {
                lucky_count++;
                d_LogInfoF("  Found LUCKY card: %d (player_id=%d, face_up=%d)",
                          card->card_id, player_id, card->face_up);
            }

            // Check for JAGGED tag (+10% damage per card)
            if (HasCardTag(card->card_id, CARD_TAG_JAGGED)) {
                brutal_count++;
                d_LogInfoF("  Found JAGGED card: %d (player_id=%d, face_up=%d)",
                          card->card_id, player_id, card->face_up);
            }
        }
    }

    // Apply bonuses from card tags
    player->crit_chance = lucky_count * 10;      // +10% per LUCKY card
    player->damage_percent = brutal_count * 10;  // +10% per BRUTAL card

    d_LogInfoF("Card tag bonuses: damage_percent=%d%%, crit_chance=%d%% (LUCKY=%d, BRUTAL=%d)",
               player->damage_percent, player->crit_chance, lucky_count, brutal_count);

    // Apply trinket affixes and stack bonuses (additive with card tags)
    AggregateTrinketStats(player);

    // Clear dirty flag
    player->combat_stats_dirty = false;

    d_LogInfo("Combat stats recalculation complete");
}

bool RollForCrit(Player_t* player, int base_damage, int* out_damage) {
    if (!player || !out_damage) {
        if (out_damage) *out_damage = base_damage;
        return false;
    }

    // Base case: no damage = no crit
    if (base_damage <= 0) {
        *out_damage = base_damage;
        return false;
    }

    // Recalculate stats if dirty
    if (player->combat_stats_dirty) {
        CalculatePlayerCombatStats(player);
    }

    // No crit chance = no crit
    if (player->crit_chance <= 0) {
        *out_damage = base_damage;
        return false;
    }

    // Roll for crit (0-99 vs crit_chance%)
    int roll = rand() % 100;
    if (roll < player->crit_chance) {
        // CRIT!
        int crit_multiplier = 50 + player->crit_bonus;  // Base 50% + any bonus
        int crit_damage = (base_damage * crit_multiplier) / 100;
        *out_damage = base_damage + crit_damage;

        d_LogInfoF("💥 CRITICAL HIT! %d → %d (+%d%% = +%d damage) [roll=%d < %d%%]",
                   base_damage, *out_damage, crit_multiplier, crit_damage, roll, player->crit_chance);
        return true;
    }

    // No crit
    *out_damage = base_damage;
    return false;
}

int ApplyPlayerDamageModifiers(Player_t* player, int base_damage, bool* out_is_crit) {
    // Validate inputs
    if (!player || base_damage <= 0) {
        if (out_is_crit) *out_is_crit = false;
        return base_damage;
    }

    // Recalculate stats if dirty (ADR-09: Dirty-flag aggregation)
    if (player->combat_stats_dirty) {
        CalculatePlayerCombatStats(player);
    }

    int modified_damage = base_damage;

    // Step 1: Apply percentage damage increase (BRUTAL tag)
    if (player->damage_percent > 0) {
        int bonus_damage = (modified_damage * player->damage_percent) / 100;
        modified_damage += bonus_damage;
        d_LogInfoF("  [Damage Modifier] +%d%% damage: %d → %d (+%d)",
                   player->damage_percent, base_damage, modified_damage, bonus_damage);
    }

    // Step 2: Apply flat damage bonus (currently unused, reserved for future trinkets)
    if (player->damage_flat > 0) {
        modified_damage += player->damage_flat;
        d_LogInfoF("  [Damage Modifier] +%d flat damage: %d → %d",
                   player->damage_flat, modified_damage - player->damage_flat, modified_damage);
    }

    // Step 3: Roll for crit (LUCKY tag)
    bool is_crit = RollForCrit(player, modified_damage, &modified_damage);
    if (out_is_crit) {
        *out_is_crit = is_crit;
    }

    d_LogInfoF("  [Damage Modifier] Final: %d → %d%s",
               base_damage, modified_damage, is_crit ? " (CRIT!)" : "");

    return modified_damage;
}
