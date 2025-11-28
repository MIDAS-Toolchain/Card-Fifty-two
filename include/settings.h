#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>

// ============================================================================
// SETTINGS DATA STRUCTURE
// ============================================================================

/**
 * Settings_t - Global game settings
 *
 * Organized into tabs:
 * - Audio: Volume controls and toggles
 * - Gameplay: Game behavior preferences
 * - UI: Interface display options
 * - Graphics: Resolution, fullscreen, vsync
 *
 * Persisted to settings.duf in user config directory
 */
typedef struct {
    // ========================================================================
    // AUDIO TAB
    // ========================================================================
    int sound_volume;       // 0-100 (default: 50)
    int music_volume;       // 0-100 (default: 50)
    bool sound_enabled;     // Master sound toggle (default: true)
    bool music_enabled;     // Master music toggle (default: true)

    // ========================================================================
    // GAMEPLAY TAB
    // ========================================================================
    bool show_damage_numbers;       // Floating damage text (default: true)
    bool auto_advance_dialogue;     // Auto-advance tutorial dialogs (default: false)
    int tutorial_hints;             // 0=Off, 1=Basic, 2=Detailed (default: 2)

    // ========================================================================
    // UI TAB
    // ========================================================================
    bool show_fps;          // FPS counter (default: false)
    bool screen_shake;      // Damage screen shake (default: true)
    int ui_scale;           // 0=100%, 1=125%, 2=150% (default: 0)

    // ========================================================================
    // GRAPHICS TAB
    // ========================================================================
    bool fullscreen;        // Fullscreen mode (default: false)
    bool vsync;             // V-Sync (default: true)
    int resolution_index;   // Index into resolution table (default: 0 = 1280x720)
} Settings_t;

// ============================================================================
// RESOLUTION TABLE
// ============================================================================

typedef struct {
    int width;
    int height;
    const char* label;  // For display in dropdown
} Resolution_t;

extern const Resolution_t AVAILABLE_RESOLUTIONS[];
extern const int RESOLUTION_COUNT;

// ============================================================================
// GLOBAL SETTINGS API
// ============================================================================

/**
 * Settings_Init - Create settings with defaults
 *
 * Returns: New Settings_t* with default values
 */
Settings_t* Settings_Init(void);

/**
 * Settings_Destroy - Free settings structure
 *
 * @param settings: Pointer to Settings_t* (set to NULL after free)
 */
void Settings_Destroy(Settings_t** settings);

/**
 * Settings_Load - Load settings from settings.duf
 *
 * @param settings: Settings structure to populate
 *
 * If file doesn't exist or is invalid, uses defaults
 */
void Settings_Load(Settings_t* settings);

/**
 * Settings_Save - Save settings to settings.duf
 *
 * @param settings: Settings to save
 *
 * Creates/overwrites settings.duf in user config directory
 */
void Settings_Save(const Settings_t* settings);

/**
 * Settings_Apply - Apply current settings to game engine
 *
 * @param settings: Settings to apply
 *
 * Immediately updates:
 * - SDL_mixer volume (sound_volume, music_volume)
 * - Window size/fullscreen (resolution_index, fullscreen)
 * - V-Sync (vsync)
 *
 * Other settings (show_fps, screen_shake, etc) are read directly
 * by game systems when needed.
 */
void Settings_Apply(const Settings_t* settings);

// ============================================================================
// SETTERS (with immediate Apply)
// ============================================================================

/**
 * Settings_SetSoundVolume - Set sound volume and apply immediately
 *
 * @param settings: Settings structure
 * @param volume: 0-100 volume level
 *
 * Clamps to 0-100, updates SDL_mixer immediately
 */
void Settings_SetSoundVolume(Settings_t* settings, int volume);

/**
 * Settings_SetMusicVolume - Set music volume and apply immediately
 *
 * @param settings: Settings structure
 * @param volume: 0-100 volume level
 *
 * Clamps to 0-100, updates SDL_mixer immediately
 */
void Settings_SetMusicVolume(Settings_t* settings, int volume);

/**
 * Settings_SetResolution - Set resolution and apply immediately
 *
 * @param settings: Settings structure
 * @param index: Index into AVAILABLE_RESOLUTIONS
 *
 * Clamps to valid range, resizes window immediately (no restart)
 */
void Settings_SetResolution(Settings_t* settings, int index);

/**
 * Settings_SetFullscreen - Toggle fullscreen and apply immediately
 *
 * @param settings: Settings structure
 * @param enabled: true=fullscreen, false=windowed
 *
 * Updates SDL window mode immediately (no restart)
 */
void Settings_SetFullscreen(Settings_t* settings, bool enabled);

/**
 * Settings_SetVSync - Toggle V-Sync and apply immediately
 *
 * @param settings: Settings structure
 * @param enabled: true=V-Sync on, false=V-Sync off
 *
 * Updates SDL renderer immediately
 */
void Settings_SetVSync(Settings_t* settings, bool enabled);

// ============================================================================
// GETTERS
// ============================================================================

/**
 * Settings_GetCurrent - Get current resolution as Resolution_t
 *
 * @param settings: Settings structure
 * @return: Resolution_t for current resolution_index
 */
const Resolution_t* Settings_GetCurrentResolution(const Settings_t* settings);

#endif // SETTINGS_H
