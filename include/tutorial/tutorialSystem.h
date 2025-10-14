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

// Overlay opacity
#define TUTORIAL_OVERLAY_ALPHA      200  // Dark overlay alpha (0-255)
#define TUTORIAL_SPOTLIGHT_PADDING  20   // Extra padding around spotlight area

// Dialogue constants
#define DIALOGUE_WIDTH              600
#define DIALOGUE_HEIGHT             200
#define DIALOGUE_PADDING            30
#define DIALOGUE_ARROW_SIZE         40   // Size of continue arrow (→) - increased for visibility
#define DIALOGUE_ARROW_MARGIN       15   // Margin from dialogue edge

// Colors (palette-based)
#define DIALOGUE_BG                 ((aColor_t){9, 10, 20, 240})     // #090a14 - almost black
#define DIALOGUE_BORDER             ((aColor_t){168, 181, 178, 255}) // #a8b5b2 - light gray
#define DIALOGUE_TEXT               ((aColor_t){235, 237, 233, 255}) // #ebede9 - off-white
#define DIALOGUE_ARROW              ((aColor_t){115, 190, 211, 255}) // #73bed3 - light cyan

// ============================================================================
// TUTORIAL ENUMS
// ============================================================================

// Spotlight shape types
typedef enum {
    SPOTLIGHT_NONE,       // No spotlight
    SPOTLIGHT_RECTANGLE,  // Rectangular spotlight
    SPOTLIGHT_CIRCLE      // Circular spotlight
} SpotlightType_t;

// Tutorial event types (what triggers next step)
typedef enum {
    TUTORIAL_EVENT_NONE,            // No event (manual advance)
    TUTORIAL_EVENT_BUTTON_CLICK,    // Specific button clicked
    TUTORIAL_EVENT_STATE_CHANGE,    // Game state changed
    TUTORIAL_EVENT_FUNCTION_CALL,   // Specific function called
    TUTORIAL_EVENT_KEY_PRESS        // Specific key pressed
} TutorialEventType_t;

// ============================================================================
// TUTORIAL STRUCTURES
// ============================================================================

// Spotlight definition (highlights area of screen)
typedef struct TutorialSpotlight {
    SpotlightType_t type;   // Shape type
    int x, y;               // Position
    int w, h;               // Size (w=h for circle radius)
    bool show_arrow;        // Show pointing arrow from dialogue
} TutorialSpotlight_t;

// Tutorial event listener
typedef struct TutorialListener {
    TutorialEventType_t event_type;  // What event to listen for
    void* event_data;                // Event-specific data (button pointer, state value, etc.)
    bool triggered;                  // Has event been triggered
} TutorialListener_t;

// Tutorial step (one dialogue + spotlight + listener)
typedef struct TutorialStep {
    dString_t* dialogue_text;        // Dialogue content
    TutorialSpotlight_t spotlight;   // Optional spotlight highlight
    TutorialListener_t listener;     // Event trigger for next step
    struct TutorialStep* next_step;  // Next step pointer (NULL = end)
    bool is_final_step;              // Is this the last step (shows "Finish" instead of "Skip")
    int dialogue_y_position;         // Y position for dialogue (0 = center, 60 = top)
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
 */
void StartTutorial(TutorialSystem_t* system, TutorialStep_t* first_step);

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
 * RenderTutorial - Render tutorial overlay, spotlight, and dialogue
 *
 * @param system - Tutorial system
 *
 * Renders:
 * - Dark overlay covering screen
 * - Spotlight "cutout" highlighting specific area
 * - Dialogue modal with text and continue arrow
 * - Optional pointing arrow from dialogue to spotlight
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
 * @param dialogue_text - Dialogue content
 * @param spotlight - Spotlight definition (can be SPOTLIGHT_NONE)
 * @param listener - Event listener configuration
 * @param is_final_step - Is this the final step (shows "Finish" instead of "Skip")
 * @return TutorialStep_t* - Heap-allocated step (must be freed)
 */
TutorialStep_t* CreateTutorialStep(const char* dialogue_text,
                                   TutorialSpotlight_t spotlight,
                                   TutorialListener_t listener,
                                   bool is_final_step,
                                   int dialogue_y_position);

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
