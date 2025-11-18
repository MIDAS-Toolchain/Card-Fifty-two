#ifndef STATS_H
#define STATS_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// DAMAGE SOURCE TRACKING
// ============================================================================

typedef enum {
    DAMAGE_SOURCE_TURN_WIN = 0,     // Damage from winning blackjack hand (chip threat)
    DAMAGE_SOURCE_TURN_PUSH,        // Damage from push (half of win damage)
    DAMAGE_SOURCE_TRINKET_PASSIVE,  // Damage from trinket passive effects
    DAMAGE_SOURCE_TRINKET_ACTIVE,   // Damage from trinket active abilities
    DAMAGE_SOURCE_ABILITY,          // Damage from player abilities (future)
    DAMAGE_SOURCE_MAX
} DamageSource_t;

// ============================================================================
// GLOBAL STATS STRUCTURE
// ============================================================================

typedef struct {
    // Card statistics
    uint64_t cards_drawn;           // Total cards drawn this run

    // Damage statistics (by source)
    uint64_t damage_dealt_total;    // Total damage dealt (sum of all sources)
    uint64_t damage_by_source[DAMAGE_SOURCE_MAX];  // Breakdown by source

    // Turn statistics (blackjack hands)
    uint64_t turns_played;          // Total blackjack hands played
    uint64_t turns_won;             // Hands won by player
    uint64_t turns_lost;            // Hands lost by player
    uint64_t turns_pushed;          // Hands pushed (tie)

    // Combat statistics
    uint64_t combats_won;           // Enemies defeated

    // Chip statistics
    uint64_t chips_bet;             // Total chips wagered
    uint64_t chips_won;             // Total chips won from hands
    uint64_t chips_lost;            // Total chips lost from losing hands
    uint64_t chips_drained;         // Total chips lost to status effect drains

    // Chip peak tracking
    int highest_chips;              // Highest chip count reached this run
    uint64_t highest_chips_turn;    // Turn number when highest was reached
    int lowest_chips;               // Lowest chip count reached this run
    uint64_t lowest_chips_turn;     // Turn number when lowest was reached

    // Bet peak tracking
    int highest_bet;                // Highest single bet placed this run
    uint64_t highest_bet_turn;      // Turn number when highest bet was placed
} GlobalStats_t;

// ============================================================================
// GLOBAL STATS MANAGER
// ============================================================================

/**
 * Initialize the global stats system.
 * Call once at application startup.
 */
void Stats_Init(void);

/**
 * Increment cards drawn counter.
 */
void Stats_IncrementCardsDrawn(void);

/**
 * Record damage dealt with specific source.
 * Automatically updates both total and source-specific counters.
 */
void Stats_RecordDamage(DamageSource_t source, int damage);

/**
 * Record turn outcomes.
 */
void Stats_RecordTurnPlayed(void);
void Stats_RecordTurnWon(void);
void Stats_RecordTurnLost(void);
void Stats_RecordTurnPushed(void);

/**
 * Record combat victory.
 */
void Stats_RecordCombatWon(void);

/**
 * Record chip transactions.
 */
void Stats_RecordChipsBet(int amount);
void Stats_RecordChipsWon(int amount);
void Stats_RecordChipsLost(int amount);
void Stats_RecordChipsDrained(int amount);

/**
 * Update chip peak tracking (highest/lowest).
 * Call whenever player chips change.
 */
void Stats_UpdateChipsPeak(int current_chips);

/**
 * Update bet peak tracking (highest bet).
 * Call whenever a bet is placed.
 */
void Stats_UpdateBetPeak(int current_bet);

/**
 * Get average bet per turn.
 * Returns 0 if no turns have been played.
 */
int Stats_GetAverageBet(void);

/**
 * Get current stats (read-only).
 */
const GlobalStats_t* Stats_GetCurrent(void);

/**
 * Reset all stats to zero (call at start of new run).
 */
void Stats_Reset(void);

/**
 * Get human-readable damage source name.
 */
const char* Stats_GetDamageSourceName(DamageSource_t source);

#endif // STATS_H
