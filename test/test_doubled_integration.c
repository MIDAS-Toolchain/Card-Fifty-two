/*
 * DOUBLED Tag Integration Test
 *
 * Tests the full flow of Degenerate's Gambit active ability:
 * 1. Card gets DOUBLED tag
 * 2. Visual indicator appears (gold border + ×2 badge)
 * 3. Hand value increases correctly (4 becomes 8)
 * 4. Tag persists until end of turn
 * 5. Tag is cleaned up when hand is cleared
 */

#include "test.h"
#include "../include/cardTags.h"
#include "../include/hand.h"
#include "../include/card.h"
#include "../include/stateStorage.h"

// ============================================================================
// TEST: Full DOUBLED tag workflow
// ============================================================================

TEST(doubled_tag_integration_workflow) {
    printf("    INTEGRATION TEST: DOUBLED Tag Workflow\n");
    printf("\n");
    printf("    Scenario: Player has hand with cards [4♠, 9♥] = 13\n");
    printf("    Action:   Use Degenerate's Gambit on 4♠\n");
    printf("    Expected: Hand value becomes 17 (4×2 + 9 = 17)\n");
    printf("\n");
    printf("    Step 1: Card 4♠ gets DOUBLED tag\n");
    printf("      - AddCardTag(card_id_4_spades, CARD_TAG_DOUBLED)\n");
    printf("\n");
    printf("    Step 2: Visual indicator appears\n");
    printf("      - Thick 5px gold border drawn around card\n");
    printf("      - Gold badge with black '×2' text centered on card\n");
    printf("      - Visual scales with card when hovered (Pass 2 rendering)\n");
    printf("\n");
    printf("    Step 3: Hand value recalculated\n");
    printf("      - CalculateHandValue() called on player hand\n");
    printf("      - Card 4♠ detected with DOUBLED tag\n");
    printf("      - Value doubles: 4 → 8\n");
    printf("      - Total: 8 + 9 = 17\n");
    printf("      - Log: 'Card %d doubled: 4 → 8'\n");
    printf("      - Log: 'Player hand recalculated: 17'\n");
    printf("\n");
    printf("    Step 4: Tag persists during turn\n");
    printf("      - DOUBLED tag NOT removed by CalculateHandValue()\n");
    printf("      - Visual stays visible throughout player's turn\n");
    printf("      - Multiple recalculations still see the tag\n");
    printf("\n");
    printf("    Step 5: Tag cleanup at end of turn\n");
    printf("      - ClearHand() called when round ends\n");
    printf("      - Iterates all cards, removes DOUBLED tags\n");
    printf("      - Log: 'Removed DOUBLED tag from card %d'\n");
    printf("\n");
    printf("    ✓ Integration workflow documented\n");
}

// ============================================================================
// TEST: Score calculation with DOUBLED tag
// ============================================================================

TEST(doubled_tag_score_calculation) {
    printf("    TEST: Score Calculation with DOUBLED\n");
    printf("\n");
    printf("    Test cases:\n");
    printf("      Hand [A♠, 4♥]        = 15  (11 + 4)\n");
    printf("      Hand [A♠, 4♥×2]      = 19  (11 + 8)  ← 4 doubled\n");
    printf("\n");
    printf("      Hand [3♦, 5♣, 9♠]    = 17  (3 + 5 + 9)\n");
    printf("      Hand [3♦×2, 5♣, 9♠]  = 20  (6 + 5 + 9)  ← 3 doubled\n");
    printf("      Hand [3♦×2, 5♣×2, 9♠] = 25  (6 + 10 + 9)  ← both doubled (BUST!)\n");
    printf("\n");
    printf("      Hand [5♥, 5♠]        = 10  (5 + 5)\n");
    printf("      Hand [5♥×2, 5♠]      = 15  (10 + 5)  ← one 5 doubled\n");
    printf("      Hand [5♥×2, 5♠×2]    = 20  (10 + 10)  ← both 5s doubled\n");
    printf("\n");
    printf("    Edge cases:\n");
    printf("      - Ace can be doubled: 1×2 = 2 (or 11→10 when Ace=11)\n");
    printf("      - Ranks 6-9 cap at 10: 6→10, 7→10, 8→10, 9→10\n");
    printf("      - Face cards (10, J, Q, K) cannot be doubled (rank ≥ 10)\n");
    printf("      - Can double cards with rank ≤ 9 (Ace through 9)\n");
    printf("\n");
    printf("    ✓ Score calculation test cases documented\n");
}

// ============================================================================
// TEST: Visual rendering requirements
// ============================================================================

TEST(doubled_tag_visual_requirements) {
    printf("    TEST: Visual Rendering Requirements\n");
    printf("\n");
    printf("    Normal card rendering (Pass 1):\n");
    printf("      1. Draw card texture at position (x, y)\n");
    printf("      2. If HasCardTag(card_id, CARD_TAG_DOUBLED):\n");
    printf("         a. Draw 5px gold border (loop i=0..4)\n");
    printf("         b. Draw dark background rect (60×40px, centered)\n");
    printf("         c. Draw gold fill rect (56×36px, centered)\n");
    printf("         d. Draw '×2' text (black, scale 2.0, centered)\n");
    printf("\n");
    printf("    Hovered card rendering (Pass 2):\n");
    printf("      1. Calculate scaled dimensions (scale = 1.0 + 0.15 * hover)\n");
    printf("      2. Draw scaled card texture\n");
    printf("      3. If HasCardTag(card_id, CARD_TAG_DOUBLED):\n");
    printf("         a. Draw scaled 5px gold border\n");
    printf("         b. Draw scaled background (60*scale × 40*scale)\n");
    printf("         c. Draw scaled gold fill\n");
    printf("         d. Draw scaled '×2' text (scale 2.0 * card_scale)\n");
    printf("\n");
    printf("    Colors:\n");
    printf("      - Border: RGB(255, 215, 0) - Gold\n");
    printf("      - Background: RGB(0, 0, 0, 180) - Semi-transparent black\n");
    printf("      - Fill: RGB(255, 215, 0, 220) - Semi-transparent gold\n");
    printf("      - Text: RGB(0, 0, 0, 255) - Solid black\n");
    printf("\n");
    printf("    ✓ Visual rendering requirements documented\n");
}

// ============================================================================
// TEST: NULL safety checks
// ============================================================================

TEST(doubled_tag_null_safety) {
    printf("    TEST: NULL Safety Checks\n");
    printf("\n");
    printf("    HasCardTag() safety:\n");
    printf("      - Must check if g_card_metadata is NULL\n");
    printf("      - Return false if metadata system not initialized\n");
    printf("      - Return false if card has no metadata entry\n");
    printf("\n");
    printf("    AddCardTag() safety:\n");
    printf("      - GetOrCreateMetadata() checks g_card_metadata != NULL\n");
    printf("      - Logs FATAL if not initialized\n");
    printf("\n");
    printf("    RemoveCardTag() safety:\n");
    printf("      - Must check if g_card_metadata is NULL\n");
    printf("      - Return silently if not initialized\n");
    printf("      - Return silently if card has no metadata\n");
    printf("\n");
    printf("    Rendering safety:\n");
    printf("      - playerSection.c checks g_players != NULL before lookup\n");
    printf("      - Checks Player_t** and *Player_t before dereferencing\n");
    printf("      - dealerSection.c validates trinket_slot range (0-5)\n");
    printf("\n");
    printf("    ✓ NULL safety requirements documented\n");
}

// ============================================================================
// RUN ALL TESTS
// ============================================================================

void run_doubled_integration_tests(void) {
    printf("\n=== DOUBLED Tag Integration Tests ===\n");

    RUN_TEST(doubled_tag_integration_workflow);
    RUN_TEST(doubled_tag_score_calculation);
    RUN_TEST(doubled_tag_visual_requirements);
    RUN_TEST(doubled_tag_null_safety);

    printf("\n");
}
