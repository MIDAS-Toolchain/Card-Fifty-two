/*
 * Character Stats Modal Component Implementation
 * Displays character info and sanity threshold effects
 */

#include "../../../include/scenes/components/characterStatsModal.h"

// Color constants
#define COLOR_BG            ((aColor_t){23, 32, 56, 230})    // #172038 with alpha
#define COLOR_BORDER        ((aColor_t){60, 94, 139, 255})   // #3c5e8b
#define COLOR_TITLE         ((aColor_t){235, 237, 233, 255}) // #ebede9 - off-white
#define COLOR_TEXT          ((aColor_t){200, 200, 200, 255}) // Light gray
#define COLOR_THRESHOLD     ((aColor_t){164, 221, 219, 255}) // #a4dddb - cyan
#define COLOR_WARNING       ((aColor_t){222, 158, 65, 255})  // #de9e41 - orange

// ============================================================================
// LIFECYCLE
// ============================================================================

CharacterStatsModal_t* CreateCharacterStatsModal(Player_t* player) {
    if (!player) {
        d_LogError("CreateCharacterStatsModal: NULL player");
        return NULL;
    }

    CharacterStatsModal_t* modal = malloc(sizeof(CharacterStatsModal_t));
    if (!modal) {
        d_LogFatal("Failed to allocate CharacterStatsModal");
        return NULL;
    }

    modal->is_visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->w = STATS_MODAL_WIDTH;
    modal->h = STATS_MODAL_HEIGHT;
    modal->player = player;  // Reference only, not owned

    // Create FlexBox for content layout
    modal->layout = a_CreateFlexBox(0, 0, STATS_MODAL_WIDTH, STATS_MODAL_HEIGHT);
    a_FlexConfigure(modal->layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, STATS_MODAL_GAP);
    a_FlexSetPadding(modal->layout, STATS_MODAL_PADDING);

    // Add FlexBox items for each section
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 30, NULL);  // 0: Title
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 25, NULL);  // 1: Current sanity
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 15, NULL);  // 2: Spacer
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 25, NULL);  // 3: Thresholds header
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 45, NULL);  // 4: Threshold 75+
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 45, NULL);  // 5: Threshold 50-74
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 45, NULL);  // 6: Threshold 25-49
    a_FlexAddItem(modal->layout, STATS_MODAL_WIDTH, 45, NULL);  // 7: Threshold 0-24

    d_LogInfo("CharacterStatsModal created");
    return modal;
}

void DestroyCharacterStatsModal(CharacterStatsModal_t** modal) {
    if (!modal || !*modal) return;

    // Destroy FlexBox if it exists
    if ((*modal)->layout) {
        a_DestroyFlexBox(&(*modal)->layout);
    }

    free(*modal);
    *modal = NULL;
    d_LogInfo("CharacterStatsModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowCharacterStatsModal(CharacterStatsModal_t* modal, int x, int y) {
    if (!modal) return;

    modal->is_visible = true;
    modal->x = x;
    modal->y = y;
}

void HideCharacterStatsModal(CharacterStatsModal_t* modal) {
    if (!modal) return;
    modal->is_visible = false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderCharacterStatsModal(CharacterStatsModal_t* modal) {
    if (!modal || !modal->is_visible || !modal->player || !modal->layout) return;

    // Background
    a_DrawFilledRect(modal->x, modal->y, modal->w, modal->h,
                     COLOR_BG.r, COLOR_BG.g, COLOR_BG.b, COLOR_BG.a);

    // Border
    a_DrawRect(modal->x, modal->y, modal->w, modal->h,
               COLOR_BORDER.r, COLOR_BORDER.g, COLOR_BORDER.b, COLOR_BORDER.a);

    // Update FlexBox bounds
    a_FlexSetBounds(modal->layout, modal->x, modal->y, modal->w, modal->h);
    a_FlexLayout(modal->layout);

    int center_x = modal->x + modal->w / 2;
    int left_x = modal->x + STATS_MODAL_PADDING;

    // ========================================================================
    // FlexBox Item 0: Title - Character Name
    // ========================================================================

    int title_y = a_FlexGetItemY(modal->layout, 0);
    a_DrawText((char*)d_StringPeek(modal->player->name), center_x, title_y,
               COLOR_TITLE.r, COLOR_TITLE.g, COLOR_TITLE.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // ========================================================================
    // FlexBox Item 1: Current Sanity Display
    // ========================================================================

    int sanity_y = a_FlexGetItemY(modal->layout, 1);
    dString_t* sanity_text = d_StringInit();
    d_StringFormat(sanity_text, "Sanity: %d / %d", modal->player->sanity, modal->player->max_sanity);
    a_DrawText((char*)d_StringPeek(sanity_text), center_x, sanity_y,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_StringDestroy(sanity_text);

    // ========================================================================
    // FlexBox Item 2: Spacer (no rendering)
    // ========================================================================

    // ========================================================================
    // FlexBox Item 3: Sanity Thresholds Header
    // ========================================================================

    int header_y = a_FlexGetItemY(modal->layout, 3);
    a_DrawText("Sanity Thresholds:", left_x, header_y,
               COLOR_THRESHOLD.r, COLOR_THRESHOLD.g, COLOR_THRESHOLD.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

    // ========================================================================
    // FlexBox Item 4: Threshold 75+ Sanity
    // ========================================================================

    int threshold1_y = a_FlexGetItemY(modal->layout, 4);
    a_DrawText("75+ Sanity:", left_x + 10, threshold1_y,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    a_DrawText("[PLACEHOLDER: Bonus effect]", left_x + 20, threshold1_y + 20,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

    // ========================================================================
    // FlexBox Item 5: Threshold 50-74 Sanity
    // ========================================================================

    int threshold2_y = a_FlexGetItemY(modal->layout, 5);
    a_DrawText("50-74 Sanity:", left_x + 10, threshold2_y,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    a_DrawText("[PLACEHOLDER: Penalty begins]", left_x + 20, threshold2_y + 20,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

    // ========================================================================
    // FlexBox Item 6: Threshold 25-49 Sanity
    // ========================================================================

    int threshold3_y = a_FlexGetItemY(modal->layout, 6);
    a_DrawText("25-49 Sanity:", left_x + 10, threshold3_y,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    a_DrawText("[PLACEHOLDER: Severe penalty]", left_x + 20, threshold3_y + 20,
               COLOR_WARNING.r, COLOR_WARNING.g, COLOR_WARNING.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);

    // ========================================================================
    // FlexBox Item 7: Threshold 0-24 Sanity
    // ========================================================================

    int threshold4_y = a_FlexGetItemY(modal->layout, 7);
    a_DrawText("0-24 Sanity:", left_x + 10, threshold4_y,
               COLOR_TEXT.r, COLOR_TEXT.g, COLOR_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
    a_DrawText("[PLACEHOLDER: Extreme difficulty]", left_x + 20, threshold4_y + 20,
               COLOR_WARNING.r, COLOR_WARNING.g, COLOR_WARNING.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_LEFT, 0);
}
