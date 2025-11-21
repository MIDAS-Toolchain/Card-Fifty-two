/*
 * Blackjack Tutorial Content Implementation
 */

#include "../../include/tutorial/blackjackTutorial.h"
#include "../../include/game.h"
#include "../../include/scenes/sceneBlackjack.h"  // For layout constants
#include "../../include/scenes/components/abilityDisplay.h"  // For ABILITY_CARD_* constants

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

    // Wait for state change from BETTING to DEALING (bet placed)
    TutorialListener_t bet_listener = {
        .event_type = TUTORIAL_EVENT_STATE_CHANGE,
        .event_data = (void*)(intptr_t)STATE_DEALING,
        .triggered = false
    };

    TutorialArrow_t no_arrow = {.enabled = false};

    // Arrow pointing from dialogue bottom to bet buttons center
    TutorialArrow_t bet_buttons_arrow = {
        .enabled = true,
        .from_x = (250 / 2) - 32,  // Center of dialogue width minus 32px offset
        .from_y = 200,      // Slightly higher than bottom edge (was 250)
        .to_x = buttons_x + (bet_buttons_width / 2),  // Center of bet buttons
        .to_y = buttons_y
    };

    TutorialStep_t* step1 = CreateTutorialStep(
        "Place your bet.\n"
        "Min (1), Med (2), or Max (3).\n\n"
        "The dealer is watching.\n\n",
        bet_listener,
        false,  // Not final step
        -32,  // Move 32px to the left
        (SCREEN_HEIGHT - 250) / 2,  // CENTER - vertically center the dialogue
        STATE_BETTING,  // This step shows during betting
        bet_buttons_arrow,
        false  // Don't advance immediately
    );

    // Step 2: Player turn tutorial (3 action buttons)
    int action_buttons_width = (ACTION_BUTTON_WIDTH * NUM_ACTION_BUTTONS) + (BUTTON_GAP * (NUM_ACTION_BUTTONS - 1));

    // Auto-advance when player takes action (state changes from PLAYER_TURN)
    // We listen for transition to DEALER_TURN (state 5)
    TutorialListener_t action_listener = {
        .event_type = TUTORIAL_EVENT_STATE_CHANGE,
        .event_data = (void*)(intptr_t)STATE_DEALER_TURN,
        .triggered = false
    };

    // Arrow pointing from dialogue bottom to action buttons center
    TutorialArrow_t action_buttons_arrow = {
        .enabled = true,
        .from_x = 250 / 2,  // Center of dialogue width (DIALOGUE_WIDTH / 2)
        .from_y = 200,      // Same as step 1 arrow height
        .to_x = buttons_x + (action_buttons_width / 2),  // Center of action buttons
        .to_y = buttons_y
    };

    TutorialStep_t* step2 = CreateTutorialStep(
        "HIT to draw. (1)\n"
        "STAND to wait. (2)\n"
        "DOUBLE to escalate. (3)\n\n",
        action_listener,
        false,  // Not final step
        0,  // Centered horizontally (where step 1 USED to be)
        (SCREEN_HEIGHT - 250) / 2,  // CENTER - same position step 1 used to have
        STATE_PLAYER_TURN,  // This step shows during player turn
        action_buttons_arrow,
        false  // Don't advance immediately
    );

    // Step 3: Betting Power (chips) explanation
    // Calculate chips display position in left sidebar (FlexBox items 0-1)
    int chips_x = 20;  // Left edge with padding
    int chips_y = LAYOUT_TOP_MARGIN + 20;  // Top of sidebar
    int chips_width = SIDEBAR_WIDTH - 40;  // Full sidebar width minus padding
    int chips_height = 60;  // Cover "Betting Power" label + chips value

    // Use specific event data to differentiate chips hover from bet hover
    static int chips_hover_id = 3;
    TutorialListener_t hover_chips_listener = {
        .event_type = TUTORIAL_EVENT_HOVER,
        .event_data = (void*)(intptr_t)chips_hover_id,  // Specific to chips hover
        .triggered = false
    };

    // Arrow pointing from dialogue left edge to chips display center
    TutorialArrow_t chips_arrow = {
        .enabled = true,
        .from_x = 0,  // Left edge of dialogue box
        .from_y = 125,  // Middle of dialogue height
        .to_x = chips_x + (chips_width / 2),  // Center of chips area
        .to_y = chips_y + (chips_height / 2)
    };

    TutorialStep_t* step3 = CreateTutorialStep(
        "Chips are your life force.\n\n"
        "Don't lose them all.\n"
        "Hover your chips to learn more.",
        hover_chips_listener,
        false,  // Not final step
        0,  // Centered horizontally
        450,  // Y position (moved up from 550)
        STATE_BETTING,  // This step shows during betting
        chips_arrow,
        false  // Don't advance immediately
    );

    // Step 4: Active Bet explanation
    // Calculate Active Bet display position in left sidebar
    // Note: FlexBox items 0-1 = "Betting Power" + chips, items 2-3 = "Active Bet" + bet value
    int bet_x = 20;  // Left edge with padding
    int bet_y = LAYOUT_TOP_MARGIN + 80;  // Below "Betting Power" section
    int bet_width = SIDEBAR_WIDTH - 40;  // Full sidebar width minus padding
    int bet_height = 60;  // Cover "Active Bet" label + bet value only

    // Use specific event data to differentiate bet hover from ability hover
    static int bet_hover_id = 1;
    TutorialListener_t hover_bet_listener = {
        .event_type = TUTORIAL_EVENT_HOVER,
        .event_data = (void*)(intptr_t)bet_hover_id,  // Specific to bet hover
        .triggered = false
    };

    // Arrow pointing from dialogue left edge to bet display center (offset right 32px from chips arrow)
    TutorialArrow_t bet_arrow = {
        .enabled = true,
        .from_x = 0,  // 32px right offset from chips arrow (which was at 0)
        .from_y = 125,  // Middle of dialogue height
        .to_x = bet_x + (bet_width / 2),  // Center of bet area
        .to_y = bet_y + (bet_height / 2)
    };

    TutorialStep_t* step4 = CreateTutorialStep(
        "Chips are also your betting power.\n\n"
        "The house tracks everything.\n\n"
        "Nothing is forgotten.",
        hover_bet_listener,
        false,  // Not final step
        0,  // Centered horizontally
        450,  // Y position
        STATE_BETTING,  // Wait for betting state before showing step 4
        bet_arrow,
        false  // Don't advance immediately
    );

    // Step 5: Enemy abilities explanation
    // Calculate enemy ability display position (right of sidebar, vertical column)
    int abilities_x = 290;  // SIDEBAR_WIDTH (280) + 10px margin
    int abilities_y = 70;   // Below top bar + margin
    int abilities_height = (ABILITY_CARD_HEIGHT * 3) + (ABILITY_CARD_SPACING * 2);  // 3 cards max

    // Use specific event data to differentiate ability hover from bet/chips hover
    static int ability_hover_id = 2;
    TutorialListener_t hover_abilities_listener = {
        .event_type = TUTORIAL_EVENT_HOVER,
        .event_data = (void*)(intptr_t)ability_hover_id,  // Specific to ability hover
        .triggered = false
    };

    // Arrow pointing from dialogue left edge to abilities icons
    TutorialArrow_t abilities_arrow = {
        .enabled = true,
        .from_x = 0, 
        .from_y = 125,  // Middle of dialogue height
        .to_x = abilities_x + (ABILITY_CARD_WIDTH / 2),  // Center of ability icons
        .to_y = abilities_y + (abilities_height / 2)
    };

    TutorialStep_t* step5 = CreateTutorialStep(
        "The dealer's abilities\n"
        "trigger automatically.\n\n"
        "Study them. Each one changes\n"
        "how you must play.",
        hover_abilities_listener,
        false,  // Not final step
        330,  // Move right 330px
        ((SCREEN_HEIGHT - 250) / 2) + 250,  // Move down 250px from center
        STATE_BETTING,  // Show this step during betting (like steps 3 and 4)
        abilities_arrow,
        false  // Show dialogue immediately on hover (like steps 3 and 4)
    );

    // Step 6: Class trinket usage
    // Calculate class trinket position (bottom-right area)
    int class_trinket_x = SCREEN_WIDTH - CLASS_TRINKET_SIZE - CLASS_TRINKET_GAP - (3 * TRINKET_SLOT_SIZE) - (2 * TRINKET_SLOT_GAP) - TRINKET_UI_PADDING;
    int class_trinket_y = SCREEN_HEIGHT - (2 * TRINKET_SLOT_SIZE) - TRINKET_SLOT_GAP - TRINKET_UI_PADDING;

    // Wait for trinket click (FUNCTION_CALL event)
    TutorialListener_t trinket_listener = {
        .event_type = TUTORIAL_EVENT_FUNCTION_CALL,
        .event_data = (void*)0x1000,  // Unique ID for "class_trinket_clicked"
        .triggered = false
    };

    // Arrow pointing from dialogue to class trinket center
    TutorialArrow_t trinket_arrow = {
        .enabled = true,
        .from_x = 250 / 2,  // Center of dialogue
        .from_y = 125,      // Middle height
        .to_x = class_trinket_x + (CLASS_TRINKET_SIZE / 2),
        .to_y = class_trinket_y + (CLASS_TRINKET_SIZE / 2)
    };

    TutorialStep_t* step6 = CreateTutorialStep(
        "This power is yours.\n\n"
        "Use it. You'll need every advantage\n"
        "to survive what's coming.",
        trinket_listener,
        false,  // Not final step
        0,  // Centered horizontally
        (SCREEN_HEIGHT - 250) / 2,  // Centered vertically
        STATE_PLAYER_TURN,  // Show during player turn
        trinket_arrow,
        true  // ADVANCE IMMEDIATELY (unblock input, but wait for PLAYER_TURN to show dialogue)
    );

    // Step 7: Completion
    TutorialListener_t manual_advance_final = {
        .event_type = TUTORIAL_EVENT_NONE,
        .event_data = NULL,
        .triggered = false
    };

    TutorialStep_t* step7 = CreateTutorialStep(
        "Good.\n\n"
        "You understand the rules.\n\n"
        "Now let's see if you can survive them.",
        manual_advance_final,
        true,  // FINAL STEP - shows "Finish" instead of "Skip"
        0,  // Centered horizontally
        (SCREEN_HEIGHT - 200) / 2,  // CENTER - back to center for final message
        STATE_BETTING,  // Wait for betting state before showing
        no_arrow,
        true  // ADVANCE IMMEDIATELY (advance from step 6 right away, but wait for BETTING to show)
    );

    // Link steps together
    LinkTutorialSteps(step1, step2);
    LinkTutorialSteps(step2, step3);
    LinkTutorialSteps(step3, step4);
    LinkTutorialSteps(step4, step5);
    LinkTutorialSteps(step5, step6);
    LinkTutorialSteps(step6, step7);

    d_LogInfo("Blackjack tutorial created (7 steps)");
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
