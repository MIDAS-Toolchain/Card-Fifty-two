/*
 * Intro Narrative Modal Component
 * Tutorial story intro displayed before first combat
 */

#include "../../../include/scenes/components/introNarrativeModal.h"
#include "../../../include/tween/tween.h"
#include "../../../include/defs.h"
#include <stdio.h>
#include <string.h>

// Color palette (matching EventModal for consistency)
static const SDL_Color COLOR_OVERLAY = {9, 10, 20, 180};         // #090a14 - almost black overlay
static const SDL_Color COLOR_PANEL_BG = {9, 10, 20, 240};        // #090a14 - almost black
static const SDL_Color COLOR_HEADER_BG = {37, 58, 94, 255};      // #253a5e - dark navy blue
static const SDL_Color COLOR_HEADER_BORDER = {60, 94, 139, 255}; // #3c5e8b - medium blue
static const SDL_Color COLOR_HEADER_TEXT = {231, 213, 179, 255}; // #e7d5b3 - cream
static const SDL_Color COLOR_NARRATIVE_TEXT = {168, 181, 178, 255}; // #a8b5b2 - light gray

// Layout constants (almost full screen, slight inset)
#define MODAL_WIDTH          (SCREEN_WIDTH - 32)  // Screen width minus 32
#define MODAL_HEIGHT         700
#define HEADER_HEIGHT        50
#define PADDING              30
#define MODAL_OFFSET_X       8    // Offset 8px from left

// ============================================================================
// LIFECYCLE
// ============================================================================

IntroNarrativeModal_t* CreateIntroNarrativeModal(void) {
    IntroNarrativeModal_t* modal = malloc(sizeof(IntroNarrativeModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate IntroNarrativeModal");
        return NULL;
    }

    // Initialize state
    modal->is_visible = false;
    modal->fade_in_alpha = 0.0f;
    modal->portrait = NULL;
    modal->line_count = 0;
    modal->current_block = 0;
    modal->total_blocks = 0;

    // Initialize text buffers
    modal->title[0] = '\0';
    modal->portrait_path[0] = '\0';

    // Initialize narrative lines
    for (int i = 0; i < MAX_NARRATIVE_LINES; i++) {
        modal->narrative_lines[i][0] = '\0';
        modal->line_fade_alpha[i] = 0.0f;
    }

    // Create FlexBox layout for content area (portrait + text)
    // Content area position: below header
    int modal_x = (SCREEN_WIDTH - MODAL_WIDTH) / 2 + MODAL_OFFSET_X;
    int modal_y = (SCREEN_HEIGHT - MODAL_HEIGHT) / 2;
    int content_y = modal_y + HEADER_HEIGHT;
    int content_height = MODAL_HEIGHT - HEADER_HEIGHT;

    modal->layout = a_FlexBoxCreate(
        modal_x + PADDING,  // x position (inside modal padding)
        content_y + PADDING,  // y position (below header, inside padding)
        MODAL_WIDTH - (PADDING * 2),  // width (modal width minus left+right padding)
        content_height - (PADDING * 2)  // height (content area minus top+bottom padding)
    );

    if (!modal->layout) {
        d_LogError("Failed to create FlexBox layout for IntroNarrativeModal");
        free(modal);
        return NULL;
    }

    // Configure FlexBox: horizontal layout with gap between items
    a_FlexConfigure(modal->layout, FLEX_DIR_ROW, FLEX_JUSTIFY_START, 20);  // 20px gap

    // Add portrait item (fixed size 256x512)
    a_FlexAddItem(modal->layout, 256, 512, NULL);

    // Add text area item (fills remaining space)
    int remaining_width = (MODAL_WIDTH - (PADDING * 2)) - 256 - 20;  // Total width - portrait - gap
    a_FlexAddItem(modal->layout, remaining_width, content_height - (PADDING * 2), NULL);

    // Calculate layout positions
    a_FlexLayout(modal->layout);

    // Create Continue button (centered at bottom of modal)
    int button_w = 200;
    int button_h = 50;
    int button_x = (SCREEN_WIDTH - button_w) / 2;
    int button_y = (SCREEN_HEIGHT + MODAL_HEIGHT) / 2 - button_h - PADDING;

    modal->continue_button = CreateButton(button_x, button_y, button_w, button_h, "Continue");
    if (!modal->continue_button) {
        d_LogError("Failed to create continue button for IntroNarrativeModal");
        a_FlexBoxDestroy(&modal->layout);
        free(modal);
        return NULL;
    }

    d_LogInfo("IntroNarrativeModal created with FlexBox layout");
    return modal;
}

void DestroyIntroNarrativeModal(IntroNarrativeModal_t** modal) {
    if (!modal || !*modal) return;

    IntroNarrativeModal_t* m = *modal;

    // Destroy FlexBox layout
    if (m->layout) {
        a_FlexBoxDestroy(&m->layout);
    }

    // Destroy button
    if (m->continue_button) {
        DestroyButton(&m->continue_button);
    }

    // Destroy portrait texture
    if (m->portrait) {
        SDL_DestroyTexture(m->portrait);
        m->portrait = NULL;
    }

    free(m);
    *modal = NULL;
    d_LogInfo("IntroNarrativeModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowIntroNarrativeModal(IntroNarrativeModal_t* modal,
                             const char* title,
                             const char** narrative_blocks,
                             int block_count,
                             const char* portrait_path) {
    if (!modal) return;

    // Copy title
    if (title) {
        strncpy(modal->title, title, sizeof(modal->title) - 1);
        modal->title[sizeof(modal->title) - 1] = '\0';
    } else {
        modal->title[0] = '\0';
    }

    // Store all narrative blocks (strip newlines from each)
    modal->line_count = 0;
    modal->total_blocks = block_count;
    for (int block_idx = 0; block_idx < block_count && block_idx < MAX_NARRATIVE_LINES; block_idx++) {
        const char* narrative = narrative_blocks[block_idx];
        if (!narrative) continue;

        // Copy and replace newlines with spaces for proper text wrapping
        int src_idx = 0;
        int dst_idx = 0;
        while (narrative[src_idx] != '\0' && dst_idx < MAX_LINE_TEXT_LENGTH - 1) {
            if (narrative[src_idx] == '\n') {
                // Replace newlines with spaces (collapse multiple newlines into single space)
                if (dst_idx > 0 && modal->narrative_lines[block_idx][dst_idx - 1] != ' ') {
                    modal->narrative_lines[block_idx][dst_idx++] = ' ';
                }
                src_idx++;
            } else {
                modal->narrative_lines[block_idx][dst_idx++] = narrative[src_idx++];
            }
        }
        modal->narrative_lines[block_idx][dst_idx] = '\0';
        modal->line_fade_alpha[block_idx] = 0.0f;
        modal->line_count++;

        d_LogInfoF("IntroNarrativeModal: Block %d processed (%d chars)", block_idx, dst_idx);
    }

    // Load portrait if path provided
    if (portrait_path && portrait_path[0] != '\0') {
        strncpy(modal->portrait_path, portrait_path, sizeof(modal->portrait_path) - 1);
        modal->portrait_path[sizeof(modal->portrait_path) - 1] = '\0';

        // Destroy old portrait if exists
        if (modal->portrait) {
            SDL_DestroyTexture(modal->portrait);
            modal->portrait = NULL;
        }

        // Load new portrait
        SDL_Surface* surface = IMG_Load(portrait_path);
        if (surface) {
            modal->portrait = SDL_CreateTextureFromSurface(app.renderer, surface);
            SDL_FreeSurface(surface);
            if (!modal->portrait) {
                d_LogErrorF("Failed to create texture from portrait: %s", portrait_path);
            }
        } else {
            d_LogErrorF("Failed to load portrait image: %s", portrait_path);
        }
    } else {
        modal->portrait_path[0] = '\0';
        if (modal->portrait) {
            SDL_DestroyTexture(modal->portrait);
            modal->portrait = NULL;
        }
    }

    modal->is_visible = true;
    modal->fade_in_alpha = 1.0f;  // No fade-in, show immediately
    modal->current_block = 0;

    // Start fade-in tweens for all text blocks
    extern TweenManager_t g_tween_manager;
    for (int i = 0; i < modal->line_count; i++) {
        modal->line_fade_alpha[i] = 0.0f;  // Start invisible
        TweenFloat(&g_tween_manager, &modal->line_fade_alpha[i], 1.0f, 2.0f, TWEEN_EASE_OUT_CUBIC);
    }

    d_LogInfoF("IntroNarrativeModal shown: '%s' with %d blocks", modal->title, modal->line_count);
}

void HideIntroNarrativeModal(IntroNarrativeModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
    d_LogInfo("IntroNarrativeModal hidden");
}

bool IsIntroNarrativeModalVisible(const IntroNarrativeModal_t* modal) {
    return modal && modal->is_visible;
}

// ============================================================================
// INPUT & UPDATE
// ============================================================================

bool HandleIntroNarrativeModalInput(IntroNarrativeModal_t* modal, float dt) {
    (void)dt;  // Unused - no fade-in animation
    if (!modal || !modal->is_visible) return false;

    // No fade-in animation - alpha stays at 1.0 (immediate display)

    // Check for ENTER key press
    if (app.keyboard[SDL_SCANCODE_RETURN] || app.keyboard[SDL_SCANCODE_KP_ENTER]) {
        app.keyboard[SDL_SCANCODE_RETURN] = 0;
        app.keyboard[SDL_SCANCODE_KP_ENTER] = 0;
        d_LogInfo("IntroNarrativeModal: ENTER pressed");
        return true;  // Signal ready to close
    }

    // Check for Continue button click
    if (IsButtonClicked(modal->continue_button)) {
        d_LogInfo("IntroNarrativeModal: Continue clicked");
        return true;  // Signal ready to close
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderIntroNarrativeModal(const IntroNarrativeModal_t* modal) {
    if (!modal || !modal->is_visible) return;

    Uint8 fade_alpha = (Uint8)(modal->fade_in_alpha * 255);

    // Draw full-screen dark overlay
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
                     (aColor_t){COLOR_OVERLAY.r, COLOR_OVERLAY.g, COLOR_OVERLAY.b,
                     (Uint8)(COLOR_OVERLAY.a * modal->fade_in_alpha / 255.0f)});

    // Calculate modal position (centered, then shifted right)
    int modal_x = (SCREEN_WIDTH - MODAL_WIDTH) / 2 + MODAL_OFFSET_X;
    int modal_y = (SCREEN_HEIGHT - MODAL_HEIGHT) / 2;

    // Draw main panel background
    a_DrawFilledRect((aRectf_t){modal_x, modal_y, MODAL_WIDTH, MODAL_HEIGHT},
                     (aColor_t){COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, fade_alpha});

    // Draw header background
    a_DrawFilledRect((aRectf_t){modal_x, modal_y, MODAL_WIDTH, HEADER_HEIGHT},
                     (aColor_t){COLOR_HEADER_BG.r, COLOR_HEADER_BG.g, COLOR_HEADER_BG.b, fade_alpha});

    // Draw header border
    a_DrawRect((aRectf_t){modal_x, modal_y, MODAL_WIDTH, HEADER_HEIGHT},
               (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, fade_alpha});

    // Draw title (centered in header, moved up slightly)
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {COLOR_HEADER_TEXT.r, COLOR_HEADER_TEXT.g, COLOR_HEADER_TEXT.b, fade_alpha},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.2f
    };
    a_DrawText(modal->title,
                     modal_x + MODAL_WIDTH / 2,
                     modal_y,  // Fixed position from top of header
                     title_config);

    // Get FlexBox calculated positions
    const FlexItem_t* portrait_item = a_FlexGetItem(modal->layout, 0);  // Portrait (left)
    const FlexItem_t* text_item = a_FlexGetItem(modal->layout, 1);      // Text area (right)

    // Draw portrait using FlexBox position
    if (portrait_item) {
        if (modal->portrait) {
            // Draw portrait texture using FlexBox calculated position
            SDL_Rect dst = {portrait_item->calc_x, portrait_item->calc_y, portrait_item->w, portrait_item->h};
            SDL_SetTextureAlphaMod(modal->portrait, fade_alpha);
            SDL_RenderCopy(app.renderer, modal->portrait, NULL, &dst);
        } else {
            // Black placeholder using FlexBox calculated position
            a_DrawFilledRect((aRectf_t){portrait_item->calc_x, portrait_item->calc_y, portrait_item->w, portrait_item->h},
                             (aColor_t){0, 0, 0, fade_alpha});
        }
    }

    // Draw all narrative blocks vertically with spacing
    if (text_item && modal->line_count > 0) {
        int text_w = text_item->w;  // Use full FlexBox calculated width
        int current_y = text_item->calc_y;
        const int BLOCK_SPACING = 40;  // Space between blocks

        for (int i = 0; i < modal->line_count; i++) {
            Uint8 block_alpha = (Uint8)(modal->line_fade_alpha[i] * 255);

            aTextStyle_t text_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {COLOR_NARRATIVE_TEXT.r, COLOR_NARRATIVE_TEXT.g, COLOR_NARRATIVE_TEXT.b, block_alpha},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = text_w,
                .scale = 1.0f
            };

            // Draw this narrative block
            a_DrawText(modal->narrative_lines[i], text_item->calc_x, current_y, text_config);

            // Get actual wrapped text height from Archimedes (cast away const)
            int block_height = a_GetWrappedTextHeight((char*)modal->narrative_lines[i], FONT_ENTER_COMMAND, text_w);

            current_y += block_height + BLOCK_SPACING;
        }
    }

    // Render Continue button
    RenderButton(modal->continue_button);
}
