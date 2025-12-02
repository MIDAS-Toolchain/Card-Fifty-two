/*
 * SettingsMenuSection Implementation
 * Simple fixed-position layout (no complex FlexBox nesting)
 */

#include "../../../include/scenes/sections/settingsMenuSection.h"
#include "../../../include/audioHelper.h"
#include "../../../include/settings.h"
#include "../../../include/tween/tween.h"

// External tween manager from sceneBlackjack.c
extern TweenManager_t g_tween_manager;

// Layout helpers - scaled proportionally to screen size (runtime, resolution-independent)
static inline int GetSettingsTitleY(void)           { return GetWindowHeight() * 0.11f; }   // 11% from top (was 80/720)
static inline int GetSettingsAudioHeaderY(void)     { return GetWindowHeight() * 0.25f; }   // 25% from top (was 180/720)
static inline int GetSettingsAudioStartY(void)      { return GetWindowHeight() * 0.31f; }   // 31% from top (was 220/720)
static inline int GetSettingsGraphicsHeaderY(void)  { return GetWindowHeight() * 0.50f; }   // 50% from top (was 360/720)
static inline int GetSettingsGraphicsStartY(void)   { return GetWindowHeight() * 0.56f; }   // 56% from top (was 400/720)
static inline int GetSettingsButtonsY(void)         { return GetWindowHeight() * 0.81f; }   // 81% from top (was 580/720)
static inline int GetSettingsItemHeight(void)       { return GetWindowHeight() * 0.07f; }   // 7% of height (was 50/720)
static inline int GetSettingsInstructionsY(void)    { return GetWindowHeight() - (GetWindowHeight() * 0.07f); } // 7% from bottom (was 50/720)

static inline int GetSettingsLabelX(void)           { return GetWindowWidth() * 0.16f; }    // 16% from left (was 200/1280)
static inline int GetSettingsValueX(void)           { return GetWindowWidth() * 0.47f; }    // 47% from left (was 600/1280)
static inline int GetSettingsBarX(void)             { return GetWindowWidth() * 0.59f; }    // 59% from left (was 750/1280)

// Colors (from your palette)
#define COLOR_TITLE         ((aColor_t){231, 213, 179, 255})  // #e7d5b3
#define COLOR_LABEL         ((aColor_t){255, 255, 255, 255})  // white
#define COLOR_VALUE         ((aColor_t){115, 190, 211, 255})  // #73bed3
#define COLOR_SELECTED      ((aColor_t){255, 200, 100, 255})  // highlight
#define COLOR_INSTRUC       ((aColor_t){150, 150, 150, 255})  // gray
#define COLOR_BAR_INACTIVE  ((aColor_t){255, 255, 255, 60})   // white transparent
#define COLOR_BAR_FILL      ((aColor_t){115, 190, 211, 180})  // semi-transparent blue

// ============================================================================
// LIFECYCLE
// ============================================================================

SettingsMenuSection_t* CreateSettingsMenuSection(Settings_t* settings) {
    if (!settings) {
        d_LogError("CreateSettingsMenuSection: NULL settings");
        return NULL;
    }

    SettingsMenuSection_t* section = malloc(sizeof(SettingsMenuSection_t));
    if (!section) {
        d_LogError("Failed to allocate SettingsMenuSection");
        return NULL;
    }

    section->settings = settings;
    section->selected_index = 0;
    section->hovered_index = -1;  // No hover initially
    section->prev_hovered_index = -1;  // No previous hover
    section->total_items = 6;  // 3 audio + 1 graphics (resolution) + 2 buttons

    // Initialize slider drag state
    section->slider_state.is_dragging = false;
    section->slider_state.dragging_index = -1;

    // Initialize resolution dropdown state
    section->resolution_dropdown_open = false;
    section->dropdown_hovered_index = -1;

    // Initialize arrow alphas for all settings
    for (int i = 0; i < 8; i++) {
        section->arrow_alpha[i] = 0.0f;
    }

    // Create FlexBox for resolution dropdown (positioned below resolution row)
    // NOTE: Will be recreated on resolution change to fix positioning
    section->resolution_dropdown_layout = a_FlexBoxCreate(
        GetSettingsValueX(),               // X: Align with resolution value
        GetSettingsGraphicsStartY() + 40,  // Y: Below resolution row
        300,                                // Width: Fits "1600x900 (HD+)"
        RESOLUTION_COUNT * 40              // Height: 5 items × 40px
    );
    a_FlexConfigure(section->resolution_dropdown_layout,
                   FLEX_DIR_COLUMN,       // Vertical stack
                   FLEX_JUSTIFY_START,    // Top-aligned
                   2);                    // 2px gap between items

    // Add all resolution items to FlexBox (pre-populated!)
    for (int i = 0; i < RESOLUTION_COUNT; i++) {
        a_FlexAddItem(section->resolution_dropdown_layout, 280, 35, NULL);
    }

    // Set default text
    strncpy(section->title, "SETTINGS", sizeof(section->title) - 1);
    section->title[sizeof(section->title) - 1] = '\0';

    strncpy(section->instructions,
            "[MOUSE] Click Resolution | [ARROWS] Navigate | [ESC] Back",
            sizeof(section->instructions) - 1);
    section->instructions[sizeof(section->instructions) - 1] = '\0';

    // No FlexBox - using simple fixed positions
    section->main_layout = NULL;
    section->audio_layout = NULL;
    section->graphics_layout = NULL;
    section->button_layout = NULL;

    d_LogInfo("SettingsMenuSection created");
    return section;
}

void DestroySettingsMenuSection(SettingsMenuSection_t** section) {
    if (!section || !*section) return;

    // Clean up resolution dropdown FlexBox
    if ((*section)->resolution_dropdown_layout) {
        a_FlexBoxDestroy(&(*section)->resolution_dropdown_layout);
    }

    free(*section);
    *section = NULL;
    d_LogInfo("SettingsMenuSection destroyed");
}

// ============================================================================
// DROPDOWN POSITIONING FIX
// ============================================================================

/**
 * RecreateResolutionDropdown - Fix dropdown position after resolution change
 *
 * Bug: Dropdown FlexBox was created at init time with old window dimensions.
 * When resolution changes, GetSettingsValueX() returns NEW pixel coordinates,
 * but FlexBox still has OLD coordinates baked in.
 *
 * Solution: Destroy and recreate the dropdown with current window dimensions.
 */
static void RecreateResolutionDropdown(SettingsMenuSection_t* section) {
    if (!section) return;

    // Destroy old dropdown
    if (section->resolution_dropdown_layout) {
        a_FlexBoxDestroy(&section->resolution_dropdown_layout);
    }

    // Recreate with CURRENT window dimensions
    section->resolution_dropdown_layout = a_FlexBoxCreate(
        GetSettingsValueX(),               // X: Current resolution's value position
        GetSettingsGraphicsStartY() + 40,  // Y: Current resolution's row position
        300,                                // Width: Fits "1600x900 (HD+)"
        RESOLUTION_COUNT * 40              // Height: 5 items × 40px
    );
    a_FlexConfigure(section->resolution_dropdown_layout,
                   FLEX_DIR_COLUMN,
                   FLEX_JUSTIFY_START,
                   2);

    // Re-populate items
    for (int i = 0; i < RESOLUTION_COUNT; i++) {
        a_FlexAddItem(section->resolution_dropdown_layout, 280, 35, NULL);
    }
}

// ============================================================================
// HELPER - Arrow blinking callbacks for ping-pong tween (per-setting)
// ============================================================================

// Callback data for arrow blink (includes setting index)
typedef struct {
    SettingsMenuSection_t* section;
    int setting_index;  // Which setting (0-7)
} ArrowBlinkData_t;

static ArrowBlinkData_t g_arrow_blink_data[8];  // One per setting

static void OnArrowBlinkComplete(void* user_data) {
    ArrowBlinkData_t* data = (ArrowBlinkData_t*)user_data;
    if (!data || !data->section) return;

    SettingsMenuSection_t* section = data->section;
    int idx = data->setting_index;

    // Only continue blinking if this setting is still selected/hovered
    bool should_blink = (section->selected_index == idx || section->hovered_index == idx);
    if (should_blink) {
        // Reverse tween direction (0.6 ↔ 1.0 ping-pong)
        float target = (section->arrow_alpha[idx] > 0.8f) ? 0.6f : 1.0f;
        TweenFloatWithCallback(&g_tween_manager, &section->arrow_alpha[idx], target,
                              0.8f, TWEEN_EASE_IN_OUT_QUAD,
                              OnArrowBlinkComplete, data);
    }
}

static void StartArrowBlink(SettingsMenuSection_t* section, int setting_index) {
    if (!section || setting_index < 0 || setting_index >= 8) return;

    // Stop any existing arrow tweens for this setting
    StopTweensForTarget(&g_tween_manager, &section->arrow_alpha[setting_index]);

    // Start blinking (fade to 1.0, then callback will ping-pong)
    section->arrow_alpha[setting_index] = 0.6f;

    // Setup callback data
    g_arrow_blink_data[setting_index].section = section;
    g_arrow_blink_data[setting_index].setting_index = setting_index;

    TweenFloatWithCallback(&g_tween_manager, &section->arrow_alpha[setting_index], 1.0f,
                          0.8f, TWEEN_EASE_IN_OUT_QUAD,
                          OnArrowBlinkComplete, &g_arrow_blink_data[setting_index]);
}

static void StopArrowBlink(SettingsMenuSection_t* section, int setting_index) {
    if (!section || setting_index < 0 || setting_index >= 8) return;

    // Stop tweens and fade out arrows
    StopTweensForTarget(&g_tween_manager, &section->arrow_alpha[setting_index]);

    TweenFloat(&g_tween_manager, &section->arrow_alpha[setting_index], 0.0f,
              0.2f, TWEEN_EASE_OUT_QUAD);
}

// ============================================================================
// HELPER - Check if mouse is over an item's bounds
// ============================================================================

typedef struct {
    int index;
    int x, y, w, h;
} ItemBounds_t;

static bool IsMouseOverItem(int mx, int my, ItemBounds_t bounds) {
    return (mx >= bounds.x && mx <= bounds.x + bounds.w &&
            my >= bounds.y && my <= bounds.y + bounds.h);
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

void HandleSettingsInput(SettingsMenuSection_t* section) {
    if (!section || !section->settings) return;

    // Mouse position
    int mx, my;
    SDL_GetMouseState(&mx, &my);

    // Define clickable regions for all items
    ItemBounds_t items[] = {
        // Audio items (full row width)
        {0, 0, GetSettingsAudioStartY() - 10, GetWindowWidth(), GetSettingsItemHeight()},
        {1, 0, GetSettingsAudioStartY() + GetSettingsItemHeight() - 10, GetWindowWidth(), GetSettingsItemHeight()},
        {2, 0, GetSettingsAudioStartY() + 2 * GetSettingsItemHeight() - 10, GetWindowWidth(), GetSettingsItemHeight()},
        // Graphics items (just resolution now)
        {3, 0, GetSettingsGraphicsStartY() - 10, GetWindowWidth(), GetSettingsItemHeight()},
        // Buttons (specific bounds)
        {4, GetWindowWidth() / 2 - 220, GetSettingsButtonsY(), 200, 50},  // Save
        {5, GetWindowWidth() / 2 + 20, GetSettingsButtonsY(), 200, 50}    // Back
    };

    // Volume slider bar bounds (for mouse drag detection)
    ItemBounds_t slider_bars[] = {
        {0, GetSettingsBarX(), GetSettingsAudioStartY() + 5, 200, 20},                // Sound volume bar
        {1, GetSettingsBarX(), GetSettingsAudioStartY() + GetSettingsItemHeight() + 5, 200, 20},  // Music volume bar
        {2, GetSettingsBarX(), GetSettingsAudioStartY() + 2 * GetSettingsItemHeight() + 5, 200, 20}  // UI volume bar
    };

    // Handle mouse drag for volume sliders
    bool mouse_down = (app.mouse.pressed == 1);  // Mouse currently held down

    if (mouse_down) {
        // Check if starting a drag on a slider bar
        if (!section->slider_state.is_dragging) {
            for (int i = 0; i < 3; i++) {
                if (IsMouseOverItem(mx, my, slider_bars[i])) {
                    section->slider_state.is_dragging = true;
                    section->slider_state.dragging_index = i;
                    break;
                }
            }
        }

        // Update slider value while dragging
        if (section->slider_state.is_dragging) {
            int idx = section->slider_state.dragging_index;
            if (idx >= 0 && idx < 3) {
                // Calculate value from mouse X position
                int bar_x = slider_bars[idx].x;
                int bar_w = slider_bars[idx].w;
                int relative_x = mx - bar_x;

                // Clamp to bar bounds
                if (relative_x < 0) relative_x = 0;
                if (relative_x > bar_w) relative_x = bar_w;

                // Convert to 0-100 volume
                int volume = (relative_x * 100) / bar_w;

                // Apply to correct setting
                if (idx == 0) {
                    Settings_SetSoundVolume(section->settings, volume);
                } else if (idx == 1) {
                    Settings_SetMusicVolume(section->settings, volume);
                } else {
                    Settings_SetUIVolume(section->settings, volume);
                }
            }
        }
    } else {
        // Mouse released - stop dragging
        section->slider_state.is_dragging = false;
        section->slider_state.dragging_index = -1;
    }

    // Update hover state (skip if dragging to avoid conflicts)
    section->hovered_index = -1;
    if (!section->slider_state.is_dragging) {
        for (int i = 0; i < 8; i++) {
            if (IsMouseOverItem(mx, my, items[i])) {
                section->hovered_index = items[i].index;
                break;
            }
        }
    }

    // Play hover sound when hover state changes
    if (section->hovered_index != section->prev_hovered_index && section->hovered_index >= 0) {
        PlayUIHoverSound();
    }

    // Start/stop arrow blink animations for all adjustable settings (0-3)
    for (int i = 0; i < 4; i++) {  // 0=sound_vol, 1=music_vol, 2=ui_vol, 3=resolution
        bool was_active = (section->prev_hovered_index == i || section->selected_index == i);
        bool is_active = (section->hovered_index == i || section->selected_index == i);

        if (is_active && !was_active) {
            // Just became active - start blinking
            StartArrowBlink(section, i);
        } else if (!is_active && was_active) {
            // Just became inactive - stop blinking
            StopArrowBlink(section, i);
        }
    }

    section->prev_hovered_index = section->hovered_index;

    // Handle dropdown hover detection (if dropdown is open)
    if (section->resolution_dropdown_open && section->resolution_dropdown_layout) {
        section->dropdown_hovered_index = -1;

        // Check if mouse is over any dropdown item (only valid ones can be hovered)
        for (int i = 0; i < RESOLUTION_COUNT; i++) {
            // Skip invalid resolutions (they can't be selected)
            const Resolution_t* res = &AVAILABLE_RESOLUTIONS[i];
            if (!IsResolutionValidForDisplay(res->width, res->height)) {
                continue;  // Can't hover invalid resolutions
            }

            // Get item bounds from FlexBox (need to call layout first)
            a_FlexLayout(section->resolution_dropdown_layout);
            int item_x = a_FlexGetItemX(section->resolution_dropdown_layout, i);
            int item_y = a_FlexGetItemY(section->resolution_dropdown_layout, i);

            ItemBounds_t dropdown_item = {i, item_x, item_y, 280, 35};
            if (IsMouseOverItem(mx, my, dropdown_item)) {
                section->dropdown_hovered_index = i;
                break;
            }
        }
    }

    // Mouse click handling (only if not dragging)
    // Use app.mouse.pressed for single-frame detection (set to 1 on click, we consume it)
    if (app.mouse.pressed && !section->slider_state.is_dragging) {
        // Check dropdown clicks FIRST (highest priority)
        if (section->resolution_dropdown_open && section->dropdown_hovered_index >= 0) {
            PlayUIClickSound();
            Settings_SetResolution(section->settings, section->dropdown_hovered_index);
            section->resolution_dropdown_open = false;  // Close dropdown
            section->dropdown_hovered_index = -1;
            RecreateResolutionDropdown(section);  // Fix positioning after resolution change
        }
        // Then check main UI clicks
        else if (section->hovered_index >= 0) {
            PlayUIClickSound();  // Play click sound for all interactions

            switch (section->hovered_index) {
                case 0:  // Sound volume - click to select
                case 1:  // Music volume - click to select
                case 2:  // UI volume - click to select
                    section->selected_index = section->hovered_index;
                    break;
                case 3:  // Resolution - click to toggle dropdown
                    section->resolution_dropdown_open = !section->resolution_dropdown_open;
                    section->dropdown_hovered_index = -1;
                    break;
                case 4:  // Save button
                    Settings_Save(section->settings);
                    d_LogInfo("Settings saved!");
                    break;
                case 5:  // Back button
                    app.keyboard[SDL_SCANCODE_ESCAPE] = 1;
                    break;
            }
        }
        // Click outside dropdown - close it
        else if (section->resolution_dropdown_open) {
            section->resolution_dropdown_open = false;
            section->dropdown_hovered_index = -1;
        }

        // Always consume the pressed flag after processing
        app.mouse.pressed = 0;
    }

    // Keyboard: Navigate UP
    if (app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_UP] = 0;

        // If dropdown is open, navigate dropdown items
        if (section->resolution_dropdown_open) {
            section->dropdown_hovered_index--;
            if (section->dropdown_hovered_index < 0) {
                section->dropdown_hovered_index = RESOLUTION_COUNT - 1;
            }
        } else {
            section->selected_index--;
            if (section->selected_index < 0) {
                section->selected_index = section->total_items - 1;
            }
        }
        PlayUIHoverSound();  // Navigation sound
    }

    // Navigate DOWN
    if (app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_DOWN] = 0;

        // If dropdown is open, navigate dropdown items
        if (section->resolution_dropdown_open) {
            section->dropdown_hovered_index++;
            if (section->dropdown_hovered_index >= RESOLUTION_COUNT) {
                section->dropdown_hovered_index = 0;
            }
        } else {
            section->selected_index++;
            if (section->selected_index >= section->total_items) {
                section->selected_index = 0;
            }
        }
        PlayUIHoverSound();  // Navigation sound
    }

    // Adjust value LEFT
    if (app.keyboard[SDL_SCANCODE_LEFT]) {
        app.keyboard[SDL_SCANCODE_LEFT] = 0;

        switch (section->selected_index) {
            case 0:  // Sound volume
                Settings_SetSoundVolume(section->settings,
                                        section->settings->sound_volume - 5);
                StartArrowBlink(section, 0);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
            case 1:  // Music volume
                Settings_SetMusicVolume(section->settings,
                                        section->settings->music_volume - 5);
                StartArrowBlink(section, 1);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
            case 2:  // UI volume
                Settings_SetUIVolume(section->settings,
                                     section->settings->ui_volume - 5);
                StartArrowBlink(section, 2);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
            case 3:  // Resolution
                Settings_SetResolution(section->settings,
                                       section->settings->resolution_index - 1);
                RecreateResolutionDropdown(section);  // Fix positioning
                StartArrowBlink(section, 3);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
        }
    }

    // Adjust value RIGHT
    if (app.keyboard[SDL_SCANCODE_RIGHT]) {
        app.keyboard[SDL_SCANCODE_RIGHT] = 0;

        switch (section->selected_index) {
            case 0:  // Sound volume
                Settings_SetSoundVolume(section->settings,
                                        section->settings->sound_volume + 5);
                StartArrowBlink(section, 0);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
            case 1:  // Music volume
                Settings_SetMusicVolume(section->settings,
                                        section->settings->music_volume + 5);
                StartArrowBlink(section, 1);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
            case 2:  // UI volume
                Settings_SetUIVolume(section->settings,
                                     section->settings->ui_volume + 5);
                StartArrowBlink(section, 2);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
            case 3:  // Resolution
                Settings_SetResolution(section->settings,
                                       section->settings->resolution_index + 1);
                RecreateResolutionDropdown(section);  // Fix positioning
                StartArrowBlink(section, 3);
                PlayUIHoverSound();  // Audio feedback on value change
                break;
        }
    }

    // Toggle with ENTER / SPACE
    if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_SPACE]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_SPACE] = 0;

        PlayUIClickSound();  // Activation sound

        // If dropdown is open, select hovered item (or current if none hovered)
        if (section->resolution_dropdown_open) {
            int target_index = (section->dropdown_hovered_index >= 0)
                             ? section->dropdown_hovered_index
                             : section->settings->resolution_index;
            Settings_SetResolution(section->settings, target_index);
            RecreateResolutionDropdown(section);  // Fix positioning
            section->resolution_dropdown_open = false;
            section->dropdown_hovered_index = -1;
        } else {
            switch (section->selected_index) {
                case 3:  // Resolution - toggle dropdown
                    section->resolution_dropdown_open = !section->resolution_dropdown_open;
                    section->dropdown_hovered_index = section->settings->resolution_index;  // Start at current
                    break;
                case 4:  // Save button
                    Settings_Save(section->settings);
                    d_LogInfo("Settings saved!");
                    break;
                case 5:  // Back button
                    // Triggers ESC in scene
                    app.keyboard[SDL_SCANCODE_ESCAPE] = 1;
                    break;
            }
        }
    }

    // ESC - Close dropdown if open
    if (app.keyboard[SDL_SCANCODE_ESCAPE] && section->resolution_dropdown_open) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        section->resolution_dropdown_open = false;
        section->dropdown_hovered_index = -1;
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderSettingsMenuSection(SettingsMenuSection_t* section) {
    if (!section || !section->settings) return;

    // Get UI scale from settings (100%, 125%, or 150%)
    float ui_scale = GetUIScale();

    aTextStyle_t title_style = {.type=FONT_ENTER_COMMAND, .fg=COLOR_TITLE, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=ui_scale, .padding=0};
    aTextStyle_t header_style = {.type=FONT_GAME, .fg=COLOR_TITLE, .bg={0,0,0,0},
                                 .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=ui_scale, .padding=0};
    aTextStyle_t label_style = {.type=FONT_GAME, .fg=COLOR_LABEL, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=ui_scale, .padding=0};
    aTextStyle_t value_style = {.type=FONT_GAME, .fg=COLOR_VALUE, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=ui_scale, .padding=0};
    aTextStyle_t selected_style = {.type=FONT_GAME, .fg=COLOR_SELECTED, .bg={0,0,0,0},
                                   .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=ui_scale, .padding=0};

    // Title
    a_DrawText(section->title, GetWindowWidth() / 2, GetSettingsTitleY(), title_style);

    // ========================================================================
    // AUDIO SECTION
    // ========================================================================

    a_DrawText("AUDIO", GetSettingsLabelX(), GetSettingsAudioHeaderY(), header_style);

    // Effect Volume (index 0)
    int y = GetSettingsAudioStartY();
    bool is_selected_0 = (section->selected_index == 0);
    bool is_hovered_0 = (section->hovered_index == 0);
    aTextStyle_t lbl0 = (is_selected_0 || is_hovered_0) ? selected_style : label_style;
    aTextStyle_t val0 = (is_selected_0 || is_hovered_0) ? selected_style : value_style;

    a_DrawText("Effect Volume", GetSettingsLabelX(), y, lbl0);

    dString_t* vol1 = d_StringInit();
    d_StringFormat(vol1, "%d%%", section->settings->sound_volume);
    int vol_x = GetSettingsValueX();
    a_DrawText(d_StringPeek(vol1), vol_x, y, val0);

    // Draw animated arrows for sound volume (conditional on bounds)
    if (is_selected_0 || is_hovered_0) {
        float vol_width = 0, vol_height = 0;
        a_CalcTextDimensions(d_StringPeek(vol1), FONT_GAME, &vol_width, &vol_height);

        aTextStyle_t arrow_style = {
            .type = FONT_GAME,
            .fg = {COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b, (Uint8)(255 * section->arrow_alpha[0])},
            .align = TEXT_ALIGN_LEFT,
            .scale = ui_scale
        };

        // Only show left arrow if not at minimum (0)
        if (section->settings->sound_volume > 0) {
            a_DrawText("<", vol_x - 15, y, arrow_style);
        }

        // Only show right arrow if not at maximum (100)
        if (section->settings->sound_volume < 100) {
            a_DrawText(">", vol_x + vol_width + 15, y, arrow_style);
        }
    }
    d_StringDestroy(vol1);

    // Volume bar (ALWAYS VISIBLE)
    int bar_w = 200;
    int bar_h = 20;
    int filled_w = (bar_w * section->settings->sound_volume) / 100;
    aColor_t bar_color = is_selected_0 ? COLOR_LABEL : COLOR_BAR_INACTIVE;
    aColor_t fill_color = is_selected_0 ? COLOR_VALUE : COLOR_BAR_FILL;
    a_DrawRect((aRectf_t){GetSettingsBarX(), y + 5, bar_w, bar_h}, bar_color);
    a_DrawFilledRect((aRectf_t){GetSettingsBarX(), y + 5, filled_w, bar_h}, fill_color);

    // Music Volume (index 1)
    y += GetSettingsItemHeight();
    bool is_selected_1 = (section->selected_index == 1);
    bool is_hovered_1 = (section->hovered_index == 1);
    aTextStyle_t lbl1 = (is_selected_1 || is_hovered_1) ? selected_style : label_style;
    aTextStyle_t val1 = (is_selected_1 || is_hovered_1) ? selected_style : value_style;

    a_DrawText("Music Volume", GetSettingsLabelX(), y, lbl1);

    dString_t* vol2 = d_StringInit();
    d_StringFormat(vol2, "%d%%", section->settings->music_volume);
    vol_x = GetSettingsValueX();
    a_DrawText(d_StringPeek(vol2), vol_x, y, val1);

    // Draw animated arrows for music volume (conditional on bounds)
    if (is_selected_1 || is_hovered_1) {
        float vol_width = 0, vol_height = 0;
        a_CalcTextDimensions(d_StringPeek(vol2), FONT_GAME, &vol_width, &vol_height);

        aTextStyle_t arrow_style = {
            .type = FONT_GAME,
            .fg = {COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b, (Uint8)(255 * section->arrow_alpha[1])},
            .align = TEXT_ALIGN_LEFT,
            .scale = ui_scale
        };

        // Only show left arrow if not at minimum (0)
        if (section->settings->music_volume > 0) {
            a_DrawText("<", vol_x - 15, y, arrow_style);
        }

        // Only show right arrow if not at maximum (100)
        if (section->settings->music_volume < 100) {
            a_DrawText(">", vol_x + vol_width + 15, y, arrow_style);
        }
    }
    d_StringDestroy(vol2);

    // Volume bar (ALWAYS VISIBLE)
    filled_w = (bar_w * section->settings->music_volume) / 100;
    bar_color = is_selected_1 ? COLOR_LABEL : COLOR_BAR_INACTIVE;
    fill_color = is_selected_1 ? COLOR_VALUE : COLOR_BAR_FILL;
    a_DrawRect((aRectf_t){GetSettingsBarX(), y + 5, bar_w, bar_h}, bar_color);
    a_DrawFilledRect((aRectf_t){GetSettingsBarX(), y + 5, filled_w, bar_h}, fill_color);

    // UI Volume (index 2)
    y += GetSettingsItemHeight();
    bool is_selected_2 = (section->selected_index == 2);
    bool is_hovered_2 = (section->hovered_index == 2);
    aTextStyle_t lbl2 = (is_selected_2 || is_hovered_2) ? selected_style : label_style;
    aTextStyle_t val2 = (is_selected_2 || is_hovered_2) ? selected_style : value_style;

    a_DrawText("UI Volume", GetSettingsLabelX(), y, lbl2);

    dString_t* vol3 = d_StringInit();
    d_StringFormat(vol3, "%d%%", section->settings->ui_volume);
    vol_x = GetSettingsValueX();
    a_DrawText(d_StringPeek(vol3), vol_x, y, val2);

    // Draw animated arrows for UI volume (conditional on bounds)
    if (is_selected_2 || is_hovered_2) {
        float vol_width = 0, vol_height = 0;
        a_CalcTextDimensions(d_StringPeek(vol3), FONT_GAME, &vol_width, &vol_height);

        aTextStyle_t arrow_style = {
            .type = FONT_GAME,
            .fg = {COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b, (Uint8)(255 * section->arrow_alpha[2])},
            .align = TEXT_ALIGN_LEFT,
            .scale = ui_scale
        };

        // Only show left arrow if not at minimum (0)
        if (section->settings->ui_volume > 0) {
            a_DrawText("<", vol_x - 15, y, arrow_style);
        }

        // Only show right arrow if not at maximum (100)
        if (section->settings->ui_volume < 100) {
            a_DrawText(">", vol_x + vol_width + 15, y, arrow_style);
        }
    }
    d_StringDestroy(vol3);

    // Volume bar (ALWAYS VISIBLE)
    filled_w = (bar_w * section->settings->ui_volume) / 100;
    bar_color = is_selected_2 ? COLOR_LABEL : COLOR_BAR_INACTIVE;
    fill_color = is_selected_2 ? COLOR_VALUE : COLOR_BAR_FILL;
    a_DrawRect((aRectf_t){GetSettingsBarX(), y + 5, bar_w, bar_h}, bar_color);
    a_DrawFilledRect((aRectf_t){GetSettingsBarX(), y + 5, filled_w, bar_h}, fill_color);

    // ========================================================================
    // GRAPHICS SECTION
    // ========================================================================

    a_DrawText("GRAPHICS", GetSettingsLabelX(), GetSettingsGraphicsHeaderY(), header_style);

    // Resolution (index 3)
    y = GetSettingsGraphicsStartY();
    bool is_selected_3 = (section->selected_index == 3);
    bool is_hovered_3 = (section->hovered_index == 3);
    aTextStyle_t lbl3 = (is_selected_3 || is_hovered_3) ? selected_style : label_style;
    aTextStyle_t val3 = (is_selected_3 || is_hovered_3) ? selected_style : value_style;

    a_DrawText("Resolution", GetSettingsLabelX(), y, lbl3);

    const Resolution_t* res = Settings_GetCurrentResolution(section->settings);

    // Draw resolution value
    int res_value_x = GetSettingsValueX();
    a_DrawText(res ? res->label : "Unknown", res_value_x, y, val3);

    // Calculate text width for arrow positioning
    float res_text_width = 0, res_text_height = 0;
    a_CalcTextDimensions(res ? res->label : "Unknown", FONT_GAME, &res_text_width, &res_text_height);

    // Draw animated arrows for resolution (conditional on bounds + validity)
    if (is_selected_3 || is_hovered_3) {
        aTextStyle_t arrow_style = {
            .type = FONT_GAME,
            .fg = {COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b, (Uint8)(255 * section->arrow_alpha[3])},
            .align = TEXT_ALIGN_LEFT,
            .scale = ui_scale
        };

        // Only show left arrow if there's a valid resolution to the left
        bool has_valid_left = false;
        if (section->settings->resolution_index > 0) {
            // Check if there's any valid resolution below current
            for (int i = section->settings->resolution_index - 1; i >= 0; i--) {
                if (IsResolutionValidForDisplay(AVAILABLE_RESOLUTIONS[i].width,
                                                AVAILABLE_RESOLUTIONS[i].height)) {
                    has_valid_left = true;
                    break;
                }
            }
        }
        if (has_valid_left) {
            a_DrawText("<", res_value_x - 15, y, arrow_style);
        }

        // Only show right arrow if there's a valid resolution to the right
        bool has_valid_right = false;
        if (section->settings->resolution_index < RESOLUTION_COUNT - 1) {
            // Check if there's any valid resolution above current
            for (int i = section->settings->resolution_index + 1; i < RESOLUTION_COUNT; i++) {
                if (IsResolutionValidForDisplay(AVAILABLE_RESOLUTIONS[i].width,
                                                AVAILABLE_RESOLUTIONS[i].height)) {
                    has_valid_right = true;
                    break;
                }
            }
        }
        if (has_valid_right) {
            a_DrawText(">", res_value_x + res_text_width + 15, y, arrow_style);
        }
    }

    // ========================================================================
    // BOTTOM BUTTONS
    // ========================================================================

    // Save button (index 4)
    int btn_w = 200;
    int btn_h = 50;
    int save_x = GetWindowWidth() / 2 - btn_w - 20;
    bool is_selected_4 = (section->selected_index == 4);
    bool is_hovered_4 = (section->hovered_index == 4);
    aColor_t save_color = (is_selected_4 || is_hovered_4) ? COLOR_SELECTED : COLOR_LABEL;

    a_DrawRect((aRectf_t){save_x, GetSettingsButtonsY(), btn_w, btn_h}, save_color);
    a_DrawText("SAVE", save_x + btn_w / 2, GetSettingsButtonsY() + 15,
               (aTextStyle_t){.type=FONT_GAME, .fg=save_color, .bg={0,0,0,0},
                              .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Back button (index 5)
    int back_x = GetWindowWidth() / 2 + 20;
    bool is_selected_5 = (section->selected_index == 5);
    bool is_hovered_5 = (section->hovered_index == 5);
    aColor_t back_color = (is_selected_5 || is_hovered_5) ? COLOR_SELECTED : COLOR_LABEL;

    a_DrawRect((aRectf_t){back_x, GetSettingsButtonsY(), btn_w, btn_h}, back_color);
    a_DrawText("BACK", back_x + btn_w / 2, GetSettingsButtonsY() + 15,
               (aTextStyle_t){.type=FONT_GAME, .fg=back_color, .bg={0,0,0,0},
                              .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Instructions
    a_DrawText(section->instructions, GetWindowWidth() / 2, GetSettingsInstructionsY(),
               (aTextStyle_t){.type=FONT_GAME, .fg=COLOR_INSTRUC, .bg={0,0,0,0},
                              .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // ========================================================================
    // RESOLUTION DROPDOWN (Rendered LAST to be on top!)
    // ========================================================================

    if (section->resolution_dropdown_open && section->resolution_dropdown_layout) {
        // Calculate FlexBox layout positions
        a_FlexLayout(section->resolution_dropdown_layout);

        // Get dropdown bounds for background
        int dropdown_x = GetSettingsValueX() - 10;
        int dropdown_y = GetSettingsGraphicsStartY() + 40;
        int dropdown_w = 300;
        int dropdown_h = RESOLUTION_COUNT * 40;

        // Draw SOLID dark background with bright border (z-index on top!)
        a_DrawFilledRect((aRectf_t){dropdown_x, dropdown_y, dropdown_w, dropdown_h},
                        (aColor_t){9, 10, 20, 255});  // SOLID dark blue (same as scene background)
        a_DrawRect((aRectf_t){dropdown_x, dropdown_y, dropdown_w, dropdown_h},
                  (aColor_t){COLOR_LABEL.r, COLOR_LABEL.g, COLOR_LABEL.b, 255});  // Bright border

        // Render each resolution item
        for (int i = 0; i < RESOLUTION_COUNT; i++) {
            int item_x = a_FlexGetItemX(section->resolution_dropdown_layout, i);
            int item_y = a_FlexGetItemY(section->resolution_dropdown_layout, i);
            int item_w = 280;
            int item_h = 35;

            bool is_current = (i == section->settings->resolution_index);
            bool is_dd_hovered = (i == section->dropdown_hovered_index);

            // Check if this resolution is valid for the primary display
            const Resolution_t* res = &AVAILABLE_RESOLUTIONS[i];
            bool is_valid = IsResolutionValidForDisplay(res->width, res->height);

            // Highlight current resolution (subtle blue)
            if (is_current) {
                a_DrawFilledRect((aRectf_t){item_x, item_y, item_w, item_h},
                                (aColor_t){COLOR_VALUE.r, COLOR_VALUE.g, COLOR_VALUE.b, 60});
            }

            // Highlight hovered item (bright orange) - only if valid
            if (is_dd_hovered && is_valid) {
                a_DrawFilledRect((aRectf_t){item_x, item_y, item_w, item_h},
                                (aColor_t){COLOR_SELECTED.r, COLOR_SELECTED.g, COLOR_SELECTED.b, 100});
            }

            // Draw resolution label
            aColor_t text_color;
            if (!is_valid) {
                // Invalid resolutions: grey + dim
                text_color = (aColor_t){128, 128, 128, 255};
            } else {
                text_color = is_dd_hovered ? COLOR_SELECTED : (is_current ? COLOR_VALUE : COLOR_LABEL);
            }

            aTextStyle_t dropdown_text_style = {
                .type = FONT_GAME,
                .fg = text_color,
                .align = TEXT_ALIGN_LEFT,
                .scale = ui_scale
            };

            // Build label with "(Too Large)" suffix if invalid
            dString_t* label = d_StringInit();
            if (is_valid) {
                d_StringFormat(label, "%s", res->label);
            } else {
                d_StringFormat(label, "%s (Too Large)", res->label);
            }
            a_DrawText(d_StringPeek(label), item_x + 10, item_y + 10, dropdown_text_style);
            d_StringDestroy(label);
        }
    }
}
