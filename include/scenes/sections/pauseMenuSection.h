/*
 * Pause Menu Section
 * Modal overlay for in-game pause menu with confirmation dialog
 * Constitutional pattern: MenuItem components, section-based architecture
 */

#ifndef PAUSE_MENU_SECTION_H
#define PAUSE_MENU_SECTION_H

#include "../../common.h"
#include "../components/menuItem.h"
#include "../components/menuItemRow.h"

// ============================================================================
// PAUSE ACTION ENUM
// ============================================================================

typedef enum PauseAction {
    PAUSE_ACTION_NONE = 0,
    PAUSE_ACTION_RESUME,
    PAUSE_ACTION_STATS,
    PAUSE_ACTION_SETTINGS,
    PAUSE_ACTION_HELP,
    PAUSE_ACTION_QUIT_TO_MENU
} PauseAction_t;

// ============================================================================
// PAUSE MENU SECTION
// ============================================================================

typedef struct PauseMenuSection {
    MenuItem_t* menu_items[5];        // Resume, Stats, Settings, Help, Return to Main Menu (vertical column layout)
    MenuItemRow_t* confirm_items[2];  // Yes, No (for confirmation dialog - horizontal row layout)
    bool is_visible;                  // Whether pause menu is displayed
    bool show_confirm_dialog;         // Whether confirmation dialog is shown
    bool show_stats_overlay;          // Whether stats overlay is shown
    int selected_option;              // Currently selected main menu option (0-4)
    int confirm_selected;             // Currently selected confirm option (0-1)
} PauseMenuSection_t;

/**
 * CreatePauseMenuSection - Initialize pause menu section
 *
 * @return PauseMenuSection_t* - Heap-allocated section (caller must destroy)
 */
PauseMenuSection_t* CreatePauseMenuSection(void);

/**
 * DestroyPauseMenuSection - Cleanup pause menu section resources
 *
 * @param section - Pointer to section pointer (will be set to NULL)
 */
void DestroyPauseMenuSection(PauseMenuSection_t** section);

/**
 * ShowPauseMenu - Display the pause menu overlay
 *
 * @param section - Pause menu section
 */
void ShowPauseMenu(PauseMenuSection_t* section);

/**
 * HidePauseMenu - Hide the pause menu overlay
 *
 * @param section - Pause menu section
 */
void HidePauseMenu(PauseMenuSection_t* section);

/**
 * HandlePauseMenuInput - Process keyboard and mouse input for pause menu
 *
 * @param section - Pause menu section
 * @return PauseAction_t - Action to perform based on user input
 *
 * Handles:
 * - W/S/Up/Down navigation
 * - Enter/Space selection
 * - ESC to close menu or cancel confirmation
 * - Mouse hover and click
 * - Confirmation dialog for "Return to Main Menu"
 */
PauseAction_t HandlePauseMenuInput(PauseMenuSection_t* section);

/**
 * RenderPauseMenuSection - Render pause menu modal overlay
 *
 * @param section - Pause menu section
 *
 * Renders:
 * - Semi-transparent full-screen overlay
 * - Centered menu panel with options
 * - "Return to Main Menu" in red
 * - Confirmation dialog (if active)
 */
void RenderPauseMenuSection(PauseMenuSection_t* section);

#endif // PAUSE_MENU_SECTION_H
