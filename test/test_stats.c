/*
 * Stats Tracking Tests
 * Tests bet tracking, chip tracking, and stats calculations
 */

#include "test.h"
#include "../include/stats.h"
#include "../include/player.h"
#include "../include/common.h"

// ============================================================================
// TEST FIXTURES
// ============================================================================

static void setup_stats(void) {
    // Reset stats before each test
    Stats_Init();
    Stats_Reset();
}

// ============================================================================
// BET TRACKING TESTS
// ============================================================================

TEST(bet_tracking_records_highest_bet) {
    setup_stats();

    // Place bets of increasing amounts
    Stats_RecordChipsBet(10);
    Stats_UpdateBetPeak(10);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(25);
    Stats_UpdateBetPeak(25);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(15);
    Stats_UpdateBetPeak(15);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(50);  // Highest!
    Stats_UpdateBetPeak(50);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(20);
    Stats_UpdateBetPeak(20);
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->highest_bet, 50);
    ASSERT_EQ(stats->highest_bet_turn, 3);  // Peak recorded BEFORE turn incremented (turn 3)
}

TEST(bet_tracking_calculates_average_bet) {
    setup_stats();

    // Place bets: 10, 20, 30
    Stats_RecordChipsBet(10);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(20);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(30);
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    // Total bet = 60, turns = 3, average = 20
    ASSERT_EQ(stats->chips_bet, 60);
    ASSERT_EQ(stats->turns_played, 3);
    ASSERT_EQ(Stats_GetAverageBet(), 20);
}

TEST(bet_tracking_handles_zero_turns) {
    setup_stats();

    // No turns played yet
    ASSERT_EQ(Stats_GetAverageBet(), 0);
}

TEST(bet_tracking_handles_varying_bets) {
    setup_stats();

    // Place varying bets: 10, 10, 15, 10, 20
    Stats_RecordChipsBet(10);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(10);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(15);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(10);
    Stats_RecordTurnPlayed();

    Stats_RecordChipsBet(20);
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    // Total bet = 65, turns = 5, average = 13
    ASSERT_EQ(stats->chips_bet, 65);
    ASSERT_EQ(stats->turns_played, 5);
    ASSERT_EQ(Stats_GetAverageBet(), 13);
}

TEST(bet_peak_updates_only_on_higher_bets) {
    setup_stats();

    Stats_UpdateBetPeak(10);
    Stats_RecordTurnPlayed();

    Stats_UpdateBetPeak(5);  // Lower than 10, should NOT update
    Stats_RecordTurnPlayed();

    Stats_UpdateBetPeak(15);  // Higher than 10, SHOULD update
    Stats_RecordTurnPlayed();

    Stats_UpdateBetPeak(10);  // Equal to old highest, should NOT update
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->highest_bet, 15);
    ASSERT_EQ(stats->highest_bet_turn, 2);  // Turn 2 when we bet 15
}

// ============================================================================
// CHIP PEAK TRACKING TESTS
// ============================================================================

TEST(chip_peak_tracks_highest_chips) {
    setup_stats();

    Stats_UpdateChipsPeak(100);  // Starting chips
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(150);  // Won some
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(120);  // Lost some
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(200);  // Big win! Highest
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(180);  // Down a bit
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->highest_chips, 200);
    ASSERT_EQ(stats->highest_chips_turn, 3);  // Peak recorded BEFORE turn incremented (turn 3)
}

TEST(chip_peak_tracks_lowest_chips) {
    setup_stats();

    Stats_UpdateChipsPeak(100);  // Starting chips
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(80);  // Lost some
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(120);  // Won some
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(50);  // Big loss! Lowest
    Stats_RecordTurnPlayed();

    Stats_UpdateChipsPeak(70);  // Recovered a bit
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->lowest_chips, 50);
    ASSERT_EQ(stats->lowest_chips_turn, 3);  // Peak recorded BEFORE turn incremented (turn 3)
}

// ============================================================================
// STATS RESET TESTS
// ============================================================================

TEST(stats_reset_clears_all_bet_stats) {
    setup_stats();

    // Record some bets
    Stats_RecordChipsBet(100);
    Stats_UpdateBetPeak(100);
    Stats_RecordTurnPlayed();

    // Reset stats
    Stats_Reset();

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->chips_bet, 0);
    ASSERT_EQ(stats->highest_bet, 0);
    ASSERT_EQ(stats->highest_bet_turn, 0);
    ASSERT_EQ(stats->turns_played, 0);
}

TEST(stats_reset_initializes_chip_peaks) {
    setup_stats();

    const GlobalStats_t* stats = Stats_GetCurrent();

    // After reset, chip peaks should be initialized to STARTING_CHIPS
    ASSERT_EQ(stats->highest_chips, STARTING_CHIPS);
    ASSERT_EQ(stats->lowest_chips, STARTING_CHIPS);
}

// ============================================================================
// INTEGRATION TESTS (with Player)
// ============================================================================

TEST(placeBet_updates_bet_peak) {
    setup_stats();

    // Create minimal player
    Player_t player = {0};
    player.chips = 200;
    player.current_bet = 0;
    player.name = d_StringInit();
    d_StringSet(player.name, "TestPlayer");

    // Place bets (reset current_bet between hands like the game does)
    PlaceBet(&player, 10);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    PlaceBet(&player, 50);  // Highest
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    PlaceBet(&player, 25);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->highest_bet, 50);
    ASSERT_EQ(stats->chips_bet, 85);  // 10 + 50 + 25

    // Cleanup
    d_StringDestroy(player.name);
}

TEST(placeBet_calculates_correct_average) {
    setup_stats();

    // Create minimal player
    Player_t player = {0};
    player.chips = 500;
    player.current_bet = 0;
    player.name = d_StringInit();
    d_StringSet(player.name, "TestPlayer");

    // Place consistent bets (reset current_bet between hands like the game does)
    PlaceBet(&player, 10);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    PlaceBet(&player, 10);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    PlaceBet(&player, 10);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    PlaceBet(&player, 10);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    PlaceBet(&player, 10);
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();
    player.current_bet = 0;  // Reset after hand resolves

    const GlobalStats_t* stats = Stats_GetCurrent();

    ASSERT_EQ(stats->chips_bet, 50);
    ASSERT_EQ(stats->turns_played, 5);
    ASSERT_EQ(Stats_GetAverageBet(), 10);  // Should be exactly 10, not 13!

    // Cleanup
    d_StringDestroy(player.name);
}

TEST(placeBet_double_down_tracking) {
    setup_stats();

    // Create minimal player
    Player_t player = {0};
    player.chips = 500;
    player.current_bet = 0;
    player.name = d_StringInit();
    d_StringSet(player.name, "TestPlayer");

    // Place initial bet of 10
    PlaceBet(&player, 10);

    // Simulate double-down: bet another 10 (current_bet should become 20)
    PlaceBet(&player, 10);  // This is the double-down (current_bet > 0)

    // Round resolves - record the TOTAL bet (20 chips)
    Stats_RecordChipsBet(player.current_bet);  // Simulate round resolution
    Stats_RecordTurnPlayed();

    const GlobalStats_t* stats = Stats_GetCurrent();

    // Total wagered should be 20 (initial 10 + double 10)
    ASSERT_EQ(stats->chips_bet, 20);

    // Average should be 20 / 1 turn = 20 (only ONE turn, because we doubled)
    ASSERT_EQ(stats->turns_played, 1);
    ASSERT_EQ(Stats_GetAverageBet(), 20);

    // Highest bet should be 20 (the doubled bet)
    ASSERT_EQ(stats->highest_bet, 20);

    // Current bet should be 20 (initial 10 + double 10)
    ASSERT_EQ(player.current_bet, 20);

    // Cleanup
    d_StringDestroy(player.name);
}

// ============================================================================
// TEST SUITE RUNNER
// ============================================================================

void run_stats_tests(void) {
    TEST_SUITE_BEGIN(Stats Tracking);

    RUN_TEST(bet_tracking_records_highest_bet);
    RUN_TEST(bet_tracking_calculates_average_bet);
    RUN_TEST(bet_tracking_handles_zero_turns);
    RUN_TEST(bet_tracking_handles_varying_bets);
    RUN_TEST(bet_peak_updates_only_on_higher_bets);

    RUN_TEST(chip_peak_tracks_highest_chips);
    RUN_TEST(chip_peak_tracks_lowest_chips);

    RUN_TEST(stats_reset_clears_all_bet_stats);
    RUN_TEST(stats_reset_initializes_chip_peaks);

    RUN_TEST(placeBet_updates_bet_peak);
    RUN_TEST(placeBet_calculates_correct_average);
    RUN_TEST(placeBet_double_down_tracking);

    TEST_SUITE_END();
}
