/*
 * Event Modal Component
 * Displays event encounters with choice selection
 * Pattern: Matches RewardModal architecture for visual consistency
 */

#include "../../../include/scenes/components/eventModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include <stdio.h>

// Color palette (EXACTLY matching RewardModal)
static const SDL_Color COLOR_OVERLAY = {9, 10, 20, 180};         // #090a14 - almost black overlay
static const SDL_Color COLOR_PANEL_BG = {9, 10, 20, 240};        // #090a14 - almost black
static const SDL_Color COLOR_HEADER_BG = {37, 58, 94, 255};      // #253a5e - dark navy blue
static const SDL_Color COLOR_HEADER_BORDER = {60, 94, 139, 255}; // #3c5e8b - medium blue
static const SDL_Color COLOR_HEADER_TEXT = {231, 213, 179, 255}; // #e7d5b3 - cream
static const SDL_Color COLOR_EVENT_INFO_TEXT = {168, 181, 178, 255}; // #a8b5b2 - light gray (matches RewardModal)
static const SDL_Color COLOR_CHOICE_BG = {37, 58, 94, 255};      // #253a5e - dark navy (same as header)
static const SDL_Color COLOR_CHOICE_HOVER = {60, 94, 139, 255};  // #3c5e8b - medium blue (same as border)
static const SDL_Color COLOR_CHIPS_POSITIVE = {117, 255, 67, 255};   // Green (+chips)
static const SDL_Color COLOR_CHIPS_NEGATIVE = {255, 67, 67, 255};    // Red (-chips)
static const SDL_Color COLOR_SANITY_POSITIVE = {67, 200, 255, 255};  // Cyan (+sanity)
static const SDL_Color COLOR_SANITY_NEGATIVE = {165, 48, 48, 255};   // Dark red (-sanity)

// ============================================================================
// LIFECYCLE
// ============================================================================

EventModal_t* CreateEventModal(void) {
    EventModal_t* modal = malloc(sizeof(EventModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate EventModal");
        return NULL;
    }

    // Initialize state
    modal->is_visible = false;
    modal->current_event = NULL;
    modal->hovered_choice = -1;
    modal->selected_choice = -1;
    modal->fade_in_alpha = 0.0f;

    // Calculate modal position (matching RewardModal pattern)
    int modal_x = ((SCREEN_WIDTH - EVENT_MODAL_WIDTH) / 2) + 96;  // Match RewardModal offset
    int modal_y = (SCREEN_HEIGHT - EVENT_MODAL_HEIGHT) / 2;
    int panel_body_y = modal_y + EVENT_MODAL_HEADER_HEIGHT;

    // Create FlexBox for header (vertical layout for description)
    int header_y = panel_body_y + 20;
    modal->header_layout = a_CreateFlexBox(
        modal_x + EVENT_MODAL_PADDING,
        header_y,
        EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2),
        100
    );

    // Create FlexBox for choice list (vertical layout for choice buttons)
    int choice_y = header_y + 120;  // Below description area
    modal->choice_layout = a_CreateFlexBox(
        modal_x + EVENT_MODAL_PADDING,
        choice_y,
        EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2),
        400
    );

    if (!modal->header_layout || !modal->choice_layout) {
        d_LogError("Failed to create EventModal FlexBox layouts");
        if (modal->header_layout) a_DestroyFlexBox(&modal->header_layout);
        if (modal->choice_layout) a_DestroyFlexBox(&modal->choice_layout);
        free(modal);
        return NULL;
    }

    // Configure layouts (matching RewardModal pattern)
    a_FlexSetDirection(modal->header_layout, FLEX_DIR_COLUMN);
    a_FlexSetGap(modal->header_layout, 10);

    a_FlexSetDirection(modal->choice_layout, FLEX_DIR_COLUMN);
    a_FlexSetGap(modal->choice_layout, EVENT_CHOICE_SPACING);

    d_LogInfo("EventModal created");
    return modal;
}

void DestroyEventModal(EventModal_t** modal) {
    if (!modal || !*modal) return;

    // Destroy FlexBox layouts
    if ((*modal)->header_layout) {
        a_DestroyFlexBox(&(*modal)->header_layout);
    }
    if ((*modal)->choice_layout) {
        a_DestroyFlexBox(&(*modal)->choice_layout);
    }

    free(*modal);
    *modal = NULL;
    d_LogInfo("EventModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowEventModal(EventModal_t* modal, EventEncounter_t* event) {
    if (!modal || !event) {
        d_LogError("ShowEventModal: NULL modal or event");
        return;
    }

    modal->is_visible = true;
    modal->current_event = event;  // Reference, not owned
    modal->hovered_choice = -1;
    modal->selected_choice = -1;
    modal->fade_in_alpha = 0.0f;  // Start fade animation

    d_LogInfoF("EventModal shown: %s", d_StringPeek(event->title));
}

void HideEventModal(EventModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
    modal->current_event = NULL;
    d_LogInfo("EventModal hidden");
}

bool IsEventModalVisible(const EventModal_t* modal) {
    return modal && modal->is_visible;
}

// ============================================================================
// INPUT & UPDATE
// ============================================================================

bool HandleEventModalInput(EventModal_t* modal, float dt) {
    if (!modal || !modal->is_visible || !modal->current_event) {
        return false;
    }

    // Update fade-in animation (0.0 → 1.0 over 0.3 seconds)
    if (modal->fade_in_alpha < 1.0f) {
        modal->fade_in_alpha += dt * 3.33f;  // 3.33 = 1.0 / 0.3s
        if (modal->fade_in_alpha > 1.0f) {
            modal->fade_in_alpha = 1.0f;
        }
    }

    // Get choice count
    int choice_count = (int)modal->current_event->choices->count;
    if (choice_count == 0) {
        d_LogWarning("Event has no choices!");
        return false;
    }

    // Calculate modal bounds
    int modal_x = ((SCREEN_WIDTH - EVENT_MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - EVENT_MODAL_HEIGHT) / 2;
    int choice_start_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + 140;

    // Mouse position
    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Check hover state
    modal->hovered_choice = -1;
    for (int i = 0; i < choice_count; i++) {
        int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
        int choice_x = modal_x + EVENT_MODAL_PADDING;
        int choice_w = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2);

        if (mx >= choice_x && mx <= choice_x + choice_w &&
            my >= choice_y && my <= choice_y + EVENT_CHOICE_HEIGHT) {
            modal->hovered_choice = i;
            break;
        }
    }

    // Handle keyboard hotkeys (1, 2, 3) - instant selection
    if (choice_count >= 1 && app.keyboard[SDL_SCANCODE_1]) {
        app.keyboard[SDL_SCANCODE_1] = 0;  // Clear key
        modal->selected_choice = 0;
        d_LogInfoF("Event choice selected via hotkey: 1");
        return true;
    }
    if (choice_count >= 2 && app.keyboard[SDL_SCANCODE_2]) {
        app.keyboard[SDL_SCANCODE_2] = 0;  // Clear key
        modal->selected_choice = 1;
        d_LogInfoF("Event choice selected via hotkey: 2");
        return true;
    }
    if (choice_count >= 3 && app.keyboard[SDL_SCANCODE_3]) {
        app.keyboard[SDL_SCANCODE_3] = 0;  // Clear key
        modal->selected_choice = 2;
        d_LogInfoF("Event choice selected via hotkey: 3");
        return true;
    }

    // Check for click
    if (app.mouse.pressed && modal->hovered_choice != -1) {
        modal->selected_choice = modal->hovered_choice;
        d_LogInfoF("Event choice selected: %d", modal->selected_choice);
        return true;  // Choice made, caller should close modal
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEventModal(const EventModal_t* modal) {
    if (!modal || !modal->is_visible || !modal->current_event) return;

    // Apply fade-in alpha
    Uint8 fade_alpha = (Uint8)(modal->fade_in_alpha * 255);

    // Full-screen overlay
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                     COLOR_OVERLAY.r, COLOR_OVERLAY.g, COLOR_OVERLAY.b,
                     (Uint8)(COLOR_OVERLAY.a * modal->fade_in_alpha));

    // Modal panel (shifted 96px right from center, matching RewardModal)
    int modal_x = ((SCREEN_WIDTH - EVENT_MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - EVENT_MODAL_HEIGHT) / 2;

    // Draw panel body
    int panel_body_y = modal_y + EVENT_MODAL_HEADER_HEIGHT;
    int panel_body_height = EVENT_MODAL_HEIGHT - EVENT_MODAL_HEADER_HEIGHT;
    a_DrawFilledRect(modal_x, panel_body_y, EVENT_MODAL_WIDTH, panel_body_height,
                     COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, fade_alpha);

    // Draw header
    a_DrawFilledRect(modal_x, modal_y, EVENT_MODAL_WIDTH, EVENT_MODAL_HEADER_HEIGHT,
                     COLOR_HEADER_BG.r, COLOR_HEADER_BG.g, COLOR_HEADER_BG.b, fade_alpha);
    a_DrawRect(modal_x, modal_y, EVENT_MODAL_WIDTH, EVENT_MODAL_HEADER_HEIGHT,
               COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, fade_alpha);

    // Draw title in header (centered on modal panel)
    int title_center_x = modal_x + (EVENT_MODAL_WIDTH / 2);
    aFontConfig_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .color = {COLOR_HEADER_TEXT.r, COLOR_HEADER_TEXT.g, COLOR_HEADER_TEXT.b, fade_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.3f
    };
    a_DrawTextStyled((char*)d_StringPeek(modal->current_event->title),
                     title_center_x, modal_y + 10, &title_config);

    // Draw description (wrapped text, centered on modal panel)
    aFontConfig_t desc_config = {
        .type = FONT_ENTER_COMMAND,
        .color = {COLOR_EVENT_INFO_TEXT.r, COLOR_EVENT_INFO_TEXT.g, COLOR_EVENT_INFO_TEXT.b, fade_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2),
        .scale = 0.85f  // Smaller to fit more text
    };
    a_DrawTextStyled((char*)d_StringPeek(modal->current_event->description),
                     title_center_x, modal_y + EVENT_MODAL_HEADER_HEIGHT + 15, &desc_config);

    // Draw choice buttons (list items with hover highlight)
    int choice_count = (int)modal->current_event->choices->count;
    int choice_start_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + 200;  // More space for description (180 → 200)

    for (int i = 0; i < choice_count; i++) {
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, i);
        if (!choice || !choice->text) continue;

        int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
        int choice_x = modal_x + EVENT_MODAL_PADDING;
        int choice_w = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2);

        // Background (highlight if hovered)
        SDL_Color bg_color = (modal->hovered_choice == i) ? COLOR_CHOICE_HOVER : COLOR_CHOICE_BG;
        a_DrawFilledRect(choice_x, choice_y, choice_w, EVENT_CHOICE_HEIGHT,
                         bg_color.r, bg_color.g, bg_color.b, fade_alpha);
        a_DrawRect(choice_x, choice_y, choice_w, EVENT_CHOICE_HEIGHT,
                   COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, fade_alpha);

        // Big number hotkey (left side, matching RewardModal style)
        int number_x = choice_x + 25;
        int number_y = choice_y + (EVENT_CHOICE_HEIGHT / 2) - 20;

        char number_str[2];
        snprintf(number_str, sizeof(number_str), "%d", i + 1);

        aFontConfig_t number_font = {
            .type = FONT_GAME,
            .align = TEXT_ALIGN_LEFT,
            .scale = 3.0f,  // Big like RewardModal
            .color = {255, 255, 255, (Uint8)(fade_alpha * 0.6f)}  // White 60% opacity
        };
        a_DrawTextStyled(number_str, number_x, number_y, &number_font);

        // Choice text (left-aligned, shifted right to make room for number)
        int text_padding = 75;  // Shifted right for number space
        aFontConfig_t choice_text_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {255, 255, 255, fade_alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = choice_w - text_padding - 20,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(choice->text),
                         choice_x + text_padding, choice_y + 10, &choice_text_config);

        // Chips delta (color-coded: green = positive, red = negative)
        if (choice->chips_delta != 0) {
            SDL_Color chips_color = (choice->chips_delta > 0) ? COLOR_CHIPS_POSITIVE : COLOR_CHIPS_NEGATIVE;
            char chips_text[32];
            snprintf(chips_text, sizeof(chips_text), "%+d chips", choice->chips_delta);
            aFontConfig_t chips_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {chips_color.r, chips_color.g, chips_color.b, fade_alpha},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = 0,
                .scale = 0.9f
            };
            a_DrawTextStyled(chips_text, choice_x + text_padding, choice_y + 40, &chips_config);
        }

        // Sanity delta (color-coded: cyan = positive, dark red = negative)
        if (choice->sanity_delta != 0) {
            SDL_Color sanity_color = (choice->sanity_delta > 0) ? COLOR_SANITY_POSITIVE : COLOR_SANITY_NEGATIVE;
            char sanity_text[32];
            snprintf(sanity_text, sizeof(sanity_text), "%+d sanity", choice->sanity_delta);
            aFontConfig_t sanity_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {sanity_color.r, sanity_color.g, sanity_color.b, fade_alpha},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = 0,
                .scale = 0.9f
            };
            int sanity_x = choice_x + text_padding + (choice->chips_delta != 0 ? 180 : 0);  // Offset if chips shown (increased spacing)
            a_DrawTextStyled(sanity_text, sanity_x, choice_y + 40, &sanity_config);
        }
    }
}

// ============================================================================
// QUERIES
// ============================================================================

int GetSelectedChoiceIndex(const EventModal_t* modal) {
    if (!modal) return -1;
    return modal->selected_choice;
}
