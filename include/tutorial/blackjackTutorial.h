/*
 * Blackjack Tutorial Content
 * Defines tutorial steps for teaching Blackjack gameplay
 * Constitutional pattern: Event-driven tutorial progression
 */

#ifndef BLACKJACK_TUTORIAL_H
#define BLACKJACK_TUTORIAL_H

#include "tutorialSystem.h"
#include "../scenes/sceneBlackjack.h"

// ============================================================================
// BLACKJACK TUTORIAL API
// ============================================================================

/**
 * CreateBlackjackTutorial - Create Blackjack tutorial step chain
 *
 * @return TutorialStep_t* - First step of tutorial (caller must manage)
 *
 * Creates complete Blackjack tutorial:
 * 1. Welcome message
 * 2. Betting instructions (spotlight on bet buttons)
 * 3. Player turn instructions (spotlight on hit/stand buttons)
 * 4. Winning/losing explanation
 * 5. Completion message
 */
TutorialStep_t* CreateBlackjackTutorial(void);

/**
 * DestroyBlackjackTutorial - Cleanup Blackjack tutorial steps
 *
 * @param first_step - First step of tutorial chain
 *
 * Destroys entire tutorial step chain
 */
void DestroyBlackjackTutorial(TutorialStep_t* first_step);

#endif // BLACKJACK_TUTORIAL_H
