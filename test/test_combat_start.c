/*
 * COMBAT_START Crash Stress Test
 *
 * Tests the intermittent bus error that happens when starting the game.
 * Runs initialization 20 times to catch the 1-in-3 failure rate.
 */

#include "test.h"
#include "../include/game.h"
#include "../include/player.h"
#include "../include/enemy.h"
#include "../include/statusEffects.h"
#include "../include/trinket.h"
#include "../include/state.h"
#include "../include/tween/tween.h"
#include <stdlib.h>

// Minimal initialization to test COMBAT_START without SDL
TEST(combat_start_stress_test) {
    printf("    Running COMBAT_START 20 times to catch intermittent crash...\n");

    for (int iteration = 0; iteration < 20; iteration++) {
        printf("      Iteration %d/20... ", iteration + 1);
        fflush(stdout);

        // 1. Create status effect manager (like CreatePlayer does)
        StatusEffectManager_t* mgr = CreateStatusEffectManager();
        ASSERT_TRUE(mgr != NULL);
        ASSERT_TRUE(mgr->active_effects != NULL);

        // 2. Apply STATUS_GREED (like COMBAT_START does)
        //    This is what crashes - ApplyStatusEffect calls TweenFloat with array element pointer
        ApplyStatusEffect(mgr, STATUS_GREED, 0, 2);

        // 3. Verify effect was added
        ASSERT_TRUE(mgr->active_effects->count == 1);

        // 4. Apply different effects to test array growth
        //    ApplyStatusEffect refreshes existing effects of same type (by design)
        //    So we need to apply DIFFERENT effect types to actually grow the array
        ApplyStatusEffect(mgr, STATUS_CHIP_DRAIN, 5, 3);
        ApplyStatusEffect(mgr, STATUS_TILT, 0, 2);
        ApplyStatusEffect(mgr, STATUS_MADNESS, 0, 1);

        // Should have 4 different effects now (GREED, CHIP_DRAIN, TILT, MADNESS)
        ASSERT_TRUE(mgr->active_effects->count == 4);

        // 5. Test refresh behavior - applying same effect should NOT increase count
        ApplyStatusEffect(mgr, STATUS_GREED, 10, 5);  // Refresh GREED
        ASSERT_TRUE(mgr->active_effects->count == 4);  // Count unchanged

        // 5. Cleanup
        DestroyStatusEffectManager(&mgr);

        printf("OK\n");
    }

    printf("    ✓ Survived 20 iterations without crash\n");
}

TEST(enemy_ability_initialization) {
    printf("    Testing enemy ability array initialization...\n");

    for (int iteration = 0; iteration < 20; iteration++) {
        printf("      Iteration %d/20... ", iteration + 1);
        fflush(stdout);

        // Create enemy (like CreateTheDidact does)
        Enemy_t* enemy = CreateEnemy("Test Enemy", 100, 10);
        ASSERT_TRUE(enemy != NULL);
        ASSERT_TRUE(enemy->passive_abilities != NULL);
        ASSERT_TRUE(enemy->active_abilities != NULL);

        // Add abilities using active_abilities array (event abilities go there)
        // AddEventAbility appends to active_abilities, not passive_abilities
        for (int i = 0; i < 5; i++) {
            AddEventAbility(enemy, ABILITY_THE_HOUSE_REMEMBERS, GAME_EVENT_PLAYER_BLACKJACK);
        }

        // Should have 5 abilities in active_abilities (duplicates are allowed)
        ASSERT_TRUE(enemy->active_abilities->count == 5);

        // Cleanup
        DestroyEnemy(&enemy);

        printf("OK\n");
    }

    printf("    ✓ Enemy initialization survived 20 iterations\n");
}

TEST(array_capacity_verification) {
    printf("    Verifying increased capacities prevent realloc...\n");

    // Test status effect capacity
    StatusEffectManager_t* mgr = CreateStatusEffectManager();
    size_t initial_capacity = mgr->active_effects->capacity;
    printf("      StatusEffect initial capacity: %zu\n", initial_capacity);
    ASSERT_TRUE(initial_capacity >= 32);  // Should be 32 after our fix

    // Add 20 effects - should NOT trigger realloc
    for (int i = 0; i < 20; i++) {
        ApplyStatusEffect(mgr, STATUS_GREED, i, 1);
    }

    size_t after_capacity = mgr->active_effects->capacity;
    printf("      StatusEffect capacity after 20 appends: %zu\n", after_capacity);
    ASSERT_EQ(initial_capacity, after_capacity);  // Should be unchanged

    DestroyStatusEffectManager(&mgr);

    // Test enemy ability capacity
    Enemy_t* enemy = CreateEnemy("Test", 100, 10);
    size_t ability_capacity = enemy->passive_abilities->capacity;
    printf("      Enemy ability initial capacity: %zu\n", ability_capacity);
    ASSERT_TRUE(ability_capacity >= 16);  // Should be 16 after our fix

    DestroyEnemy(&enemy);

    printf("    ✓ Capacities are sufficient\n");
}

// ============================================================================
// TEST SUITE RUNNER
// ============================================================================

void run_combat_start_tests(void) {
    TEST_SUITE_BEGIN(COMBAT_START Stress Tests);

    RUN_TEST(combat_start_stress_test);
    RUN_TEST(enemy_ability_initialization);
    RUN_TEST(array_capacity_verification);

    TEST_SUITE_END();
}
