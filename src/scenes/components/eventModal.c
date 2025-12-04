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
#include "../../../include/loaders/trinketLoader.h"  // For GetTrinketTemplate
#include "../../../include/trinketDrop.h"  // For RollAffixes
#include "../../../include/tween/tween.h"
#include "../../../include/audioHelper.h"
#include <stdio.h>
#include <string.h>

// External tween manager (from sceneBlackjack.c)
extern TweenManager_t g_tween_manager;

// Color palette (EXACTLY matching RewardModal)
static const aColor_t COLOR_OVERLAY = {9, 10, 20, 180};         // #090a14 - almost black overlay
static const aColor_t COLOR_PANEL_BG = {9, 10, 20, 240};        // #090a14 - almost black
static const aColor_t COLOR_HEADER_BG = {37, 58, 94, 255};      // #253a5e - dark navy blue
static const aColor_t COLOR_HEADER_BORDER = {60, 94, 139, 255}; // #3c5e8b - medium blue
static const aColor_t COLOR_HEADER_TEXT = {231, 213, 179, 255}; // #e7d5b3 - cream
static const aColor_t COLOR_EVENT_INFO_TEXT = {168, 181, 178, 255}; // #a8b5b2 - light gray (matches RewardModal)
static const aColor_t COLOR_CHOICE_BG = {37, 58, 94, 255};      // #253a5e - dark navy (same as header)
static const aColor_t COLOR_CHOICE_HOVER = {60, 94, 139, 255};  // #3c5e8b - medium blue (same as border)
static const aColor_t COLOR_CHOICE_LOCKED = {20, 20, 25, 255};  // Very dark gray (locked choice)
static const aColor_t COLOR_LOCKED_TEXT = {100, 100, 110, 255}; // Dim gray (locked choice text)

// Tooltip line sentiment colors (from palette)
static const aColor_t COLOR_TOOLTIP_POSITIVE = {117, 167, 67, 255};  // #75a743 - green (upside)
static const aColor_t COLOR_TOOLTIP_NEGATIVE = {165, 48, 48, 255};   // #a53030 - red (downside)
static const aColor_t COLOR_TOOLTIP_NEUTRAL = {232, 193, 112, 255};  // #e8c170 - yellow (neutral)

// Tooltip line data structure
#define MAX_TOOLTIP_LINES 8

typedef enum {
    TOOLTIP_LINE_POSITIVE,
    TOOLTIP_LINE_NEGATIVE,
    TOOLTIP_LINE_NEUTRAL
} TooltipLineSentiment_t;

typedef struct {
    char text[128];
    TooltipLineSentiment_t sentiment;
} TooltipLine_t;

// Runtime modal width helper (responsive like IntroNarrativeModal)
static inline int GetEventModalWidth(void) {
    return GetWindowWidth() - EVENT_MODAL_INSET;
}

// Hover sound tracking
static int last_hovered_choice = -1;

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
 * GenerateChoiceTooltipLines - Build tooltip lines with sentiments
 *
 * @param choice - The event choice to describe
 * @param lines - Output array of TooltipLine_t
 * @param max_lines - Maximum number of lines to generate
 * @return Number of lines generated
 */
static int GenerateChoiceTooltipLines(const EventChoice_t* choice, TooltipLine_t* lines, int max_lines) {
    if (!choice || !lines || max_lines <= 0) return 0;

    int count = 0;

    // Chips delta
    if (choice->chips_delta != 0 && count < max_lines) {
        snprintf(lines[count].text, sizeof(lines[count].text), "%+d chips", choice->chips_delta);
        lines[count].sentiment = (choice->chips_delta > 0) ? TOOLTIP_LINE_POSITIVE : TOOLTIP_LINE_NEGATIVE;
        count++;
    }

    // Sanity delta
    if (choice->sanity_delta != 0 && count < max_lines) {
        snprintf(lines[count].text, sizeof(lines[count].text), "%+d sanity", choice->sanity_delta);
        lines[count].sentiment = (choice->sanity_delta > 0) ? TOOLTIP_LINE_POSITIVE : TOOLTIP_LINE_NEGATIVE;
        count++;
    }

    // Tag grants - consolidate duplicates
    int tag_counts[CARD_TAG_MAX][16] = {0};
    for (size_t i = 0; i < choice->granted_tags->count; i++) {
        CardTag_t* tag = (CardTag_t*)d_ArrayGet(choice->granted_tags, i);
        TagTargetStrategy_t* strategy = (TagTargetStrategy_t*)d_ArrayGet(choice->tag_target_strategies, i);
        if (tag && strategy && *tag < CARD_TAG_MAX && *strategy < 16) {
            tag_counts[*tag][*strategy]++;
        }
    }

    for (int t = 0; t < CARD_TAG_MAX && count < max_lines; t++) {
        for (int s = 0; s < 16 && count < max_lines; s++) {
            if (tag_counts[t][s] == 0) continue;

            const char* target = "";
            switch (s) {
                case TAG_TARGET_RANDOM_CARD:
                    target = tag_counts[t][s] > 1 ? "(random cards)" : "(random card)";
                    break;
                case TAG_TARGET_HIGHEST_UNTAGGED: target = "(highest card)"; break;
                case TAG_TARGET_LOWEST_UNTAGGED:  target = "(lowest card)"; break;
                case TAG_TARGET_SUIT_HEARTS:      target = "(all hearts)"; break;
                case TAG_TARGET_SUIT_DIAMONDS:    target = "(all diamonds)"; break;
                case TAG_TARGET_SUIT_CLUBS:       target = "(all clubs)"; break;
                case TAG_TARGET_SUIT_SPADES:      target = "(all spades)"; break;
                case TAG_TARGET_RANK_ACES:        target = "(all aces)"; break;
                case TAG_TARGET_RANK_FACE_CARDS:  target = "(all face cards)"; break;
                case TAG_TARGET_ALL_CARDS:        target = "(all cards)"; break;
                default: target = ""; break;
            }

            if (tag_counts[t][s] > 1) {
                snprintf(lines[count].text, sizeof(lines[count].text), "%dx %s %s",
                         tag_counts[t][s], GetCardTagName((CardTag_t)t), target);
            } else {
                snprintf(lines[count].text, sizeof(lines[count].text), "%s %s",
                         GetCardTagName((CardTag_t)t), target);
            }
            lines[count].sentiment = TOOLTIP_LINE_POSITIVE;  // Tags are beneficial
            count++;
        }
    }

    // Tag removals
    for (size_t i = 0; i < choice->removed_tags->count && count < max_lines; i++) {
        CardTag_t* tag = (CardTag_t*)d_ArrayGet(choice->removed_tags, i);
        if (!tag) continue;

        snprintf(lines[count].text, sizeof(lines[count].text), "Lose %s (all cards)", GetCardTagName(*tag));
        lines[count].sentiment = TOOLTIP_LINE_NEGATIVE;
        count++;
    }

    // Enemy HP modifier (next enemy only)
    if (choice->next_enemy_hp_multi != 1.0f && count < max_lines) {
        int hp_percent = (int)(choice->next_enemy_hp_multi * 100);
        snprintf(lines[count].text, sizeof(lines[count].text), "Next Daemon at %d%% HP", hp_percent);
        lines[count].sentiment = (choice->next_enemy_hp_multi < 1.0f) ? TOOLTIP_LINE_POSITIVE : TOOLTIP_LINE_NEGATIVE;
        count++;
    }

    // Trinket reward (name + pre-rolled affix)
    if (choice->has_trinket_reward && count < max_lines) {
        // Load template to get display name
        TrinketTemplate_t* template = GetTrinketTemplate(choice->trinket_reward_key);
        if (template && template->name) {
            // Line 1: Trinket name
            snprintf(lines[count].text, sizeof(lines[count].text), "Trinket: %s", d_StringPeek(template->name));
            lines[count].sentiment = TOOLTIP_LINE_POSITIVE;
            count++;

            // Line 2+: Affixes (pre-rolled in ShowEventModal)
            // Cast away const: We're only reading from this struct
            TrinketInstance_t* inst = (TrinketInstance_t*)&choice->trinket_reward_instance;
            for (int i = 0; i < inst->affix_count && count < max_lines; i++) {
                const char* stat_key = d_StringPeek(inst->affixes[i].stat_key);
                int value = inst->affixes[i].rolled_value;
                snprintf(lines[count].text, sizeof(lines[count].text), "  +%d %s", value, stat_key);
                lines[count].sentiment = TOOLTIP_LINE_NEUTRAL;  // Blue text in rendering
                count++;
            }

            // NOTE: template is borrowed pointer from cache - do NOT free!
        } else {
            // Fallback if template not found
            snprintf(lines[count].text, sizeof(lines[count].text), "Trinket: %s", choice->trinket_reward_key);
            lines[count].sentiment = TOOLTIP_LINE_POSITIVE;
            count++;
        }
    }

    return count;
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
    modal->phase = EVENT_PHASE_CHOOSING;
    modal->confirmed_choice = -1;
    modal->fade_in_alpha = 0.0f;
    modal->choices_alpha = 1.0f;
    modal->result_alpha = 0.0f;

    // Calculate modal position (matching RewardModal pattern)
    int modal_x = ((GetWindowWidth() - GetEventModalWidth()) / 2);  // Offset right (past deck UI)
    int modal_y = (GetWindowHeight() - EVENT_MODAL_HEIGHT) / 2;
    int panel_body_y = modal_y + EVENT_MODAL_HEADER_HEIGHT;

    // Create FlexBox for header (vertical layout for description)
    int header_y = panel_body_y + 20;
    modal->header_layout = a_FlexBoxCreate(
        modal_x + EVENT_MODAL_PADDING,
        header_y,
        GetEventModalWidth() - (EVENT_MODAL_PADDING * 2),
        100
    );

    // Create FlexBox for choice list (vertical layout for choice buttons)
    int choice_y = header_y + 120;  // Below description area
    modal->choice_layout = a_FlexBoxCreate(
        modal_x + EVENT_MODAL_PADDING,
        choice_y,
        GetEventModalWidth() - (EVENT_MODAL_PADDING * 2),
        400
    );

    if (!modal->header_layout || !modal->choice_layout) {
        d_LogError("Failed to create EventModal FlexBox layouts");
        if (modal->header_layout) a_FlexBoxDestroy(&modal->header_layout);
        if (modal->choice_layout) a_FlexBoxDestroy(&modal->choice_layout);
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
        a_FlexBoxDestroy(&(*modal)->header_layout);
    }
    if ((*modal)->choice_layout) {
        a_FlexBoxDestroy(&(*modal)->choice_layout);
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
    modal->phase = EVENT_PHASE_CHOOSING;  // Start in choosing phase
    modal->confirmed_choice = -1;
    modal->fade_in_alpha = 0.0f;  // Start fade animation
    modal->choices_alpha = 1.0f;  // Choices fully visible
    modal->result_alpha = 0.0f;   // Result hidden

    // Pre-roll trinket affixes for each choice (so players see exact rolls in tooltip)
    for (size_t i = 0; i < event->choices->count; i++) {
        EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(event->choices, i);
        if (!choice) continue;

        if (choice->trinket_reward_key[0] != '\0') {
            // Load template from DUF
            TrinketTemplate_t* template = GetTrinketTemplate(choice->trinket_reward_key);
            if (!template) {
                d_LogErrorF("ShowEventModal: Trinket key '%s' not found", choice->trinket_reward_key);
                continue;
            }

            // Initialize instance with template data
            TrinketInstance_t* inst = &choice->trinket_reward_instance;

            // Initialize base_trinket_key
            if (!inst->base_trinket_key) inst->base_trinket_key = d_StringInit();
            d_StringSet(inst->base_trinket_key, choice->trinket_reward_key, 0);

            inst->rarity = template->rarity;
            inst->tier = 1;  // Tutorial = Act 1
            inst->sell_value = template->base_value;

            // Roll affixes (1 for Act 1)
            RollAffixes(1, template->rarity, inst);

            // Copy trinket stack data (for Stack Trace)
            inst->trinket_stack_max = template->passive_stack_max;
            inst->trinket_stack_value = template->passive_stack_value;
            if (template->passive_stack_stat) {
                if (!inst->trinket_stack_stat) inst->trinket_stack_stat = d_StringInit();
                d_StringSet(inst->trinket_stack_stat, d_StringPeek(template->passive_stack_stat), 0);
            }

            // Initialize runtime state
            inst->trinket_stacks = 0;

            // Data-driven stat init
            memset(inst->tracked_stats, 0, sizeof(inst->tracked_stats));

            inst->buffed_tag = -1;
            inst->tag_buff_value = 0;
            inst->shake_offset_x = 0.0f;
            inst->shake_offset_y = 0.0f;
            inst->flash_alpha = 0.0f;

            choice->has_trinket_reward = true;

            // NOTE: template is borrowed pointer from cache - no cleanup needed

            d_LogInfoF("Pre-rolled trinket '%s' for choice %zu: %d affix(es)",
                       choice->trinket_reward_key, i, inst->affix_count);
        }
    }

    d_LogInfoF("EventModal shown: %s", d_StringPeek(event->title));
}

void HideEventModal(EventModal_t* modal) {
    if (!modal) return;

    // Cleanup pre-rolled trinket instances
    if (modal->current_event) {
        for (size_t i = 0; i < modal->current_event->choices->count; i++) {
            EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(modal->current_event->choices, i);
            if (choice && choice->has_trinket_reward) {
                TrinketInstance_t* inst = &choice->trinket_reward_instance;

                // Cleanup base_trinket_key
                if (inst->base_trinket_key) {
                    d_StringDestroy(inst->base_trinket_key);
                    inst->base_trinket_key = NULL;
                }

                // Cleanup affixes
                for (int j = 0; j < inst->affix_count; j++) {
                    if (inst->affixes[j].stat_key) {
                        d_StringDestroy(inst->affixes[j].stat_key);
                        inst->affixes[j].stat_key = NULL;
                    }
                }

                // Cleanup trinket_stack_stat
                if (inst->trinket_stack_stat) {
                    d_StringDestroy(inst->trinket_stack_stat);
                    inst->trinket_stack_stat = NULL;
                }

                choice->has_trinket_reward = false;
                d_LogDebugF("Cleaned up pre-rolled trinket for choice %zu", i);
            }
        }
    }

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

// Helper: Confirm a choice and start transition to result phase
static void ConfirmChoice(EventModal_t* modal, int choice_index) {
    modal->confirmed_choice = choice_index;
    modal->selected_choice = choice_index;
    modal->phase = EVENT_PHASE_FADE_OUT;

    // Start fade-out tween for choices
    TweenFloat(&g_tween_manager, &modal->choices_alpha, 0.0f, 0.3f, TWEEN_EASE_OUT_QUAD);

    d_LogInfoF("Event choice confirmed: %d, transitioning to result", choice_index);
}

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

    // Phase-specific logic
    switch (modal->phase) {
        case EVENT_PHASE_CHOOSING: {
            // Get choice count
            int choice_count = (int)modal->current_event->choices->count;
            if (choice_count == 0) {
                d_LogWarning("Event has no choices!");
                return false;
            }

            // Calculate modal bounds
            int modal_x = ((GetWindowWidth() - GetEventModalWidth()) / 2);
            int modal_y = (GetWindowHeight() - EVENT_MODAL_HEIGHT) / 2;
            int content_row_height = 282;
            int choice_start_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + content_row_height + 20;

            // Mouse position
            int mx = app.mouse.x;
            int my = app.mouse.y;

            // Check hover state and play hover sound on change
            modal->hovered_choice = -1;
            for (int i = 0; i < choice_count; i++) {
                EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(modal->current_event->choices, i);
                if (!choice) continue;

                int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
                int choice_x = modal_x + EVENT_MODAL_PADDING;
                int choice_w = GetEventModalWidth() - (EVENT_MODAL_PADDING * 2);

                if (mx >= choice_x && mx <= choice_x + choice_w &&
                    my >= choice_y && my <= choice_y + EVENT_CHOICE_HEIGHT) {
                    // Only set hovered if unlocked (don't hover over locked choices)
                    if (IsChoiceRequirementMet(&choice->requirement, player)) {
                        modal->hovered_choice = i;
                    }
                    break;
                }
            }

            // Play hover sound if hovering a new choice
            if (modal->hovered_choice != -1 && modal->hovered_choice != last_hovered_choice) {
                PlayUIHoverSound();
            }
            last_hovered_choice = modal->hovered_choice;

            // Handle keyboard hotkeys (1, 2, 3)
            for (int i = 0; i < choice_count && i < 3; i++) {
                SDL_Scancode key = (i == 0) ? SDL_SCANCODE_1 : (i == 1) ? SDL_SCANCODE_2 : SDL_SCANCODE_3;
                if (app.keyboard[key]) {
                    app.keyboard[key] = 0;
                    EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(modal->current_event->choices, i);
                    if (choice && IsChoiceRequirementMet(&choice->requirement, player)) {
                        PlayUIClickSound();
                        ConfirmChoice(modal, i);
                        return false;  // Don't close yet - show result first
                    } else {
                        d_LogInfoF("Choice %d is locked", i + 1);
                    }
                }
            }

            // Check for click
            if (app.mouse.pressed && modal->hovered_choice != -1) {
                EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(modal->current_event->choices, modal->hovered_choice);
                if (choice && IsChoiceRequirementMet(&choice->requirement, player)) {
                    PlayUIClickSound();
                    ConfirmChoice(modal, modal->hovered_choice);
                    return false;  // Don't close yet - show result first
                } else {
                    d_LogInfo("Clicked choice is locked");
                }
            }
            break;
        }

        case EVENT_PHASE_FADE_OUT:
            // Wait for fade-out to complete
            if (modal->choices_alpha <= 0.01f) {
                modal->choices_alpha = 0.0f;
                modal->phase = EVENT_PHASE_RESULT;
                // Start fade-in for result
                TweenFloat(&g_tween_manager, &modal->result_alpha, 1.0f, 0.4f, TWEEN_EASE_OUT_QUAD);
                d_LogInfo("Transitioned to result phase");
            }
            break;

        case EVENT_PHASE_RESULT:
            // Wait for result fade-in, then check for Continue input
            if (modal->result_alpha >= 0.99f) {
                // Check for ENTER or click to continue
                if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
                    app.keyboard[SDL_SCANCODE_RETURN] = 0;
                    app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;
                    PlayUIClickSound();
                    modal->phase = EVENT_PHASE_COMPLETE;
                    d_LogInfo("Result confirmed, completing event");
                    return true;  // NOW signal to close
                }

                // Check for mouse click anywhere to continue
                if (app.mouse.pressed) {
                    PlayUIClickSound();
                    modal->phase = EVENT_PHASE_COMPLETE;
                    d_LogInfo("Result confirmed via click, completing event");
                    return true;
                }
            }
            break;

        case EVENT_PHASE_COMPLETE:
            return true;  // Ready to close
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

/**
 * SplitTextIntoBlocks - Split text by double newlines into separate blocks
 *
 * @param source - Source text with \n\n as block separators
 * @param blocks - Output array of block strings
 * @param max_blocks - Maximum number of blocks
 * @param block_size - Size of each block buffer
 * @return Number of blocks extracted
 */
static int SplitTextIntoBlocks(const char* source, char blocks[][512], int max_blocks, size_t block_size) {
    if (!source || !blocks || max_blocks <= 0) return 0;

    int count = 0;
    const char* start = source;
    const char* end;

    while (*start && count < max_blocks) {
        // Skip leading whitespace/newlines
        while (*start == '\n' || *start == ' ') start++;
        if (!*start) break;

        // Find double newline or end of string
        end = strstr(start, "\n\n");
        if (!end) {
            end = start + strlen(start);
        }

        // Copy block (strip single newlines within block)
        size_t len = end - start;
        if (len >= block_size) len = block_size - 1;

        size_t dst_idx = 0;
        for (size_t i = 0; i < len && dst_idx < block_size - 1; i++) {
            if (start[i] == '\n') {
                // Replace single newlines with space
                if (dst_idx > 0 && blocks[count][dst_idx - 1] != ' ') {
                    blocks[count][dst_idx++] = ' ';
                }
            } else {
                blocks[count][dst_idx++] = start[i];
            }
        }
        blocks[count][dst_idx] = '\0';

        count++;
        start = end;
        if (*start) start += 2;  // Skip past \n\n
    }

    return count;
}

/**
 * RenderEventResult - Render Phase 2: result text + reward summary
 * Uses block-based layout like introNarrativeModal
 */
static void RenderEventResult(const EventModal_t* modal, int modal_x, int modal_y, Uint8 fade_alpha) {
    if (!modal || modal->confirmed_choice < 0) return;

    const EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(
        modal->current_event->choices, modal->confirmed_choice);
    if (!choice) return;

    Uint8 result_alpha = (Uint8)(modal->result_alpha * fade_alpha);

    // Content area for result
    int content_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + EVENT_MODAL_PADDING;
    int content_w = GetEventModalWidth() - (EVENT_MODAL_PADDING * 2);
    int text_x = modal_x + EVENT_MODAL_PADDING;
    const int BLOCK_SPACING = 30;  // Space between narrative blocks

    // Draw result narrative text as separate blocks (split by \n\n)
    if (choice->result_text && d_StringGetLength(choice->result_text) > 0) {
        char blocks[4][512];  // Up to 4 blocks
        int block_count = SplitTextIntoBlocks(d_StringPeek(choice->result_text), blocks, 4, 512);

        for (int i = 0; i < block_count; i++) {
            aTextStyle_t block_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {COLOR_EVENT_INFO_TEXT.r, COLOR_EVENT_INFO_TEXT.g, COLOR_EVENT_INFO_TEXT.b, result_alpha},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = content_w,
                .scale = 1.0f
            };
            a_DrawText(blocks[i], text_x, content_y, block_config);

            int block_h = a_GetWrappedTextHeight(blocks[i], FONT_ENTER_COMMAND, content_w);
            content_y += block_h + BLOCK_SPACING;
        }
    }

    // Draw divider line
    a_DrawFilledRect((aRectf_t){text_x, content_y, content_w, 2},
                     (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, result_alpha});
    content_y += 20;

    // Draw "Outcome:" header
    aTextStyle_t header_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_HEADER_TEXT.r, COLOR_HEADER_TEXT.g, COLOR_HEADER_TEXT.b, result_alpha},
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = 0,
        .scale = 1.2f
    };
    a_DrawText("Outcome:", text_x, content_y, header_config);
    content_y += 35;

    // Generate and render color-coded consequence lines
    TooltipLine_t lines[MAX_TOOLTIP_LINES];
    int line_count = GenerateChoiceTooltipLines(choice, lines, MAX_TOOLTIP_LINES);
    int text_w = content_w;
    int line_spacing = 8;

    for (int i = 0; i < line_count; i++) {
        aColor_t line_color;
        switch (lines[i].sentiment) {
            case TOOLTIP_LINE_POSITIVE:
                line_color = COLOR_TOOLTIP_POSITIVE;
                break;
            case TOOLTIP_LINE_NEGATIVE:
                line_color = COLOR_TOOLTIP_NEGATIVE;
                break;
            default:
                line_color = COLOR_TOOLTIP_NEUTRAL;
                break;
        }

        aTextStyle_t line_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {line_color.r, line_color.g, line_color.b, result_alpha},
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = text_w,
            .scale = 1.0f
        };
        a_DrawText(lines[i].text, text_x, content_y, line_config);

        int line_h = a_GetWrappedTextHeight(lines[i].text, FONT_ENTER_COMMAND, text_w);
        content_y += line_h + line_spacing;
    }

    // Draw "Click to continue" prompt at bottom
    int prompt_y = modal_y + EVENT_MODAL_HEIGHT - 60;
    aTextStyle_t prompt_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_EVENT_INFO_TEXT.r, COLOR_EVENT_INFO_TEXT.g, COLOR_EVENT_INFO_TEXT.b,
                  (Uint8)(result_alpha * 0.7f)},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 0.9f
    };
    a_DrawText("[ Click or press ENTER to continue ]",
                     modal_x + GetEventModalWidth() / 2, prompt_y, prompt_config);
}

void RenderEventModal(const EventModal_t* modal, const Player_t* player) {
    if (!modal || !modal->is_visible || !modal->current_event) return;

    // Apply fade-in alpha
    Uint8 fade_alpha = (Uint8)(modal->fade_in_alpha * 255);

    // Runtime window dimensions for responsiveness
    int window_w = GetWindowWidth();
    int window_h = GetWindowHeight();

    // Full-screen overlay
    a_DrawFilledRect((aRectf_t){0, 0, window_w, window_h},
                     (aColor_t){COLOR_OVERLAY.r, COLOR_OVERLAY.g, COLOR_OVERLAY.b,
                     (Uint8)(COLOR_OVERLAY.a * modal->fade_in_alpha)});

    // Modal panel (shifted right from center, matching RewardModal)
    int modal_x = ((window_w - GetEventModalWidth()) / 2);
    int modal_y = (window_h - EVENT_MODAL_HEIGHT) / 2;

    // Draw panel body
    int panel_body_y = modal_y + EVENT_MODAL_HEADER_HEIGHT;
    int panel_body_height = EVENT_MODAL_HEIGHT - EVENT_MODAL_HEADER_HEIGHT;
    a_DrawFilledRect((aRectf_t){modal_x, panel_body_y, GetEventModalWidth(), panel_body_height},
                     (aColor_t){COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, fade_alpha});

    // Draw header
    a_DrawFilledRect((aRectf_t){modal_x, modal_y, GetEventModalWidth(), EVENT_MODAL_HEADER_HEIGHT},
                     (aColor_t){COLOR_HEADER_BG.r, COLOR_HEADER_BG.g, COLOR_HEADER_BG.b, fade_alpha});
    a_DrawRect((aRectf_t){modal_x, modal_y, GetEventModalWidth(), EVENT_MODAL_HEADER_HEIGHT},
               (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, fade_alpha});

    // Draw title in header (centered on modal panel)
    int title_center_x = modal_x + (GetEventModalWidth() / 2);
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_HEADER_TEXT.r, COLOR_HEADER_TEXT.g, COLOR_HEADER_TEXT.b, fade_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.3f
    };
    a_DrawText((char*)d_StringPeek(modal->current_event->title),
                     title_center_x, modal_y, title_config);

    // Content area height (needed for choice positioning)
    int content_area_height = 282;

    // Phase 1: Draw event description + image (only during CHOOSING/FADE_OUT)
    if (modal->phase == EVENT_PHASE_CHOOSING || modal->phase == EVENT_PHASE_FADE_OUT) {
        Uint8 content_alpha = (Uint8)(modal->choices_alpha * fade_alpha);

        int content_row_y = modal_y + EVENT_MODAL_HEADER_HEIGHT;
        int content_area_width = GetEventModalWidth() - (EVENT_MODAL_PADDING * 2);

        // Create temporary FlexBox for two-column layout (image left, text right)
        FlexBox_t* content_box = a_FlexBoxCreate(
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
                a_DrawFilledRect((aRectf_t){image_item->calc_x, image_item->calc_y,
                               image_item->w, image_item->h},
                               (aColor_t){0, 0, 0, content_alpha});
            }

            // Draw description text using FlexBox position
            const FlexItem_t* text_item = a_FlexGetItem(content_box, 1);
            if (text_item) {
                // Preprocess description text (strip newlines for proper wrapping)
                char processed_desc[2048];
                StripNewlinesFromText(d_StringPeek(modal->current_event->description),
                                    processed_desc, sizeof(processed_desc));

                aTextStyle_t desc_config = {
                    .type = FONT_ENTER_COMMAND,
                    .fg = {COLOR_EVENT_INFO_TEXT.r, COLOR_EVENT_INFO_TEXT.g, COLOR_EVENT_INFO_TEXT.b, content_alpha},
                    .align = TEXT_ALIGN_LEFT,
                    .wrap_width = text_item->w,
                    .scale = 0.85f
                };
                a_DrawText(processed_desc, text_item->calc_x, text_item->calc_y, desc_config);
            }

            // Cleanup temporary FlexBox
            a_FlexBoxDestroy(&content_box);
        }
    } else if (modal->phase == EVENT_PHASE_RESULT || modal->phase == EVENT_PHASE_COMPLETE) {
        // Phase 2: Darken the panel body further for result screen
        int panel_body_y_inner = modal_y + EVENT_MODAL_HEADER_HEIGHT;
        int panel_body_height_inner = EVENT_MODAL_HEIGHT - EVENT_MODAL_HEADER_HEIGHT;
        a_DrawFilledRect((aRectf_t){modal_x, panel_body_y_inner, GetEventModalWidth(), panel_body_height_inner},
                         (aColor_t){0, 0, 0, (Uint8)(150 * modal->result_alpha)});  // Extra dark overlay
    }

    // Phase-specific rendering
    if (modal->phase == EVENT_PHASE_CHOOSING || modal->phase == EVENT_PHASE_FADE_OUT) {
        // PHASE 1: Draw choice buttons (with fade-out alpha during transition)
        Uint8 choices_alpha_scaled = (Uint8)(modal->choices_alpha * fade_alpha);

        int choice_count = (int)modal->current_event->choices->count;
        int choice_start_y = modal_y + EVENT_MODAL_HEADER_HEIGHT + content_area_height + 20;

        for (int i = 0; i < choice_count; i++) {
            EventChoice_t* choice = (EventChoice_t*)d_ArrayGet(modal->current_event->choices, i);
            if (!choice || !choice->text) continue;

            int choice_y = choice_start_y + i * (EVENT_CHOICE_HEIGHT + EVENT_CHOICE_SPACING);
            int choice_x = modal_x + EVENT_MODAL_PADDING;
            int choice_w = GetEventModalWidth() - (EVENT_MODAL_PADDING * 2);

            // Check if choice is locked (requirement not met)
            bool is_locked = !IsChoiceRequirementMet(&choice->requirement, player);

            // Background (gray if locked, highlight if hovered, normal otherwise)
            aColor_t bg_color = COLOR_CHOICE_BG;
            if (is_locked) {
                bg_color = COLOR_CHOICE_LOCKED;
            } else if (modal->hovered_choice == i) {
                bg_color = COLOR_CHOICE_HOVER;
            }
            a_DrawFilledRect((aRectf_t){choice_x, choice_y, choice_w, EVENT_CHOICE_HEIGHT},
                             (aColor_t){bg_color.r, bg_color.g, bg_color.b, choices_alpha_scaled});
            a_DrawRect((aRectf_t){choice_x, choice_y, choice_w, EVENT_CHOICE_HEIGHT},
                       (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, choices_alpha_scaled});

            // Big number hotkey OR lock icon (left side, matching RewardModal style)
            int number_x = choice_x + 25;
            int number_y = choice_y + (EVENT_CHOICE_HEIGHT / 2) - 20;

            if (is_locked) {
                // Draw lock icon (🔒 unicode character)
                aTextStyle_t lock_font = {
                    .type = FONT_GAME,
                    .align = TEXT_ALIGN_LEFT,
                    .scale = 2.5f,
                    .fg = {COLOR_LOCKED_TEXT.r, COLOR_LOCKED_TEXT.g, COLOR_LOCKED_TEXT.b, choices_alpha_scaled}
                };
                a_DrawText("🔒", number_x, number_y + 5, lock_font);
            } else {
                // Draw number hotkey
                dString_t* number_str = d_StringInit();
                d_StringFormat(number_str, "%d", i + 1);

                aTextStyle_t number_font = {
                    .type = FONT_GAME,
                    .align = TEXT_ALIGN_LEFT,
                    .scale = 3.0f,  // Big like RewardModal
                    .fg = {255, 255, 255, (Uint8)(choices_alpha_scaled * 0.6f)}  // White 60% opacity
                };
                a_DrawText(d_StringPeek(number_str), number_x, number_y, number_font);
                d_StringDestroy(number_str);
            }

            // Choice text (left-aligned, shifted right to make room for number)
            int text_padding = 75;  // Shifted right for number space
            aColor_t text_color = is_locked ? COLOR_LOCKED_TEXT : (aColor_t){255, 255, 255, 255};
            aTextStyle_t choice_text_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {text_color.r, text_color.g, text_color.b, choices_alpha_scaled},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = choice_w - text_padding - 20,
                .scale = 1.0f
            };
            a_DrawText((char*)d_StringPeek(choice->text),
                             choice_x + text_padding, choice_y + 10, choice_text_config);
        }

        // Render tooltip AFTER all choices (so it's on top / higher z-order)
        // Only during CHOOSING phase (not during fade-out)
        if (modal->phase == EVENT_PHASE_CHOOSING &&
            modal->hovered_choice >= 0 && modal->hovered_choice < (int)modal->current_event->choices->count) {
            const EventChoice_t* hovered = (EventChoice_t*)d_ArrayGet(modal->current_event->choices, modal->hovered_choice);
            bool hovered_locked = !IsChoiceRequirementMet(&hovered->requirement, player);

            // Tooltip sizing constants
            int tooltip_padding = 12;
            int text_w = 320;
            int line_spacing = 6;

            if (hovered_locked) {
                // Locked: show requirement (single gray text)
                dString_t* tooltip = d_StringInit();
                GetRequirementTooltip(&hovered->requirement, tooltip);

                if (d_StringGetLength(tooltip) > 0) {
                    char processed[512];
                    StripNewlinesFromText(d_StringPeek(tooltip), processed, sizeof(processed));

                    int text_h = a_GetWrappedTextHeight(processed, FONT_ENTER_COMMAND, text_w);
                    int tooltip_w = text_w + (tooltip_padding * 2);
                    int tooltip_h = text_h + (tooltip_padding * 2);
                    if (tooltip_h < 40) tooltip_h = 40;

                    int mx = app.mouse.x;
                    int my = app.mouse.y;
                    int tooltip_x = mx + 15;
                    int tooltip_y = my + 15;

                    if (tooltip_x + tooltip_w > GetWindowWidth()) tooltip_x = mx - tooltip_w - 15;
                    if (tooltip_y + tooltip_h > GetWindowHeight()) tooltip_y = my - tooltip_h - 15;

                    a_DrawFilledRect((aRectf_t){tooltip_x, tooltip_y, tooltip_w, tooltip_h},
                                    (aColor_t){COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, 250});
                    a_DrawRect((aRectf_t){tooltip_x, tooltip_y, tooltip_w, tooltip_h},
                               (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, 255});

                    aTextStyle_t config = {
                        .type = FONT_ENTER_COMMAND,
                        .fg = {COLOR_LOCKED_TEXT.r, COLOR_LOCKED_TEXT.g, COLOR_LOCKED_TEXT.b, 255},
                        .align = TEXT_ALIGN_LEFT,
                        .wrap_width = text_w,
                        .scale = 1.0f
                    };
                    a_DrawText(processed, tooltip_x + tooltip_padding, tooltip_y + tooltip_padding, config);
                }
                d_StringDestroy(tooltip);
            } else {
                // Unlocked: show color-coded consequence lines
                TooltipLine_t lines[MAX_TOOLTIP_LINES];
                int line_count = GenerateChoiceTooltipLines(hovered, lines, MAX_TOOLTIP_LINES);

                if (line_count > 0) {
                    // Calculate total height (each line height + spacing)
                    int total_h = 0;
                    int line_heights[MAX_TOOLTIP_LINES];
                    for (int i = 0; i < line_count; i++) {
                        line_heights[i] = a_GetWrappedTextHeight(lines[i].text, FONT_ENTER_COMMAND, text_w);
                        total_h += line_heights[i];
                        if (i < line_count - 1) total_h += line_spacing;
                    }

                    int tooltip_w = text_w + (tooltip_padding * 2);
                    int tooltip_h = total_h + (tooltip_padding * 2);
                    if (tooltip_h < 40) tooltip_h = 40;

                    int mx = app.mouse.x;
                    int my = app.mouse.y;
                    int tooltip_x = mx + 15;
                    int tooltip_y = my + 15;

                    if (tooltip_x + tooltip_w > GetWindowWidth()) tooltip_x = mx - tooltip_w - 15;
                    if (tooltip_y + tooltip_h > GetWindowHeight()) tooltip_y = my - tooltip_h - 15;

                    // Draw background
                    a_DrawFilledRect((aRectf_t){tooltip_x, tooltip_y, tooltip_w, tooltip_h},
                                    (aColor_t){COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, 250});
                    a_DrawRect((aRectf_t){tooltip_x, tooltip_y, tooltip_w, tooltip_h},
                               (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, 255});

                    // Draw each line with its sentiment color
                    int current_y = tooltip_y + tooltip_padding;
                    for (int i = 0; i < line_count; i++) {
                        aColor_t line_color;
                        switch (lines[i].sentiment) {
                            case TOOLTIP_LINE_POSITIVE:
                                line_color = COLOR_TOOLTIP_POSITIVE;
                                break;
                            case TOOLTIP_LINE_NEGATIVE:
                                line_color = COLOR_TOOLTIP_NEGATIVE;
                                break;
                            default:
                                line_color = COLOR_TOOLTIP_NEUTRAL;
                                break;
                        }

                        aTextStyle_t config = {
                            .type = FONT_ENTER_COMMAND,
                            .fg = {line_color.r, line_color.g, line_color.b, 255},
                            .align = TEXT_ALIGN_LEFT,
                            .wrap_width = text_w,
                            .scale = 1.0f
                        };
                        a_DrawText(lines[i].text, tooltip_x + tooltip_padding, current_y, config);

                        current_y += line_heights[i] + line_spacing;
                    }
                }
            }
        }
    } else if (modal->phase == EVENT_PHASE_RESULT || modal->phase == EVENT_PHASE_COMPLETE) {
        // PHASE 2: Draw result text + reward summary
        RenderEventResult(modal, modal_x, modal_y, fade_alpha);
    }
}

// ============================================================================
// QUERIES
// ============================================================================

int GetSelectedChoiceIndex(const EventModal_t* modal) {
    if (!modal) return -1;
    return modal->selected_choice;
}
