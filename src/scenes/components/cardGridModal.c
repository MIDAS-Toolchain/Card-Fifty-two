/*
 * Card Grid Modal Component Implementation
 */

#include "../../../include/scenes/components/cardGridModal.h"
#include "../../../include/card.h"
#include "../../../include/cardTags.h"
#include "../../../include/random.h"
#include <stdlib.h>
#include <string.h>

// External globals
extern dTable_t* g_card_textures;
extern SDL_Surface* g_card_back_texture;

// Color constants (matching pause menu palette)
#define COLOR_OVERLAY           ((aColor_t){9, 10, 20, 180})     // #090a14 - almost black overlay
#define COLOR_PANEL_BG          ((aColor_t){9, 10, 20, 240})     // #090a14 - almost black
#define COLOR_HEADER_BG         ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy blue
#define COLOR_HEADER_TEXT       ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream
#define COLOR_HEADER_BORDER     ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue
#define COLOR_CLOSE_BUTTON      ((aColor_t){207, 87, 60, 255})   // #cf573c - red-orange
#define COLOR_CLOSE_HOVER       ((aColor_t){222, 158, 65, 255})  // #de9e41 - orange
#define COLOR_SCROLLBAR_BG      ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy
#define COLOR_SCROLLBAR_HANDLE  ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue
#define COLOR_SCROLLBAR_HOVER   ((aColor_t){76, 117, 171, 255})  // Lighter blue

// Helper: Fisher-Yates shuffle for indices
static void ShuffleIndices(int* indices, int count) {
    for (int i = count - 1; i > 0; i--) {
        int j = GetRandomInt(0, i);
        int temp = indices[i];
        indices[i] = indices[j];
        indices[j] = temp;
    }
}

// ============================================================================
// LIFECYCLE
// ============================================================================

CardGridModal_t* CreateCardGridModal(const char* title, dArray_t* cards, bool should_shuffle_display) {
    CardGridModal_t* modal = malloc(sizeof(CardGridModal_t));
    if (!modal) {
        d_LogError("Failed to allocate CardGridModal");
        return NULL;
    }

    // Set title using strncpy (static storage pattern)
    strncpy(modal->title, title ? title : "Cards", sizeof(modal->title) - 1);
    modal->title[sizeof(modal->title) - 1] = '\0';

    modal->cards = cards;  // NOT owned - just a pointer
    modal->is_visible = false;
    modal->should_shuffle_display = should_shuffle_display;
    modal->scroll_offset = 0;
    modal->max_scroll = 0;
    modal->dragging_scrollbar = false;
    modal->drag_start_y = 0;
    modal->drag_start_scroll = 0;
    modal->shuffled_indices = NULL;
    modal->hovered_card_index = -1;

    // Create tooltip
    modal->tooltip = CreateCardTooltipModal();
    if (!modal->tooltip) {
        free(modal);
        d_LogError("Failed to create CardTooltipModal");
        return NULL;
    }

    d_LogInfo("CardGridModal created");
    return modal;
}

void DestroyCardGridModal(CardGridModal_t** modal) {
    if (!modal || !*modal) return;

    CardGridModal_t* m = *modal;

    // No string cleanup needed - using fixed buffer!

    if (m->shuffled_indices) {
        free(m->shuffled_indices);
    }

    // Destroy tooltip
    if (m->tooltip) {
        DestroyCardTooltipModal(&m->tooltip);
    }

    free(m);
    *modal = NULL;
    d_LogInfo("CardGridModal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowCardGridModal(CardGridModal_t* modal) {
    if (!modal) return;

    modal->is_visible = true;
    modal->scroll_offset = 0;
    modal->dragging_scrollbar = false;

    // Create shuffled indices for draw pile
    if (modal->should_shuffle_display && modal->cards) {
        size_t card_count = modal->cards->count;

        // Free old indices if any
        if (modal->shuffled_indices) {
            free(modal->shuffled_indices);
        }

        // Create and shuffle indices
        modal->shuffled_indices = malloc(sizeof(int) * card_count);
        if (modal->shuffled_indices) {
            for (size_t i = 0; i < card_count; i++) {
                modal->shuffled_indices[i] = (int)i;
            }
            ShuffleIndices(modal->shuffled_indices, (int)card_count);
        }
    }

    d_LogInfo("CardGridModal shown");
}

void HideCardGridModal(CardGridModal_t* modal) {
    if (!modal) return;

    modal->is_visible = false;
    modal->scroll_offset = 0;
    modal->dragging_scrollbar = false;

    // Free shuffled indices
    if (modal->shuffled_indices) {
        free(modal->shuffled_indices);
        modal->shuffled_indices = NULL;
    }

    d_LogInfo("CardGridModal hidden");
}

// ============================================================================
// INPUT HANDLING
// ============================================================================

bool HandleCardGridModalInput(CardGridModal_t* modal) {
    if (!modal || !modal->is_visible) return false;

    // Match rendering position (shifted 96px right)
    int modal_x = ((SCREEN_WIDTH - MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - MODAL_HEIGHT) / 2;

    // ESC, V, or C key closes modal (V and C are the hotkeys for draw/discard pile)
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
        return true;  // Request close
    }
    if (app.keyboard[SDL_SCANCODE_V]) {
        app.keyboard[SDL_SCANCODE_V] = 0;
        return true;  // Request close
    }
    if (app.keyboard[SDL_SCANCODE_C]) {
        app.keyboard[SDL_SCANCODE_C] = 0;
        return true;  // Request close
    }

    // X button in top-right (centered in header)
    int close_button_size = 30;
    int close_button_x = modal_x + MODAL_WIDTH - close_button_size - 10;
    int close_button_y = modal_y + (MODAL_HEADER_HEIGHT - close_button_size) / 2;

    bool mouse_over_close = (app.mouse.x >= close_button_x &&
                             app.mouse.x <= close_button_x + close_button_size &&
                             app.mouse.y >= close_button_y &&
                             app.mouse.y <= close_button_y + close_button_size);

    if (mouse_over_close && app.mouse.pressed) {
        return true;  // Request close
    }

    // Update hovered card index (for hover-to-enlarge effect)
    modal->hovered_card_index = -1;
    if (modal->cards && modal->cards->count > 0) {
        // Calculate actual grid width for centering
        int actual_grid_width = CARD_GRID_COLS * CARD_GRID_CARD_WIDTH + (CARD_GRID_COLS - 1) * CARD_GRID_SPACING;
        int grid_x = modal_x + (MODAL_WIDTH - SCROLLBAR_WIDTH - 20 - actual_grid_width) / 2;
        int grid_y = modal_y + MODAL_HEADER_HEIGHT + CARD_GRID_PADDING;
        int mx = app.mouse.x;
        int my = app.mouse.y;

        int card_count = (int)modal->cards->count;
        for (int i = 0; i < card_count; i++) {
            int col = i % CARD_GRID_COLS;
            int row = i / CARD_GRID_COLS;
            int x = grid_x + col * (CARD_GRID_CARD_WIDTH + CARD_GRID_SPACING);
            int y = grid_y + row * (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) - modal->scroll_offset;

            // Check if mouse is over this card
            if (mx >= x && mx <= x + CARD_GRID_CARD_WIDTH &&
                my >= y && my <= y + CARD_GRID_CARD_HEIGHT) {
                modal->hovered_card_index = i;
                break;
            }
        }
    }

    // Calculate scrollbar bounds
    int scrollbar_x = modal_x + MODAL_WIDTH - SCROLLBAR_WIDTH - 10;
    int scrollbar_y = modal_y + MODAL_HEADER_HEIGHT + 10;
    int scrollbar_height = MODAL_HEIGHT - MODAL_HEADER_HEIGHT - 20;

    // Calculate grid dimensions
    if (modal->cards) {
        int card_count = (int)modal->cards->count;
        int rows = (card_count + CARD_GRID_COLS - 1) / CARD_GRID_COLS;
        int grid_height = rows * (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) + CARD_GRID_PADDING;
        int visible_height = MODAL_HEIGHT - MODAL_HEADER_HEIGHT - 20;
        modal->max_scroll = (grid_height > visible_height) ? (grid_height - visible_height) : 0;

        // Scrollbar dragging
        if (modal->dragging_scrollbar) {
            if (app.mouse.pressed) {
                int delta_y = app.mouse.y - modal->drag_start_y;
                float scroll_ratio = (float)modal->max_scroll / (float)(scrollbar_height - SCROLLBAR_MIN_HANDLE_HEIGHT);
                modal->scroll_offset = modal->drag_start_scroll + (int)(delta_y * scroll_ratio);

                // Clamp
                if (modal->scroll_offset < 0) modal->scroll_offset = 0;
                if (modal->scroll_offset > modal->max_scroll) modal->scroll_offset = modal->max_scroll;
            } else {
                modal->dragging_scrollbar = false;
            }
        } else {
            // Check scrollbar click
            if (app.mouse.pressed && modal->max_scroll > 0) {
                float scroll_ratio = (float)modal->scroll_offset / (float)modal->max_scroll;
                int handle_height = (scrollbar_height > SCROLLBAR_MIN_HANDLE_HEIGHT) ? scrollbar_height * visible_height / grid_height : SCROLLBAR_MIN_HANDLE_HEIGHT;
                if (handle_height < SCROLLBAR_MIN_HANDLE_HEIGHT) handle_height = SCROLLBAR_MIN_HANDLE_HEIGHT;
                if (handle_height > scrollbar_height) handle_height = scrollbar_height;

                int handle_y = scrollbar_y + (int)(scroll_ratio * (scrollbar_height - handle_height));

                bool mouse_over_handle = (app.mouse.x >= scrollbar_x &&
                                         app.mouse.x <= scrollbar_x + SCROLLBAR_WIDTH &&
                                         app.mouse.y >= handle_y &&
                                         app.mouse.y <= handle_y + handle_height);

                if (mouse_over_handle) {
                    modal->dragging_scrollbar = true;
                    modal->drag_start_y = app.mouse.y;
                    modal->drag_start_scroll = modal->scroll_offset;
                }
            }
        }

        // Mouse wheel scrolling
        if (app.mouse.wheel != 0) {
            // Scroll by ~3 cards per wheel tick
            int scroll_speed = (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) * 3;
            modal->scroll_offset -= app.mouse.wheel * scroll_speed;  // Negative wheel = scroll down

            // Clamp scroll offset
            if (modal->scroll_offset < 0) modal->scroll_offset = 0;
            if (modal->scroll_offset > modal->max_scroll) modal->scroll_offset = modal->max_scroll;

            // Reset wheel (consume the event)
            app.mouse.wheel = 0;
        }
    }

    return false;  // Don't close
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderCardGridModal(CardGridModal_t* modal) {
    if (!modal || !modal->is_visible) return;

    // Full-screen overlay
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, COLOR_OVERLAY);

    // Modal panel (shifted 96px right from center, matching reward modal)
    int modal_x = ((SCREEN_WIDTH - MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - MODAL_HEIGHT) / 2;

    // Draw panel body
    int panel_body_y = modal_y + MODAL_HEADER_HEIGHT;
    int panel_body_height = MODAL_HEIGHT - MODAL_HEADER_HEIGHT;
    a_DrawFilledRect((aRectf_t){modal_x, panel_body_y, MODAL_WIDTH, panel_body_height}, COLOR_PANEL_BG);

    // Draw header
    a_DrawFilledRect((aRectf_t){modal_x, modal_y, MODAL_WIDTH, MODAL_HEADER_HEIGHT}, COLOR_HEADER_BG);
    a_DrawRect((aRectf_t){modal_x, modal_y, MODAL_WIDTH, MODAL_HEADER_HEIGHT}, COLOR_HEADER_BORDER);

    // Draw title (centered on modal panel, not screen)
    int title_center_x = modal_x + (MODAL_WIDTH / 2);
    // Cast safe: a_DrawText is read-only, using fixed char buffer
    a_DrawText((char*)modal->title, title_center_x, modal_y + 12,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_HEADER_TEXT.r,COLOR_HEADER_TEXT.g,COLOR_HEADER_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw X button (centered in header)
    int close_button_size = 30;
    int close_button_x = modal_x + MODAL_WIDTH - close_button_size - 10;
    int close_button_y = modal_y + (MODAL_HEADER_HEIGHT - close_button_size) / 2;

    bool mouse_over_close = (app.mouse.x >= close_button_x &&
                             app.mouse.x <= close_button_x + close_button_size &&
                             app.mouse.y >= close_button_y &&
                             app.mouse.y <= close_button_y + close_button_size);

    aColor_t close_color = mouse_over_close ? COLOR_CLOSE_HOVER : COLOR_CLOSE_BUTTON;
    a_DrawFilledRect((aRectf_t){close_button_x, close_button_y, close_button_size, close_button_size}, close_color);
    // Center X text vertically in button
    a_DrawText("X", close_button_x + close_button_size / 2, close_button_y + 3,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,255,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    // Draw card grid with clipping
    if (modal->cards && modal->cards->count > 0) {
        // Calculate actual grid width for centering
        // Grid: 6 cols * 75px cards + 5 * 10px spacing = 500px
        int actual_grid_width = CARD_GRID_COLS * CARD_GRID_CARD_WIDTH + (CARD_GRID_COLS - 1) * CARD_GRID_SPACING;

        // Center grid horizontally in modal (leaving room for scrollbar)
        int grid_x = modal_x + (MODAL_WIDTH - SCROLLBAR_WIDTH - 20 - actual_grid_width) / 2;
        int grid_y = panel_body_y + CARD_GRID_PADDING;
        int visible_height = panel_body_height - CARD_GRID_PADDING * 2;

        // Enable scissor for clipping
        SDL_Rect clip_rect = {
            modal_x,
            panel_body_y,
            MODAL_WIDTH - SCROLLBAR_WIDTH - 10,
            panel_body_height
        };
        SDL_RenderSetClipRect(app.renderer, &clip_rect);

        int card_count = (int)modal->cards->count;

        // First pass: Draw non-hovered cards
        for (int i = 0; i < card_count; i++) {
            if (i == modal->hovered_card_index) continue;  // Skip hovered card, draw it last

            // Get actual card index (shuffled or ordered)
            int card_idx = modal->should_shuffle_display ? modal->shuffled_indices[i] : i;
            Card_t* card = (Card_t*)d_ArrayGet(modal->cards, card_idx);
            if (!card) continue;

            // Ensure texture is loaded (lazy loading)
            if (!card->texture) {
                SDL_Surface** tex_ptr = (SDL_Surface**)d_TableGet(g_card_textures, &card->card_id);
                if (tex_ptr && *tex_ptr) {
                    card->texture = *tex_ptr;
                }
            }

            // Calculate position
            int col = i % CARD_GRID_COLS;
            int row = i / CARD_GRID_COLS;
            int x = grid_x + col * (CARD_GRID_CARD_WIDTH + CARD_GRID_SPACING);
            int y = grid_y + row * (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) - modal->scroll_offset;

            // Skip if off-screen
            if (y + CARD_GRID_CARD_HEIGHT < panel_body_y || y > panel_body_y + panel_body_height) {
                continue;
            }

            // Draw card - ALWAYS show face (ignore card->face_up, this is a viewer)
            if (card->texture) {
                a_BlitSurfaceRect(card->texture, (aRectf_t){x, y, CARD_GRID_CARD_WIDTH, CARD_GRID_CARD_HEIGHT}, 1);
            } else {
                a_DrawFilledRect((aRectf_t){x, y, CARD_GRID_CARD_WIDTH, CARD_GRID_CARD_HEIGHT}, (aColor_t){200, 200, 200, 255});
                a_DrawRect((aRectf_t){x, y, CARD_GRID_CARD_WIDTH, CARD_GRID_CARD_HEIGHT}, (aColor_t){100, 100, 100, 255});
            }

            // Draw tag badge on top-right of card
            const dArray_t* tags = GetCardTags(card->card_id);
            if (tags && tags->count > 0) {
                CardTag_t* tag = (CardTag_t*)d_ArrayGet((dArray_t*)tags, 0);
                const char* tag_text = GetCardTagName(*tag);

                int r, g, b;
                GetCardTagColor(*tag, &r, &g, &b);

                // Minimum width: 16px less than current CARD_GRID_TAG_BADGE_W (80px)
                int badge_w = 64;  // 80 - 16 = 64px min width
                int badge_h = CARD_GRID_TAG_BADGE_H;
                int badge_x = x + CARD_GRID_CARD_WIDTH - badge_w - 3 + 12;  // +12px right (was +8px)
                int badge_y = y - badge_h - 3 + 24;  // +24px down (was +16px)

                if (badge_y >= panel_body_y) {
                    a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){r, g, b, 255});
                    a_DrawRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){0, 0, 0, 255});

                    // Truncate tag text to first 3 letters
                    char truncated[4] = {0};
                    strncpy(truncated, tag_text, 3);
                    truncated[3] = '\0';

                    aTextStyle_t tag_config = {
                        .type = FONT_ENTER_COMMAND,
                        .fg = {0, 0, 0, 180},
                        .align = TEXT_ALIGN_CENTER,
                        .scale = 0.7f
                    };
                    a_DrawText(truncated, badge_x + badge_w / 2, badge_y - 3, tag_config);
                }
            }
        }

        // Disable scissor temporarily for hovered card (so it can render on top)
        SDL_RenderSetClipRect(app.renderer, NULL);

        // Second pass: Draw hovered card enlarged and on top
        if (modal->hovered_card_index >= 0 && modal->hovered_card_index < card_count) {
            int i = modal->hovered_card_index;
            int card_idx = modal->should_shuffle_display ? modal->shuffled_indices[i] : i;
            Card_t* card = (Card_t*)d_ArrayGet(modal->cards, card_idx);

            if (card && card->texture) {
                // Calculate base position in grid
                int col = i % CARD_GRID_COLS;
                int row = i / CARD_GRID_COLS;
                int base_x = grid_x + col * (CARD_GRID_CARD_WIDTH + CARD_GRID_SPACING);
                int base_y = grid_y + row * (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) - modal->scroll_offset;

                // Enlarged size (1.5x scale, like hand hover)
                int hover_w = (int)(CARD_GRID_CARD_WIDTH * HOVER_CARD_SCALE);
                int hover_h = (int)(CARD_GRID_CARD_HEIGHT * HOVER_CARD_SCALE);

                // Center enlarged card on its base position (not mouse)
                int hover_x = base_x + (CARD_GRID_CARD_WIDTH - hover_w) / 2;
                int hover_y = base_y + (CARD_GRID_CARD_HEIGHT - hover_h) / 2;

                // Draw enlarged card
                a_BlitSurfaceRect(card->texture, (aRectf_t){hover_x, hover_y, hover_w, hover_h}, 1);

                // Draw tag badge on enlarged card
                const dArray_t* tags = GetCardTags(card->card_id);
                if (tags && tags->count > 0) {
                    CardTag_t* tag = (CardTag_t*)d_ArrayGet((dArray_t*)tags, 0);
                    const char* tag_text = GetCardTagName(*tag);

                    int r, g, b;
                    GetCardTagColor(*tag, &r, &g, &b);

                    // Scale badge with card (use 64px base width)
                    int hover_badge_w = (int)(64 * HOVER_CARD_SCALE);  // 64px min width scaled
                    int hover_badge_h = (int)(CARD_GRID_TAG_BADGE_H * HOVER_CARD_SCALE);
                    int hover_badge_x = hover_x + hover_w - hover_badge_w - 5 + (int)(12 * HOVER_CARD_SCALE);  // +12px right (scaled)
                    int hover_badge_y = hover_y - hover_badge_h - 5 + (int)(24 * HOVER_CARD_SCALE);  // +24px down (scaled)

                    a_DrawFilledRect((aRectf_t){hover_badge_x, hover_badge_y, hover_badge_w, hover_badge_h}, (aColor_t){r, g, b, 255});
                    a_DrawRect((aRectf_t){hover_badge_x, hover_badge_y, hover_badge_w, hover_badge_h}, (aColor_t){0, 0, 0, 255});

                    // Truncate tag text to first 3 letters
                    char truncated[4] = {0};
                    strncpy(truncated, tag_text, 3);
                    truncated[3] = '\0';

                    aTextStyle_t hover_tag_config = {
                        .type = FONT_ENTER_COMMAND,
                        .fg = {0, 0, 0, 200},
                        .align = TEXT_ALIGN_CENTER,
                        .scale = 0.7f * HOVER_CARD_SCALE
                    };
                    a_DrawText(truncated, hover_badge_x + hover_badge_w / 2, hover_badge_y + (int)(3 * HOVER_CARD_SCALE) - 10, hover_tag_config);
                }
            }
        }

        // Show/hide tooltip for hovered card
        if (modal->hovered_card_index >= 0 && modal->hovered_card_index < card_count && modal->tooltip) {
            int i = modal->hovered_card_index;
            int card_idx = modal->should_shuffle_display ? modal->shuffled_indices[i] : i;
            Card_t* card = (Card_t*)d_ArrayGet(modal->cards, card_idx);

            if (card) {
                // Calculate card position in grid
                int col = i % CARD_GRID_COLS;
                int row = i / CARD_GRID_COLS;
                int card_x = grid_x + col * (CARD_GRID_CARD_WIDTH + CARD_GRID_SPACING);
                int card_y = grid_y + row * (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) - modal->scroll_offset;

                // Flip tooltip left for last 2 columns (columns 4 and 5 in 6-column grid)
                bool force_left = (col >= CARD_GRID_COLS - 2);
                ShowCardTooltipModalWithSide(modal->tooltip, card, card_x, card_y, force_left);
            }
        } else if (modal->tooltip) {
            HideCardTooltipModal(modal->tooltip);
        }

        // Render tooltip (on top of everything)
        if (modal->tooltip) {
            RenderCardTooltipModal(modal->tooltip);
        }

        // Draw scrollbar if needed
        if (modal->max_scroll > 0) {
            int scrollbar_x = modal_x + MODAL_WIDTH - SCROLLBAR_WIDTH - 10;
            int scrollbar_y = panel_body_y + 10;
            int scrollbar_height = panel_body_height - 20;

            // Background
            a_DrawFilledRect((aRectf_t){scrollbar_x, scrollbar_y, SCROLLBAR_WIDTH, scrollbar_height}, COLOR_SCROLLBAR_BG);

            // Calculate handle size and position
            int rows = (card_count + CARD_GRID_COLS - 1) / CARD_GRID_COLS;
            int grid_height = rows * (CARD_GRID_CARD_HEIGHT + CARD_GRID_SPACING) + CARD_GRID_PADDING;
            int handle_height = scrollbar_height * visible_height / grid_height;
            if (handle_height < SCROLLBAR_MIN_HANDLE_HEIGHT) handle_height = SCROLLBAR_MIN_HANDLE_HEIGHT;
            if (handle_height > scrollbar_height) handle_height = scrollbar_height;

            float scroll_ratio = (float)modal->scroll_offset / (float)modal->max_scroll;
            int handle_y = scrollbar_y + (int)(scroll_ratio * (scrollbar_height - handle_height));

            // Check hover
            bool mouse_over_handle = (app.mouse.x >= scrollbar_x &&
                                     app.mouse.x <= scrollbar_x + SCROLLBAR_WIDTH &&
                                     app.mouse.y >= handle_y &&
                                     app.mouse.y <= handle_y + handle_height);

            aColor_t handle_color = (mouse_over_handle || modal->dragging_scrollbar) ? COLOR_SCROLLBAR_HOVER : COLOR_SCROLLBAR_HANDLE;

            // Draw handle
            a_DrawFilledRect((aRectf_t){scrollbar_x, handle_y, SCROLLBAR_WIDTH, handle_height}, handle_color);
        }
    } else {
        // No cards message (centered on modal panel)
        int msg_center_x = modal_x + (MODAL_WIDTH / 2);
        a_DrawText("No cards to display", msg_center_x, modal_y + MODAL_HEIGHT / 2,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={200,200,200,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }
}
