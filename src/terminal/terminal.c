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
    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    terminal->command_history = d_ArrayInit(16, sizeof(dString_t*));
    if (!terminal->command_history) {
        d_StringDestroy(terminal->input_buffer);
        free(terminal);
        d_LogFatal("Failed to allocate terminal command history");
        return NULL;
    }

    // Initialize output log (array of dString_t pointers)
    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    terminal->output_log = d_ArrayInit(TERMINAL_MAX_OUTPUT_LINES, sizeof(dString_t*));
    if (!terminal->output_log) {
        d_StringDestroy(terminal->input_buffer);
        d_ArrayDestroy(terminal->command_history);
        free(terminal);
        d_LogFatal("Failed to allocate terminal output log");
        return NULL;
    }

    // Initialize command registry (key: char*, value: CommandHandler_t*)
    terminal->registered_commands = d_TableInit(sizeof(char*), sizeof(CommandHandler_t*),
                                                 d_HashString, d_CompareString, 16);
    if (!terminal->registered_commands) {
        d_StringDestroy(terminal->input_buffer);
        d_ArrayDestroy(terminal->command_history);
        d_ArrayDestroy(terminal->output_log);
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
        d_ArrayDestroy(terminal->command_history);
        d_ArrayDestroy(terminal->output_log);
        d_TableDestroy(&terminal->registered_commands);
        free(terminal);
        d_LogFatal("Failed to create terminal FlexBox layout");
        return NULL;
    }

    a_FlexConfigure(terminal->output_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, 8);
    a_FlexSetPadding(terminal->output_layout, 0);

    // Initialize autocomplete state
    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    terminal->autocomplete_matches = d_ArrayInit(16, sizeof(char*));
    terminal->autocomplete_index = -1;
    terminal->autocomplete_suggestion = d_StringInit();
    if (!terminal->autocomplete_matches || !terminal->autocomplete_suggestion) {
        d_LogFatal("Failed to initialize autocomplete state");
        // Cleanup and return NULL
        if (terminal->autocomplete_matches) d_ArrayDestroy(terminal->autocomplete_matches);
        if (terminal->autocomplete_suggestion) d_StringDestroy(terminal->autocomplete_suggestion);
        if (terminal->output_layout) a_FlexBoxDestroy(&terminal->output_layout);
        d_StringDestroy(terminal->input_buffer);
        d_ArrayDestroy(terminal->command_history);
        d_ArrayDestroy(terminal->output_log);
        d_TableDestroy(&terminal->registered_commands);
        free(terminal);
        return NULL;
    }

    // Initialize text selection state
    terminal->cursor_position = 0;
    terminal->selection_start = -1;
    terminal->selection_end = -1;
    terminal->highlighted_text = d_StringInit();
    if (!terminal->highlighted_text) {
        d_LogFatal("Failed to initialize text selection state");
        // Cleanup and return NULL
        if (terminal->autocomplete_matches) d_ArrayDestroy(terminal->autocomplete_matches);
        if (terminal->autocomplete_suggestion) d_StringDestroy(terminal->autocomplete_suggestion);
        if (terminal->output_layout) a_FlexBoxDestroy(&terminal->output_layout);
        d_StringDestroy(terminal->input_buffer);
        d_ArrayDestroy(terminal->command_history);
        d_ArrayDestroy(terminal->output_log);
        d_TableDestroy(&terminal->registered_commands);
        free(terminal);
        return NULL;
    }

    // Register built-in commands
    RegisterBuiltinCommands(terminal);

    // Welcome message
    TerminalPrint(terminal, "Developer Terminal v1.1");
    TerminalPrint(terminal, "Type 'help' for available commands");

    d_LogInfo("Terminal initialized");
    return terminal;
}

void CleanupTerminal(Terminal_t** terminal) {
    if (!terminal || !*terminal) return;

    Terminal_t* t = *terminal;

    // Clean up command history
    for (size_t i = 0; i < t->command_history->count; i++) {
        dString_t** str_ptr = (dString_t**)d_ArrayGet(t->command_history, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }
    d_ArrayDestroy(t->command_history);

    // Clean up output log
    for (size_t i = 0; i < t->output_log->count; i++) {
        dString_t** str_ptr = (dString_t**)d_ArrayGet(t->output_log, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }
    d_ArrayDestroy(t->output_log);

    // Clean up registered commands
    if (t->registered_commands) {
        // Get all command names (keys) from table
        dArray_t* cmd_names = d_TableGetAllKeys(t->registered_commands);
        if (cmd_names) {
            for (size_t i = 0; i < cmd_names->count; i++) {
                char** key_ptr = (char**)d_ArrayGet(cmd_names, i);
                if (key_ptr && *key_ptr) {
                    CommandHandler_t** handler = (CommandHandler_t**)d_TableGet(t->registered_commands, key_ptr);
                    if (handler && *handler) {
                        d_StringDestroy((*handler)->name);
                        d_StringDestroy((*handler)->help_text);
                        free(*handler);
                    }
                }
            }
            d_ArrayDestroy(cmd_names);
        }
        d_TableDestroy(&t->registered_commands);
    }

    // Clean up FlexBox layout
    if (t->output_layout) {
        a_FlexBoxDestroy(&t->output_layout);
    }

    // Clean up autocomplete state
    // Note: autocomplete_matches stores pointers owned by registered_commands
    // (already freed above), so just destroy the array, not the strings
    if (t->autocomplete_matches) {
        d_ArrayDestroy(t->autocomplete_matches);
    }
    if (t->autocomplete_suggestion) {
        d_StringDestroy(t->autocomplete_suggestion);
    }

    // Clean up text selection state
    if (t->highlighted_text) {
        d_StringDestroy(t->highlighted_text);
    }

    d_StringDestroy(t->input_buffer);
    free(t);
    *terminal = NULL;

    d_LogInfo("Terminal cleaned up");
}

// ============================================================================
// AUTOCOMPLETE
// ============================================================================

static int compare_strings(const void* a, const void* b) {
    const char* str_a = *(const char**)a;
    const char* str_b = *(const char**)b;
    return strcmp(str_a, str_b);
}

void FindCommandMatches(Terminal_t* terminal, const char* prefix) {
    if (!terminal || !prefix) return;

    // Clear previous matches
    if (terminal->autocomplete_matches) {
        // Don't free the strings - they're owned by registered_commands
        d_ArrayClear(terminal->autocomplete_matches);
    }

    size_t prefix_len = strlen(prefix);
    if (prefix_len == 0) {
        terminal->autocomplete_index = -1;
        return;
    }

    // Get all command names from registered_commands
    dArray_t* all_commands = d_TableGetAllKeys(terminal->registered_commands);
    if (!all_commands) return;

    // Find commands that start with prefix (case-insensitive)
    for (size_t i = 0; i < all_commands->count; i++) {
        char** cmd_ptr = (char**)d_ArrayGet(all_commands, i);
        if (cmd_ptr && *cmd_ptr) {
            const char* cmd_name = *cmd_ptr;

            // Case-insensitive prefix match
            if (strncasecmp(cmd_name, prefix, prefix_len) == 0) {
                d_ArrayAppend(terminal->autocomplete_matches, cmd_ptr);
            }
        }
    }

    // Sort matches alphabetically for consistent order
    if (terminal->autocomplete_matches->count > 1) {
        qsort(terminal->autocomplete_matches->data,
              terminal->autocomplete_matches->count,
              sizeof(char*),
              compare_strings);
    }

    d_ArrayDestroy(all_commands);

    // Reset index
    terminal->autocomplete_index = -1;
}

void UpdateAutocompleteSuggestion(Terminal_t* terminal) {
    if (!terminal) return;

    // Clear previous suggestion
    d_StringClear(terminal->autocomplete_suggestion);

    const char* input = d_StringPeek(terminal->input_buffer);
    size_t input_len = strlen(input);

    if (input_len == 0) {
        terminal->autocomplete_index = -1;
        return;
    }

    // Check if we're in "argument mode" (command name + space + partial arg)
    const char* space_pos = strchr(input, ' ');
    if (space_pos) {
        // Extract command name
        size_t cmd_len = space_pos - input;
        char cmd_name[128] = {0};
        if (cmd_len < 128) {
            strncpy(cmd_name, input, cmd_len);
            cmd_name[cmd_len] = '\0';

            // Look up command handler
            const char* key = cmd_name;
            CommandHandler_t** handler_ptr = (CommandHandler_t**)d_TableGet(terminal->registered_commands, &key);

            if (handler_ptr && *handler_ptr && (*handler_ptr)->suggest_args) {
                // Command has argument suggestions!
                const char* partial_arg = space_pos + 1;  // Text after space

                // Clear old matches and get argument suggestions
                d_ArrayClear(terminal->autocomplete_matches);
                dArray_t* arg_suggestions = (*handler_ptr)->suggest_args(terminal, partial_arg);

                if (arg_suggestions && arg_suggestions->count > 0) {
                    // Copy suggestions to autocomplete_matches
                    for (size_t i = 0; i < arg_suggestions->count; i++) {
                        char** suggestion = (char**)d_ArrayGet(arg_suggestions, i);
                        if (suggestion) {
                            d_ArrayAppend(terminal->autocomplete_matches, suggestion);
                        }
                    }
                    d_ArrayDestroy(arg_suggestions);  // Destroy array (not strings - they're static)
                    return;  // Argument autocomplete active
                }

                if (arg_suggestions) d_ArrayDestroy(arg_suggestions);
            }
        }
    }

    // Not in argument mode - do command name autocomplete
    FindCommandMatches(terminal, input);

    if (terminal->autocomplete_matches->count == 1) {
        // Single match - show rest of command as ghost text
        char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches, 0);
        if (match_ptr && *match_ptr) {
            const char* match = *match_ptr;
            const char* remaining = match + input_len;  // Skip the part already typed
            d_StringSet(terminal->autocomplete_suggestion, remaining, strlen(remaining));
        }
    } else if (terminal->autocomplete_matches->count > 1) {
        // Multiple matches - find common prefix
        char** first_match = (char**)d_ArrayGet(terminal->autocomplete_matches, 0);
        if (first_match && *first_match) {
            const char* first = *first_match;
            size_t common_len = strlen(first);

            // Find shortest common prefix among all matches
            for (size_t i = 1; i < terminal->autocomplete_matches->count; i++) {
                char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches, i);
                if (match_ptr && *match_ptr) {
                    const char* match = *match_ptr;

                    for (size_t j = input_len; j < common_len && j < strlen(match); j++) {
                        if (tolower(first[j]) != tolower(match[j])) {
                            common_len = j;
                            break;
                        }
                    }
                }
            }

            // Set suggestion to common prefix (if any)
            if (common_len > input_len) {
                const char* remaining = first + input_len;
                size_t remaining_len = common_len - input_len;
                d_StringSet(terminal->autocomplete_suggestion, remaining, remaining_len);
            }
        }
    }
}

void CycleAutocompleteSuggestion(Terminal_t* terminal) {
    if (!terminal) return;

    if (terminal->autocomplete_matches->count == 0) {
        return;  // No matches, nothing to cycle
    }

    // Determine if we're in argument mode by checking if matches came from suggest_args
    // We can't trust the buffer content because it may have been modified by previous cycles
    const char* input = d_StringPeek(terminal->input_buffer);
    const char* space_pos = strchr(input, ' ');

    dString_t* cmd_prefix = NULL;
    bool is_argument_mode = (space_pos != NULL);

    if (is_argument_mode) {
        // Extract ONLY the command name + space (ignore any argument text)
        cmd_prefix = d_StringInit();
        size_t prefix_len = (space_pos - input) + 1;  // "command "
        d_StringAppend(cmd_prefix, input, prefix_len);
    }

    if (terminal->autocomplete_matches->count == 1) {
        // Only one match - complete it
        char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches, 0);
        if (match_ptr && *match_ptr) {
            d_StringClear(terminal->input_buffer);

            if (is_argument_mode) {
                // Argument autocomplete - rebuild as "command argument"
                d_StringAppend(terminal->input_buffer, d_StringPeek(cmd_prefix), d_StringGetLength(cmd_prefix));
                d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
            } else {
                // Command name autocomplete
                d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
            }

            terminal->cursor_position = (int)d_StringGetLength(terminal->input_buffer);
        }

        // Clear autocomplete state (no dropdown for single match)
        d_ArrayClear(terminal->autocomplete_matches);
        d_StringClear(terminal->autocomplete_suggestion);
        terminal->autocomplete_index = -1;

        if (cmd_prefix) d_StringDestroy(cmd_prefix);
        return;
    }

    // Multiple matches - cycle to next
    terminal->autocomplete_index++;
    if (terminal->autocomplete_index >= (int)terminal->autocomplete_matches->count) {
        terminal->autocomplete_index = 0;
    }

    // Update input buffer to show current selection in real-time
    char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches,
                                                      terminal->autocomplete_index);
    if (match_ptr && *match_ptr) {
        d_StringClear(terminal->input_buffer);

        if (is_argument_mode) {
            // Argument autocomplete - rebuild as "command argument"
            d_StringAppend(terminal->input_buffer, d_StringPeek(cmd_prefix), d_StringGetLength(cmd_prefix));
            d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
        } else {
            // Command name autocomplete
            d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
        }

        terminal->cursor_position = (int)d_StringGetLength(terminal->input_buffer);
    }

    // Clean up command prefix
    if (cmd_prefix) d_StringDestroy(cmd_prefix);
}

void AcceptAutocompleteSuggestion(Terminal_t* terminal) {
    if (!terminal) return;

    if (terminal->autocomplete_matches->count == 0) {
        return;  // No matches, nothing to accept
    }

    // Detect if we're in argument mode (command + space + argument)
    const char* input = d_StringPeek(terminal->input_buffer);
    const char* space_pos = strchr(input, ' ');
    bool is_argument_mode = (space_pos != NULL);

    // Extract command prefix if in argument mode
    dString_t* cmd_prefix = NULL;
    if (is_argument_mode) {
        cmd_prefix = d_StringInit();
        size_t prefix_len = (space_pos - input) + 1;  // "command "
        d_StringAppend(cmd_prefix, input, prefix_len);
    }

    // If cycling through matches, accept the highlighted one
    if (terminal->autocomplete_index >= 0 &&
        terminal->autocomplete_index < (int)terminal->autocomplete_matches->count) {
        char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches,
                                                          terminal->autocomplete_index);
        if (match_ptr && *match_ptr) {
            d_StringClear(terminal->input_buffer);

            if (is_argument_mode) {
                // Rebuild as "command argument"
                d_StringAppend(terminal->input_buffer, d_StringPeek(cmd_prefix), d_StringGetLength(cmd_prefix));
                d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
            } else {
                // Command name autocomplete
                d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
            }

            terminal->cursor_position = (int)d_StringGetLength(terminal->input_buffer);
        }
    } else if (terminal->autocomplete_matches->count == 1) {
        // Single match and not cycling yet - just accept it
        char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches, 0);
        if (match_ptr && *match_ptr) {
            d_StringClear(terminal->input_buffer);

            if (is_argument_mode) {
                // Rebuild as "command argument"
                d_StringAppend(terminal->input_buffer, d_StringPeek(cmd_prefix), d_StringGetLength(cmd_prefix));
                d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
            } else {
                // Command name autocomplete
                d_StringAppend(terminal->input_buffer, *match_ptr, strlen(*match_ptr));
            }

            terminal->cursor_position = (int)d_StringGetLength(terminal->input_buffer);
        }
    }

    // Clean up command prefix
    if (cmd_prefix) d_StringDestroy(cmd_prefix);

    // Clear autocomplete state after accepting
    d_ArrayClear(terminal->autocomplete_matches);
    d_StringClear(terminal->autocomplete_suggestion);
    terminal->autocomplete_index = -1;
}

// ============================================================================
// TEXT SELECTION HELPERS
// ============================================================================

/**
 * ClearSelection - Reset selection state
 */
static void ClearSelection(Terminal_t* terminal) {
    if (!terminal) return;
    terminal->selection_start = -1;
    terminal->selection_end = -1;
    d_StringClear(terminal->highlighted_text);
}

/**
 * UpdateHighlightedText - Copy selected region to highlighted_text
 */
static void UpdateHighlightedText(Terminal_t* terminal) {
    if (!terminal) return;

    if (terminal->selection_start < 0 || terminal->selection_end < 0) {
        d_StringClear(terminal->highlighted_text);
        return;
    }

    // Ensure start < end (swap if needed)
    int start = (terminal->selection_start < terminal->selection_end)
                 ? terminal->selection_start : terminal->selection_end;
    int end = (terminal->selection_start < terminal->selection_end)
               ? terminal->selection_end : terminal->selection_start;

    const char* buffer = d_StringPeek(terminal->input_buffer);
    int len = end - start;

    if (len > 0) {
        d_StringSet(terminal->highlighted_text, buffer + start, len);
    } else {
        d_StringClear(terminal->highlighted_text);
    }
}

/**
 * FindWordStart - Find start of word at position (move left until whitespace)
 */
static int FindWordStart(const char* text, int pos) {
    if (!text || pos <= 0) return 0;

    // Move left until whitespace or start
    while (pos > 0 && !isspace((unsigned char)text[pos - 1])) {
        pos--;
    }

    return pos;
}

/**
 * FindWordEnd - Find end of word at position (move right until whitespace)
 */
static int FindWordEnd(const char* text, int pos) {
    if (!text) return pos;

    int len = (int)strlen(text);
    if (pos >= len) return len;

    // Move right until whitespace or end
    while (pos < len && !isspace((unsigned char)text[pos])) {
        pos++;
    }

    return pos;
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

            // Limit history size
            if (terminal->command_history->count >= TERMINAL_MAX_HISTORY) {
                // Remove oldest entry
                dString_t** old_cmd = (dString_t**)d_ArrayGet(terminal->command_history, 0);
                if (old_cmd && *old_cmd) {
                    d_StringDestroy(*old_cmd);
                }
                d_ArrayRemove(terminal->command_history, 0);
            }

            // Add to history
            dString_t* history_entry = d_StringInit();
            d_StringSet(history_entry, command, strlen(command));
            d_ArrayAppend(terminal->command_history, &history_entry);

            // Echo command (use TerminalPrint to add FlexBox item too)
            TerminalPrint(terminal, "> %s", command);

            // Execute command
            ExecuteCommand(terminal, command);

            // Clear input buffer and reset cursor
            d_StringClear(terminal->input_buffer);
            terminal->cursor_position = 0;
            terminal->history_index = -1;
        }
        return;
    }

    // Tab - Cycle through autocomplete suggestions
    if (app.keyboard[SDL_SCANCODE_TAB]) {
        app.keyboard[SDL_SCANCODE_TAB] = 0;
        CycleAutocompleteSuggestion(terminal);
        return;
    }

    // Space - Accept autocomplete suggestion
    if (app.keyboard[SDL_SCANCODE_SPACE]) {
        // Only accept if we have matches and are cycling
        if (terminal->autocomplete_matches->count > 0 && terminal->autocomplete_index >= 0) {
            app.keyboard[SDL_SCANCODE_SPACE] = 0;
            AcceptAutocompleteSuggestion(terminal);
            return;
        }
        // Otherwise, let space be typed normally (handled in SDL_TEXTINPUT below)
    }

    // Backspace - Delete character or word at cursor position
    if (app.keyboard[SDL_SCANCODE_BACKSPACE]) {
        app.keyboard[SDL_SCANCODE_BACKSPACE] = 0;

        ClearSelection(terminal);  // Clear selection on backspace

        // Check if Ctrl is held for word deletion
        SDL_Keymod mods = SDL_GetModState();
        bool ctrl_held = (mods & KMOD_CTRL) != 0;

        if (terminal->cursor_position > 0) {
            const char* current_text = d_StringPeek(terminal->input_buffer);
            int current_len = (int)d_StringGetLength(terminal->input_buffer);

            int delete_start_pos = terminal->cursor_position;

            if (ctrl_held) {
                // Ctrl+Backspace - delete entire word before cursor
                delete_start_pos = FindWordStart(current_text, terminal->cursor_position);
            } else {
                // Normal backspace - delete one character
                delete_start_pos = terminal->cursor_position - 1;
            }

            // Build new string: [before delete_start] + [after cursor]
            dString_t* new_buffer = d_StringInit();

            // Part 1: Text before deletion point
            if (delete_start_pos > 0) {
                d_StringAppend(new_buffer, current_text, delete_start_pos);
            }

            // Part 2: Text after cursor (skip the deleted portion)
            if (terminal->cursor_position < current_len) {
                d_StringAppend(new_buffer, current_text + terminal->cursor_position,
                               current_len - terminal->cursor_position);
            }

            // Replace buffer with new string
            d_StringClear(terminal->input_buffer);
            const char* new_text = d_StringPeek(new_buffer);
            if (new_text && strlen(new_text) > 0) {
                d_StringAppend(terminal->input_buffer, new_text, strlen(new_text));
            }
            d_StringDestroy(new_buffer);

            // Move cursor to deletion start point
            terminal->cursor_position = delete_start_pos;

            // Update autocomplete suggestions (reset cycling)
            terminal->autocomplete_index = -1;
            UpdateAutocompleteSuggestion(terminal);
        }
        return;
    }

    // Check modifier keys for text selection
    SDL_Keymod mods = SDL_GetModState();
    bool shift_held = (mods & KMOD_SHIFT) != 0;
    bool ctrl_held = (mods & KMOD_CTRL) != 0;

    // Left arrow - Move cursor left (with optional selection)
    if (app.keyboard[SDL_SCANCODE_LEFT]) {
        app.keyboard[SDL_SCANCODE_LEFT] = 0;

        if (shift_held) {
            // Start selection if not already selecting
            if (terminal->selection_start < 0) {
                terminal->selection_start = terminal->cursor_position;
            }

            // Move cursor left (word or char)
            if (ctrl_held) {
                // Ctrl+Shift+Left - move cursor to word start
                const char* text = d_StringPeek(terminal->input_buffer);
                terminal->cursor_position = FindWordStart(text, terminal->cursor_position);
            } else {
                // Shift+Left - move cursor left one char
                if (terminal->cursor_position > 0) {
                    terminal->cursor_position--;
                }
            }

            // Update selection end (selection rendering disabled, but tracking still works for future copy/paste)
            terminal->selection_end = terminal->cursor_position;
            UpdateHighlightedText(terminal);
        } else {
            // No shift - clear selection and just move cursor
            ClearSelection(terminal);

            if (ctrl_held) {
                // Ctrl+Left - jump to word start
                const char* text = d_StringPeek(terminal->input_buffer);
                terminal->cursor_position = FindWordStart(text, terminal->cursor_position);
            } else {
                // Left - move one char
                if (terminal->cursor_position > 0) {
                    terminal->cursor_position--;
                }
            }
        }

        // Reset cursor blink to make it visible immediately
        terminal->cursor_blink_timer = 0.0f;
        terminal->cursor_visible = true;

        // Debug logging
        TerminalPrint(terminal, "[DEBUG] Left arrow: cursor=%d, buffer='%s'",
                      terminal->cursor_position, d_StringPeek(terminal->input_buffer));
        return;
    }

    // Right arrow - Move cursor right (with optional selection)
    if (app.keyboard[SDL_SCANCODE_RIGHT]) {
        app.keyboard[SDL_SCANCODE_RIGHT] = 0;

        int buffer_len = (int)d_StringGetLength(terminal->input_buffer);

        if (shift_held) {
            // Start selection if not already selecting
            if (terminal->selection_start < 0) {
                terminal->selection_start = terminal->cursor_position;
            }

            // Move cursor right (word or char)
            if (ctrl_held) {
                // Ctrl+Shift+Right - move cursor to word end
                const char* text = d_StringPeek(terminal->input_buffer);
                terminal->cursor_position = FindWordEnd(text, terminal->cursor_position);
            } else {
                // Shift+Right - move cursor right one char
                if (terminal->cursor_position < buffer_len) {
                    terminal->cursor_position++;
                }
            }

            // Update selection end (selection rendering disabled, but tracking still works for future copy/paste)
            terminal->selection_end = terminal->cursor_position;
            UpdateHighlightedText(terminal);
        } else {
            // No shift - clear selection and just move cursor
            ClearSelection(terminal);

            if (ctrl_held) {
                // Ctrl+Right - jump to word end
                const char* text = d_StringPeek(terminal->input_buffer);
                terminal->cursor_position = FindWordEnd(text, terminal->cursor_position);
            } else {
                // Right - move one char
                if (terminal->cursor_position < buffer_len) {
                    terminal->cursor_position++;
                }
            }
        }

        // Reset cursor blink to make it visible immediately
        terminal->cursor_blink_timer = 0.0f;
        terminal->cursor_visible = true;

        // Debug logging
        TerminalPrint(terminal, "[DEBUG] Right arrow: cursor=%d, buffer='%s'",
                      terminal->cursor_position, d_StringPeek(terminal->input_buffer));
        return;
    }

    // Up arrow - Previous command
    if (app.keyboard[SDL_SCANCODE_UP]) {
        app.keyboard[SDL_SCANCODE_UP] = 0;

        ClearSelection(terminal);  // Clear selection when navigating history

        if (terminal->command_history->count > 0) {
            if (terminal->history_index == -1) {
                terminal->history_index = terminal->command_history->count - 1;
            } else if (terminal->history_index > 0) {
                terminal->history_index--;
            }

            dString_t** cmd_ptr = (dString_t**)d_ArrayGet(terminal->command_history, terminal->history_index);
            if (cmd_ptr && *cmd_ptr) {
                d_StringClear(terminal->input_buffer);
                const char* cmd = d_StringPeek(*cmd_ptr);
                d_StringSet(terminal->input_buffer, cmd, strlen(cmd));

                // Move cursor to end of loaded command
                terminal->cursor_position = (int)d_StringGetLength(terminal->input_buffer);
            }
        }
        return;
    }

    // Down arrow - Next command
    if (app.keyboard[SDL_SCANCODE_DOWN]) {
        app.keyboard[SDL_SCANCODE_DOWN] = 0;

        ClearSelection(terminal);  // Clear selection when navigating history

        if (terminal->history_index != -1) {
            if (terminal->history_index < (int)terminal->command_history->count - 1) {
                terminal->history_index++;
                dString_t** cmd_ptr = (dString_t**)d_ArrayGet(terminal->command_history, terminal->history_index);
                if (cmd_ptr && *cmd_ptr) {
                    d_StringClear(terminal->input_buffer);
                    const char* cmd = d_StringPeek(*cmd_ptr);
                    d_StringSet(terminal->input_buffer, cmd, strlen(cmd));

                    // Move cursor to end of loaded command
                    terminal->cursor_position = (int)d_StringGetLength(terminal->input_buffer);
                }
            } else {
                terminal->history_index = -1;
                d_StringClear(terminal->input_buffer);
                terminal->cursor_position = 0;
            }
        }
        return;
    }

    // SDL_TEXTINPUT - Handle all character input (letters, numbers, symbols, unicode)
    // Archimedes fills app.input_text via SDL_TEXTINPUT events in a_DoInput()
    if (app.input_text[0] != '\0') {
        ClearSelection(terminal);  // Clear selection on typing

        size_t input_len = strlen(app.input_text);
        size_t current_len = d_StringGetLength(terminal->input_buffer);

        // Prevent buffer overflow
        if (current_len + input_len < TERMINAL_MAX_INPUT_LENGTH) {
            // Insert text at cursor position (not always at end)
            const char* current_text = d_StringPeek(terminal->input_buffer);

            // Build new string: [before cursor] + [new text] + [after cursor]
            dString_t* new_buffer = d_StringInit();

            // Part 1: Text before cursor
            if (terminal->cursor_position > 0) {
                d_StringAppend(new_buffer, current_text, terminal->cursor_position);
            }

            // Part 2: New text being typed
            d_StringAppend(new_buffer, app.input_text, input_len);

            // Part 3: Text after cursor
            if (terminal->cursor_position < (int)current_len) {
                d_StringAppend(new_buffer, current_text + terminal->cursor_position,
                               current_len - terminal->cursor_position);
            }

            // Replace buffer with new string
            d_StringClear(terminal->input_buffer);
            const char* new_text = d_StringPeek(new_buffer);
            if (new_text && strlen(new_text) > 0) {
                d_StringAppend(terminal->input_buffer, new_text, strlen(new_text));
            }
            d_StringDestroy(new_buffer);

            // Move cursor forward by number of characters typed
            terminal->cursor_position += (int)input_len;

            // Update autocomplete suggestions (reset cycling)
            terminal->autocomplete_index = -1;
            UpdateAutocompleteSuggestion(terminal);
        }

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
                dString_t** line_ptr = (dString_t**)d_ArrayGet(terminal->output_log, i);
                if (line_ptr && *line_ptr) {
                    const char* line = d_StringPeek(*line_ptr);
                    a_DrawText((char*)line, 10, item_y,
                              (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={TERMINAL_TEXT_COLOR.r, TERMINAL_TEXT_COLOR.g, TERMINAL_TEXT_COLOR.b, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=0.8f, .padding=0});
                }
            }
        }
    }

    // Draw input line (simple, no selection rendering)
    const char* full_text = d_StringPeek(terminal->input_buffer);

    // Draw prompt + input text as a single line
    dString_t* input_line = d_StringInit();
    d_StringFormat(input_line, "%s%s", TERMINAL_PROMPT, full_text);

    a_DrawText((char*)d_StringPeek(input_line), 10, term_height - 40,
              (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={TERMINAL_INPUT_COLOR.r, TERMINAL_INPUT_COLOR.g, TERMINAL_INPUT_COLOR.b, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=0.8f, .padding=0});

    d_StringDestroy(input_line);

    // Draw autocomplete ghost text (gray suggestion)
    if (d_StringGetLength(terminal->autocomplete_suggestion) > 0) {
        // Calculate width of prompt + input text
        dString_t* temp_line = d_StringInit();
        d_StringFormat(temp_line, "%s%s", TERMINAL_PROMPT, full_text);

        float input_width = 0;
        float input_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(temp_line), FONT_ENTER_COMMAND, &input_width, &input_height);
        int ghost_x = 10 + (int)(input_width * 0.8f);  // Right after input text

        a_DrawText((char*)d_StringPeek(terminal->autocomplete_suggestion), ghost_x, term_height - 40,
                  (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={100, 100, 100, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=0.8f, .padding=0});

        d_StringDestroy(temp_line);
    }

    // Draw autocomplete dropdown (show for any matches to display parameter hints)
    // Show dropdown when there are matches and user is typing
    if (terminal->autocomplete_matches->count > 0 &&
        d_StringGetLength(terminal->input_buffer) > 0) {
        int dropdown_x = 10;
        int dropdown_y = term_height - 65;  // Above input line
        int item_height = 20;
        int dropdown_height = (int)(terminal->autocomplete_matches->count * item_height) + 10;

        // Calculate dropdown width based on longest text
        float max_text_width = 0.0f;
        for (size_t i = 0; i < terminal->autocomplete_matches->count; i++) {
            char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches, i);
            if (match_ptr && *match_ptr) {
                // Look up command handler to get help text
                CommandHandler_t** handler_ptr = (CommandHandler_t**)d_TableGet(
                    terminal->registered_commands, match_ptr);

                // Build display string
                dString_t* display_text = d_StringInit();
                d_StringSet(display_text, *match_ptr, strlen(*match_ptr));

                if (handler_ptr && *handler_ptr && (*handler_ptr)->help_text) {
                    const char* help = d_StringPeek((*handler_ptr)->help_text);
                    if (help && strlen(help) > 0) {
                        d_StringAppend(display_text, " - ", 3);
                        d_StringAppend(display_text, help, strlen(help));
                    }
                }

                // Measure text width
                float text_w = 0.0f;
                float text_h = 0.0f;
                a_CalcTextDimensions((char*)d_StringPeek(display_text), FONT_ENTER_COMMAND, &text_w, &text_h);
                float scaled_width = text_w * 0.7f;  // Account for 0.7 scale
                if (scaled_width > max_text_width) {
                    max_text_width = scaled_width;
                }

                d_StringDestroy(display_text);
            }
        }

        // Set dropdown width with padding (20px total: 10px left + 10px right)
        int dropdown_width = (int)(max_text_width) + 20;
        if (dropdown_width < 200) dropdown_width = 200;  // Minimum width
        if (dropdown_width > SCREEN_WIDTH - 20) dropdown_width = SCREEN_WIDTH - 20;  // Max width

        // Draw dropdown background
        a_DrawFilledRect((aRectf_t){dropdown_x, dropdown_y - dropdown_height, dropdown_width, dropdown_height},
                        (aColor_t){30, 30, 30, 240});

        // Draw border
        a_DrawRect((aRectf_t){dropdown_x, dropdown_y - dropdown_height, dropdown_width, dropdown_height},
                  (aColor_t){100, 100, 100, 255});

        // Draw each match with help text
        for (size_t i = 0; i < terminal->autocomplete_matches->count; i++) {
            char** match_ptr = (char**)d_ArrayGet(terminal->autocomplete_matches, i);
            if (match_ptr && *match_ptr) {
                int item_y = dropdown_y - dropdown_height + 5 + (int)(i * item_height);

                // Look up command handler to get help text
                CommandHandler_t** handler_ptr = (CommandHandler_t**)d_TableGet(
                    terminal->registered_commands, match_ptr);

                // Build display string: "command_name - help text"
                dString_t* display_text = d_StringInit();
                d_StringSet(display_text, *match_ptr, strlen(*match_ptr));

                if (handler_ptr && *handler_ptr && (*handler_ptr)->help_text) {
                    const char* help = d_StringPeek((*handler_ptr)->help_text);
                    if (help && strlen(help) > 0) {
                        d_StringAppend(display_text, " - ", 3);
                        d_StringAppend(display_text, help, strlen(help));
                    }
                }

                // Change text color if selected (no background)
                aColor_t fg_color = {200, 200, 200, 255};  // Light gray (unselected)

                if ((int)i == terminal->autocomplete_index) {
                    fg_color = (aColor_t){232, 193, 112, 255};  // Gold/yellow (selected) - matches TERMINAL_INPUT_COLOR
                }

                // Use padding to center text vertically in item_height (20px)
                // wrap_width=0 means no wrapping - text stays on one line
                a_DrawText((char*)d_StringPeek(display_text), dropdown_x + 5, item_y - 2,
                          (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg=fg_color, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=0.7f, .padding=1});

                d_StringDestroy(display_text);
            }
        }
    }

    // Draw cursor at cursor_position (not always at end)
    if (terminal->cursor_visible) {
        // Calculate cursor position based on cursor_position
        dString_t* pre_cursor = d_StringInit();
        const char* full_text = d_StringPeek(terminal->input_buffer);
        if (terminal->cursor_position > 0) {
            d_StringAppend(pre_cursor, full_text, terminal->cursor_position);
        }

        dString_t* prompt_plus_pre = d_StringInit();
        d_StringFormat(prompt_plus_pre, "%s%s", TERMINAL_PROMPT, d_StringPeek(pre_cursor));

        float text_width = 0, text_height = 0;
        a_CalcTextDimensions((char*)d_StringPeek(prompt_plus_pre), FONT_ENTER_COMMAND, &text_width, &text_height);
        int cursor_x = 10 + (int)(text_width * 0.8f);  // Apply 0.8 scale to match rendered text width

        a_DrawText("_", cursor_x, term_height - 40,
                  (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={TERMINAL_CURSOR_COLOR.r, TERMINAL_CURSOR_COLOR.g, TERMINAL_CURSOR_COLOR.b, 255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=0.8f, .padding=0});

        d_StringDestroy(pre_cursor);
        d_StringDestroy(prompt_plus_pre);
    }

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
    d_ArrayAppend(terminal->output_log, &line);

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
    if (terminal->output_log->count > TERMINAL_MAX_OUTPUT_LINES) {
        // Need to rebuild entire FlexBox when trimming (no a_FlexRemoveItem API)
        size_t lines_to_remove = terminal->output_log->count - TERMINAL_MAX_OUTPUT_LINES;

        // Destroy old lines
        for (size_t i = 0; i < lines_to_remove; i++) {
            dString_t** old_line = (dString_t**)d_ArrayGet(terminal->output_log, 0);
            if (old_line && *old_line) {
                d_StringDestroy(*old_line);
            }
            d_ArrayRemove(terminal->output_log, 0);
        }

        // Rebuild FlexBox layout with remaining lines
        if (terminal->output_layout) {
            a_FlexBoxDestroy(&terminal->output_layout);
            int term_height = (int)(SCREEN_HEIGHT * TERMINAL_HEIGHT_RATIO);
            terminal->output_layout = a_FlexBoxCreate(10, 10,
                                                       SCREEN_WIDTH - 20,
                                                       term_height - 60);
            a_FlexConfigure(terminal->output_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, 8);
            a_FlexSetPadding(terminal->output_layout, 0);

            // Re-add all remaining lines to FlexBox
            for (size_t i = 0; i < terminal->output_log->count; i++) {
                dString_t** line_ptr = (dString_t**)d_ArrayGet(terminal->output_log, i);
                if (line_ptr && *line_ptr) {
                    a_FlexAddItem(terminal->output_layout, SCREEN_WIDTH - 20, 24, *line_ptr);
                }
            }
            a_FlexLayout(terminal->output_layout);
        }
    }
}

void TerminalClear(Terminal_t* terminal) {
    if (!terminal) return;

    // Destroy all output lines
    for (size_t i = 0; i < terminal->output_log->count; i++) {
        dString_t** str_ptr = (dString_t**)d_ArrayGet(terminal->output_log, i);
        if (str_ptr && *str_ptr) {
            d_StringDestroy(*str_ptr);
        }
    }

    // Clear array (hacky: destroy and recreate)
    d_ArrayDestroy(terminal->output_log);
    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    terminal->output_log = d_ArrayInit(TERMINAL_MAX_OUTPUT_LINES, sizeof(dString_t*));

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

void RegisterCommand(Terminal_t* terminal, const char* name, CommandFunc_t execute, const char* help_text, ArgSuggestFunc_t suggest_args) {
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

    handler->suggest_args = suggest_args;  // Optional: NULL if no argument autocomplete

    // Store in table (key is the name string)
    const char* key = d_StringPeek(handler->name);
    d_TableSet(terminal->registered_commands, &key, &handler);

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
    CommandHandler_t** handler_ptr = (CommandHandler_t**)d_TableGet(terminal->registered_commands, &key);

    if (!handler_ptr || !*handler_ptr) {
        TerminalPrint(terminal, "[Error] Unknown command: %s", cmd_name);
        TerminalPrint(terminal, "Type 'help' for available commands");
        return;
    }

    // Execute command
    CommandHandler_t* handler = *handler_ptr;
    handler->execute(terminal, args);
}
