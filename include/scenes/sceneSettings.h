#ifndef SCENE_SETTINGS_H
#define SCENE_SETTINGS_H

// ============================================================================
// SETTINGS SCENE
// ============================================================================

/**
 * InitSettingsScene - Initialize the settings scene
 *
 * Displays game settings and options
 * Press ESC to return to menu
 */
void InitSettingsScene(void);

/**
 * InitSettingsSceneFromMenu - Open settings from main menu
 *
 * Returns to main menu when ESC is pressed
 */
void InitSettingsSceneFromMenu(void);

/**
 * InitSettingsSceneFromGame - Open settings from in-game pause menu
 *
 * Returns to game (paused) when ESC is pressed
 * Game state is preserved during settings navigation
 */
void InitSettingsSceneFromGame(void);

#endif // SCENE_SETTINGS_H
