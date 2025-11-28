#ifndef SETTINGS_MENU_SECTION_H
#define SETTINGS_MENU_SECTION_H

#include "../../common.h"
#include "../../settings.h"

/**
 * SliderState - Mouse drag state for volume sliders
 */
typedef struct {
    bool is_dragging;       // Mouse held down on slider bar
    int dragging_index;     // Which slider (-1=none, 0=sound, 1=music)
} SliderState_t;

/**
 * SettingsMenuSection Component
 *
 * FlexBox-based settings menu with beautiful UI/UX
 * Follows the same architecture pattern as MainMenuSection
 * Now supports mouse drag for volume sliders!
 */
typedef struct SettingsMenuSection {
    Settings_t* settings;           // Reference to settings (NOT owned!)

    // Layout containers
    FlexBox_t* main_layout;         // Overall vertical layout
    FlexBox_t* audio_layout;        // Audio settings group
    FlexBox_t* graphics_layout;     // Graphics settings group
    FlexBox_t* button_layout;       // Bottom buttons (Save, Back)

    // UI state
    int selected_index;             // Currently selected setting (for keyboard nav)
    int hovered_index;              // Currently hovered item (for mouse nav, -1 = none)
    int prev_hovered_index;         // Previous frame hover (for hover sound detection)
    int total_items;                // Total number of interactive items

    // Mouse drag state for volume sliders
    SliderState_t slider_state;

    // Display text
    char title[256];                // "SETTINGS"
    char instructions[512];         // Controls help text
} SettingsMenuSection_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateSettingsMenuSection - Create settings menu section
 *
 * @param settings Pointer to Settings_t (scene owns this!)
 * @return Heap-allocated SettingsMenuSection, or NULL on failure
 */
SettingsMenuSection_t* CreateSettingsMenuSection(Settings_t* settings);

/**
 * DestroySettingsMenuSection - Cleanup section
 *
 * NOTE: Does NOT destroy settings - scene owns that!
 * Only destroys FlexBoxes owned by section.
 */
void DestroySettingsMenuSection(SettingsMenuSection_t** section);

// ============================================================================
// INPUT HANDLING
// ============================================================================

/**
 * HandleSettingsInput - Process keyboard input for settings navigation
 *
 * Handles:
 * - UP/DOWN: Navigate between settings
 * - LEFT/RIGHT: Adjust values
 * - ENTER: Toggle/activate
 * - ESC: Return to menu (handled by scene)
 *
 * @param section SettingsMenuSection to handle input for
 */
void HandleSettingsInput(SettingsMenuSection_t* section);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderSettingsMenuSection - Render entire settings menu
 *
 * Renders:
 * - Title (centered, top)
 * - Audio settings group (volume sliders, toggles)
 * - Graphics settings group (resolution, fullscreen, vsync)
 * - Bottom buttons (Save, Back)
 * - Instructions (centered, bottom)
 *
 * @param section SettingsMenuSection to render
 */
void RenderSettingsMenuSection(SettingsMenuSection_t* section);

#endif // SETTINGS_MENU_SECTION_H
