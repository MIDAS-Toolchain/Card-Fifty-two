/*
 * Card Tooltip Modal Implementation
 */

#include "../../../include/scenes/components/cardTooltipModal.h"
#include "../../../include/card.h"
#include "../../../include/cardTags.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include <stdlib.h>

// ============================================================================
// LIFECYCLE
// ============================================================================

CardTooltipModal_t* CreateCardTooltipModal(void) {
    CardTooltipModal_t* modal = malloc(sizeof(CardTooltipModal_t));
    if (!modal) {
        d_LogFatal("CreateCardTooltipModal: Failed to allocate modal");
        return NULL;
    }

    modal->visible = false;
    modal->x = 0;
    modal->y = 0;
    modal->card = NULL;

    d_LogInfo("CardTooltipModal created");
    return modal;
}

void DestroyCardTooltipModal(CardTooltipModal_t** modal) {
    if (!modal || !*modal) return;

    free(*modal);
    *modal = NULL;
    d_LogInfo("CardTooltipModal destroyed");
}

// ============================================================================
// SHOW/HIDE
// ============================================================================

void ShowCardTooltipModal(CardTooltipModal_t* modal,
                          const Card_t* card,
                          int card_x, int card_y) {
    ShowCardTooltipModalWithSide(modal, card, card_x, card_y, false);
}

void ShowCardTooltipModalWithSide(CardTooltipModal_t* modal,
                                   const Card_t* card,
                                   int card_x, int card_y,
                                   bool force_left) {
    if (!modal || !card) return;

    modal->visible = true;
    modal->card = card;

    // Position based on force_left flag
    if (force_left) {
        // Always show on left
        modal->x = card_x - CARD_TOOLTIP_WIDTH - 15;
    } else {
        // Default to right of card (with margin)
        modal->x = card_x + 75 + 15;  // card width + margin

        // If too close to screen edge, show on left instead
        if (modal->x + CARD_TOOLTIP_WIDTH > SCREEN_WIDTH - 10) {
            modal->x = card_x - CARD_TOOLTIP_WIDTH - 15;
        }
    }

    modal->y = card_y;

    // Clamp Y to screen bounds (use MIN_HEIGHT for estimation)
    if (modal->y + CARD_TOOLTIP_MIN_HEIGHT > SCREEN_HEIGHT - 10) {
        modal->y = SCREEN_HEIGHT - CARD_TOOLTIP_MIN_HEIGHT - 10;
    }
    if (modal->y < TOP_BAR_HEIGHT + 10) {
        modal->y = TOP_BAR_HEIGHT + 10;
    }
}

void HideCardTooltipModal(CardTooltipModal_t* modal) {
    if (!modal) return;
    modal->visible = false;
    modal->card = NULL;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderCardTooltipModal(const CardTooltipModal_t* modal) {
    if (!modal || !modal->visible || !modal->card) return;

    int x = modal->x;
    int y = modal->y;

    // Padding and spacing constants
    int padding = 16;
    int content_width = CARD_TOOLTIP_WIDTH - (padding * 2);

    // Get card name
    dString_t* card_name = d_StringInit();
    CardToString(modal->card, card_name);
    const char* name = d_StringPeek(card_name);

    int title_height = a_GetWrappedTextHeight((char*)name, FONT_ENTER_COMMAND, content_width);

    // Calculate content height dynamically
    int current_y = padding;  // Start with top padding
    current_y += title_height + 10;  // Title + margin
    current_y += 1 + 12;  // Divider + spacing

    // Get tags
    const dArray_t* tags = GetCardTags(modal->card->card_id);
    if (tags && tags->count > 0) {
        // Calculate height for all tags
        for (size_t tag_idx = 0; tag_idx < tags->count; tag_idx++) {
            CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray((dArray_t*)tags, tag_idx);
            const char* tag_desc = GetCardTagDescription(*tag);

            current_y += 30 + 8;  // Tag badge height + spacing

            // Measure wrapped description height
            int desc_height = a_GetWrappedTextHeight((char*)tag_desc, FONT_GAME, content_width);
            current_y += desc_height + 12;  // Description + gap
        }
    } else {
        // "No tags" message
        current_y += a_GetWrappedTextHeight("No tags", FONT_GAME, content_width) + 8;
    }

    current_y += padding;  // Bottom padding

    // Calculate modal height
    int modal_height = current_y;
    if (modal_height < CARD_TOOLTIP_MIN_HEIGHT) {
        modal_height = CARD_TOOLTIP_MIN_HEIGHT;
    }

    // Draw background (dark with transparency)
    a_DrawFilledRect((aRectf_t){x, y, CARD_TOOLTIP_WIDTH, modal_height},
                    (aColor_t){20, 20, 30, 230});

    // Draw border
    a_DrawRect((aRectf_t){x, y, CARD_TOOLTIP_WIDTH, modal_height},
              (aColor_t){255, 255, 255, 255});

    // Now draw content on top
    int content_x = x + padding;
    int content_y = y + padding;
    current_y = content_y;

    // Title (card name) - gold, centered
    aTextStyle_t title_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {232, 193, 112, 255},  // Gold
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = content_width,
        .scale = 1.0f
    };
    a_DrawText((char*)name, content_x + content_width / 2, current_y, title_config);
    current_y += title_height + 10;  // Actual measured height + margin

    // Divider
    a_DrawFilledRect((aRectf_t){content_x, current_y, content_width, 1},
                    (aColor_t){100, 100, 100, 200});
    current_y += 12;  // Spacing around divider

    // Tags
    if (tags && tags->count > 0) {
        for (size_t tag_idx = 0; tag_idx < tags->count; tag_idx++) {
            CardTag_t* tag = (CardTag_t*)d_IndexDataFromArray((dArray_t*)tags, tag_idx);
            const char* tag_name = GetCardTagName(*tag);
            const char* tag_desc = GetCardTagDescription(*tag);

            // Get tag color
            int r, g, b;
            GetCardTagColor(*tag, &r, &g, &b);

            // Tag name with colored background (full width)
            int tag_badge_w = content_width;
            int tag_badge_h = 30;
            a_DrawFilledRect((aRectf_t){content_x, current_y, tag_badge_w, tag_badge_h}, (aColor_t){r, g, b, 255});
            a_DrawRect((aRectf_t){content_x, current_y, tag_badge_w, tag_badge_h}, (aColor_t){0, 0, 0, 255});

            aTextStyle_t tag_name_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = {0, 0, 0, 255},  // Black text
                .align = TEXT_ALIGN_CENTER,
                .scale = 1.1f
            };
            a_DrawText(tag_name, content_x + tag_badge_w / 2, current_y - 8, tag_name_config);
            current_y += tag_badge_h + 12;

            // Tag description (word-wrapped, gray text)
            int desc_height = a_GetWrappedTextHeight((char*)tag_desc, FONT_GAME, content_width);

            aTextStyle_t desc_config = {
                .type = FONT_GAME,
                .fg = {180, 180, 180, 255},
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = content_width,
                .scale = 1.0f
            };
            a_DrawText((char*)tag_desc, content_x, current_y, desc_config);
            current_y += desc_height + 12;  // Gap between tags
        }
    } else {
        // No tags message
        aTextStyle_t no_tags_config = {
            .type = FONT_GAME,
            .fg = {150, 150, 150, 255},
            .align = TEXT_ALIGN_CENTER,
            .scale = 0.9f
        };
        a_DrawText("No tags", content_x + content_width / 2, current_y, no_tags_config);
    }

    d_StringDestroy(card_name);
}
