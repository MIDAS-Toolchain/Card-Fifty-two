/*
 * Combat Preview Modal Component
 * Shows elite/boss combat title with 3-second timer and disabled reroll button
 */

#include "../../../include/scenes/components/combatPreviewModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/random.h"
#include "../../../include/audioHelper.h"
#include <stdio.h>

// ============================================================================
// RANDOM HEADER TITLES
// ============================================================================

static const char* COMBAT_HEADERS[] = {
    "COMBAT AHEAD",
    "PREPARE FOR BATTLE",
    "HOSTILE DETECTED",
    "BRACE YOURSELF",
    "THREAT APPROACHING"
};

#define COMBAT_HEADER_COUNT 5

// ============================================================================
// LIFECYCLE
// ============================================================================

CombatPreviewModal_t* CreateCombatPreviewModal(GameContext_t* game,
                                                const char* enemy_name,
                                                const char* encounter_type) {
    (void)game;  // Unused (kept for API consistency with EventPreviewModal)

    CombatPreviewModal_t* modal = malloc(sizeof(CombatPreviewModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate CombatPreviewModal");
        return NULL;
    }

    // Copy enemy name and type (static buffer pattern)
    strncpy(modal->enemy_name, enemy_name ? enemy_name : "", sizeof(modal->enemy_name) - 1);
    modal->enemy_name[sizeof(modal->enemy_name) - 1] = '\0';

    strncpy(modal->encounter_type, encounter_type ? encounter_type : "", sizeof(modal->encounter_type) - 1);
    modal->encounter_type[sizeof(modal->encounter_type) - 1] = '\0';

    // Initialize state
    modal->is_visible = false;
    modal->title_alpha = 0.0f;  // Start invisible, fade in

    // Create continue button (active, bottom button, centered)
    modal->continue_button = CreateButton(
        GetWindowWidth() / 2 - 100,  // Centered
        GetWindowHeight() / 2 + 150, // Second row (80 + 50 + 20)
        200,
        50,
        "Continue"
    );

    if (!modal->continue_button) {
        d_LogError("Failed to create combat preview modal buttons");
        DestroyCombatPreviewModal(&modal);
        return NULL;
    }

    d_LogInfo("CombatPreviewModal created");
    return modal;
}

void DestroyCombatPreviewModal(CombatPreviewModal_t** modal) {
    if (!modal || !*modal) return;

    // Destroy continue button
    if ((*modal)->continue_button) {
        DestroyButton(&(*modal)->continue_button);
    }

    free(*modal);
    *modal = NULL;
    d_LogInfo("CombatPreviewModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowCombatPreviewModal(CombatPreviewModal_t* modal) {
    if (!modal) return;
    modal->is_visible = true;
    modal->title_alpha = 0.0f;  // Start fade animation

    // Randomly select header title
    int header_index = GetRandomInt(0, COMBAT_HEADER_COUNT - 1);
    strncpy(modal->header_title, COMBAT_HEADERS[header_index], sizeof(modal->header_title) - 1);
    modal->header_title[sizeof(modal->header_title) - 1] = '\0';

    d_LogInfoF("CombatPreviewModal shown with header: '%s'", modal->header_title);
}

void HideCombatPreviewModal(CombatPreviewModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
    d_LogInfo("CombatPreviewModal hidden");
}

bool IsCombatPreviewModalVisible(const CombatPreviewModal_t* modal) {
    return modal && modal->is_visible;
}

// ============================================================================
// UPDATE
// ============================================================================

void UpdateCombatPreviewModal(CombatPreviewModal_t* modal, float dt) {
    if (!modal || !modal->is_visible) return;

    // Fade in title (0.0 → 1.0 over 0.5 seconds)
    if (modal->title_alpha < 1.0f) {
        modal->title_alpha += dt * 2.0f;  // 2.0 = 1.0 / 0.5s
        if (modal->title_alpha > 1.0f) {
            modal->title_alpha = 1.0f;
        }
    }
}

void UpdateCombatPreviewContent(CombatPreviewModal_t* modal,
                                 const char* enemy_name,
                                 const char* encounter_type) {
    if (!modal || !enemy_name || !encounter_type) {
        d_LogError("UpdateCombatPreviewContent: NULL parameter");
        return;
    }

    // Update enemy name and type (static buffer copy)
    strncpy(modal->enemy_name, enemy_name, sizeof(modal->enemy_name) - 1);
    modal->enemy_name[sizeof(modal->enemy_name) - 1] = '\0';

    strncpy(modal->encounter_type, encounter_type, sizeof(modal->encounter_type) - 1);
    modal->encounter_type[sizeof(modal->encounter_type) - 1] = '\0';

    // Reset fade animation for new title
    modal->title_alpha = 0.0f;

    d_LogInfoF("CombatPreviewModal content updated: '%s - %s'", encounter_type, enemy_name);
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderCombatPreviewModal(const CombatPreviewModal_t* modal, const GameContext_t* game) {
    if (!modal || !modal->is_visible) return;

    // Draw dark overlay (full screen, 70% opacity)
    a_DrawFilledRect((aRectf_t){0, 0, GetWindowWidth(), GetWindowHeight()}, (aColor_t){0, 0, 0, 178});  // 178 = 0.7 * 255

    // Draw random header title (top, large, dark gray)
    aTextStyle_t header_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, 200},  
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 2.0f  // Larger for emphasis
    };
    a_DrawText(modal->header_title, GetWindowWidth() / 2, GetWindowHeight() / 2 - 216, header_config);

    // Build title string: "{Type} - {Name}" (e.g., "Elite Combat - The Daemon")
    char title[128];
    snprintf(title, sizeof(title), "%s - %s", modal->encounter_type, modal->enemy_name);

    // Draw centered combat title with fade
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, 200}, 
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 800,  // Wrap long titles
        .scale = 1.5f       // Larger text for emphasis
    };
    a_DrawText(title, GetWindowWidth() / 2, GetWindowHeight() / 2 - 100, title_config);

    // Draw countdown timer bar (3.0 → 0.0)
    float timer_progress = game->combat_preview_timer / 3.0f;  // 1.0 → 0.0
    int timer_bar_width = 400;
    int timer_bar_height = 10;
    int timer_bar_x = (GetWindowWidth() - timer_bar_width) / 2;
    int timer_bar_y = GetWindowHeight() / 2 - 40;

    // Background (dark gray)
    a_DrawFilledRect((aRectf_t){timer_bar_x, timer_bar_y, timer_bar_width, timer_bar_height}, (aColor_t){40, 40, 40, 255});

    // Foreground (white, shrinks as timer decreases)
    int filled_width = (int)(timer_bar_width * timer_progress);
    a_DrawFilledRect((aRectf_t){timer_bar_x, timer_bar_y, filled_width, timer_bar_height}, (aColor_t){255, 255, 255, 255});

    // Draw timer text (e.g., "2.3s")
    char timer_text[32];
    snprintf(timer_text, sizeof(timer_text), "%.1fs", game->combat_preview_timer);
    aTextStyle_t timer_text_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {200, 200, 200, 255},  // Light gray
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText(timer_text, GetWindowWidth() / 2, timer_bar_y + 20, timer_text_config);

    // Draw "Encounter Inevitable" text (where reroll button used to be)
    aTextStyle_t inevitable_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {150, 150, 150, 255},  // Gray text (non-interactive)
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText("Encounter Inevitable", GetWindowWidth() / 2, GetWindowHeight() / 2 + 105, inevitable_config);

    // Render continue button
    RenderButton(modal->continue_button);
}
