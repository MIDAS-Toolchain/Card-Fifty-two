/*
 * Tutorial System
 * Event-driven tutorial system with dialogue modals and spotlight highlights
 * Constitutional pattern: dString_t for text, event-based progression
 */

#ifndef TUTORIAL_SYSTEM_H
#define TUTORIAL_SYSTEM_H

#include "../common.h"

// ============================================================================
// TUTORIAL SYSTEM CONSTANTS
// ============================================================================

// Tutorial modal constants (modern design matching EventModal/RewardModal)
#define TUTORIAL_MODAL_WIDTH         700   // Wider modal (between old 600 and final 800)
#define TUTORIAL_MODAL_HEIGHT        240   // Taller for header + body
#define TUTORIAL_MODAL_HEADER_HEIGHT 50    // Navy blue header bar (matches EventModal/RewardModal)
#define TUTORIAL_MODAL_PADDING       30    // Body padding (consistent with modern modals)
#define TUTORIAL_BUTTON_MARGIN       15    // Margin for skip/finish button

// Colors (modern modal palette matching EventModal/RewardModal/IntroNarrativeModal)
#define TUTORIAL_HEADER_BG          ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy blue
#define TUTORIAL_HEADER_BORDER      ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue
#define TUTORIAL_HEADER_TEXT        ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream
#define TUTORIAL_BODY_BG            ((aColor_t){9, 10, 20, 240})     // #090a14 - almost black
#define TUTORIAL_BODY_TEXT          ((aColor_t){168, 181, 178, 255}) // #a8b5b2 - light gray
#define TUTORIAL_ARROW_COLOR        ((aColor_t){115, 190, 211, 255}) // #73bed3 - light cyan

// ============================================================================
// TUTORIAL ENUMS
// ============================================================================

// Tutorial event types (what triggers next step)
typedef enum {
    TUTORIAL_EVENT_NONE,            // No event (manual advance)
    TUTORIAL_EVENT_BUTTON_CLICK,    // Specific button clicked
    TUTORIAL_EVENT_STATE_CHANGE,    // Game state changed
    TUTORIAL_EVENT_FUNCTION_CALL,   // Specific function called
    TUTORIAL_EVENT_KEY_PRESS,       // Specific key pressed
    TUTORIAL_EVENT_HOVER            // Hover over specific area
} TutorialEventType_t;

// ============================================================================
// TUTORIAL STRUCTURES
// ============================================================================

// Tutorial event listener
typedef struct TutorialListener {
    TutorialEventType_t event_type;  // What event to listen for
    void* event_data;                // Event-specific data (button pointer, state value, etc.)
    bool triggered;                  // Has event been triggered
} TutorialListener_t;

// Arrow configuration for tutorial dialogues
typedef struct {
    bool enabled;                    // Whether to draw an arrow
    int from_x, from_y;             // Arrow start point (on dialogue box edge)
    int to_x, to_y;                 // Arrow end point (target on screen)
} TutorialArrow_t;

// Tutorial step (one dialogue + listener)
typedef struct TutorialStep {
    dString_t* title;                // Step title (displayed in header bar)
    dString_t* dialogue_text;        // Dialogue content (body text)
    TutorialListener_t listener;     // Event trigger for next step
    struct TutorialStep* next_step;  // Next step pointer (NULL = end)
    bool is_final_step;              // Is this the last step (shows "Finish" instead of "Skip")
    int dialogue_x_offset;           // X offset from center (0 = centered)
    int dialogue_y_position;         // Y position for dialogue (0 = center, 60 = top)
    int wait_for_game_state;         // Wait for this game state before advancing (-1 = don't wait)
    TutorialArrow_t arrow;           // Optional arrow pointing from dialogue to target
    bool advance_immediately;        // Advance to this step right away (don't wait for previous step's state)
} TutorialStep_t;

// Skip confirmation state
typedef struct {
    bool visible;                    // Is confirmation modal visible
    bool skip_confirmed;             // User confirmed skip
} TutorialSkipConfirmation_t;

// Tutorial system state
typedef struct TutorialSystem {
    TutorialStep_t* current_step;    // Currently active step
    bool active;                     // Is tutorial active
    bool dialogue_visible;           // Is dialogue modal visible
    TutorialStep_t* first_step;      // First step (for reset)
    TutorialSkipConfirmation_t skip_confirmation;  // Skip confirmation modal
    bool waiting_to_advance;         // Waiting for delay before advancing
    float advance_delay_timer;       // Countdown timer for advance delay (seconds)
    bool waiting_for_betting_state;  // Waiting for game to return to BETTING state
    int current_step_number;         // Current step number (1-indexed, 0 = inactive)
    void* dealer_section;            // Dealer section reference (for clearing tooltips)
    void* player_section;            // Player section reference (for clearing tooltips)
} TutorialSystem_t;

// ============================================================================
// TUTORIAL SYSTEM API
// ============================================================================

/**
 * CreateTutorialSystem - Initialize tutorial system
 *
 * @return TutorialSystem_t* - Heap-allocated system (caller must destroy)
 */
TutorialSystem_t* CreateTutorialSystem(void);

/**
 * DestroyTutorialSystem - Cleanup tutorial system resources
 *
 * @param system - Pointer to system pointer (will be set to NULL)
 */
void DestroyTutorialSystem(TutorialSystem_t** system);

/**
 * StartTutorial - Begin tutorial from first step
 *
 * @param system - Tutorial system
 * @param first_step - First tutorial step
 * @param dealer_section - Dealer section (to clear tooltips, can be NULL)
 * @param player_section - Player section (to clear tooltips, can be NULL)
 */
void StartTutorial(TutorialSystem_t* system, TutorialStep_t* first_step, void* dealer_section, void* player_section);

/**
 * StopTutorial - End tutorial and cleanup
 *
 * @param system - Tutorial system
 */
void StopTutorial(TutorialSystem_t* system);

/**
 * AdvanceTutorial - Move to next tutorial step
 *
 * @param system - Tutorial system
 *
 * NOTE: Uses stored section pointers from StartTutorial() to clear tooltips
 */
void AdvanceTutorial(TutorialSystem_t* system);

/**
 * UpdateTutorialListeners - Check if tutorial events triggered
 *
 * @param system - Tutorial system
 * @param dt - Delta time in seconds
 *
 * Checks current step's listener and advances after 0.5s delay if triggered
 */
void UpdateTutorialListeners(TutorialSystem_t* system, float dt);

/**
 * TriggerTutorialEvent - Manually trigger tutorial event
 *
 * @param system - Tutorial system
 * @param event_type - Event type to trigger
 * @param event_data - Event-specific data
 *
 * Used to notify tutorial system of game events
 */
void TriggerTutorialEvent(TutorialSystem_t* system, TutorialEventType_t event_type, void* event_data);

/**
 * CheckTutorialGameState - Notify tutorial of game state changes
 *
 * @param system - Tutorial system
 * @param game_state - Current game state (STATE_BETTING = 0, etc)
 *
 * Used to allow tutorial to advance only when game returns to BETTING state
 */
void CheckTutorialGameState(TutorialSystem_t* system, int game_state);

/**
 * RenderTutorial - Render tutorial dialogue
 *
 * @param system - Tutorial system
 *
 * Renders:
 * - Dialogue modal with text and continue arrow
 * - Optional pointing arrow from dialogue to target
 */
void RenderTutorial(TutorialSystem_t* system);

/**
 * IsTutorialActive - Check if tutorial is currently active
 *
 * @param system - Tutorial system
 * @return true if tutorial active
 */
bool IsTutorialActive(const TutorialSystem_t* system);

/**
 * HandleTutorialInput - Handle input for tutorial (continue clicks)
 *
 * @param system - Tutorial system
 * @return true if input was handled by tutorial
 */
bool HandleTutorialInput(TutorialSystem_t* system);

// ============================================================================
// TUTORIAL STEP CREATION HELPERS
// ============================================================================

/**
 * CreateTutorialStep - Create a new tutorial step
 *
 * @param title - Step title (displayed in header bar)
 * @param dialogue_text - Dialogue content (body text)
 * @param listener - Event listener configuration
 * @param is_final_step - Is this the final step (shows "Finish" instead of "Skip")
 * @param dialogue_x_offset - X offset from center (0 = centered)
 * @param dialogue_y_position - Y position for dialogue
 * @param wait_for_game_state - Wait for this game state before showing dialogue (-1 = don't wait)
 * @param arrow - Optional arrow pointing from dialogue to target
 * @param advance_immediately - Advance to this step right away (don't wait for previous step's state)
 * @return TutorialStep_t* - Heap-allocated step (must be freed)
 */
TutorialStep_t* CreateTutorialStep(const char* title,
                                   const char* dialogue_text,
                                   TutorialListener_t listener,
                                   bool is_final_step,
                                   int dialogue_x_offset,
                                   int dialogue_y_position,
                                   int wait_for_game_state,
                                   TutorialArrow_t arrow,
                                   bool advance_immediately);

/**
 * DestroyTutorialStep - Destroy a tutorial step
 *
 * @param step - Pointer to step pointer (will be set to NULL)
 */
void DestroyTutorialStep(TutorialStep_t** step);

/**
 * LinkTutorialSteps - Link two tutorial steps together
 *
 * @param current - Current step
 * @param next - Next step
 */
void LinkTutorialSteps(TutorialStep_t* current, TutorialStep_t* next);

#endif // TUTORIAL_SYSTEM_H
