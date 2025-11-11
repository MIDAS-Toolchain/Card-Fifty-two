/*
 * Blackjack Tutorial Content Implementation
 */

#include "../../include/tutorial/blackjackTutorial.h"
#include "../../include/game.h"
#include "../../include/scenes/sceneBlackjack.h"  // For layout constants

// ============================================================================
// TUTORIAL STEP CREATION
// ============================================================================

TutorialStep_t* CreateBlackjackTutorial(void) {
    // Calculate actual button area position (using layout constants)
    int button_area_y = LAYOUT_TOP_MARGIN + DEALER_AREA_HEIGHT;
    int buttons_y = button_area_y + ACTION_PANEL_Y_OFFSET + 95; 
    int buttons_x = GAME_AREA_X + ACTION_PANEL_LEFT_MARGIN;

    // Step 1: Betting tutorial (3 bet buttons)
    int bet_buttons_width = (BET_BUTTON_WIDTH * NUM_BET_BUTTONS) + (BUTTON_GAP * (NUM_BET_BUTTONS - 1));
    TutorialSpotlight_t bet_spotlight = {
        .type = SPOTLIGHT_RECTANGLE,
        .x = buttons_x,
        .y = buttons_y,
        .w = bet_buttons_width,
        .h = BET_BUTTON_HEIGHT,
        .show_arrow = true
    };

    // Wait for state change from BETTING to DEALING (bet placed)
    TutorialListener_t bet_listener = {
        .event_type = TUTORIAL_EVENT_STATE_CHANGE,
        .event_data = (void*)(intptr_t)STATE_DEALING,
        .triggered = false
    };

    TutorialStep_t* step1 = CreateTutorialStep(
        "Place your bet!\n\n"
        "Min/Med/Max at 1, 5, and 10\n\n"
        "You start with 100 chips.\n"
        "Place a bet to continue",
        bet_spotlight,
        bet_listener,
        false,  // Not final step
        (SCREEN_HEIGHT - 250) / 2  // CENTER - vertically center the dialogue
    );

    // Step 2: Player turn tutorial (3 action buttons)
    int action_buttons_width = (ACTION_BUTTON_WIDTH * NUM_ACTION_BUTTONS) + (BUTTON_GAP * (NUM_ACTION_BUTTONS - 1));
    TutorialSpotlight_t action_spotlight = {
        .type = SPOTLIGHT_RECTANGLE,
        .x = buttons_x,
        .y = buttons_y,
        .w = action_buttons_width,
        .h = ACTION_BUTTON_HEIGHT,
        .show_arrow = true
    };

    // Manual advance - triggered when player takes any action (hit/stand/double)
    TutorialListener_t action_listener = {
        .event_type = TUTORIAL_EVENT_NONE,
        .event_data = NULL,
        .triggered = false
    };

    TutorialStep_t* step2 = CreateTutorialStep(
        "HIT: Draw another card (H)\n"
        "STAND: Keep your current hand (S)\n"
        "DOUBLE: Double bet + draw one (D)\n\n"
        "Make your choice",
        action_spotlight,
        action_listener,
        false,  // Not final step
        60  // TOP - avoid blocking player cards/score
    );

    // Step 3: Completion
    TutorialSpotlight_t no_spotlight = {
        .type = SPOTLIGHT_NONE,
        .x = 0, .y = 0, .w = 0, .h = 0,
        .show_arrow = false
    };

    TutorialListener_t manual_advance = {
        .event_type = TUTORIAL_EVENT_NONE,
        .event_data = NULL,
        .triggered = false
    };

    TutorialStep_t* step3 = CreateTutorialStep(
        "Congrats, bitch!\n\n"
        "You're ready to play Blackjack.\n\n"
        "Press ESC anytime to pause.",
        no_spotlight,
        manual_advance,
        true,  // FINAL STEP - shows "Finish" instead of "Skip"
        (SCREEN_HEIGHT - 200) / 2  // CENTER - back to center for final message
    );

    // Link steps together
    LinkTutorialSteps(step1, step2);
    LinkTutorialSteps(step2, step3);

    d_LogInfo("Blackjack tutorial created (3 steps)");
    return step1;
}

void DestroyBlackjackTutorial(TutorialStep_t* first_step) {
    if (!first_step) return;

    TutorialStep_t* current = first_step;
    TutorialStep_t* next = NULL;

    // Walk through step chain and destroy each
    while (current) {
        next = current->next_step;
        DestroyTutorialStep(&current);
        current = next;
    }

    d_LogInfo("Blackjack tutorial destroyed");
}
