/*
 * Tutorial System Implementation
 */

#include "../../include/tutorial/tutorialSystem.h"

// ============================================================================
// TUTORIAL SYSTEM LIFECYCLE
// ============================================================================

TutorialSystem_t* CreateTutorialSystem(void) {
    TutorialSystem_t* system = malloc(sizeof(TutorialSystem_t));
    if (!system) {
        d_LogFatal("Failed to allocate TutorialSystem");
        return NULL;
    }

    system->current_step = NULL;
    system->first_step = NULL;
    system->active = false;
    system->dialogue_visible = false;
    system->skip_confirmation.visible = false;
    system->skip_confirmation.skip_confirmed = false;
    system->waiting_to_advance = false;
    system->advance_delay_timer = 0.0f;

    d_LogInfo("TutorialSystem created");
    return system;
}

void DestroyTutorialSystem(TutorialSystem_t** system) {
    if (!system || !*system) return;

    // Note: Tutorial steps are owned by tutorial content (e.g., blackjackTutorial.c)
    // We don't destroy them here to allow step reuse

    free(*system);
    *system = NULL;
    d_LogInfo("TutorialSystem destroyed");
}

// ============================================================================
// TUTORIAL STEP CREATION
// ============================================================================

TutorialStep_t* CreateTutorialStep(const char* dialogue_text,
                                   TutorialSpotlight_t spotlight,
                                   TutorialListener_t listener,
                                   bool is_final_step,
                                   int dialogue_y_position) {
    if (!dialogue_text) {
        d_LogError("CreateTutorialStep: NULL dialogue_text");
        return NULL;
    }

    TutorialStep_t* step = malloc(sizeof(TutorialStep_t));
    if (!step) {
        d_LogFatal("Failed to allocate TutorialStep");
        return NULL;
    }

    // Create dialogue text (dString_t)
    step->dialogue_text = d_StringInit();
    if (!step->dialogue_text) {
        free(step);
        d_LogFatal("Failed to allocate dialogue text");
        return NULL;
    }
    d_StringSet(step->dialogue_text, dialogue_text, 0);

    // Copy spotlight and listener configs
    step->spotlight = spotlight;
    step->listener = listener;
    step->next_step = NULL;
    step->is_final_step = is_final_step;
    step->dialogue_y_position = dialogue_y_position;

    return step;
}

void DestroyTutorialStep(TutorialStep_t** step) {
    if (!step || !*step) return;

    if ((*step)->dialogue_text) {
        d_StringDestroy((*step)->dialogue_text);
    }

    free(*step);
    *step = NULL;
}

void LinkTutorialSteps(TutorialStep_t* current, TutorialStep_t* next) {
    if (!current) return;
    current->next_step = next;
}

// ============================================================================
// TUTORIAL CONTROL
// ============================================================================

void StartTutorial(TutorialSystem_t* system, TutorialStep_t* first_step) {
    if (!system || !first_step) return;

    system->first_step = first_step;
    system->current_step = first_step;
    system->active = true;
    system->dialogue_visible = true;

    d_LogInfo("Tutorial started");
}

void StopTutorial(TutorialSystem_t* system) {
    if (!system) return;

    system->active = false;
    system->dialogue_visible = false;
    system->current_step = NULL;

    d_LogInfo("Tutorial stopped");
}

void AdvanceTutorial(TutorialSystem_t* system) {
    if (!system || !system->current_step) return;

    // Move to next step
    system->current_step = system->current_step->next_step;

    if (!system->current_step) {
        // No more steps - end tutorial
        StopTutorial(system);
        d_LogInfo("Tutorial completed");
    } else {
        // Show dialogue for next step
        system->dialogue_visible = true;
        d_LogInfo("Tutorial advanced to next step - showing dialogue");
    }
}

// ============================================================================
// TUTORIAL EVENT SYSTEM
// ============================================================================

void UpdateTutorialListeners(TutorialSystem_t* system, float dt) {
    if (!system || !system->active || !system->current_step) return;

    // If waiting to advance, count down timer
    if (system->waiting_to_advance) {
        system->advance_delay_timer -= dt;
        if (system->advance_delay_timer <= 0.0f) {
            // Timer expired - advance now
            system->waiting_to_advance = false;
            system->advance_delay_timer = 0.0f;
            AdvanceTutorial(system);
            system->current_step->listener.triggered = false;  // Reset listener
        }
        return;  // Don't check for new triggers while waiting
    }

    TutorialListener_t* listener = &system->current_step->listener;

    // Check if listener triggered
    if (listener->triggered) {
        // Hide dialogue immediately so user can see game action
        system->dialogue_visible = false;

        // Start 1.0 second delay before advancing
        system->waiting_to_advance = true;
        system->advance_delay_timer = 1.0f;
        d_LogInfo("Tutorial event triggered - hiding dialogue, waiting 1.0s before advancing");
    }
}

void TriggerTutorialEvent(TutorialSystem_t* system, TutorialEventType_t event_type, void* event_data) {
    if (!system || !system->active || !system->current_step) return;

    TutorialListener_t* listener = &system->current_step->listener;

    // Check if event matches current listener
    if (listener->event_type == event_type) {
        // Event-specific matching
        switch (event_type) {
            case TUTORIAL_EVENT_BUTTON_CLICK:
                // Check if clicked button matches expected button
                if (listener->event_data == event_data) {
                    listener->triggered = true;
                }
                break;

            case TUTORIAL_EVENT_STATE_CHANGE:
                // Check if state matches expected state
                if (listener->event_data == event_data) {
                    listener->triggered = true;
                }
                break;

            case TUTORIAL_EVENT_FUNCTION_CALL:
            case TUTORIAL_EVENT_KEY_PRESS:
                // Direct match
                if (listener->event_data == event_data) {
                    listener->triggered = true;
                }
                break;

            case TUTORIAL_EVENT_NONE:
            default:
                break;
        }
    }
}

// ============================================================================
// TUTORIAL INPUT HANDLING
// ============================================================================

bool HandleTutorialInput(TutorialSystem_t* system) {
    if (!system || !system->active || !system->dialogue_visible) {
        return false;
    }

    // If skip confirmation visible, handle it exclusively
    if (system->skip_confirmation.visible) {
        int conf_w = 400;
        int conf_h = 150;
        int conf_x = (SCREEN_WIDTH - conf_w) / 2;
        int conf_y = (SCREEN_HEIGHT - conf_h) / 2;

        // Keyboard shortcuts for confirmation modal
        if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
            app.keyboard[SDL_SCANCODE_RETURN] = 0;
            app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;
            // ENTER confirms skip (YES)
            system->skip_confirmation.skip_confirmed = true;
            system->skip_confirmation.visible = false;
            StopTutorial(system);
            return true;
        }

        if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
            app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
            // ESC cancels skip (NO)
            system->skip_confirmation.visible = false;
            return true;
        }

        if (app.mouse.pressed) {
            // YES button (60, 90, 100, 40 relative to conf_x, conf_y)
            int yes_x = conf_x + 60;
            int yes_y = conf_y + 90;
            if (app.mouse.x >= yes_x && app.mouse.x <= yes_x + 100 &&
                app.mouse.y >= yes_y && app.mouse.y <= yes_y + 40) {
                // User confirmed skip
                system->skip_confirmation.skip_confirmed = true;
                system->skip_confirmation.visible = false;
                StopTutorial(system);
                return true;
            }

            // NO button (240, 90, 100, 40 relative to conf_x, conf_y)
            int no_x = conf_x + 240;
            int no_y = conf_y + 90;
            if (app.mouse.x >= no_x && app.mouse.x <= no_x + 100 &&
                app.mouse.y >= no_y && app.mouse.y <= no_y + 40) {
                // User canceled skip
                system->skip_confirmation.visible = false;
                return true;
            }
        }
        return true;  // Consume all input while confirmation visible
    }

    // Check for skip/finish button click
    bool button_clicked = false;

    // Mouse click on skip/finish button
    if (app.mouse.pressed) {
        int dialogue_x = (SCREEN_WIDTH - DIALOGUE_WIDTH) / 2;
        int dialogue_y = system->current_step->dialogue_y_position;  // Use step-specific position
        int button_x = dialogue_x + DIALOGUE_WIDTH - DIALOGUE_ARROW_MARGIN - 80;
        int button_y = dialogue_y + DIALOGUE_HEIGHT - DIALOGUE_ARROW_MARGIN - 30;

        if (app.mouse.x >= button_x && app.mouse.x <= button_x + 80 &&
            app.mouse.y >= button_y && app.mouse.y <= button_y + 30) {
            button_clicked = true;
        }
    }

    // Keyboard shortcuts
    if (system->current_step->is_final_step) {
        // Final step: Allow ENTER or ESC to finish tutorial
        if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER] ||
            app.keyboard[SDL_SCANCODE_ESCAPE]) {
            button_clicked = true;
            app.keyboard[SDL_SCANCODE_RETURN] = 0;
            app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;
            app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        }
    }
    // Non-final steps: No keyboard shortcuts (ENTER reserved for game controls, mouse click only)

    if (button_clicked) {
        if (system->current_step->is_final_step) {
            // Final step - just advance (end tutorial)
            AdvanceTutorial(system);
        } else {
            // Non-final step - show skip confirmation
            system->skip_confirmation.visible = true;
        }
        return true;
    }

    return false;
}

// ============================================================================
// TUTORIAL RENDERING HELPERS
// ============================================================================

// Helper function to draw properly centered tutorial button with hover effect
static void DrawTutorialButton(int x, int y, int w, int h,
                               const char* text,
                               aColor_t bg_color,
                               aColor_t text_color,
                               bool draw_border,
                               bool is_hovered) {
    // Apply hover effect (25% lighter, like Button component)
    aColor_t final_bg = bg_color;
    if (is_hovered) {
        final_bg.r = (bg_color.r * 1.25f > 255) ? 255 : (unsigned char)(bg_color.r * 1.25f);
        final_bg.g = (bg_color.g * 1.25f > 255) ? 255 : (unsigned char)(bg_color.g * 1.25f);
        final_bg.b = (bg_color.b * 1.25f > 255) ? 255 : (unsigned char)(bg_color.b * 1.25f);
    }

    // Draw background with hover color
    a_DrawFilledRect(x, y, w, h, final_bg.r, final_bg.g, final_bg.b, final_bg.a);

    // Draw border if requested
    if (draw_border) {
        a_DrawRect(x, y, w, h, 255, 255, 255, 255);
    }

    // Calculate text dimensions and center properly (like Button component)
    int text_w, text_h;
    a_CalcTextDimensions((char*)text, FONT_ENTER_COMMAND, &text_w, &text_h);

    int text_x = x + w / 2;
    int text_y = y + (h - text_h) / 2;  // Proper vertical centering!

    a_DrawText((char*)text, text_x, text_y,
               text_color.r, text_color.g, text_color.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
}

// ============================================================================
// TUTORIAL RENDERING
// ============================================================================

static void RenderOverlayWithSpotlight(const TutorialSpotlight_t* spotlight) {
    // If no spotlight, draw full overlay
    if (!spotlight || spotlight->type == SPOTLIGHT_NONE) {
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, TUTORIAL_OVERLAY_ALPHA);
        return;
    }

    // Calculate spotlight bounds with padding
    int spotlight_x, spotlight_y, spotlight_w, spotlight_h;

    switch (spotlight->type) {
        case SPOTLIGHT_RECTANGLE:
            spotlight_x = spotlight->x - TUTORIAL_SPOTLIGHT_PADDING;
            spotlight_y = spotlight->y - TUTORIAL_SPOTLIGHT_PADDING;
            spotlight_w = spotlight->w + (TUTORIAL_SPOTLIGHT_PADDING * 2);
            spotlight_h = spotlight->h + (TUTORIAL_SPOTLIGHT_PADDING * 2);
            break;

        case SPOTLIGHT_CIRCLE:
            spotlight_x = spotlight->x - spotlight->w - TUTORIAL_SPOTLIGHT_PADDING;
            spotlight_y = spotlight->y - spotlight->h - TUTORIAL_SPOTLIGHT_PADDING;
            spotlight_w = (spotlight->w * 2) + (TUTORIAL_SPOTLIGHT_PADDING * 2);
            spotlight_h = (spotlight->h * 2) + (TUTORIAL_SPOTLIGHT_PADDING * 2);
            break;

        default:
            a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, TUTORIAL_OVERLAY_ALPHA);
            return;
    }

    // Draw overlay as 4 rectangles around spotlight (creates true cutout)
    // Top rectangle
    if (spotlight_y > 0) {
        a_DrawFilledRect(0, 0, SCREEN_WIDTH, spotlight_y, 0, 0, 0, TUTORIAL_OVERLAY_ALPHA);
    }

    // Left rectangle
    if (spotlight_x > 0) {
        a_DrawFilledRect(0, spotlight_y, spotlight_x, spotlight_h, 0, 0, 0, TUTORIAL_OVERLAY_ALPHA);
    }

    // Right rectangle
    int right_x = spotlight_x + spotlight_w;
    if (right_x < SCREEN_WIDTH) {
        a_DrawFilledRect(right_x, spotlight_y, SCREEN_WIDTH - right_x, spotlight_h, 0, 0, 0, TUTORIAL_OVERLAY_ALPHA);
    }

    // Bottom rectangle
    int bottom_y = spotlight_y + spotlight_h;
    if (bottom_y < SCREEN_HEIGHT) {
        a_DrawFilledRect(0, bottom_y, SCREEN_WIDTH, SCREEN_HEIGHT - bottom_y, 0, 0, 0, TUTORIAL_OVERLAY_ALPHA);
    }
}

static void RenderDialogue(const TutorialStep_t* step) {
    if (!step || !step->dialogue_text) return;

    // Dialogue box position (use step-specific Y position)
    int dialogue_x = (SCREEN_WIDTH - DIALOGUE_WIDTH) / 2;
    int dialogue_y = step->dialogue_y_position;

    // Draw dialogue background
    a_DrawFilledRect(dialogue_x, dialogue_y, DIALOGUE_WIDTH, DIALOGUE_HEIGHT,
                     DIALOGUE_BG.r, DIALOGUE_BG.g, DIALOGUE_BG.b, DIALOGUE_BG.a);

    // Draw dialogue border
    a_DrawRect(dialogue_x, dialogue_y, DIALOGUE_WIDTH, DIALOGUE_HEIGHT,
               DIALOGUE_BORDER.r, DIALOGUE_BORDER.g, DIALOGUE_BORDER.b, DIALOGUE_BORDER.a);

    // Draw dialogue text (split by \n and render each line separately)
    int text_x = dialogue_x + DIALOGUE_PADDING;
    int text_y = dialogue_y + DIALOGUE_PADDING;

    // Get full text
    const char* full_text = d_StringPeek(step->dialogue_text);

    // Split by \n and render each line
    char line_buffer[256];
    int line_index = 0;
    int char_index = 0;

    for (int i = 0; full_text[i] != '\0'; i++) {
        if (full_text[i] == '\n') {
            // End of line - render it
            line_buffer[char_index] = '\0';

            // Calculate line height for this line
            int line_w, line_h;
            a_CalcTextDimensions(line_buffer, FONT_ENTER_COMMAND, &line_w, &line_h);

            // Render line
            a_DrawText(line_buffer, text_x, text_y,
                      DIALOGUE_TEXT.r, DIALOGUE_TEXT.g, DIALOGUE_TEXT.b,
                      FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

            // Move Y position down by line height
            text_y += line_h;

            // Reset for next line
            char_index = 0;
            line_index++;
        } else {
            // Add character to current line
            line_buffer[char_index++] = full_text[i];
        }
    }

    // Render final line (no trailing \n)
    if (char_index > 0) {
        line_buffer[char_index] = '\0';
        a_DrawText(line_buffer, text_x, text_y,
                  DIALOGUE_TEXT.r, DIALOGUE_TEXT.g, DIALOGUE_TEXT.b,
                  FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    }

    // Draw skip/finish button at bottom-right using helper
    const char* button_text = step->is_final_step ? "Finish" : "Skip";
    int button_x = dialogue_x + DIALOGUE_WIDTH - DIALOGUE_ARROW_MARGIN - 80;
    int button_y = dialogue_y + DIALOGUE_HEIGHT - DIALOGUE_ARROW_MARGIN - 30;

    // Check if button is hovered
    bool skip_hovered = (app.mouse.x >= button_x && app.mouse.x <= button_x + 80 &&
                         app.mouse.y >= button_y && app.mouse.y <= button_y + 30);

    // Use helper for properly centered text with hover effect
    aColor_t button_bg = {DIALOGUE_ARROW.r, DIALOGUE_ARROW.g, DIALOGUE_ARROW.b, 100};
    DrawTutorialButton(button_x, button_y, 80, 30,
                       button_text,
                       button_bg,
                       DIALOGUE_TEXT,
                       true,
                       skip_hovered);
}

static void RenderSkipConfirmation(void) {
    // Semi-transparent overlay
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 0, 0, 0, 150);

    // Confirmation dialogue (smaller than main dialogue)
    int conf_w = 400;
    int conf_h = 150;
    int conf_x = (SCREEN_WIDTH - conf_w) / 2;
    int conf_y = (SCREEN_HEIGHT - conf_h) / 2;

    // Draw confirmation background
    a_DrawFilledRect(conf_x, conf_y, conf_w, conf_h,
                     DIALOGUE_BG.r, DIALOGUE_BG.g, DIALOGUE_BG.b, DIALOGUE_BG.a);

    // Draw confirmation border
    a_DrawRect(conf_x, conf_y, conf_w, conf_h,
               DIALOGUE_BORDER.r, DIALOGUE_BORDER.g, DIALOGUE_BORDER.b, DIALOGUE_BORDER.a);

    // Draw confirmation text
    a_DrawText("Skip Tutorial?", conf_x + conf_w / 2, conf_y + 30,
               DIALOGUE_TEXT.r, DIALOGUE_TEXT.g, DIALOGUE_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw YES button using helper with hover detection
    int yes_x = conf_x + 60;
    int yes_y = conf_y + 90;
    bool yes_hovered = (app.mouse.x >= yes_x && app.mouse.x <= yes_x + 100 &&
                        app.mouse.y >= yes_y && app.mouse.y <= yes_y + 40);

    aColor_t red_bg = {165, 48, 48, 255};
    aColor_t white_text = {255, 255, 255, 255};
    DrawTutorialButton(yes_x, yes_y, 100, 40,
                       "YES",
                       red_bg,
                       white_text,
                       true,
                       yes_hovered);

    // Draw NO button using helper with hover detection
    int no_x = conf_x + 240;
    int no_y = conf_y + 90;
    bool no_hovered = (app.mouse.x >= no_x && app.mouse.x <= no_x + 100 &&
                       app.mouse.y >= no_y && app.mouse.y <= no_y + 40);

    aColor_t green_bg = {117, 167, 67, 255};
    DrawTutorialButton(no_x, no_y, 100, 40,
                       "NO",
                       green_bg,
                       white_text,
                       true,
                       no_hovered);
}

static void RenderPointingArrow(const TutorialSpotlight_t* spotlight) {
    if (!spotlight || !spotlight->show_arrow || spotlight->type == SPOTLIGHT_NONE) return;

    // TODO: Draw arrow from dialogue to spotlight
    // For now, skip arrow rendering (requires vector drawing)
}

void RenderTutorial(TutorialSystem_t* system) {
    if (!system || !system->active || !system->dialogue_visible || !system->current_step) {
        return;
    }

    TutorialStep_t* step = system->current_step;

    // Render overlay with spotlight
    RenderOverlayWithSpotlight(&step->spotlight);

    // Render dialogue modal
    RenderDialogue(step);

    // Render pointing arrow (if enabled)
    RenderPointingArrow(&step->spotlight);

    // Render skip confirmation if visible
    if (system->skip_confirmation.visible) {
        RenderSkipConfirmation();
    }
}

// ============================================================================
// TUTORIAL QUERIES
// ============================================================================

bool IsTutorialActive(const TutorialSystem_t* system) {
    return system && system->active;
}
