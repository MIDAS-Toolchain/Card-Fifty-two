/*
 * Event Preview Modal Component
 * Shows event title with 3-second timer and reroll/continue buttons
 */

#include "../../../include/scenes/components/eventPreviewModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include <stdio.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

EventPreviewModal_t* CreateEventPreviewModal(GameContext_t* game, const char* event_title) {
    if (!event_title) {
        d_LogError("CreateEventPreviewModal: NULL event_title");
        return NULL;
    }

    EventPreviewModal_t* modal = malloc(sizeof(EventPreviewModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate EventPreviewModal");
        return NULL;
    }

    // Copy event title (static buffer pattern)
    strncpy(modal->event_title, event_title, sizeof(modal->event_title) - 1);
    modal->event_title[sizeof(modal->event_title) - 1] = '\0';

    // Initialize state
    modal->is_visible = false;
    modal->title_alpha = 0.0f;  // Start invisible, fade in

    // Create reroll button (top button, centered)
    char reroll_text[64];
    snprintf(reroll_text, sizeof(reroll_text), "Reroll (%d chips)", game->event_reroll_cost);
    modal->reroll_button = CreateButton(
        SCREEN_WIDTH / 2 - 100,  // Centered
        SCREEN_HEIGHT / 2 + 80,  // First row
        200,
        50,
        reroll_text
    );

    // Create continue button (bottom button, centered, 20px gap)
    modal->continue_button = CreateButton(
        SCREEN_WIDTH / 2 - 100,  // Centered
        SCREEN_HEIGHT / 2 + 150, // Second row (80 + 50 + 20)
        200,
        50,
        "Continue"
    );

    if (!modal->reroll_button || !modal->continue_button) {
        d_LogError("Failed to create event preview modal buttons");
        DestroyEventPreviewModal(&modal);
        return NULL;
    }

    d_LogInfo("EventPreviewModal created");
    return modal;
}

void DestroyEventPreviewModal(EventPreviewModal_t** modal) {
    if (!modal || !*modal) return;

    // Destroy buttons
    if ((*modal)->reroll_button) {
        DestroyButton(&(*modal)->reroll_button);
    }
    if ((*modal)->continue_button) {
        DestroyButton(&(*modal)->continue_button);
    }

    free(*modal);
    *modal = NULL;
    d_LogInfo("EventPreviewModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowEventPreviewModal(EventPreviewModal_t* modal) {
    if (!modal) return;
    modal->is_visible = true;
    modal->title_alpha = 0.0f;  // Start fade animation
    d_LogInfo("EventPreviewModal shown");
}

void HideEventPreviewModal(EventPreviewModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
    d_LogInfo("EventPreviewModal hidden");
}

bool IsEventPreviewModalVisible(const EventPreviewModal_t* modal) {
    return modal && modal->is_visible;
}

// ============================================================================
// UPDATE
// ============================================================================

void UpdateEventPreviewModal(EventPreviewModal_t* modal, float dt) {
    if (!modal || !modal->is_visible) return;

    // Fade in title (0.0 → 1.0 over 0.5 seconds)
    if (modal->title_alpha < 1.0f) {
        modal->title_alpha += dt * 2.0f;  // 2.0 = 1.0 / 0.5s
        if (modal->title_alpha > 1.0f) {
            modal->title_alpha = 1.0f;
        }
    }
}

void UpdateEventPreviewModalCost(EventPreviewModal_t* modal, int current_cost) {
    if (!modal || !modal->reroll_button) return;

    // Update reroll button label with current cost
    char reroll_text[64];
    snprintf(reroll_text, sizeof(reroll_text), "Reroll (%d chips)", current_cost);
    SetButtonLabel(modal->reroll_button, reroll_text);
}

void UpdateEventPreviewContent(EventPreviewModal_t* modal, const char* new_title, int new_cost) {
    if (!modal || !new_title) {
        d_LogError("UpdateEventPreviewContent: NULL parameter");
        return;
    }

    // Update event title (static buffer copy)
    strncpy(modal->event_title, new_title, sizeof(modal->event_title) - 1);
    modal->event_title[sizeof(modal->event_title) - 1] = '\0';

    // Reset fade animation for new title
    modal->title_alpha = 0.0f;

    // Update reroll button cost
    UpdateEventPreviewModalCost(modal, new_cost);

    d_LogInfoF("EventPreviewModal content updated: '%s' (cost: %d)", new_title, new_cost);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEventPreviewModal(const EventPreviewModal_t* modal, const GameContext_t* game) {
    if (!modal || !modal->is_visible) return;

    // Draw dark overlay (full screen, 70% opacity)
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (aColor_t){0, 0, 0, 178});  // 178 = 0.7 * 255

    // Draw centered event title with fade
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, (Uint8)(modal->title_alpha * 255)},  // White with fade
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 800,  // Wrap long titles
        .scale = 1.5f       // Larger text for emphasis
    };
    a_DrawText(modal->event_title, SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 100, title_config);

    // Draw countdown timer bar (3.0 → 0.0)
    float timer_progress = game->event_preview_timer / 3.0f;  // 1.0 → 0.0
    int timer_bar_width = 400;
    int timer_bar_height = 10;
    int timer_bar_x = (SCREEN_WIDTH - timer_bar_width) / 2;
    int timer_bar_y = SCREEN_HEIGHT / 2 - 40;

    // Background (dark gray)
    a_DrawFilledRect((aRectf_t){timer_bar_x, timer_bar_y, timer_bar_width, timer_bar_height}, (aColor_t){40, 40, 40, 255});

    // Foreground (white, shrinks as timer decreases)
    int filled_width = (int)(timer_bar_width * timer_progress);
    a_DrawFilledRect((aRectf_t){timer_bar_x, timer_bar_y, filled_width, timer_bar_height}, (aColor_t){255, 255, 255, 255});

    // Draw timer text (e.g., "2.3s")
    char timer_text[32];
    snprintf(timer_text, sizeof(timer_text), "%.1fs", game->event_preview_timer);
    aTextStyle_t timer_text_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {200, 200, 200, 255},  // Light gray
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText(timer_text, SCREEN_WIDTH / 2, timer_bar_y + 20, timer_text_config);

    // Render buttons
    RenderButton(modal->reroll_button);
    RenderButton(modal->continue_button);
}
