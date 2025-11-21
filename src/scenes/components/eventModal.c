/*
 * Event Modal Component
 * Displays event encounters with choice selection
 * Pattern: Matches RewardModal architecture for visual consistency
 */

#include "../../../include/scenes/components/eventModal.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/event.h"
#include "../../../include/player.h"
#include "../../../include/trinket.h"
#include <stdio.h>
#include <string.h>

// Color palette (EXACTLY matching RewardModal)
static const SDL_Color COLOR_OVERLAY = {9, 10, 20, 180};         // #090a14 - almost black overlay
static const SDL_Color COLOR_PANEL_BG = {9, 10, 20, 240};        // #090a14 - almost black
static const SDL_Color COLOR_HEADER_BG = {37, 58, 94, 255};      // #253a5e - dark navy blue
static const SDL_Color COLOR_HEADER_BORDER = {60, 94, 139, 255}; // #3c5e8b - medium blue
static const SDL_Color COLOR_HEADER_TEXT = {231, 213, 179, 255}; // #e7d5b3 - cream
static const SDL_Color COLOR_EVENT_INFO_TEXT = {168, 181, 178, 255}; // #a8b5b2 - light gray (matches RewardModal)
static const SDL_Color COLOR_CHOICE_BG = {37, 58, 94, 255};      // #253a5e - dark navy (same as header)
static const SDL_Color COLOR_CHOICE_HOVER = {60, 94, 139, 255};  // #3c5e8b - medium blue (same as border)
static const SDL_Color COLOR_CHOICE_LOCKED = {20, 20, 25, 255};  // Very dark gray (locked choice)
static const SDL_Color COLOR_LOCKED_TEXT = {100, 100, 110, 255}; // Dim gray (locked choice text)

// ============================================================================
// TEXT PREPROCESSING HELPERS
// ============================================================================

/**
 * StripNewlinesFromText - Preprocess text for proper Archimedes wrapping
 *
 * @param source - Source text (may contain \n)
 * @param dest - Destination buffer
 * @param dest_size - Size of destination buffer
 *
 * Strips newlines and collapses multiple spaces into single space.
 * Lets Archimedes handle ALL text wrapping (no manual line breaks).
 *
 * Pattern: Same as IntroNarrativeModal (lines 159-173)
 */
static void StripNewlinesFromText(const char* source, char* dest, size_t dest_size) {
    if (!source || !dest || dest_size == 0) return;

    size_t src_idx = 0;
    size_t dst_idx = 0;

    while (source[src_idx] != '\0' && dst_idx < dest_size - 1) {
        if (source[src_idx] == '\n') {
            // Replace newlines with spaces (collapse multiple newlines into single space)
            if (dst_idx > 0 && dest[dst_idx - 1] != ' ') {
                dest[dst_idx++] = ' ';
            }
            src_idx++;
        } else {
            dest[dst_idx++] = source[src_idx++];
        }
    }
    dest[dst_idx] = '\0';
}

// ============================================================================
// CONSEQUENCE DISPLAY HELPERS
// ============================================================================

/**
 * GenerateChoiceTooltip - Build tooltip text with all choice consequences
 *
 * @param choice - The event choice to describe
 * @param tooltip - Output dString_t to append tooltip text
 *
 * Generates multi-line tooltip with: chips, sanity, tags, HP modifier, trinkets
 */
static void GenerateChoiceTooltip(const EventChoice_t* choice, dString_t* tooltip) {
    if (!choice || !tooltip) return;

    // Chips delta
    if (choice->chips_delta != 0) {
        char line[64];
        snprintf(line, sizeof(line), "%s%+d chips\n",
                 choice->chips_delta > 0 ? "[+] " : "[-] ", choice->chips_delta);
        d_StringAppend(tooltip, line, strlen(line));
    }

    // Sanity delta
    if (choice->sanity_delta != 0) {
        char line[64];
        snprintf(line, sizeof(line), "%s%+d sanity\n",
                 choice->sanity_delta > 0 ? "[+] " : "[-] ", choice->sanity_delta);
        d_StringAppend(tooltip, line, strlen(line));
    }

    // Tag grants - consolidate duplicates (e.g., 3x CURSED random -> "3x Cursed (random cards)")
    // Count tags by type+strategy combination
    int tag_counts[CARD_TAG_MAX][16] = {0};  // [tag][strategy] = count
    for (size_t i = 0; i < choice->granted_tags->count; i++) {
        CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray(choice->granted_tags, i);
        TagTargetStrategy_t* strategy = (TagTargetStrategy_t*)d_IndexDataFromArray(choice->tag_target_strategies, i);
        if (tag && strategy && *tag < CARD_TAG_MAX && *strategy < 16) {
            tag_counts[*tag][*strategy]++;
        }
    }

    // Output consolidated tags
    for (int t = 0; t < CARD_TAG_MAX; t++) {
        for (int s = 0; s < 16; s++) {
            if (tag_counts[t][s] == 0) continue;

            const char* target = "";
            const char* target_plural = "";
            switch (s) {
                case TAG_TARGET_RANDOM_CARD:
                    target = "(random card)";
                    target_plural = "(random cards)";
                    break;
                case TAG_TARGET_HIGHEST_UNTAGGED: target = target_plural = "(highest card)"; break;
                case TAG_TARGET_LOWEST_UNTAGGED:  target = target_plural = "(lowest card)"; break;
                case TAG_TARGET_SUIT_HEARTS:      target = target_plural = "(all hearts)"; break;
                case TAG_TARGET_SUIT_DIAMONDS:    target = target_plural = "(all diamonds)"; break;
                case TAG_TARGET_SUIT_CLUBS:       target = target_plural = "(all clubs)"; break;
                case TAG_TARGET_SUIT_SPADES:      target = target_plural = "(all spades)"; break;
                case TAG_TARGET_RANK_ACES:        target = target_plural = "(all aces)"; break;
                case TAG_TARGET_RANK_FACE_CARDS:  target = target_plural = "(all face cards)"; break;
                case TAG_TARGET_ALL_CARDS:        target = target_plural = "(all cards)"; break;
                default: target = target_plural = ""; break;
            }

            char line[128];
            if (tag_counts[t][s] > 1) {
                snprintf(line, sizeof(line), "[+] %dx %s %s\n",
                         tag_counts[t][s], GetCardTagName((CardTag_t)t), target_plural);
            } else {
                snprintf(line, sizeof(line), "[+] %s %s\n",
                         GetCardTagName((CardTag_t)t), target);
            }
            d_StringAppend(tooltip, line, strlen(line));
        }
    }

    // Tag removals
    for (size_t i = 0; i < choice->removed_tags->count; i++) {
        CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray(choice->removed_tags, i);
        if (!tag) continue;

        char line[128];
        snprintf(line, sizeof(line), "[-] Lose %s (all cards)\n", GetCardTagName(*tag));
        d_StringAppend(tooltip, line, strlen(line));
    }

    // Enemy HP modifier
    if (choice->enemy_hp_multiplier != 1.0f) {
        char line[64];
        int hp_percent = (int)(choice->enemy_hp_multiplier * 100);
        snprintf(line, sizeof(line), "%sDaemon at %d%% HP\n",
                 choice->enemy_hp_multiplier > 1.0f ? "[-] " : "[+] ", hp_percent);
        d_StringAppend(tooltip, line, strlen(line));
    }

    // Trinket reward
    if (choice->trinket_reward_id >= 0) {
        Trinket_t* trinket = GetTrinketByID(choice->trinket_reward_id);
        char line[128];
        if (trinket && trinket->name) {
            snprintf(line, sizeof(line), "[+] Trinket: %s\n", d_StringPeek(trinket->name));
        } else {
            snprintf(line, sizeof(line), "[+] Trinket (ID: %d)\n", choice->trinket_reward_id);
        }
        d_StringAppend(tooltip, line, strlen(line));
    }
}

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

bool HandleEventModalInput(EventModal_t* modal, const Player_t* player, float dt) {
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
    int content_row_height = 282;  // Match rendering
    int choice_start_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + content_row_height + 20;  // Match rendering position

    // Mouse position
    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Check hover state (skip locked choices)
    modal->hovered_choice = -1;
    for (int i = 0; i < choice_count; i++) {
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, i);
        if (!choice) continue;

        // Skip locked choices for hover
        bool is_locked = !IsChoiceRequirementMet(&choice->requirement, player);
        if (is_locked) {
            // Allow hover for tooltip display, but don't enable selection
            int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
            int choice_x = modal_x + EVENT_MODAL_PADDING;
            int choice_w = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2);

            if (mx >= choice_x && mx <= choice_x + choice_w &&
                my >= choice_y && my <= choice_y + EVENT_CHOICE_HEIGHT) {
                modal->hovered_choice = i;  // Allow hover for tooltip
                break;
            }
            continue;  // Don't allow selection though
        }

        int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
        int choice_x = modal_x + EVENT_MODAL_PADDING;
        int choice_w = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2);

        if (mx >= choice_x && mx <= choice_x + choice_w &&
            my >= choice_y && my <= choice_y + EVENT_CHOICE_HEIGHT) {
            modal->hovered_choice = i;
            break;
        }
    }

    // Handle keyboard hotkeys (1, 2, 3) - block locked choices
    if (choice_count >= 1 && app.keyboard[SDL_SCANCODE_1]) {
        app.keyboard[SDL_SCANCODE_1] = 0;  // Clear key
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, 0);
        if (choice && IsChoiceRequirementMet(&choice->requirement, player)) {
            modal->selected_choice = 0;
            d_LogInfoF("Event choice selected via hotkey: 1");
            return true;
        } else {
            d_LogInfo("Choice 1 is locked (requirement not met)");
        }
    }
    if (choice_count >= 2 && app.keyboard[SDL_SCANCODE_2]) {
        app.keyboard[SDL_SCANCODE_2] = 0;  // Clear key
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, 1);
        if (choice && IsChoiceRequirementMet(&choice->requirement, player)) {
            modal->selected_choice = 1;
            d_LogInfoF("Event choice selected via hotkey: 2");
            return true;
        } else {
            d_LogInfo("Choice 2 is locked (requirement not met)");
        }
    }
    if (choice_count >= 3 && app.keyboard[SDL_SCANCODE_3]) {
        app.keyboard[SDL_SCANCODE_3] = 0;  // Clear key
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, 2);
        if (choice && IsChoiceRequirementMet(&choice->requirement, player)) {
            modal->selected_choice = 2;
            d_LogInfoF("Event choice selected via hotkey: 3");
            return true;
        } else {
            d_LogInfo("Choice 3 is locked (requirement not met)");
        }
    }

    // Check for click (block locked choices)
    if (app.mouse.pressed && modal->hovered_choice != -1) {
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, modal->hovered_choice);
        if (choice && IsChoiceRequirementMet(&choice->requirement, player)) {
            modal->selected_choice = modal->hovered_choice;
            d_LogInfoF("Event choice selected: %d", modal->selected_choice);
            return true;  // Choice made, caller should close modal
        } else {
            d_LogInfo("Clicked choice is locked (requirement not met)");
        }
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEventModal(const EventModal_t* modal, const Player_t* player) {
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

    // Content row: Two-column layout using FlexBox (like IntroNarrativeModal)
    int content_row_y = modal_y + EVENT_MODAL_HEADER_HEIGHT;
    int content_area_width = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2);
    int content_area_height = 282;  // Taller to fill space (250 + 32px)

    // Create temporary FlexBox for two-column layout (image left, text right)
    FlexBox_t* content_box = a_CreateFlexBox(
        modal_x + EVENT_MODAL_PADDING,
        content_row_y + EVENT_MODAL_PADDING,
        content_area_width,
        content_area_height - (EVENT_MODAL_PADDING * 2)
    );

    if (content_box) {
        a_FlexConfigure(content_box, FLEX_DIR_ROW, FLEX_JUSTIFY_START, EVENT_MODAL_PADDING);

        // Left item: Image (1/3 of content width)
        int image_width = content_area_width / 3;
        int image_height = content_area_height - (EVENT_MODAL_PADDING * 2);
        a_FlexAddItem(content_box, image_width, image_height, NULL);

        // Right item: Description text (fills remaining space)
        int text_width = content_area_width - image_width - EVENT_MODAL_PADDING;
        a_FlexAddItem(content_box, text_width, image_height, NULL);

        // Calculate layout
        a_FlexLayout(content_box);

        // Draw image placeholder using FlexBox position
        const FlexItem_t* image_item = a_FlexGetItem(content_box, 0);
        if (image_item) {
            a_DrawFilledRect(image_item->calc_x, image_item->calc_y,
                           image_item->w, image_item->h,
                           0, 0, 0, fade_alpha);
        }

        // Draw description text using FlexBox position
        const FlexItem_t* text_item = a_FlexGetItem(content_box, 1);
        if (text_item) {
            // Preprocess description text (strip newlines for proper wrapping)
            char processed_desc[2048];
            StripNewlinesFromText(d_StringPeek(modal->current_event->description),
                                processed_desc, sizeof(processed_desc));

            aFontConfig_t desc_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {COLOR_EVENT_INFO_TEXT.r, COLOR_EVENT_INFO_TEXT.g, COLOR_EVENT_INFO_TEXT.b, fade_alpha},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = text_item->w,  // Use FlexBox calculated width (no +100 hack!)
                .scale = 0.85f
            };
            a_DrawTextStyled(processed_desc, text_item->calc_x, text_item->calc_y, &desc_config);
        }

        // Cleanup temporary FlexBox
        a_DestroyFlexBox(&content_box);
    }

    // Draw choice buttons (pushed down below the taller content row)
    int choice_count = (int)modal->current_event->choices->count;
    int choice_start_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + content_area_height + 20;

    for (int i = 0; i < choice_count; i++) {
        EventChoice_t* choice = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, i);
        if (!choice || !choice->text) continue;

        int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
        int choice_x = modal_x + EVENT_MODAL_PADDING;
        int choice_w = EVENT_MODAL_WIDTH - (EVENT_MODAL_PADDING * 2);

        // Check if choice is locked (requirement not met)
        bool is_locked = !IsChoiceRequirementMet(&choice->requirement, player);

        // Background (gray if locked, highlight if hovered, normal otherwise)
        SDL_Color bg_color = COLOR_CHOICE_BG;
        if (is_locked) {
            bg_color = COLOR_CHOICE_LOCKED;
        } else if (modal->hovered_choice == i) {
            bg_color = COLOR_CHOICE_HOVER;
        }
        a_DrawFilledRect(choice_x, choice_y, choice_w, EVENT_CHOICE_HEIGHT,
                         bg_color.r, bg_color.g, bg_color.b, fade_alpha);
        a_DrawRect(choice_x, choice_y, choice_w, EVENT_CHOICE_HEIGHT,
                   COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, fade_alpha);

        // Big number hotkey OR lock icon (left side, matching RewardModal style)
        int number_x = choice_x + 25;
        int number_y = choice_y + (EVENT_CHOICE_HEIGHT / 2) - 20;

        if (is_locked) {
            // Draw lock icon (🔒 unicode character)
            aFontConfig_t lock_font = {
                .type = FONT_GAME,
                .align = TEXT_ALIGN_LEFT,
                .scale = 2.5f,
                .color = {COLOR_LOCKED_TEXT.r, COLOR_LOCKED_TEXT.g, COLOR_LOCKED_TEXT.b, fade_alpha}
            };
            a_DrawTextStyled("🔒", number_x, number_y + 5, &lock_font);
        } else {
            // Draw number hotkey
            dString_t* number_str = d_StringInit();
            d_StringFormat(number_str, "%d", i + 1);

            aFontConfig_t number_font = {
                .type = FONT_GAME,
                .align = TEXT_ALIGN_LEFT,
                .scale = 3.0f,  // Big like RewardModal
                .color = {255, 255, 255, (Uint8)(fade_alpha * 0.6f)}  // White 60% opacity
            };
            a_DrawTextStyled(d_StringPeek(number_str), number_x, number_y, &number_font);
            d_StringDestroy(number_str);
        }

        // Choice text (left-aligned, shifted right to make room for number)
        int text_padding = 75;  // Shifted right for number space
        SDL_Color text_color = is_locked ? COLOR_LOCKED_TEXT : (SDL_Color){255, 255, 255, 255};
        aFontConfig_t choice_text_config = {
            .type = FONT_ENTER_COMMAND,
            .color = {text_color.r, text_color.g, text_color.b, fade_alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = choice_w - text_padding - 20,
            .scale = 1.0f
        };
        a_DrawTextStyled((char*)d_StringPeek(choice->text),
                         choice_x + text_padding, choice_y + 10, &choice_text_config);
    }

    // Render tooltip AFTER all choices (so it's on top / higher z-order)
    if (modal->hovered_choice >= 0 && modal->hovered_choice < (int)modal->current_event->choices->count) {
        const EventChoice_t* hovered = (EventChoice_t*)d_IndexDataFromArray(modal->current_event->choices, modal->hovered_choice);
        bool hovered_locked = !IsChoiceRequirementMet(&hovered->requirement, player);

        // Generate tooltip text
        dString_t* tooltip = d_StringInit();

        if (hovered_locked) {
            // Locked: show requirement
            GetRequirementTooltip(&hovered->requirement, tooltip);
        } else {
            // Unlocked: show consequences (rewards/risks)
            GenerateChoiceTooltip(hovered, tooltip);
        }

        // Only render if we have content
        if (d_StringGetLength(tooltip) > 0) {
            // Strip newlines for proper Archimedes wrapping (like introNarrativeModal)
            char processed_tooltip[1024];
            StripNewlinesFromText(d_StringPeek(tooltip), processed_tooltip, sizeof(processed_tooltip));

            // Tooltip sizing
            int tooltip_padding = 12;
            int text_w = 350;  // Content width for text wrapping

            // Use Archimedes to calculate actual wrapped text height
            int text_h = a_GetWrappedTextHeight(processed_tooltip, FONT_ENTER_COMMAND, text_w);

            int tooltip_w = text_w + (tooltip_padding * 2);
            int tooltip_h = text_h + (tooltip_padding * 2);
            if (tooltip_h < 40) tooltip_h = 40;  // Minimum height

            // Position near mouse cursor
            int mx = app.mouse.x;
            int my = app.mouse.y;
            int tooltip_x = mx + 15;
            int tooltip_y = my + 15;

            // Clamp tooltip to screen bounds
            if (tooltip_x + tooltip_w > SCREEN_WIDTH) {
                tooltip_x = mx - tooltip_w - 15;
            }
            if (tooltip_y + tooltip_h > SCREEN_HEIGHT) {
                tooltip_y = my - tooltip_h - 15;
            }

            // Draw tooltip background
            a_DrawFilledRect(tooltip_x, tooltip_y, tooltip_w, tooltip_h,
                            COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, 250);
            a_DrawRect(tooltip_x, tooltip_y, tooltip_w, tooltip_h,
                       COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, 255);

            // Draw tooltip text with proper wrap_width (like introNarrativeModal)
            aFontConfig_t tooltip_config = {
                .type = FONT_ENTER_COMMAND,
                .color = {COLOR_EVENT_INFO_TEXT.r, COLOR_EVENT_INFO_TEXT.g, COLOR_EVENT_INFO_TEXT.b, 255},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = text_w,
                .scale = 1.0f
            };
            a_DrawTextStyled(processed_tooltip,
                            tooltip_x + tooltip_padding, tooltip_y + tooltip_padding, &tooltip_config);
        }

        d_StringDestroy(tooltip);
    }
}

// ============================================================================
// QUERIES
// ============================================================================

int GetSelectedChoiceIndex(const EventModal_t* modal) {
    if (!modal) return -1;
    return modal->selected_choice;
}
