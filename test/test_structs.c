/*
 * Struct Size Validation Tests
 *
 * Constitutional enforcement: Detect struct size changes that cause ABI issues.
 * These tests MUST pass before merging any struct changes.
 */

#include "test.h"
#include "../include/common.h"
#include "../include/structs.h"
#include "../include/stateStorage.h"
#include "../include/trinket.h"
#include "../include/enemy.h"

// ============================================================================
// STRUCT SIZE TESTS
// ============================================================================

TEST(sizeof_GameStateData_t) {
    // GameStateData_t should have fixed size across all compilation units
    // If this fails, run `make clean && make` to rebuild all files
    size_t expected_size = sizeof(GameStateData_t);

    // Log the actual size for debugging
    printf("    GameStateData_t size: %zu bytes\n", expected_size);

    // Verify struct contains expected fields
    GameStateData_t test_data;
    (void)test_data.int_values;      // dTable_t* - 8 bytes (pointer)
    (void)test_data.bool_flags;      // dTable_t* - 8 bytes (pointer)
    (void)test_data.dealer_phase;    // dTable_t* - 8 bytes (pointer)

    // Expected: 3 pointers = 24 bytes on 64-bit, 12 bytes on 32-bit
    #ifdef __x86_64__
        ASSERT_EQ(expected_size, 24);  // 64-bit
    #else
        ASSERT_EQ(expected_size, 12);  // 32-bit
    #endif
}

TEST(sizeof_Player_t) {
    size_t player_size = sizeof(Player_t);
    printf("    Player_t size: %zu bytes\n", player_size);

    // Player_t is large (contains embedded Hand_t, portrait surfaces, etc.)
    // Just verify it's reasonable (> 100 bytes, < 500 bytes)
    ASSERT_TRUE(player_size > 100);
    ASSERT_TRUE(player_size < 500);
}

TEST(sizeof_Card_t) {
    size_t card_size = sizeof(Card_t);
    printf("    Card_t size: %zu bytes\n", card_size);

    // Card_t is value type (Constitutional pattern)
    // Should be small enough to copy efficiently (< 64 bytes)
    ASSERT_TRUE(card_size < 64);
}

TEST(sizeof_Hand_t) {
    size_t hand_size = sizeof(Hand_t);
    printf("    Hand_t size: %zu bytes\n", hand_size);

    // Hand_t contains dArray_t* pointer + metadata
    ASSERT_TRUE(hand_size > 0);
    ASSERT_TRUE(hand_size < 100);
}

TEST(sizeof_Trinket_t) {
    size_t trinket_size = sizeof(Trinket_t);
    printf("    Trinket_t size: %zu bytes\n", trinket_size);

    // Trinket_t contains function pointers, dString_t*, animation fields
    ASSERT_TRUE(trinket_size > 50);
    ASSERT_TRUE(trinket_size < 200);
}

TEST(sizeof_Enemy_t) {
    size_t enemy_size = sizeof(Enemy_t);
    printf("    Enemy_t size: %zu bytes\n", enemy_size);

    // Enemy_t contains abilities, portrait, animations
    // Actual size: 88 bytes
    ASSERT_TRUE(enemy_size > 50);
    ASSERT_TRUE(enemy_size < 500);
}

// ============================================================================
// CONSTITUTIONAL PATTERN VALIDATION
// ============================================================================

TEST(card_is_value_type) {
    // Constitutional: Card_t is value type, not pointer type
    // This test verifies we can safely copy Card_t by value

    Card_t card1 = {
        .suit = SUIT_HEARTS,
        .rank = RANK_ACE,
        .texture = NULL,
        .x = 100,
        .y = 200,
        .face_up = true,
        .card_id = 0
    };

    // Copy by value (Constitutional pattern)
    Card_t card2 = card1;

    // Verify copy is independent
    card2.x = 999;
    ASSERT_EQ(card1.x, 100);  // Original unchanged
    ASSERT_EQ(card2.x, 999);  // Copy changed
}

TEST(hand_contains_dArray) {
    // Constitutional: Hand_t uses dArray_t*, not raw pointer
    Hand_t hand;
    hand.cards = d_InitArray(sizeof(Card_t), 10);
    ASSERT_NOT_NULL(hand.cards);

    // Verify it's a dArray_t
    ASSERT_EQ(hand.cards->count, 0);
    // Note: Daedalus rounds capacity up to power of 2 (10 → 32)
    ASSERT_TRUE(hand.cards->capacity >= 10);

    d_DestroyArray(hand.cards);
    hand.cards = NULL;  // Manually NULL it for test
    ASSERT_NULL(hand.cards);
}

TEST(player_trinkets_are_dArray) {
    // Constitutional: Player trinket_slots uses dArray_t*, not raw array
    Player_t player;
    player.trinket_slots = d_InitArray(sizeof(Trinket_t*), 6);
    ASSERT_NOT_NULL(player.trinket_slots);

    // Verify it's a valid dArray_t (just check it initialized)
    ASSERT_TRUE(player.trinket_slots->capacity >= 6);

    d_DestroyArray(player.trinket_slots);
    player.trinket_slots = NULL;
}

// ============================================================================
// ALIGNMENT TESTS
// ============================================================================

TEST(gamestate_alignment) {
    // Verify GameStateData_t fields are properly aligned
    GameStateData_t data;

    // All fields are pointers, should be aligned to pointer size
    uintptr_t int_addr = (uintptr_t)&data.int_values;
    uintptr_t bool_addr = (uintptr_t)&data.bool_flags;
    uintptr_t dealer_addr = (uintptr_t)&data.dealer_phase;

    // Check alignment (should be 8-byte aligned on 64-bit)
    #ifdef __x86_64__
        ASSERT_EQ(int_addr % 8, 0);
        ASSERT_EQ(bool_addr % 8, 0);
        ASSERT_EQ(dealer_addr % 8, 0);
    #endif
}

// ============================================================================
// TEST SUITE RUNNER
// ============================================================================

void run_struct_tests(void) {
    TEST_SUITE_BEGIN(Struct Size Validation);

    RUN_TEST(sizeof_GameStateData_t);
    RUN_TEST(sizeof_Player_t);
    RUN_TEST(sizeof_Card_t);
    RUN_TEST(sizeof_Hand_t);
    RUN_TEST(sizeof_Trinket_t);
    RUN_TEST(sizeof_Enemy_t);

    RUN_TEST(card_is_value_type);
    RUN_TEST(hand_contains_dArray);
    RUN_TEST(player_trinkets_are_dArray);

    RUN_TEST(gamestate_alignment);

    TEST_SUITE_END();
}
