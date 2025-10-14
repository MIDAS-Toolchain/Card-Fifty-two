#ifndef SCENE_BLACKJACK_H
#define SCENE_BLACKJACK_H

#include "../common.h"

// ============================================================================
// BLACKJACK SCENE CONSTANTS
// ============================================================================

// Layout dimensions
#define LAYOUT_TOP_MARGIN       35  // Space for independent top bar (same as TOP_BAR_HEIGHT)
#define LAYOUT_BOTTOM_CLEARANCE 10
#define LAYOUT_GAP              0
#define TOP_BAR_HEIGHT          35
#define TITLE_AREA_HEIGHT       10
#define DEALER_AREA_HEIGHT      165
#define PLAYER_AREA_HEIGHT      240
#define BUTTON_AREA_HEIGHT      100

// Section internal layout (no more magic numbers!)
#define TEXT_LINE_HEIGHT        25   // Height of one line of text
#define SECTION_PADDING         20   // Padding from section top edge
#define ELEMENT_GAP             20   // Gap between elements (text→text, text→cards)

// Button dimensions
#define BUTTON_ROW_HEIGHT       100
#define BUTTON_GAP              20
#define BET_BUTTON_WIDTH        100
#define BET_BUTTON_HEIGHT       60
#define ACTION_BUTTON_WIDTH     120
#define ACTION_BUTTON_HEIGHT    60
#define DECK_BUTTON_WIDTH       100
#define DECK_BUTTON_HEIGHT      50

// Button counts
#define NUM_BET_BUTTONS         3
#define NUM_ACTION_BUTTONS      3
#define NUM_DECK_BUTTONS        2

// Bet amounts (Min/Med/Max system)
#define BET_AMOUNT_MIN          1
#define BET_AMOUNT_MED          5
#define BET_AMOUNT_MAX          10

// Player starting chips
#define PLAYER_STARTING_CHIPS   100

// Spacing constants
#define CARD_SPACING            120
#define TEXT_BUTTON_GAP         45

// Overlay opacity
#define OVERLAY_ALPHA           180

// Colors (palette-based)
#define TABLE_FELT_GREEN        ((aColor_t){37, 86, 46, 255})   // #25562e - dark green
#define TOP_BAR_BG              ((aColor_t){9, 10, 20, 255})    // #090a14 - almost black (matches main menu)

// Text colors (palette-based visual hierarchy)
#define COLOR_TITLE             ((aColor_t){235, 237, 233, 255})  // #ebede9 - off-white
#define COLOR_PLAYER_NAME       ((aColor_t){168, 202, 88, 255})  // #a8ca58 - yellow-green
#define COLOR_DEALER_NAME       ((aColor_t){207, 87, 60, 255})   // #cf573c - red-orange
#define COLOR_INFO_TEXT         ((aColor_t){168, 181, 178, 255}) // #a8b5b2 - light gray
#define COLOR_WIN               ((aColor_t){117, 167, 67, 255})  // #75a743 - green
#define COLOR_LOSE              ((aColor_t){165, 48, 48, 255})   // #a53030 - red
#define COLOR_PUSH              ((aColor_t){222, 158, 65, 255})  // #de9e41 - orange-yellow
#define COLOR_BLACKJACK         ((aColor_t){232, 193, 112, 255}) // #e8c170 - gold

// ============================================================================
// PUBLIC API
// ============================================================================

/**
 * InitBlackjackScene - Initialize the blackjack game scene
 *
 * Sets up the game table, initializes deck and hands, and sets delegates
 * Called from menu when player clicks "Play"
 */
void InitBlackjackScene(void);

#endif // SCENE_BLACKJACK_H
