/*
 * Top Bar Section
 * Persistent top navigation bar for Blackjack scene
 * Constitutional pattern: Section-based architecture with Button component
 */

#ifndef TOP_BAR_SECTION_H
#define TOP_BAR_SECTION_H

#include "../../common.h"
#include "../components/button.h"
#include "../components/enemyHealthBar.h"
#include "../../enemy.h"
#include "../../player.h"
#include "../../game.h"

// ============================================================================
// TOP BAR SECTION CONSTANTS
// ============================================================================

// Settings button layout
#define SETTINGS_BUTTON_WIDTH    40
#define SETTINGS_BUTTON_HEIGHT   25
#define SETTINGS_BUTTON_MARGIN   20  // Margin from right edge
#define SETTINGS_BUTTON_Y_OFFSET 5   // Vertical centering offset from top bar top

// Combat showcase constants
#define VERSUS_WORD_ROTATION_TIME 2.0f  // Seconds between word changes
#define VERSUS_WORD_COUNT         5     // Number of connector words
#define SHOWCASE_TEXT_X_MARGIN    20    // Left margin for combat showcase text
#define SHOWCASE_TEXT_Y_OFFSET    0    // Vertical centering offset for showcase text

// ============================================================================
// TOP BAR SECTION
// ============================================================================

typedef struct TopBarSection {
    Button_t* settings_button;      // Settings icon button (far right)

    // Combat showcase
    dString_t* showcase_text;       // Full formatted text buffer
    const char* versus_words[VERSUS_WORD_COUNT];  // Array of connector words
    int current_versus_index;       // Which word is currently displayed
    float versus_timer;             // Timer for word rotation

    // HP bars (for combat UI in top bar)
    EnemyHealthBar_t* player_hp_bar;  // Player HP bar (left side)
    EnemyHealthBar_t* enemy_hp_bar;   // Enemy HP bar (right side)
} TopBarSection_t;

/**
 * CreateTopBarSection - Initialize top bar section
 *
 * @return TopBarSection_t* - Heap-allocated section (caller must destroy)
 */
TopBarSection_t* CreateTopBarSection(void);

/**
 * DestroyTopBarSection - Cleanup top bar section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyTopBarSection(TopBarSection_t** section);

/**
 * UpdateTopBarShowcase - Update combat showcase text and rotation
 *
 * @param section - Top bar section component
 * @param dt - Delta time in seconds
 * @param player - Current player (for name)
 * @param enemy - Current combat enemy (NULL if not in combat)
 *
 * Updates versus word rotation and rebuilds showcase text
 * Called every frame during combat encounters
 */
void UpdateTopBarShowcase(TopBarSection_t* section, float dt,
                          const Player_t* player, const Enemy_t* enemy);

/**
 * RenderTopBarSection - Render top navigation bar
 *
 * @param section - Top bar section component
 * @param game - Game context (for enemy HP bar)
 * @param y - Y position from main FlexBox layout
 *
 * Handles:
 * - Combat showcase (left-aligned, if in combat)
 * - Enemy HP bar (right side, if in combat)
 * - Settings button on far right
 * - Future: back button, breadcrumbs, title on left
 */
void RenderTopBarSection(TopBarSection_t* section, const GameContext_t* game, int y);

/**
 * IsTopBarSettingsHovered - Check if settings button is hovered
 *
 * @param section - Top bar section component
 * @return true if settings button is currently hovered
 */
bool IsTopBarSettingsHovered(TopBarSection_t* section);

/**
 * IsTopBarSettingsClicked - Check if settings button was clicked
 *
 * @param section - Top bar section component
 * @return true if settings button clicked this frame
 */
bool IsTopBarSettingsClicked(TopBarSection_t* section);

#endif // TOP_BAR_SECTION_H
