/*
 * SettingsMenuSection Implementation
 * Simple fixed-position layout (no complex FlexBox nesting)
 */

#include "../../../include/scenes/sections/settingsMenuSection.h"

// Layout constants - FIXED POSITIONS
#define SETTINGS_TITLE_Y            80
#define SETTINGS_AUDIO_HEADER_Y     180
#define SETTINGS_AUDIO_START_Y      220
#define SETTINGS_GRAPHICS_HEADER_Y  360
#define SETTINGS_GRAPHICS_START_Y   400
#define SETTINGS_BUTTONS_Y          580
#define SETTINGS_ITEM_HEIGHT        50
#define SETTINGS_INSTRUCTIONS_Y     (SCREEN_HEIGHT - 50)

#define SETTINGS_LABEL_X            200
#define SETTINGS_VALUE_X            600
#define SETTINGS_BAR_X              750

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
    section->total_items = 7;  // 2 audio + 3 graphics + 2 buttons

    // Initialize slider drag state
    section->slider_state.is_dragging = false;
    section->slider_state.dragging_index = -1;

    // Set default text
    strncpy(section->title, "SETTINGS", sizeof(section->title) - 1);
    section->title[sizeof(section->title) - 1] = '\0';

    strncpy(section->instructions,
            "[MOUSE] Drag sliders | [ARROWS] Navigate | [ESC] Back",
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

    free(*section);
    *section = NULL;
    d_LogInfo("SettingsMenuSection destroyed");
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
        {0, 0, SETTINGS_AUDIO_START_Y - 10, SCREEN_WIDTH, SETTINGS_ITEM_HEIGHT},
        {1, 0, SETTINGS_AUDIO_START_Y + SETTINGS_ITEM_HEIGHT - 10, SCREEN_WIDTH, SETTINGS_ITEM_HEIGHT},
        // Graphics items
        {2, 0, SETTINGS_GRAPHICS_START_Y - 10, SCREEN_WIDTH, SETTINGS_ITEM_HEIGHT},
        {3, 0, SETTINGS_GRAPHICS_START_Y + SETTINGS_ITEM_HEIGHT - 10, SCREEN_WIDTH, SETTINGS_ITEM_HEIGHT},
        {4, 0, SETTINGS_GRAPHICS_START_Y + 2 * SETTINGS_ITEM_HEIGHT - 10, SCREEN_WIDTH, SETTINGS_ITEM_HEIGHT},
        // Buttons (specific bounds)
        {5, SCREEN_WIDTH / 2 - 220, SETTINGS_BUTTONS_Y, 200, 50},  // Save
        {6, SCREEN_WIDTH / 2 + 20, SETTINGS_BUTTONS_Y, 200, 50}    // Back
    };

    // Volume slider bar bounds (for mouse drag detection)
    ItemBounds_t slider_bars[] = {
        {0, SETTINGS_BAR_X, SETTINGS_AUDIO_START_Y + 5, 200, 20},                // Sound volume bar
        {1, SETTINGS_BAR_X, SETTINGS_AUDIO_START_Y + SETTINGS_ITEM_HEIGHT + 5, 200, 20}  // Music volume bar
    };

    // Handle mouse drag for volume sliders
    bool mouse_down = (app.mouse.pressed == 1);  // Mouse currently held down

    if (mouse_down) {
        // Check if starting a drag on a slider bar
        if (!section->slider_state.is_dragging) {
            for (int i = 0; i < 2; i++) {
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
            if (idx >= 0 && idx < 2) {
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
                } else {
                    Settings_SetMusicVolume(section->settings, volume);
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
        for (int i = 0; i < 7; i++) {
            if (IsMouseOverItem(mx, my, items[i])) {
                section->hovered_index = items[i].index;
                break;
            }
        }
    }

    // Play hover sound when hover state changes
    if (section->hovered_index != section->prev_hovered_index && section->hovered_index >= 0) {
        a_AudioPlayEffect(&g_ui_hover_sound);
    }
    section->prev_hovered_index = section->hovered_index;

    // Mouse click handling (only if not dragging)
    // Use app.mouse.pressed for single-frame detection (set to 1 on click, we consume it)
    if (app.mouse.pressed && !section->slider_state.is_dragging) {
        if (section->hovered_index >= 0) {
            a_AudioPlayEffect(&g_ui_click_sound);  // Play click sound for all interactions

            switch (section->hovered_index) {
                case 0:  // Sound volume - click to select
                case 1:  // Music volume - click to select
                case 2:  // Resolution - click to select
                    section->selected_index = section->hovered_index;
                    break;
                case 3:  // Fullscreen - click to toggle
                    Settings_SetFullscreen(section->settings, !section->settings->fullscreen);
                    break;
                case 4:  // V-Sync - click to toggle
                    Settings_SetVSync(section->settings, !section->settings->vsync);
                    break;
                case 5:  // Save button
                    Settings_Save(section->settings);
                    d_LogInfo("Settings saved!");
                    break;
                case 6:  // Back button
                    app.keyboard[SDL_SCANCODE_ESCAPE] = 1;
                    break;
            }
        }

        // Always consume the pressed flag after processing
        app.mouse.pressed = 0;
    }

    // Keyboard: Navigate UP
    if (app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_UP] = 0;
        section->selected_index--;
        if (section->selected_index < 0) {
            section->selected_index = section->total_items - 1;
        }
        a_AudioPlayEffect(&g_ui_hover_sound);  // Navigation sound
    }

    // Navigate DOWN
    if (app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_DOWN] = 0;
        section->selected_index++;
        if (section->selected_index >= section->total_items) {
            section->selected_index = 0;
        }
        a_AudioPlayEffect(&g_ui_hover_sound);  // Navigation sound
    }

    // Adjust value LEFT
    if (app.keyboard[SDL_SCANCODE_LEFT]) {
        app.keyboard[SDL_SCANCODE_LEFT] = 0;

        switch (section->selected_index) {
            case 0:  // Sound volume
                Settings_SetSoundVolume(section->settings,
                                        section->settings->sound_volume - 5);
                break;
            case 1:  // Music volume
                Settings_SetMusicVolume(section->settings,
                                        section->settings->music_volume - 5);
                break;
            case 2:  // Resolution
                Settings_SetResolution(section->settings,
                                       section->settings->resolution_index - 1);
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
                break;
            case 1:  // Music volume
                Settings_SetMusicVolume(section->settings,
                                        section->settings->music_volume + 5);
                break;
            case 2:  // Resolution
                Settings_SetResolution(section->settings,
                                       section->settings->resolution_index + 1);
                break;
        }
    }

    // Toggle with ENTER
    if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_SPACE]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_SPACE] = 0;

        a_AudioPlayEffect(&g_ui_click_sound);  // Activation sound

        switch (section->selected_index) {
            case 3:  // Fullscreen toggle
                Settings_SetFullscreen(section->settings,
                                       !section->settings->fullscreen);
                break;
            case 4:  // V-Sync toggle
                Settings_SetVSync(section->settings,
                                  !section->settings->vsync);
                break;
            case 5:  // Save button
                Settings_Save(section->settings);
                d_LogInfo("Settings saved!");
                break;
            case 6:  // Back button
                // Triggers ESC in scene
                app.keyboard[SDL_SCANCODE_ESCAPE] = 1;
                break;
        }
    }
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderSettingsMenuSection(SettingsMenuSection_t* section) {
    if (!section || !section->settings) return;

    aTextStyle_t title_style = {.type=FONT_ENTER_COMMAND, .fg=COLOR_TITLE, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0};
    aTextStyle_t header_style = {.type=FONT_GAME, .fg=COLOR_TITLE, .bg={0,0,0,0},
                                 .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0};
    aTextStyle_t label_style = {.type=FONT_GAME, .fg=COLOR_LABEL, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0};
    aTextStyle_t value_style = {.type=FONT_GAME, .fg=COLOR_VALUE, .bg={0,0,0,0},
                                .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0};
    aTextStyle_t selected_style = {.type=FONT_GAME, .fg=COLOR_SELECTED, .bg={0,0,0,0},
                                   .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0};

    // Title
    a_DrawText(section->title, SCREEN_WIDTH / 2, SETTINGS_TITLE_Y, title_style);

    // ========================================================================
    // AUDIO SECTION
    // ========================================================================

    a_DrawText("AUDIO", SETTINGS_LABEL_X, SETTINGS_AUDIO_HEADER_Y, header_style);

    // Sound Volume (index 0)
    int y = SETTINGS_AUDIO_START_Y;
    bool is_selected_0 = (section->selected_index == 0);
    bool is_hovered_0 = (section->hovered_index == 0);
    aTextStyle_t lbl0 = (is_selected_0 || is_hovered_0) ? selected_style : label_style;
    aTextStyle_t val0 = (is_selected_0 || is_hovered_0) ? selected_style : value_style;

    a_DrawText("Sound Volume", SETTINGS_LABEL_X, y, lbl0);

    dString_t* vol1 = d_StringInit();
    d_StringFormat(vol1, "%d%%", section->settings->sound_volume);
    a_DrawText(d_StringPeek(vol1), SETTINGS_VALUE_X, y, val0);
    d_StringDestroy(vol1);

    // Volume bar (ALWAYS VISIBLE)
    int bar_w = 200;
    int bar_h = 20;
    int filled_w = (bar_w * section->settings->sound_volume) / 100;
    aColor_t bar_color = is_selected_0 ? COLOR_LABEL : COLOR_BAR_INACTIVE;
    aColor_t fill_color = is_selected_0 ? COLOR_VALUE : COLOR_BAR_FILL;
    a_DrawRect((aRectf_t){SETTINGS_BAR_X, y + 5, bar_w, bar_h}, bar_color);
    a_DrawFilledRect((aRectf_t){SETTINGS_BAR_X, y + 5, filled_w, bar_h}, fill_color);

    // Music Volume (index 1)
    y += SETTINGS_ITEM_HEIGHT;
    bool is_selected_1 = (section->selected_index == 1);
    bool is_hovered_1 = (section->hovered_index == 1);
    aTextStyle_t lbl1 = (is_selected_1 || is_hovered_1) ? selected_style : label_style;
    aTextStyle_t val1 = (is_selected_1 || is_hovered_1) ? selected_style : value_style;

    a_DrawText("Music Volume", SETTINGS_LABEL_X, y, lbl1);

    dString_t* vol2 = d_StringInit();
    d_StringFormat(vol2, "%d%%", section->settings->music_volume);
    a_DrawText(d_StringPeek(vol2), SETTINGS_VALUE_X, y, val1);
    d_StringDestroy(vol2);

    // Volume bar (ALWAYS VISIBLE)
    filled_w = (bar_w * section->settings->music_volume) / 100;
    bar_color = is_selected_1 ? COLOR_LABEL : COLOR_BAR_INACTIVE;
    fill_color = is_selected_1 ? COLOR_VALUE : COLOR_BAR_FILL;
    a_DrawRect((aRectf_t){SETTINGS_BAR_X, y + 5, bar_w, bar_h}, bar_color);
    a_DrawFilledRect((aRectf_t){SETTINGS_BAR_X, y + 5, filled_w, bar_h}, fill_color);

    // ========================================================================
    // GRAPHICS SECTION
    // ========================================================================

    a_DrawText("GRAPHICS", SETTINGS_LABEL_X, SETTINGS_GRAPHICS_HEADER_Y, header_style);

    // Resolution (index 2)
    y = SETTINGS_GRAPHICS_START_Y;
    bool is_selected_2 = (section->selected_index == 2);
    bool is_hovered_2 = (section->hovered_index == 2);
    aTextStyle_t lbl2 = (is_selected_2 || is_hovered_2) ? selected_style : label_style;
    aTextStyle_t val2 = (is_selected_2 || is_hovered_2) ? selected_style : value_style;

    a_DrawText("Resolution", SETTINGS_LABEL_X, y, lbl2);

    const Resolution_t* res = Settings_GetCurrentResolution(section->settings);
    a_DrawText(res ? res->label : "Unknown", SETTINGS_VALUE_X, y, val2);

    // Fullscreen (index 3)
    y += SETTINGS_ITEM_HEIGHT;
    bool is_selected_3 = (section->selected_index == 3);
    bool is_hovered_3 = (section->hovered_index == 3);
    aTextStyle_t lbl3 = (is_selected_3 || is_hovered_3) ? selected_style : label_style;
    aTextStyle_t val3 = (is_selected_3 || is_hovered_3) ? selected_style : value_style;

    a_DrawText("Fullscreen", SETTINGS_LABEL_X, y, lbl3);
    a_DrawText(section->settings->fullscreen ? "ON" : "OFF", SETTINGS_VALUE_X, y, val3);

    // V-Sync (index 4)
    y += SETTINGS_ITEM_HEIGHT;
    bool is_selected_4 = (section->selected_index == 4);
    bool is_hovered_4 = (section->hovered_index == 4);
    aTextStyle_t lbl4 = (is_selected_4 || is_hovered_4) ? selected_style : label_style;
    aTextStyle_t val4 = (is_selected_4 || is_hovered_4) ? selected_style : value_style;

    a_DrawText("V-Sync", SETTINGS_LABEL_X, y, lbl4);
    a_DrawText(section->settings->vsync ? "ON" : "OFF", SETTINGS_VALUE_X, y, val4);

    // ========================================================================
    // BOTTOM BUTTONS
    // ========================================================================

    // Save button (index 5)
    int btn_w = 200;
    int btn_h = 50;
    int save_x = SCREEN_WIDTH / 2 - btn_w - 20;
    bool is_selected_5 = (section->selected_index == 5);
    bool is_hovered_5 = (section->hovered_index == 5);
    aColor_t save_color = (is_selected_5 || is_hovered_5) ? COLOR_SELECTED : COLOR_LABEL;

    a_DrawRect((aRectf_t){save_x, SETTINGS_BUTTONS_Y, btn_w, btn_h}, save_color);
    a_DrawText("SAVE", save_x + btn_w / 2, SETTINGS_BUTTONS_Y + 15,
               (aTextStyle_t){.type=FONT_GAME, .fg=save_color, .bg={0,0,0,0},
                              .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Back button (index 6)
    int back_x = SCREEN_WIDTH / 2 + 20;
    bool is_selected_6 = (section->selected_index == 6);
    bool is_hovered_6 = (section->hovered_index == 6);
    aColor_t back_color = (is_selected_6 || is_hovered_6) ? COLOR_SELECTED : COLOR_LABEL;

    a_DrawRect((aRectf_t){back_x, SETTINGS_BUTTONS_Y, btn_w, btn_h}, back_color);
    a_DrawText("BACK", back_x + btn_w / 2, SETTINGS_BUTTONS_Y + 15,
               (aTextStyle_t){.type=FONT_GAME, .fg=back_color, .bg={0,0,0,0},
                              .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Instructions
    a_DrawText(section->instructions, SCREEN_WIDTH / 2, SETTINGS_INSTRUCTIONS_Y,
               (aTextStyle_t){.type=FONT_GAME, .fg=COLOR_INSTRUC, .bg={0,0,0,0},
                              .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
}
