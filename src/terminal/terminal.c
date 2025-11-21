/*
 * Developer Terminal Implementation
 */

#include "../../include/terminal/terminal.h"
#include <stdarg.h>
#include <ctype.h>

// External from Archimedes
extern aApp_t app;

// ============================================================================
// TERMINAL LIFECYCLE
// ============================================================================

Terminal_t* InitTerminal(void) {
    Terminal_t* terminal = malloc(sizeof(Terminal_t));
    if (!terminal) {
        d_LogFatal("Failed to allocate Terminal");
        return NULL;
    }

    terminal->is_visible = false;
    terminal->history_index = -1;
    terminal->scroll_offset = 0;
    terminal->cursor_blink_timer = 0.0f;
    terminal->cursor_visible = true;
    terminal->scrollbar_dragging = false;
    terminal->drag_start_y = 0;
    terminal->drag_start_scroll = 0;

    // Initialize input buffer
    terminal->input_buffer = d_StringInit();
    if (!terminal->input_buffer) {
        free(terminal);
        d_LogFatal("Failed to allocate terminal input buffer");
        return NULL;
    }

    // Initialize command history (array of dString_t pointers)
    // d_InitArray(capacity, element_size) - capacity FIRST!
    terminal->command_history = d_InitArray(16, sizeof(dString_t*));
    if (!terminal->command_history) {
        d_StringDestroy(terminal->input_buffer);
        free(terminal);
        d_LogFatal("Failed to allocate terminal command history");
        return NULL;
    }

    // Initialize output log (array of dString_t pointers)
    // d_InitArray(capacity, element_size) - capacity FIRST!
    terminal->output_log = d_InitArray(TERMINAL_MAX_OUTPUT_LINES, sizeof(dString_t*));
    if (!terminal->output_log) {
        d_StringDestroy(terminal->input_buffer);
        d_DestroyArray(terminal->command_history);
        free(terminal);
        d_LogFatal("Failed to allocate terminal output log");
        return NULL;
    }

    // Initialize command registry (key: char*, value: CommandHandler_t*)
    terminal->registered_commands = d_InitTable(sizeof(char*), sizeof(CommandHandler_t*),
                                                 d_HashString, d_CompareString, 16);
    if (!terminal->registered_commands) {
        d_StringDestroy(terminal->input_buffer);
        d_DestroyArray(terminal->command_history);
        d_DestroyArray(terminal->output_log);
        free(terminal);
        d_LogFatal("Failed to allocate terminal command registry");
        return NULL;
    }

    // Create FlexBox for output layout (vertical column)
    int term_height = (int)(SCREEN_HEIGHT * TERMINAL_HEIGHT_RATIO);
    terminal->output_layout = a_FlexBoxCreate(10, 10,  // 10px margin from edges
                                               SCREEN_WIDTH - 20,
                                               term_height - 60);  // Leave room for input line
    if (!terminal->output_layout) {
        d_StringDestroy(terminal->input_buffer);
        d_DestroyArray(terminal->command_history);
        d_DestroyArray(terminal->output_log);
        d_DestroyTable(&terminal->registered_commands);
        free(terminal);
        d_LogFatal("Failed to create terminal FlexBox layout");
        return NULL;
    }

    a_FlexConfigure(terminal->output_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, 8);
    a_FlexSetPadding(terminal->output_layout, 0);

    // Register built-in commands
    RegisterBuiltinCommands(terminal);

    // Welcome message
    TerminalPrint(terminal, "Developer Terminal v1.0");
    TerminalPrint(terminal, "Type 'help' for available commands");

    d_LogInfo("Terminal initialized");
    return terminal;
}

void CleanupTerminal(Terminal_t** terminal) {
    if (!terminal || !*terminal) return;

    Terminal_t* t = *terminal;

    // Clean up command history
    for (size_t i = 0; i < t->command_history->count; i++) {
        dString_t** str_ptr = (dString_t**)d_IndexDataFromArray(t->command_history, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }
    d_DestroyArray(t->command_history);

    // Clean up output log
    for (size_t i = 0; i < t->output_log->count; i++) {
        dString_t** str_ptr = (dString_t**)d_IndexDataFromArray(t->output_log, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }
    d_DestroyArray(t->output_log);

    // Clean up registered commands
    if (t->registered_commands) {
        // Get all command names (keys) from table
        dArray_t* cmd_names = d_GetAllKeysFromTable(t->registered_commands);
        if (cmd_names) {
            for (size_t i = 0; i < cmd_names->count; i++) {
                char** key_ptr = (char**)d_IndexDataFromArray(cmd_names, i);
                if (key_ptr && *key_ptr) {
                    CommandHandler_t** handler = (CommandHandler_t**)d_GetDataFromTable(t->registered_commands, key_ptr);
                    if (handler && *handler) {
                        d_StringDestroy((*handler)->name);
                        d_StringDestroy((*handler)->help_text);
                        free(*handler);
                    }
                }
            }
            d_DestroyArray(cmd_names);
        }
        d_DestroyTable(&t->registered_commands);
    }

    // Clean up FlexBox layout
    if (t->output_layout) {
        a_FlexBoxDestroy(&t->output_layout);
    }

    d_StringDestroy(t->input_buffer);
    free(t);
    *terminal = NULL;

    d_LogInfo("Terminal cleaned up");
}

// ============================================================================
// TERMINAL CONTROL
// ============================================================================

void ToggleTerminal(Terminal_t* terminal) {
    if (!terminal) return;
    terminal->is_visible = !terminal->is_visible;

    if (terminal->is_visible) {
        SDL_StartTextInput();  // Enable SDL text input for proper character handling
        d_LogInfo("Terminal opened");
    } else {
        SDL_StopTextInput();   // Disable SDL text input
        d_LogInfo("Terminal closed");
    }
}

bool IsTerminalVisible(const Terminal_t* terminal) {
    return terminal && terminal->is_visible;
}

// ============================================================================
// TERMINAL UPDATE & RENDER
// ============================================================================

void UpdateTerminal(Terminal_t* terminal, float dt) {
    if (!terminal || !terminal->is_visible) return;

    // Cursor blink (0.5 second interval)
    terminal->cursor_blink_timer += dt;
    if (terminal->cursor_blink_timer >= 0.5f) {
        terminal->cursor_blink_timer = 0.0f;
        terminal->cursor_visible = !terminal->cursor_visible;
    }
}

void HandleTerminalInput(Terminal_t* terminal) {
    if (!terminal || !terminal->is_visible) return;

    // Calculate scroll parameters
    int term_height = (int)(SCREEN_HEIGHT * TERMINAL_HEIGHT_RATIO);
    int visible_height = term_height - 60;
    int line_height = 24 + 8;  // Item height + gap
    int total_content_height = (int)(terminal->output_log->count * line_height);
    int max_scroll_offset = (total_content_height - visible_height) / line_height;
    if (max_scroll_offset < 0) max_scroll_offset = 0;

    // Mouse wheel scrolling
    if (app.mouse.wheel != 0) {
        int scroll_delta = -app.mouse.wheel * 3;  // Scroll 3 lines per wheel tick (negative because wheel up = scroll up)
        terminal->scroll_offset += scroll_delta;

        // Clamp to valid range
        if (terminal->scroll_offset < 0) terminal->scroll_offset = 0;
        if (terminal->scroll_offset > max_scroll_offset) terminal->scroll_offset = max_scroll_offset;
    }

    // Scrollbar thumb dragging
    if (total_content_height > visible_height) {
        // Scrollbar dimensions
        int scrollbar_width = 8;
        int scrollbar_x = SCREEN_WIDTH - scrollbar_width - 5;
        int scrollbar_y = 10;
        int scrollbar_height = visible_height;

        // Calculate thumb position
        float content_ratio = (float)visible_height / (float)total_content_height;
        int thumb_height = (int)(scrollbar_height * content_ratio);
        if (thumb_height < 20) thumb_height = 20;

        float scroll_ratio = 0.0f;
        if (max_scroll_offset > 0) {
            scroll_ratio = (float)terminal->scroll_offset / (float)max_scroll_offset;
        }
        int thumb_y = scrollbar_y + (int)((scrollbar_height - thumb_height) * scroll_ratio);

        // Check if mouse is over thumb
        bool mouse_over_thumb = (app.mouse.x >= scrollbar_x &&
                                 app.mouse.x <= scrollbar_x + scrollbar_width &&
                                 app.mouse.y >= thumb_y &&
                                 app.mouse.y <= thumb_y + thumb_height);

        // Start dragging
        if (app.mouse.pressed && mouse_over_thumb && !terminal->scrollbar_dragging) {
            terminal->scrollbar_dragging = true;
            terminal->drag_start_y = app.mouse.y;
            terminal->drag_start_scroll = terminal->scroll_offset;
        }

        // During drag
        if (terminal->scrollbar_dragging && app.mouse.pressed) {
            int mouse_delta = app.mouse.y - terminal->drag_start_y;
            float scroll_delta = ((float)mouse_delta / (float)scrollbar_height) * (float)max_scroll_offset;
            terminal->scroll_offset = terminal->drag_start_scroll + (int)scroll_delta;

            // Clamp to valid range
            if (terminal->scroll_offset < 0) terminal->scroll_offset = 0;
            if (terminal->scroll_offset > max_scroll_offset) terminal->scroll_offset = max_scroll_offset;
        }

        // Stop dragging
        if (!app.mouse.pressed) {
            terminal->scrollbar_dragging = false;
        }
    }

    // ESC - Close terminal
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        ToggleTerminal(terminal);
        return;
    }

    // Enter - Execute command
    if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;

        if (d_StringGetLength(terminal->input_buffer) > 0) {
            const char* command = d_StringPeek(terminal->input_buffer);

            // Add to history
            dString_t* history_entry = d_StringInit();
            d_StringSet(history_entry, command, strlen(command));
            d_AppendDataToArray(terminal->command_history, &history_entry);

            // Echo command (use TerminalPrint to add FlexBox item too)
            TerminalPrint(terminal, "> %s", command);

            // Execute command
            ExecuteCommand(terminal, command);

            // Clear input buffer
            d_StringClear(terminal->input_buffer);
            terminal->history_index = -1;
        }
        return;
    }

    // Backspace - Delete character
    if (app.keyboard[SDL_SCANCODE_BACKSPACE]) {
        app.keyboard[SDL_SCANCODE_BACKSPACE] = 0;

        size_t len = d_StringGetLength(terminal->input_buffer);
        if (len > 0) {
            // Truncate to remove last character
            d_StringTruncate(terminal->input_buffer, len - 1);
        }
        return;
    }

    // Up arrow - Previous command
    if (app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_UP] = 0;

        if (terminal->command_history->count > 0) {
            if (terminal->history_index == -1) {
                terminal->history_index = terminal->command_history->count - 1;
            } else if (terminal->history_index > 0) {
                terminal->history_index--;
            }

            dString_t** cmd_ptr = (dString_t**)d_IndexDataFromArray(terminal->command_history, terminal->history_index);
            if (cmd_ptr && *cmd_ptr) {
                d_StringClear(terminal->input_buffer);
                const char* cmd = d_StringPeek(*cmd_ptr);
                d_StringSet(terminal->input_buffer, cmd, strlen(cmd));
            }
        }
        return;
    }

    // Down arrow - Next command
    if (app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_DOWN] = 0;

        if (terminal->history_index != -1) {
            if (terminal->history_index < (int)terminal->command_history->count - 1) {
                terminal->history_index++;
                dString_t** cmd_ptr = (dString_t**)d_IndexDataFromArray(terminal->command_history, terminal->history_index);
                if (cmd_ptr && *cmd_ptr) {
                    d_StringClear(terminal->input_buffer);
                    const char* cmd = d_StringPeek(*cmd_ptr);
                    d_StringSet(terminal->input_buffer, cmd, strlen(cmd));
                }
            } else {
                terminal->history_index = -1;
                d_StringClear(terminal->input_buffer);
            }
        }
        return;
    }

    // SDL_TEXTINPUT - Handle all character input (letters, numbers, symbols, unicode)
    // Archimedes fills app.input_text via SDL_TEXTINPUT events in a_DoInput()
    if (app.input_text[0] != '\0') {
        d_StringAppend(terminal->input_buffer, app.input_text, strlen(app.input_text));
        app.input_text[0] = '\0';  // Clear the buffer after consuming
    }
}

void RenderTerminal(Terminal_t* terminal) {
    if (!terminal || !terminal->is_visible) return;

    int term_height = (int)(SCREEN_HEIGHT * TERMINAL_HEIGHT_RATIO);

    // Draw background
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, term_height},
                     (aColor_t){TERMINAL_BG_COLOR.r, TERMINAL_BG_COLOR.g,
                     TERMINAL_BG_COLOR.b, TERMINAL_BG_COLOR.a});

    // Apply scroll offset to FlexBox and recalculate layout
    if (terminal->output_layout) {
        // Adjust Y position for scrolling (negative scroll = scroll up, showing earlier lines)
        int line_height = 24 + 8;  // Item height + gap (matches TerminalPrint)
        terminal->output_layout->y = 10 - (terminal->scroll_offset * line_height);
        a_FlexLayout(terminal->output_layout);

        // Render each line using FlexBox-calculated positions
        for (int i = 0; i < (int)terminal->output_log->count; i++) {
            int item_y = a_FlexGetItemY(terminal->output_layout, i);

            // Only render if visible within terminal bounds
            if (item_y >= 0 && item_y < term_height - 50) {
                dString_t** line_ptr = (dString_t**)d_IndexDataFromArray(terminal->output_log, i);
                if (line_ptr && *line_ptr) {
                    const char* line = d_StringPeek(*line_ptr);
                    a_DrawText((char*)line, 10, item_y,
                              (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={TERMINAL_TEXT_COLOR.r, TERMINAL_TEXT_COLOR.g, TERMINAL_TEXT_COLOR.b, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
                }
            }
        }
    }

    // Draw input line
    dString_t* input_line = d_StringInit();
    d_StringFormat(input_line, "%s%s", TERMINAL_PROMPT, d_StringPeek(terminal->input_buffer));

    a_DrawText((char*)d_StringPeek(input_line), 10, term_height - 20,
              (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={TERMINAL_INPUT_COLOR.r, TERMINAL_INPUT_COLOR.g, TERMINAL_INPUT_COLOR.b, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw cursor
    if (terminal->cursor_visible) {
        // Calculate cursor position using actual pixel width of text
        int text_width = 0;
        int text_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(input_line), FONT_ENTER_COMMAND, &text_width, &text_height);
        int cursor_x = 10 + text_width;  // Position cursor right after the text

        a_DrawText("_", cursor_x, term_height - 20,
                  (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={TERMINAL_CURSOR_COLOR.r, TERMINAL_CURSOR_COLOR.g, TERMINAL_CURSOR_COLOR.b, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    d_StringDestroy(input_line);

    // Draw scrollbar (only if content exceeds visible area)
    int line_height = 24 + 8;  // Item height + gap
    int visible_height = term_height - 60;
    int total_content_height = (int)(terminal->output_log->count * line_height);

    if (total_content_height > visible_height) {
        // Scrollbar dimensions
        int scrollbar_width = 8;
        int scrollbar_x = SCREEN_WIDTH - scrollbar_width - 5;  // 5px from right edge
        int scrollbar_y = 10;
        int scrollbar_height = visible_height;

        // Draw track (dark background)
        a_DrawFilledRect((aRectf_t){scrollbar_x, scrollbar_y, scrollbar_width, scrollbar_height},
                        (aColor_t){40, 40, 40, 200});

        // Calculate thumb size and position
        float content_ratio = (float)visible_height / (float)total_content_height;
        int thumb_height = (int)(scrollbar_height * content_ratio);
        if (thumb_height < 20) thumb_height = 20;  // Minimum 20px thumb

        int max_scroll_offset = (total_content_height - visible_height) / line_height;
        float scroll_ratio = 0.0f;
        if (max_scroll_offset > 0) {
            scroll_ratio = (float)terminal->scroll_offset / (float)max_scroll_offset;
        }
        int thumb_y = scrollbar_y + (int)((scrollbar_height - thumb_height) * scroll_ratio);

        // Draw thumb (lighter gray)
        a_DrawFilledRect((aRectf_t){scrollbar_x, thumb_y, scrollbar_width, thumb_height},
                        (aColor_t){100, 100, 100, 255});
    }
}

// ============================================================================
// TERMINAL OUTPUT
// ============================================================================

void TerminalPrint(Terminal_t* terminal, const char* format, ...) {
    if (!terminal) return;

    va_list args;
    va_start(args, format);

    // Format string
    char buffer[512];
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // Create dString_t for output
    dString_t* line = d_StringInit();
    d_StringSet(line, buffer, strlen(buffer));

    // Add to output log
    d_AppendDataToArray(terminal->output_log, &line);

    // Add FlexBox item for this line (24px height per line)
    if (terminal->output_layout) {
        a_FlexAddItem(terminal->output_layout, SCREEN_WIDTH - 20, 24, line);
        a_FlexLayout(terminal->output_layout);  // Recalculate positions
    }

    // Auto-scroll to bottom (keep terminal "sticky" to latest output)
    int term_height = (int)(SCREEN_HEIGHT * TERMINAL_HEIGHT_RATIO);
    int visible_height = term_height - 60;  // Leave room for input line
    int line_height = 24 + 8;  // Item height + gap
    int total_content_height = (int)(terminal->output_log->count * line_height);

    if (total_content_height > visible_height) {
        // Content exceeds visible area - scroll to show bottom
        terminal->scroll_offset = (total_content_height - visible_height) / line_height;
    } else {
        // Content fits - no scrolling needed
        terminal->scroll_offset = 0;
    }

    // Trim old lines if exceeding max
    while (terminal->output_log->count > TERMINAL_MAX_OUTPUT_LINES) {
        dString_t** old_line = (dString_t**)d_IndexDataFromArray(terminal->output_log, 0);
        if (old_line && *old_line) {
            d_StringDestroy(*old_line);
        }
        // TODO: Remove first element from array (Daedalus doesn't have RemoveAt?)
        // Also need to remove corresponding FlexBox item
        // For now, just let it grow
        break;
    }
}

void TerminalClear(Terminal_t* terminal) {
    if (!terminal) return;

    // Destroy all output lines
    for (size_t i = 0; i < terminal->output_log->count; i++) {
        dString_t** str_ptr = (dString_t**)d_IndexDataFromArray(terminal->output_log, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }

    // Clear array (hacky: destroy and recreate)
    d_DestroyArray(terminal->output_log);
    // d_InitArray(capacity, element_size) - capacity FIRST!
    terminal->output_log = d_InitArray(TERMINAL_MAX_OUTPUT_LINES, sizeof(dString_t*));

    // Recreate FlexBox layout (clears all items)
    if (terminal->output_layout) {
        a_FlexBoxDestroy(&terminal->output_layout);
        int term_height = (int)(SCREEN_HEIGHT * TERMINAL_HEIGHT_RATIO);
        terminal->output_layout = a_FlexBoxCreate(10, 10,
                                                   SCREEN_WIDTH - 20,
                                                   term_height - 60);
        a_FlexConfigure(terminal->output_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, 8);
        a_FlexSetPadding(terminal->output_layout, 0);
    }
}

// ============================================================================
// COMMAND REGISTRATION & EXECUTION
// ============================================================================

void RegisterCommand(Terminal_t* terminal, const char* name, CommandFunc_t execute, const char* help_text) {
    if (!terminal || !name || !execute) return;

    CommandHandler_t* handler = malloc(sizeof(CommandHandler_t));
    if (!handler) {
        d_LogError("Failed to allocate command handler");
        return;
    }

    handler->name = d_StringInit();
    d_StringSet(handler->name, name, strlen(name));

    handler->execute = execute;

    handler->help_text = d_StringInit();
    d_StringSet(handler->help_text, help_text, strlen(help_text));

    // Store in table (key is the name string)
    const char* key = d_StringPeek(handler->name);
    d_SetDataInTable(terminal->registered_commands, &key, &handler);

    d_LogInfoF("Registered command: %s", name);
}

void ExecuteCommand(Terminal_t* terminal, const char* command_line) {
    if (!terminal || !command_line || strlen(command_line) == 0) return;

    // Parse command name (first word)
    char cmd_name[64] = {0};
    const char* args = command_line;

    // Skip leading spaces
    while (*args && isspace(*args)) args++;

    // Extract command name
    int i = 0;
    while (*args && !isspace(*args) && i < 63) {
        cmd_name[i++] = *args++;
    }
    cmd_name[i] = '\0';

    // Skip spaces before args
    while (*args && isspace(*args)) args++;

    // Look up command
    const char* key = cmd_name;
    CommandHandler_t** handler_ptr = (CommandHandler_t**)d_GetDataFromTable(terminal->registered_commands, &key);

    if (!handler_ptr || !*handler_ptr) {
        TerminalPrint(terminal, "[Error] Unknown command: %s", cmd_name);
        TerminalPrint(terminal, "Type 'help' for available commands");
        return;
    }

    // Execute command
    CommandHandler_t* handler = *handler_ptr;
    handler->execute(terminal, args);
}
