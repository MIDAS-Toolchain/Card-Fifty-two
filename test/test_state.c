/*
 * State Storage System Tests
 *
 * Tests for the typed table state storage system used in targeting mode.
 */

#include "test.h"
#include "../include/common.h"
#include "../include/stateStorage.h"
#include "../include/defs.h"

// ============================================================================
// STATE STORAGE TESTS
// ============================================================================

TEST(statedata_init) {
    GameStateData_t data;
    StateData_Init(&data);

    // Verify all tables are initialized
    ASSERT_NOT_NULL(data.int_values);
    ASSERT_NOT_NULL(data.bool_flags);
    ASSERT_NOT_NULL(data.dealer_phase);

    // Note: StateData cleanup happens in game lifecycle, not per-test
}

TEST(statedata_set_get_int) {
    GameStateData_t data;
    StateData_Init(&data);

    // Set and retrieve integer value
    StateData_SetInt(&data, "targeting_trinket_slot", 2);
    int value = StateData_GetInt(&data, "targeting_trinket_slot", -1);
    ASSERT_EQ(value, 2);

    // Test default value for missing key
    int missing = StateData_GetInt(&data, "nonexistent", 999);
    ASSERT_EQ(missing, 999);
}

TEST(statedata_set_get_bool) {
    GameStateData_t data;
    StateData_Init(&data);

    // Set and retrieve boolean value
    StateData_SetBool(&data, "targeting_active", true);
    bool value = StateData_GetBool(&data, "targeting_active", false);
    ASSERT_TRUE(value);

    // Test default value for missing key
    bool missing = StateData_GetBool(&data, "nonexistent", false);
    ASSERT_FALSE(missing);
}

TEST(statedata_overwrite_value) {
    GameStateData_t data;
    StateData_Init(&data);

    // Set initial value
    StateData_SetInt(&data, "counter", 10);
    ASSERT_EQ(StateData_GetInt(&data, "counter", 0), 10);

    // Overwrite with new value
    StateData_SetInt(&data, "counter", 20);
    ASSERT_EQ(StateData_GetInt(&data, "counter", 0), 20);
}

TEST(statedata_multiple_keys) {
    GameStateData_t data;
    StateData_Init(&data);

    // Store multiple int values
    StateData_SetInt(&data, "slot", 0);
    StateData_SetInt(&data, "player_id", 1);
    StateData_SetInt(&data, "cooldown", 3);

    // Verify all values are independent
    ASSERT_EQ(StateData_GetInt(&data, "slot", -1), 0);
    ASSERT_EQ(StateData_GetInt(&data, "player_id", -1), 1);
    ASSERT_EQ(StateData_GetInt(&data, "cooldown", -1), 3);

    // Store multiple bool values (can co-exist with ints in separate table)
    StateData_SetBool(&data, "flag_a", true);
    StateData_SetBool(&data, "flag_b", false);

    ASSERT_TRUE(StateData_GetBool(&data, "flag_a", false));
    ASSERT_FALSE(StateData_GetBool(&data, "flag_b", true));
}

TEST(statedata_targeting_simulation) {
    // Simulate real targeting mode usage
    GameStateData_t data;
    StateData_Init(&data);

    // Enter targeting mode
    StateData_SetInt(&data, "targeting_trinket_slot", 1);
    StateData_SetInt(&data, "targeting_player_id", 1);

    // Verify targeting state
    ASSERT_EQ(StateData_GetInt(&data, "targeting_trinket_slot", -1), 1);
    ASSERT_EQ(StateData_GetInt(&data, "targeting_player_id", -1), 1);

    // Exit targeting mode (values should still exist until cleared)
    int slot = StateData_GetInt(&data, "targeting_trinket_slot", -1);
    ASSERT_EQ(slot, 1);
}

// ============================================================================
// TEST SUITE RUNNER
// ============================================================================

void run_state_tests(void) {
    TEST_SUITE_BEGIN(State Storage System);

    RUN_TEST(statedata_init);
    RUN_TEST(statedata_set_get_int);
    RUN_TEST(statedata_set_get_bool);
    RUN_TEST(statedata_overwrite_value);
    RUN_TEST(statedata_multiple_keys);
    RUN_TEST(statedata_targeting_simulation);

    TEST_SUITE_END();
}
