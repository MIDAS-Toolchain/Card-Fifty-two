#ifndef SCENE_BLACKJACK_H
#define SCENE_BLACKJACK_H

#include "../common.h"

// ============================================================================
// BLACKJACK SCENE CONSTANTS
// ============================================================================

// Layout dimensions
#define LAYOUT_TOP_MARGIN       45  // Space for independent top bar (same as TOP_BAR_HEIGHT)
#define LAYOUT_BOTTOM_CLEARANCE 0   // No bottom margin - sidebar extends to screen bottom
#define LAYOUT_GAP              0
#define TOP_BAR_HEIGHT          45
#define SIDEBAR_WIDTH           280  // Left sidebar width
#define GAME_AREA_X             (SIDEBAR_WIDTH)  // Game area starts after sidebar
#define GAME_AREA_WIDTH         (SCREEN_WIDTH - SIDEBAR_WIDTH)
#define TITLE_AREA_HEIGHT       10
#define DEALER_AREA_HEIGHT      185
#define PLAYER_AREA_HEIGHT      240
#define BUTTON_AREA_HEIGHT      100

// Section internal layout
#define TEXT_LINE_HEIGHT        25   // Height of one line of text
#define SECTION_PADDING         20   // Padding from section top edge
#define ELEMENT_GAP             20   // Gap between elements (text→text, text→cards)
#define ACTION_PANEL_Y_OFFSET   310  // Vertical offset for action panel buttons from section top
#define ACTION_PANEL_LEFT_MARGIN 52  // Left margin for action panel buttons

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
#define CARD_SPACING            40  // Horizontal offset between cards in fanned hand (poker-style overlap)
#define TEXT_BUTTON_GAP         0

// Overlay opacity
#define OVERLAY_ALPHA           180

// Trinket UI (class trinket + 3x2 grid, bottom-right corner)
#define TRINKET_SLOT_SIZE       64
#define TRINKET_SLOT_GAP        8
#define CLASS_TRINKET_SIZE      96
#define CLASS_TRINKET_GAP       12
#define TRINKET_UI_PADDING      20
// Class trinket on LEFT, then gap, then 3x2 grid
#define CLASS_TRINKET_X         (SCREEN_WIDTH - CLASS_TRINKET_SIZE - CLASS_TRINKET_GAP - (3 * TRINKET_SLOT_SIZE) - (2 * TRINKET_SLOT_GAP) - TRINKET_UI_PADDING)
#define CLASS_TRINKET_Y         (SCREEN_HEIGHT - (2 * TRINKET_SLOT_SIZE) - TRINKET_SLOT_GAP - TRINKET_UI_PADDING)
#define TRINKET_UI_X            (CLASS_TRINKET_X + CLASS_TRINKET_SIZE + CLASS_TRINKET_GAP)
#define TRINKET_UI_Y            (SCREEN_HEIGHT - (2 * TRINKET_SLOT_SIZE) - TRINKET_SLOT_GAP - TRINKET_UI_PADDING)

// ============================================================================
// CARD LAYOUT HELPERS
// ============================================================================

/**
 * CalculateCardFanPosition - Calculate position for card in fanned hand
 *
 * Uses fixed anchor point (center of game area) with symmetric fan layout.
 * This ensures position doesn't shift when new cards are added.
 *
 * @param card_index - Index of card in hand (0 = first card)
 * @param hand_size - Total number of cards in hand
 * @param base_y - Base Y position for this hand
 * @param out_x - Output: X position for this card
 * @param out_y - Output: Y position for this card
 *
 * Example:
 *   int x, y;
 *   CalculateCardFanPosition(2, 5, 410, &x, &y);  // 3rd card of 5
 */
static inline void CalculateCardFanPosition(size_t card_index, size_t hand_size,
                                             int base_y, int* out_x, int* out_y) {
    // Fixed anchor point at center of game area
    int anchor_x = GAME_AREA_X + (GAME_AREA_WIDTH / 2);

    // Calculate total width of fanned hand
    int total_offset = (int)((hand_size - 1) * CARD_SPACING);

    // First card starts left of center by half the total width
    int first_card_x = anchor_x - (total_offset / 2);

    // This card's position = first + (index * spacing)
    *out_x = first_card_x + ((int)card_index * CARD_SPACING);
    *out_y = base_y;
}

// Enemy portrait positioning
#define ENEMY_PORTRAIT_X_OFFSET -32     // Offset portrait to right from center
#define ENEMY_PORTRAIT_Y_OFFSET 48     // Portrait starts at screen top
#define ENEMY_PORTRAIT_SCALE    0.85  // Absolute scale (1.0 = original size, 0.5 = half size)

// Combat UI positioning (enemy HP bar and damage numbers)
#define ENEMY_HP_BAR_X_OFFSET   -300    // Offset from center (0 = centered, + = right, - = left)
#define ENEMY_HP_BAR_Y          45   // Y position (below top bar which ends at 45)
#define DAMAGE_NUMBER_Y_OFFSET  10   // Spawn Y offset above HP bar

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

/**
 * TweenEnemyHP - Start smooth HP bar drain animation for enemy
 *
 * Tweens enemy->display_hp to enemy->current_hp over 0.6s
 * Call this after applying damage with TakeDamage()
 *
 * @param enemy - Enemy to animate HP for
 */
void TweenEnemyHP(Enemy_t* enemy);

/**
 * TweenPlayerHP - Start smooth HP bar drain animation for player
 *
 * Tweens player->display_chips to player->chips over 0.6s
 * Call this after reducing player chips (chips = HP in combat)
 *
 * @param player - Player to animate HP for
 */
void TweenPlayerHP(Player_t* player);

/**
 * SpawnDamageNumber - Create floating damage/healing number
 *
 * Spawns a floating text number that rises and fades over 1.0s
 * Numbers are rendered above enemy portrait
 *
 * @param damage - Amount to display (absolute value)
 * @param world_x - X position to spawn at (center of text)
 * @param world_y - Y position to spawn at (will rise 50px)
 * @param is_healing - true for green "+X", false for red "-X"
 */
void SpawnDamageNumber(int damage, float world_x, float world_y, bool is_healing);

/**
 * TriggerScreenShake - Shake the entire screen (for tag effects, critical hits)
 *
 * Creates a screen shake effect by tweening global shake offsets.
 * All rendering is offset during shake for maximum impact.
 *
 * @param intensity - Shake magnitude in pixels (try 10-20)
 * @param duration - Shake duration in seconds (try 0.3-0.5)
 */
void TriggerScreenShake(float intensity, float duration);

/**
 * GetCardTransitionManager - Get pointer to global card transition manager
 *
 * Used by sections (playerSection, dealerSection) to query card animations
 * during rendering. Allows sections to render cards at tweened positions.
 *
 * @return Pointer to global card transition manager
 */
struct CardTransitionManager* GetCardTransitionManager(void);

/**
 * GetTweenManager - Get pointer to global tween manager
 *
 * Used by game.c for spawning card deal animations.
 *
 * @return Pointer to global tween manager
 */
struct TweenManager* GetTweenManager(void);

/**
 * SetStatusEffectDrainAmount - Track chip drain from status effects
 *
 * Called by statusEffects.c when CHIP_DRAIN is processed.
 * Used to display "token bleed" animation on result screen.
 *
 * @param drain_amount - Amount of chips lost to status effects
 */
void SetStatusEffectDrainAmount(int drain_amount);

/**
 * TriggerSidebarBetAnimation - Show floating damage number when bet is placed
 *
 * Called by player.c when PlaceBet() succeeds.
 * Shows red "-N chips" text that floats up from chip counter in sidebar.
 *
 * @param bet_amount - Amount of bet placed
 */
void TriggerSidebarBetAnimation(int bet_amount);

/**
 * IsCardValidTarget - Check if a card can be targeted by active trinket
 *
 * Used by playerSection and dealerSection to highlight valid/invalid targets.
 * Checks trinket-specific targeting rules (e.g., Degenerate's Gambit: rank ≤ 5)
 *
 * @param card - Card to check
 * @param trinket_slot - Trinket slot index (-1 for class trinket, 0-5 for regular slots)
 * @return true if card can be targeted by this trinket
 */
bool IsCardValidTarget(const struct Card* card, int trinket_slot);

#endif // SCENE_BLACKJACK_H
