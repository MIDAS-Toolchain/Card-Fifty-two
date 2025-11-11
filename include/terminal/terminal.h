/*
 * Developer Terminal (Quake-style Console)
 * Ctrl+` toggleable overlay for debugging and testing
 * Constitutional pattern: dString_t for strings, dArray_t/dTable_t for collections
 */

#ifndef TERMINAL_H
#define TERMINAL_H

#include "../common.h"

// ============================================================================
// TERMINAL CONSTANTS
// ============================================================================

#define TERMINAL_MAX_OUTPUT_LINES  100  // Keep last 100 lines
#define TERMINAL_HEIGHT_RATIO      0.5f // Top half of screen
#define TERMINAL_PROMPT            "> "
#define TERMINAL_BG_COLOR          ((aColor_t){20, 20, 20, 230})   // Semi-transparent dark
#define TERMINAL_TEXT_COLOR        ((aColor_t){0, 255, 0, 255})    // Green
#define TERMINAL_INPUT_COLOR       ((aColor_t){232, 193, 112, 255}) // Gold
#define TERMINAL_CURSOR_COLOR      ((aColor_t){255, 255, 255, 255}) // White

// ============================================================================
// COMMAND HANDLER
// ============================================================================

typedef struct Terminal Terminal_t;

/**
 * CommandHandler_t - Function pointer for terminal commands
 *
 * @param terminal - Terminal instance
 * @param args - Command arguments (everything after command name)
 */
typedef void (*CommandFunc_t)(Terminal_t* terminal, const char* args);

typedef struct CommandHandler {
    dString_t* name;       // Command name (e.g., "help")
    CommandFunc_t execute; // Function to execute
    dString_t* help_text;  // Help description
} CommandHandler_t;

// ============================================================================
// TERMINAL STRUCTURE
// ============================================================================

typedef struct Terminal {
    bool is_visible;              // Terminal overlay visible
    dString_t* input_buffer;      // Current input line
    dArray_t* command_history;    // Array of dString_t* (previous commands)
    dArray_t* output_log;         // Array of dString_t* (output lines)
    dTable_t* registered_commands; // Key: command name (char*), Value: CommandHandler_t*
    int history_index;            // Current position in command history (-1 = not navigating)
    int scroll_offset;            // Scroll position in output (0 = bottom)
    float cursor_blink_timer;     // For blinking cursor effect
    bool cursor_visible;          // Cursor blink state
    FlexBox_t* output_layout;     // FlexBox for automatic line positioning
    bool scrollbar_dragging;      // Currently dragging scrollbar thumb
    int drag_start_y;             // Mouse Y position when drag started
    int drag_start_scroll;        // Scroll offset when drag started
} Terminal_t;

// ============================================================================
// TERMINAL LIFECYCLE
// ============================================================================

/**
 * InitTerminal - Create and initialize terminal system
 *
 * @return Terminal_t* - Heap-allocated terminal (caller must destroy)
 */
Terminal_t* InitTerminal(void);

/**
 * CleanupTerminal - Destroy terminal and free resources
 *
 * @param terminal - Pointer to terminal pointer (will be set to NULL)
 */
void CleanupTerminal(Terminal_t** terminal);

// ============================================================================
// TERMINAL CONTROL
// ============================================================================

/**
 * ToggleTerminal - Show/hide terminal overlay
 *
 * @param terminal - Terminal instance
 */
void ToggleTerminal(Terminal_t* terminal);

/**
 * IsTerminalVisible - Check if terminal is open
 *
 * @param terminal - Terminal instance
 * @return true if terminal is visible
 */
bool IsTerminalVisible(const Terminal_t* terminal);

// ============================================================================
// TERMINAL UPDATE & RENDER
// ============================================================================

/**
 * UpdateTerminal - Handle terminal logic (cursor blink)
 *
 * @param terminal - Terminal instance
 * @param dt - Delta time in seconds
 */
void UpdateTerminal(Terminal_t* terminal, float dt);

/**
 * HandleTerminalInput - Process keyboard input for terminal
 * Should be called BEFORE game input when terminal is visible
 *
 * @param terminal - Terminal instance
 */
void HandleTerminalInput(Terminal_t* terminal);

/**
 * RenderTerminal - Draw terminal overlay
 * Should be called LAST (after all game rendering)
 *
 * @param terminal - Terminal instance
 */
void RenderTerminal(Terminal_t* terminal);

// ============================================================================
// TERMINAL OUTPUT
// ============================================================================

/**
 * TerminalPrint - Add line to terminal output
 *
 * @param terminal - Terminal instance
 * @param format - Printf-style format string
 * @param ... - Format arguments
 */
void TerminalPrint(Terminal_t* terminal, const char* format, ...);

/**
 * TerminalClear - Clear all output lines
 *
 * @param terminal - Terminal instance
 */
void TerminalClear(Terminal_t* terminal);

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

/**
 * RegisterCommand - Add command to terminal
 *
 * @param terminal - Terminal instance
 * @param name - Command name (case-insensitive)
 * @param execute - Function to execute when command is called
 * @param help_text - Description shown in 'help' command
 */
void RegisterCommand(Terminal_t* terminal, const char* name, CommandFunc_t execute, const char* help_text);

/**
 * ExecuteCommand - Parse and run terminal command
 *
 * @param terminal - Terminal instance
 * @param command_line - Full command string (e.g., "spawn_enemy Boss 100")
 */
void ExecuteCommand(Terminal_t* terminal, const char* command_line);

// ============================================================================
// BUILT-IN COMMANDS (implemented in commands.c)
// ============================================================================

void RegisterBuiltinCommands(Terminal_t* terminal);

#endif // TERMINAL_H
