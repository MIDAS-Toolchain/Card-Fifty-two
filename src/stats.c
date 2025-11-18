/*
 * Global Statistics Tracking System
 * Tracks player performance across all game sessions
 */

#include "../include/stats.h"
#include "../include/common.h"
#include "../include/defs.h"
#include <string.h>

// ============================================================================
// GLOBAL STATE
// ============================================================================

static GlobalStats_t g_stats;

// ============================================================================
// INITIALIZATION
// ============================================================================

void Stats_Init(void) {
    memset(&g_stats, 0, sizeof(GlobalStats_t));

    // Initialize chip peaks to starting chips (same as Stats_Reset)
    g_stats.highest_chips = STARTING_CHIPS;
    g_stats.lowest_chips = STARTING_CHIPS;

    d_LogInfo("Global stats system initialized");
}

// ============================================================================
// CARD TRACKING
// ============================================================================

void Stats_IncrementCardsDrawn(void) {
    g_stats.cards_drawn++;
}

// ============================================================================
// DAMAGE TRACKING
// ============================================================================

void Stats_RecordDamage(DamageSource_t source, int damage) {
    if (source < 0 || source >= DAMAGE_SOURCE_MAX) {
        d_LogError("Stats_RecordDamage: Invalid damage source");
        return;
    }
    
    if (damage < 0) {
        d_LogWarning("Stats_RecordDamage: Negative damage value, clamping to 0");
        damage = 0;
    }
    
    // Update source-specific counter
    g_stats.damage_by_source[source] += damage;
    
    // Update total damage
    g_stats.damage_dealt_total += damage;
    
    d_LogInfoF("📊 Stats: Recorded %d damage from %s (Total: %llu)",
               damage, Stats_GetDamageSourceName(source), g_stats.damage_dealt_total);
}

// ============================================================================
// GETTERS
// ============================================================================

const GlobalStats_t* Stats_GetCurrent(void) {
    return &g_stats;
}

const char* Stats_GetDamageSourceName(DamageSource_t source) {
    switch (source) {
        case DAMAGE_SOURCE_TURN_WIN:        return "Turn Wins";
        case DAMAGE_SOURCE_TURN_PUSH:       return "Turn Pushes";
        case DAMAGE_SOURCE_TRINKET_PASSIVE: return "Trinket Passives";
        case DAMAGE_SOURCE_TRINKET_ACTIVE:  return "Trinket Actives";
        case DAMAGE_SOURCE_ABILITY:         return "Abilities";
        default:                            return "Unknown";
    }
}

// ============================================================================
// TURN TRACKING
// ============================================================================

void Stats_RecordTurnPlayed(void) {
    g_stats.turns_played++;
}

void Stats_RecordTurnWon(void) {
    g_stats.turns_won++;
}

void Stats_RecordTurnLost(void) {
    g_stats.turns_lost++;
}

void Stats_RecordTurnPushed(void) {
    g_stats.turns_pushed++;
}

// ============================================================================
// COMBAT TRACKING
// ============================================================================

void Stats_RecordCombatWon(void) {
    g_stats.combats_won++;
    d_LogInfo("📊 Stats: Combat victory recorded");
}

// ============================================================================
// CHIP TRACKING
// ============================================================================

void Stats_RecordChipsBet(int amount) {
    if (amount > 0) {
        g_stats.chips_bet += amount;
        d_LogInfoF("📊 Stats_RecordChipsBet: +%d chips (total wagered: %llu, turns: %llu)",
                   amount, g_stats.chips_bet, g_stats.turns_played);
    }
}

void Stats_RecordChipsWon(int amount) {
    if (amount > 0) {
        g_stats.chips_won += amount;
    }
}

void Stats_RecordChipsLost(int amount) {
    if (amount > 0) {
        g_stats.chips_lost += amount;
    }
}

void Stats_RecordChipsDrained(int amount) {
    if (amount > 0) {
        g_stats.chips_drained += amount;
    }
}

void Stats_UpdateChipsPeak(int current_chips) {
    // Update highest chips
    if (current_chips > g_stats.highest_chips) {
        g_stats.highest_chips = current_chips;
        g_stats.highest_chips_turn = g_stats.turns_played;
    }

    // Update lowest chips
    if (current_chips < g_stats.lowest_chips) {
        g_stats.lowest_chips = current_chips;
        g_stats.lowest_chips_turn = g_stats.turns_played;
    }
}

void Stats_UpdateBetPeak(int current_bet) {
    if (current_bet > g_stats.highest_bet) {
        g_stats.highest_bet = current_bet;
        g_stats.highest_bet_turn = g_stats.turns_played;
        d_LogInfoF("📊 Stats_UpdateBetPeak: New highest bet: %d chips (turn %llu)",
                   current_bet, g_stats.turns_played);
    } else {
        d_LogInfoF("📊 Stats_UpdateBetPeak: Current bet %d (highest remains %d)",
                   current_bet, g_stats.highest_bet);
    }
}

int Stats_GetAverageBet(void) {
    if (g_stats.turns_played == 0) {
        return 0;
    }
    return (int)(g_stats.chips_bet / g_stats.turns_played);
}

// ============================================================================
// UTILITY
// ============================================================================

void Stats_Reset(void) {
    memset(&g_stats, 0, sizeof(GlobalStats_t));

    // Initialize chip peaks to starting chips
    g_stats.highest_chips = STARTING_CHIPS;
    g_stats.lowest_chips = STARTING_CHIPS;
    g_stats.highest_chips_turn = 0;
    g_stats.lowest_chips_turn = 0;

    // Initialize bet peaks
    g_stats.highest_bet = 0;
    g_stats.highest_bet_turn = 0;

    d_LogInfo("📊 Stats reset for new run");
}
