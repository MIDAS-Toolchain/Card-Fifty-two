/*
 * Pause Menu Section Implementation
 */

#include "../../../include/scenes/sections/pauseMenuSection.h"
#include "../../../include/scenes/sceneBlackjack.h"

// Layout constants
#define PAUSE_PANEL_HEADER_HEIGHT  50
#define PAUSE_PANEL_WIDTH          400
#define PAUSE_PANEL_HEIGHT         400  // Increased from 350 to fit header
#define CONFIRM_PANEL_WIDTH        450  // Increased from 340 for better presence
#define CONFIRM_PANEL_HEIGHT       240  // Increased from 180 for better spacing
#define MENU_ITEM_GAP              45
#define MENU_START_Y               220  // Adjusted down for header (was 200)

// Color palette (strictly adhering to provided palette)
#define COLOR_OVERLAY           ((aColor_t){9, 10, 20, 180})     // #090a14 - almost black overlay
#define COLOR_PANEL_BG          ((aColor_t){9, 10, 20, 240})     // #090a14 - almost black (matches main menu)
#define COLOR_HEADER_BG         ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy blue
#define COLOR_HEADER_TEXT       ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream
#define COLOR_HEADER_BORDER     ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue (matches button)

#define COLOR_MENU_NORMAL       ((aColor_t){164, 221, 219, 255}) // #a4dddb - light cyan
#define COLOR_MENU_SELECTED     ((aColor_t){223, 132, 165, 255}) // #df84a5 - pink
#define COLOR_MENU_HOVER        ((aColor_t){115, 190, 211, 255}) // #73bed3 - cyan blue

#define COLOR_DANGER_NORMAL     ((aColor_t){207, 87, 60, 255})   // #cf573c - red-orange
#define COLOR_DANGER_SELECTED   ((aColor_t){222, 158, 65, 255})  // #de9e41 - orange

#define COLOR_CONFIRM_BG        ((aColor_t){96, 44, 44, 245})    // #602c2c - dark red
#define COLOR_CONFIRM_TEXT      ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream

// ============================================================================
// PAUSE MENU SECTION LIFECYCLE
// ============================================================================

PauseMenuSection_t* CreatePauseMenuSection(void) {
    PauseMenuSection_t* section = malloc(sizeof(PauseMenuSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate PauseMenuSection");
        return NULL;
    }

    section->is_visible = false;
    section->show_confirm_dialog = false;
    section->selected_option = 0;
    section->confirm_selected = 1;  // Default to "No"

    // Create main menu items
    const char* labels[] = {"Resume", "Settings", "Help", "Return to Main Menu"};
    int center_x = SCREEN_WIDTH / 2;
    int start_y = MENU_START_Y;

    for (int i = 0; i < 4; i++) {
        section->menu_items[i] = CreateMenuItem(center_x, start_y + (i * MENU_ITEM_GAP), 0, 25, labels[i]);
        if (!section->menu_items[i]) {
            // Cleanup on failure
            for (int j = 0; j < i; j++) {
                DestroyMenuItem(&section->menu_items[j]);
            }
            free(section);
            d_LogFatal("Failed to create pause menu item");
            return NULL;
        }
    }

    // Mark first item as selected
    SetMenuItemSelected(section->menu_items[0], true);

    // Create confirmation dialog items
    const char* confirm_labels[] = {"Yes", "No"};
    int confirm_y = SCREEN_HEIGHT / 2 + 30;
    int confirm_spacing = 120;

    for (int i = 0; i < 2; i++) {
        int x = (SCREEN_WIDTH / 2) - 60 + (i * confirm_spacing);
        section->confirm_items[i] = CreateMenuItemRow(x, confirm_y, 0, 25, confirm_labels[i]);
        if (!section->confirm_items[i]) {
            // Cleanup on failure
            for (int j = 0; j < 4; j++) {
                DestroyMenuItem(&section->menu_items[j]);
            }
            for (int j = 0; j < i; j++) {
                DestroyMenuItemRow(&section->confirm_items[j]);
            }
            free(section);
            d_LogFatal("Failed to create confirm item");
            return NULL;
        }
    }

    // Mark "No" as selected by default
    SetMenuItemRowSelected(section->confirm_items[1], true);

    d_LogInfo("PauseMenuSection created");
    return section;
}

void DestroyPauseMenuSection(PauseMenuSection_t** section) {
    if (!section || !*section) return;

    PauseMenuSection_t* menu = *section;

    // Destroy main menu items
    for (int i = 0; i < 4; i++) {
        if (menu->menu_items[i]) {
            DestroyMenuItem(&menu->menu_items[i]);
        }
    }

    // Destroy confirmation items
    for (int i = 0; i < 2; i++) {
        if (menu->confirm_items[i]) {
            DestroyMenuItemRow(&menu->confirm_items[i]);
        }
    }

    free(menu);
    *section = NULL;
    d_LogInfo("PauseMenuSection destroyed");
}

// ============================================================================
// PAUSE MENU VISIBILITY
// ============================================================================

void ShowPauseMenu(PauseMenuSection_t* section) {
    if (!section) return;
    section->is_visible = true;
    section->show_confirm_dialog = false;
    section->selected_option = 0;
    SetMenuItemSelected(section->menu_items[0], true);
}

void HidePauseMenu(PauseMenuSection_t* section) {
    if (!section) return;
    section->is_visible = false;
    section->show_confirm_dialog = false;
}

// ============================================================================
// PAUSE MENU INPUT HANDLING
// ============================================================================

PauseAction_t HandlePauseMenuInput(PauseMenuSection_t* section) {
    if (!section || !section->is_visible) return PAUSE_ACTION_NONE;

    // Handle confirmation dialog if visible
    if (section->show_confirm_dialog) {
        // ESC - Cancel confirmation
        if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
            app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
            section->show_confirm_dialog = false;
            return PAUSE_ACTION_NONE;
        }

        // Navigation (Left/Right, A/D, or Up/Down, W/S for accessibility)
        if (app.keyboard[SDL_SCANCODE_A] || app.keyboard[SDL_SCANCODE_LEFT] ||
            app.keyboard[SDL_SCANCODE_W] || app.keyboard[SDL_SCANCODE_UP]) {
            app.keyboard[SDL_SCANCODE_A] = 0;
            app.keyboard[SDL_SCANCODE_LEFT] = 0;
            app.keyboard[SDL_SCANCODE_W] = 0;
            app.keyboard[SDL_SCANCODE_UP] = 0;
            SetMenuItemRowSelected(section->confirm_items[section->confirm_selected], false);
            section->confirm_selected = 0;  // Yes
            SetMenuItemRowSelected(section->confirm_items[section->confirm_selected], true);
            d_LogInfo("Confirm dialog: Selected 'Yes'");
        }
        if (app.keyboard[SDL_SCANCODE_D] || app.keyboard[SDL_SCANCODE_RIGHT] ||
            app.keyboard[SDL_SCANCODE_S] || app.keyboard[SDL_SCANCODE_DOWN]) {
            app.keyboard[SDL_SCANCODE_D] = 0;
            app.keyboard[SDL_SCANCODE_RIGHT] = 0;
            app.keyboard[SDL_SCANCODE_S] = 0;
            app.keyboard[SDL_SCANCODE_DOWN] = 0;
            SetMenuItemRowSelected(section->confirm_items[section->confirm_selected], false);
            section->confirm_selected = 1;  // No
            SetMenuItemRowSelected(section->confirm_items[section->confirm_selected], true);
            d_LogInfo("Confirm dialog: Selected 'No'");
        }

        // Mouse hover auto-select
        for (int i = 0; i < 2; i++) {
            if (IsMenuItemRowHovered(section->confirm_items[i]) && i != section->confirm_selected) {
                SetMenuItemRowSelected(section->confirm_items[section->confirm_selected], false);
                section->confirm_selected = i;
                SetMenuItemRowSelected(section->confirm_items[section->confirm_selected], true);
                break;
            }
        }

        // Selection (Enter/Space or mouse click)
        bool select_pressed = app.keyboard[SDL_SCANCODE_RETURN] ||
                             app.keyboard[SDL_SCANCODE_SPACE] ||
                             app.keyboard[SDL_SCANCODE_KP_ENTER];

        if (select_pressed) {
            app.keyboard[SDL_SCANCODE_RETURN] = 0;
            app.keyboard[SDL_SCANCODE_SPACE] = 0;
            app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

            d_LogInfoF("Confirm dialog: Enter pressed - confirm_selected=%d (%s)",
                       section->confirm_selected,
                       section->confirm_selected == 0 ? "Yes" : "No");

            if (section->confirm_selected == 0) {  // Yes
                d_LogInfo("Confirm dialog: Quitting to main menu");
                return PAUSE_ACTION_QUIT_TO_MENU;
            } else {  // No
                d_LogInfo("Confirm dialog: Cancelled");
                section->show_confirm_dialog = false;
                return PAUSE_ACTION_NONE;
            }
        }

        // Mouse click handling
        for (int i = 0; i < 2; i++) {
            if (IsMenuItemRowClicked(section->confirm_items[i])) {
                if (i == 0) {  // Yes
                    return PAUSE_ACTION_QUIT_TO_MENU;
                } else {  // No
                    section->show_confirm_dialog = false;
                    return PAUSE_ACTION_NONE;
                }
            }
        }

        return PAUSE_ACTION_NONE;
    }

    // Handle main menu input
    // ESC - Close pause menu
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        return PAUSE_ACTION_RESUME;
    }

    // Navigation (W/S or Up/Down)
    if (app.keyboard[SDL_SCANCODE_W] || app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_W] = 0;
        app.keyboard[SDL_SCANCODE_UP] = 0;
        SetMenuItemSelected(section->menu_items[section->selected_option], false);
        section->selected_option = (section->selected_option - 1 + 4) % 4;
        SetMenuItemSelected(section->menu_items[section->selected_option], true);
    }
    if (app.keyboard[SDL_SCANCODE_S] || app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_S] = 0;
        app.keyboard[SDL_SCANCODE_DOWN] = 0;
        SetMenuItemSelected(section->menu_items[section->selected_option], false);
        section->selected_option = (section->selected_option + 1) % 4;
        SetMenuItemSelected(section->menu_items[section->selected_option], true);
    }

    // Mouse hover auto-select
    for (int i = 0; i < 4; i++) {
        if (IsMenuItemHovered(section->menu_items[i]) && i != section->selected_option) {
            SetMenuItemSelected(section->menu_items[section->selected_option], false);
            section->selected_option = i;
            SetMenuItemSelected(section->menu_items[section->selected_option], true);
            break;
        }
    }

    // Selection (Enter/Space or mouse click)
    bool select_pressed = app.keyboard[SDL_SCANCODE_RETURN] ||
                         app.keyboard[SDL_SCANCODE_SPACE] ||
                         app.keyboard[SDL_SCANCODE_KP_ENTER];

    if (select_pressed) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_SPACE] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

        switch (section->selected_option) {
            case 0: return PAUSE_ACTION_RESUME;
            case 1: return PAUSE_ACTION_SETTINGS;
            case 2: return PAUSE_ACTION_HELP;
            case 3:
                // Show confirmation dialog
                section->show_confirm_dialog = true;
                section->confirm_selected = 1;  // Default to "No"
                SetMenuItemRowSelected(section->confirm_items[0], false);
                SetMenuItemRowSelected(section->confirm_items[1], true);
                return PAUSE_ACTION_NONE;
        }
    }

    // Mouse click handling
    for (int i = 0; i < 4; i++) {
        if (IsMenuItemClicked(section->menu_items[i])) {
            switch (i) {
                case 0: return PAUSE_ACTION_RESUME;
                case 1: return PAUSE_ACTION_SETTINGS;
                case 2: return PAUSE_ACTION_HELP;
                case 3:
                    section->show_confirm_dialog = true;
                    section->confirm_selected = 1;  // Default to "No"
                    SetMenuItemRowSelected(section->confirm_items[0], false);
                    SetMenuItemRowSelected(section->confirm_items[1], true);
                    return PAUSE_ACTION_NONE;
            }
        }
    }

    return PAUSE_ACTION_NONE;
}

// ============================================================================
// PAUSE MENU RENDERING
// ============================================================================

void RenderPauseMenuSection(PauseMenuSection_t* section) {
    if (!section || !section->is_visible) return;

    // Full-screen semi-transparent overlay
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                     COLOR_OVERLAY.r, COLOR_OVERLAY.g, COLOR_OVERLAY.b, COLOR_OVERLAY.a);

    // Centered menu panel
    int panel_x = (SCREEN_WIDTH - PAUSE_PANEL_WIDTH) / 2;
    int panel_y = (SCREEN_HEIGHT - PAUSE_PANEL_HEIGHT) / 2;

    // Draw main panel background (below header)
    int panel_body_y = panel_y + PAUSE_PANEL_HEADER_HEIGHT;
    int panel_body_height = PAUSE_PANEL_HEIGHT - PAUSE_PANEL_HEADER_HEIGHT;
    a_DrawFilledRect(panel_x, panel_body_y, PAUSE_PANEL_WIDTH, panel_body_height,
                     COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, COLOR_PANEL_BG.a);

    // Draw header bar at top
    int header_y = panel_y;
    a_DrawFilledRect(panel_x, header_y, PAUSE_PANEL_WIDTH, PAUSE_PANEL_HEADER_HEIGHT,
                     COLOR_HEADER_BG.r, COLOR_HEADER_BG.g, COLOR_HEADER_BG.b, COLOR_HEADER_BG.a);

    // Draw header bottom border line
    a_DrawRect(panel_x, header_y, PAUSE_PANEL_WIDTH, PAUSE_PANEL_HEADER_HEIGHT,
               COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, COLOR_HEADER_BORDER.a);

    // Draw header text "PAUSE MENU"
    a_DrawText("PAUSE MENU", SCREEN_WIDTH / 2, header_y + 12,
               COLOR_HEADER_TEXT.r, COLOR_HEADER_TEXT.g, COLOR_HEADER_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Render menu items with custom colors
    for (int i = 0; i < 4; i++) {
        MenuItem_t* item = section->menu_items[i];
        if (!item) continue;

        // Calculate text dimensions for hover background
        int text_w, text_h;
        a_CalcTextDimensions((char*)item->label, FONT_ENTER_COMMAND, &text_w, &text_h);

        // Disable hover effects when confirmation dialog is open
        bool is_actually_hovered = section->show_confirm_dialog ? false : IsMenuItemHovered(item);

        // Determine color based on item type and state
        aColor_t text_color;
        if (i == 3) {  // "Return to Main Menu" - danger color
            if (item->is_selected && is_actually_hovered) {
                text_color = COLOR_DANGER_SELECTED;
            } else if (item->is_selected) {
                text_color = COLOR_DANGER_SELECTED;
            } else if (is_actually_hovered) {
                text_color = COLOR_DANGER_NORMAL;
            } else {
                text_color = COLOR_DANGER_NORMAL;
            }
        } else {  // Normal menu items
            if (item->is_selected && is_actually_hovered) {
                text_color = COLOR_MENU_SELECTED;
            } else if (item->is_selected) {
                text_color = COLOR_MENU_SELECTED;
            } else if (is_actually_hovered) {
                text_color = COLOR_MENU_HOVER;
            } else {
                text_color = COLOR_MENU_NORMAL;
            }
        }

        // Update hover state (but force false if modal is open)
        MenuItem_t* mutable_item = (MenuItem_t*)item;
        mutable_item->is_hovered = is_actually_hovered;

        // Draw hover background if hovered (won't happen if modal open)
        if (is_actually_hovered && item->enabled) {
            int left = item->x - (item->w / 2);
            int top = item->y - 2;
            int width = item->w;
            int height = text_h + 4;
            a_DrawFilledRect(left, top, width, height, 255, 255, 255, 25);
        }

        // Draw selection indicator ">" if selected (dynamically positioned)
        if (item->is_selected) {
            // Position indicator based on text width to prevent overlap
            int indicator_gap = 15;  // Gap between ">" and text
            int indicator_x = item->x - (text_w / 2) - indicator_gap;

            a_DrawText(">", indicator_x, item->y,
                       text_color.r, text_color.g, text_color.b,
                       FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
        }

        // Draw label
        a_DrawText((char*)item->label, item->x, item->y,
                   text_color.r, text_color.g, text_color.b,
                   FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    }

    // Render confirmation dialog if visible
    if (section->show_confirm_dialog) {
        // Draw modal backdrop overlay (dims everything beneath)
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 120);

        // Confirmation panel (centered on screen, on top of pause menu)
        int confirm_x = (SCREEN_WIDTH - CONFIRM_PANEL_WIDTH) / 2;
        int confirm_y = (SCREEN_HEIGHT - CONFIRM_PANEL_HEIGHT) / 2;
        a_DrawFilledRect(confirm_x, confirm_y, CONFIRM_PANEL_WIDTH, CONFIRM_PANEL_HEIGHT,
                         COLOR_CONFIRM_BG.r, COLOR_CONFIRM_BG.g, COLOR_CONFIRM_BG.b, COLOR_CONFIRM_BG.a);

        // Confirmation title
        a_DrawText("Are you sure?", SCREEN_WIDTH / 2, confirm_y + 40,
                   COLOR_CONFIRM_TEXT.r, COLOR_CONFIRM_TEXT.g, COLOR_CONFIRM_TEXT.b,
                   FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Confirmation subtitle
        a_DrawText("Your progress will be lost", SCREEN_WIDTH / 2, confirm_y + 70,
                   COLOR_MENU_NORMAL.r, COLOR_MENU_NORMAL.g, COLOR_MENU_NORMAL.b,
                   FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

        // Render Yes/No buttons
        for (int i = 0; i < 2; i++) {
            MenuItemRow_t* item = section->confirm_items[i];
            if (!item) continue;

            int text_w, text_h;
            a_CalcTextDimensions((char*)item->label, FONT_ENTER_COMMAND, &text_w, &text_h);

            aColor_t text_color;
            if (item->is_selected && item->is_hovered) {
                text_color = COLOR_MENU_SELECTED;
            } else if (item->is_selected) {
                text_color = COLOR_MENU_SELECTED;
            } else if (item->is_hovered) {
                text_color = COLOR_MENU_HOVER;
            } else {
                text_color = COLOR_MENU_NORMAL;
            }

            MenuItemRow_t* mutable_item = (MenuItemRow_t*)item;
            mutable_item->is_hovered = IsMenuItemRowHovered(item);

            if (item->is_hovered && item->enabled) {
                int left = item->x - (item->w / 2);
                int top = item->y - 2;
                int width = item->w;
                int height = text_h + 4;
                a_DrawFilledRect(left, top, width, height, 255, 255, 255, 25);
            }

            // Draw selection indicator ">" if selected (dynamically positioned)
            if (item->is_selected) {
                // Tighter gap for horizontal row layout
                int indicator_gap = 10;
                int indicator_x = item->x - (text_w / 2) - indicator_gap;

                a_DrawText(">", indicator_x, item->y,
                           text_color.r, text_color.g, text_color.b,
                           FONT_ENTER_COMMAND, TEXT_ALIGN_RIGHT, 0);
            }

            a_DrawText((char*)item->label, item->x, item->y,
                       text_color.r, text_color.g, text_color.b,
                       FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
        }
    }
}
