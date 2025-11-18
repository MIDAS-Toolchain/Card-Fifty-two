/*
 * Tutorial System Implementation
 */

#include "../../include/tutorial/tutorialSystem.h"
#include <math.h>

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
    system->waiting_for_betting_state = false;
    system->current_step_number = 0;

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
                                   TutorialListener_t listener,
                                   bool is_final_step,
                                   int dialogue_x_offset,
                                   int dialogue_y_position,
                                   int wait_for_game_state,
                                   TutorialArrow_t arrow,
                                   bool advance_immediately) {
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

    // Copy listener config
    step->listener = listener;
    step->next_step = NULL;
    step->is_final_step = is_final_step;
    step->dialogue_x_offset = dialogue_x_offset;
    step->dialogue_y_position = dialogue_y_position;
    step->wait_for_game_state = wait_for_game_state;
    step->arrow = arrow;
    step->advance_immediately = advance_immediately;

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
    system->current_step_number = 1;  // Start at step 1

    d_LogInfo("Tutorial started");
}

void StopTutorial(TutorialSystem_t* system) {
    if (!system) return;

    system->active = false;
    system->dialogue_visible = false;
    system->current_step = NULL;
    system->current_step_number = 0;  // Reset to 0 (inactive)

    d_LogInfo("Tutorial stopped");
}

void AdvanceTutorial(TutorialSystem_t* system) {
    if (!system || !system->current_step) return;

    // Move to next step
    system->current_step = system->current_step->next_step;
    system->current_step_number++;  // Increment step number

    if (!system->current_step) {
        // No more steps - end tutorial
        StopTutorial(system);
        d_LogInfo("Tutorial completed");
    } else {
        // Show dialogue for next step
        system->dialogue_visible = true;
        d_LogInfoF("Tutorial advanced to step %d - showing dialogue", system->current_step_number);
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
            // Check if next step wants to advance immediately (without waiting for state)
            bool next_advances_immediately = (system->current_step->next_step &&
                                             system->current_step->next_step->advance_immediately);

            if (next_advances_immediately) {
                // Advance to next step immediately (increments step number, unblocking input)
                // BUT keep dialogue hidden until state is reached
                system->waiting_to_advance = false;
                system->advance_delay_timer = 0.0f;
                system->waiting_for_betting_state = false;

                // Move to next step
                system->current_step = system->current_step->next_step;
                system->current_step_number++;

                if (!system->current_step) {
                    StopTutorial(system);
                    d_LogInfo("Tutorial completed");
                } else {
                    // Keep dialogue HIDDEN until state is reached
                    system->dialogue_visible = false;
                    system->current_step->listener.triggered = false;
                    d_LogInfoF("Tutorial advanced to step %d (immediate advance, waiting for state %d to show dialogue)",
                              system->current_step_number, system->current_step->wait_for_game_state);
                }
            } else {
                // Normal behavior: wait for state before advancing
                if (!system->waiting_for_betting_state) {
                    system->waiting_to_advance = false;
                    system->advance_delay_timer = 0.0f;
                    AdvanceTutorial(system);
                    if (system->current_step) {
                        system->current_step->listener.triggered = false;
                    }
                }
            }
        }
        return;  // Don't check for new triggers while waiting
    }

    TutorialListener_t* listener = &system->current_step->listener;

    // Check if listener triggered
    if (listener->triggered) {
        // Hide dialogue immediately so user can see game action
        system->dialogue_visible = false;

        // Start 1.0 second delay + wait for BETTING state before advancing
        system->waiting_to_advance = true;
        system->advance_delay_timer = 1.0f;
        system->waiting_for_betting_state = true;
        d_LogInfo("Tutorial event triggered - hiding dialogue, waiting for BETTING state to advance");
    }
}

void CheckTutorialGameState(TutorialSystem_t* system, int game_state) {
    if (!system || !system->current_step) return;

    // Case 1: Waiting to advance to next step (normal flow)
    if (system->waiting_for_betting_state) {
        // Check if we've reached the required game state for the NEXT step
        if (system->current_step->next_step) {
            int required_state = system->current_step->next_step->wait_for_game_state;
            if (required_state == -1 || required_state == game_state) {
                system->waiting_for_betting_state = false;
                d_LogInfoF("Tutorial: Ready to advance (required state: %d, current: %d)", required_state, game_state);
            }
        }
    }

    // Case 2: Current step advanced immediately but dialogue hidden, waiting for state to show it
    if (!system->dialogue_visible && system->current_step->advance_immediately) {
        int required_state = system->current_step->wait_for_game_state;
        if (required_state == -1 || required_state == game_state) {
            // State reached - show dialogue now
            system->dialogue_visible = true;
            d_LogInfoF("Tutorial: Step %d state reached (%d) - showing dialogue", system->current_step_number, game_state);
        }
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
            case TUTORIAL_EVENT_HOVER:
                // Direct match (or NULL for any hover)
                if (listener->event_data == event_data || listener->event_data == NULL) {
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
        int dialogue_x = ((SCREEN_WIDTH - DIALOGUE_WIDTH) / 2) + system->current_step->dialogue_x_offset;
        int dialogue_y = system->current_step->dialogue_y_position;
        int button_x = dialogue_x + DIALOGUE_WIDTH - DIALOGUE_ARROW_MARGIN - 80;
        int button_y = dialogue_y + DIALOGUE_HEIGHT - DIALOGUE_ARROW_MARGIN - 30;

        // DEBUG: Log click and bounds (rate-limited to once per second)
        d_LogRateLimitedF(D_LOG_RATE_LIMIT_FLAG_HASH_FORMAT_STRING, D_LOG_LEVEL_INFO, 1, 1.0,
                         "🖱️ CLICK at (%d,%d) | Dialogue: (%d,%d)-(%d,%d)",
                         app.mouse.x, app.mouse.y,
                         dialogue_x, dialogue_y, dialogue_x + DIALOGUE_WIDTH, dialogue_y + DIALOGUE_HEIGHT);

        // Check if click is on skip button
        if (app.mouse.x >= button_x && app.mouse.x <= button_x + 80 &&
            app.mouse.y >= button_y && app.mouse.y <= button_y + 30) {
            d_LogInfo("  ➡️ Click on SKIP button - consuming");
            button_clicked = true;
            app.mouse.pressed = 0;
        }
        // Check if click is anywhere INSIDE dialogue box
        else if (app.mouse.x >= dialogue_x && app.mouse.x <= dialogue_x + DIALOGUE_WIDTH &&
                 app.mouse.y >= dialogue_y && app.mouse.y <= dialogue_y + DIALOGUE_HEIGHT) {
            d_LogInfo("  ➡️ Click in DIALOGUE - consuming");
            app.mouse.pressed = 0;
        }
        else {
            d_LogInfo("  ➡️ Click OUTSIDE - passing through to game");
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


static void RenderDialogue(const TutorialStep_t* step) {
    if (!step || !step->dialogue_text) return;

    // Dialogue box position (use step-specific X offset and Y position)
    int dialogue_x = ((SCREEN_WIDTH - DIALOGUE_WIDTH) / 2) + step->dialogue_x_offset;
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

    // Draw arrow if enabled (pointing from dialogue to target)
    if (step->arrow.enabled) {
        // Calculate actual from position (relative to dialogue box)
        int arrow_from_x = dialogue_x + step->arrow.from_x;
        int arrow_from_y = dialogue_y + step->arrow.from_y;
        int arrow_to_x = step->arrow.to_x;
        int arrow_to_y = step->arrow.to_y;

        // Calculate angle from 'from' to 'to'
        float dx = arrow_to_x - arrow_from_x;
        float dy = arrow_to_y - arrow_from_y;
        float angle = atan2f(dy, dx);

        // Arrowhead dimensions (larger and more visible)
        const int arrow_size = 20;  // Increased from 12
        const float arrow_angle = 0.4f;  // Narrower angle for sharper arrowhead

        // === DRAW BLACK OUTLINE FIRST ===
        // Thicker black outline for line (3px thick outline)
        SDL_SetRenderDrawColor(app.renderer, 0, 0, 0, 255);
        for (int offset = -3; offset <= 3; offset++) {
            if (offset == 0) continue;  // Skip center (will be drawn yellow)
            // Offset perpendicular to line direction
            int offset_x = (int)(offset * sinf(angle));
            int offset_y = (int)(-offset * cosf(angle));
            SDL_RenderDrawLine(app.renderer,
                arrow_from_x + offset_x, arrow_from_y + offset_y,
                arrow_to_x + offset_x, arrow_to_y + offset_y);
        }

        // Black outline for arrowhead (draw larger black triangle)
        const int outline_size = arrow_size + 3;
        for (int i = 0; i <= outline_size; i++) {
            int fill_x1 = arrow_to_x - (int)(i * cosf(angle - arrow_angle));
            int fill_y1 = arrow_to_y - (int)(i * sinf(angle - arrow_angle));
            int fill_x2 = arrow_to_x - (int)(i * cosf(angle + arrow_angle));
            int fill_y2 = arrow_to_y - (int)(i * sinf(angle + arrow_angle));
            SDL_RenderDrawLine(app.renderer, fill_x1, fill_y1, fill_x2, fill_y2);
        }

        // === DRAW YELLOW ARROW ON TOP ===
        // Thicker yellow arrow line (draw 3 lines for 3px thickness)
        SDL_SetRenderDrawColor(app.renderer, DIALOGUE_ARROW.r, DIALOGUE_ARROW.g, DIALOGUE_ARROW.b, 255);
        for (int offset = -1; offset <= 1; offset++) {
            int offset_x = (int)(offset * sinf(angle));
            int offset_y = (int)(-offset * cosf(angle));
            SDL_RenderDrawLine(app.renderer,
                arrow_from_x + offset_x, arrow_from_y + offset_y,
                arrow_to_x + offset_x, arrow_to_y + offset_y);
        }

        // Yellow filled triangle arrowhead
        for (int i = 0; i <= arrow_size; i++) {
            int fill_x1 = arrow_to_x - (int)(i * cosf(angle - arrow_angle));
            int fill_y1 = arrow_to_y - (int)(i * sinf(angle - arrow_angle));
            int fill_x2 = arrow_to_x - (int)(i * cosf(angle + arrow_angle));
            int fill_y2 = arrow_to_y - (int)(i * sinf(angle + arrow_angle));
            SDL_RenderDrawLine(app.renderer, fill_x1, fill_y1, fill_x2, fill_y2);
        }
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


void RenderTutorial(TutorialSystem_t* system) {
    if (!system || !system->active || !system->dialogue_visible || !system->current_step) {
        return;
    }

    TutorialStep_t* step = system->current_step;

    // Render dialogue modal (no overlay)
    RenderDialogue(step);

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
