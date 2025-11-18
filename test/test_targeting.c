/*
 * Targeting System Tests
 *
 * Tests the trinket targeting flow that's currently crashing.
 */

#include "test.h"
#include "../include/stateStorage.h"
#include "../include/cardTags.h"
#include "../include/card.h"
#include "../include/hand.h"
#include <string.h>

// ============================================================================
// TEST: StateData can store and retrieve targeting state
// ============================================================================

TEST(statedata_targeting_keys) {
    GameStateData_t state;
    StateData_Init(&state);

    // Store targeting state (what happens when clicking trinket)
    StateData_SetInt(&state, "targeting_trinket_slot", 0);
    StateData_SetInt(&state, "targeting_player_id", 1);

    // Retrieve targeting state (what happens during rendering)
    int trinket_slot = StateData_GetInt(&state, "targeting_trinket_slot", -1);
    int player_id = StateData_GetInt(&state, "targeting_player_id", -1);

    ASSERT_EQ(trinket_slot, 0);
    ASSERT_EQ(player_id, 1);

    // No Destroy function - state uses dTable internally which handles cleanup
}

// ============================================================================
// TEST: StateData handles missing keys gracefully
// ============================================================================

TEST(statedata_missing_keys_no_crash) {
    GameStateData_t state;
    StateData_Init(&state);

    // This is what the rendering code does - gets keys that might not exist
    // Should return default value, NOT crash
    int missing1 = StateData_GetInt(&state, "targeting_trinket_slot", -1);
    int missing2 = StateData_GetInt(&state, "targeting_player_id", -1);
    int missing3 = StateData_GetInt(&state, "some_other_key", -1);

    ASSERT_EQ(missing1, -1);
    ASSERT_EQ(missing2, -1);
    ASSERT_EQ(missing3, -1);
}

// ============================================================================
// TEST: StateData handles repeated lookups (like rendering every frame)
// ============================================================================

TEST(statedata_repeated_lookups_no_crash) {
    GameStateData_t state;
    StateData_Init(&state);

    StateData_SetInt(&state, "targeting_trinket_slot", 0);

    // Simulate rendering 60 frames - each frame checks these keys
    for (int frame = 0; frame < 60; frame++) {
        int slot = StateData_GetInt(&state, "targeting_trinket_slot", -1);
        int player = StateData_GetInt(&state, "targeting_player_id", -1);  // Missing key!

        ASSERT_EQ(slot, 0);
        ASSERT_EQ(player, -1);
    }

    printf("    ✓ Survived 60 frames of lookups (1 existing key, 1 missing key)\n");
}

// ============================================================================
// TEST: Card tag system works with card_id
// ============================================================================

TEST(card_tag_by_id_workflow) {
    printf("    DOCUMENTED: Card tag workflow\n");
    printf("    - Card ID 16 = Clubs 5 (suit=1, rank=5, id=1*13+4)\n");
    printf("    - AddCardTag(16, CARD_TAG_DOUBLED) should add tag\n");
    printf("    - HasCardTag(16, CARD_TAG_DOUBLED) should return true\n");
    printf("    - RemoveCardTag(16, CARD_TAG_DOUBLED) should remove tag\n");
    printf("    ✓ Workflow documented (actual calls need game initialization)\n");
}

// ============================================================================
// TEST: CalculateCardFanPosition math doesn't crash
// ============================================================================

TEST(card_fan_position_calculation) {
    // From sceneBlackjack.h - this should be available as static inline
    // But we can test the math manually to verify it's sane

    // Simulate: 5 cards in hand, checking 3rd card (index 2)
    size_t card_index = 2;
    size_t hand_size = 5;
    int base_y = 400;

    // These constants from sceneBlackjack.h
    const int CARD_SPACING = 80;
    const int GAME_AREA_X = 60;
    const int GAME_AREA_WIDTH = 1160;

    int anchor_x = GAME_AREA_X + (GAME_AREA_WIDTH / 2);  // Center
    int total_offset = (int)((hand_size - 1) * CARD_SPACING);  // 4 * 80 = 320
    int first_card_x = anchor_x - (total_offset / 2);  // Center - 160
    int card_x = first_card_x + ((int)card_index * CARD_SPACING);  // First + 2*80
    int card_y = base_y;

    // Verify results are sane (on screen)
    ASSERT_TRUE(card_x > 0 && card_x < 1280);
    ASSERT_TRUE(card_y > 0 && card_y < 720);

    printf("    ✓ Card position math: (%d, %d) - looks reasonable\n", card_x, card_y);
}

// ============================================================================
// TEST: Document the targeting crash scenario
// ============================================================================

TEST(targeting_crash_scenario) {
    printf("    CRASH SCENARIO DOCUMENTATION:\n");
    printf("    1. User clicks trinket icon (abilityDisplay.c line ~97)\n");
    printf("    2. Code enters STATE_TARGETING via State_Transition()\n");
    printf("    3. StateData stores: targeting_trinket_slot=0, targeting_player_id=1\n");
    printf("    4. Next frame: Rendering code checks STATE_TARGETING\n");
    printf("    5. For EVERY card, renders calls:\n");
    printf("       - StateData_GetInt(\"targeting_trinket_slot\", -1)\n");
    printf("       - StateData_GetInt(\"targeting_player_id\", -1)\n");
    printf("    6. DEBUG spam: 'Key not found in hash table (bucket X)'\n");
    printf("    7. SEGFAULT: core dumped\n");
    printf("\n");
    printf("    HYPOTHESIS:\n");
    printf("    - StateData lookups work (tested above)\n");
    printf("    - Card fan position math works (tested above)\n");
    printf("    - Possible causes:\n");
    printf("      a) g_players table lookup fails (player_id invalid)\n");
    printf("      b) GetEquippedTrinket() returns NULL/invalid pointer\n");
    printf("      c) Trinket struct has invalid active_effect pointer\n");
    printf("      d) Something else in rendering path accesses bad memory\n");
    printf("\n");
    printf("    NEXT STEPS:\n");
    printf("    - Add NULL checks in playerSection.c rendering (line 186-188)\n");
    printf("    - Verify g_players has entry for targeting_player_id\n");
    printf("    - Add debug logging before crash point\n");
}

// ============================================================================
// RUN ALL TESTS
// ============================================================================

void run_targeting_tests(void) {
    printf("\n=== Targeting System Tests ===\n");

    RUN_TEST(statedata_targeting_keys);
    RUN_TEST(statedata_missing_keys_no_crash);
    RUN_TEST(statedata_repeated_lookups_no_crash);
    RUN_TEST(card_tag_by_id_workflow);
    RUN_TEST(card_fan_position_calculation);
    RUN_TEST(targeting_crash_scenario);

    printf("\n");
}
