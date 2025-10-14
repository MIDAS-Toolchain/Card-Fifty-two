/*
 * Card Grid Modal Component Implementation
 */

#include "../../../include/scenes/components/cardGridModal.h"
#include "../../../include/card.h"
#include <stdlib.h>
#include <time.h>

// External globals
extern dTable_t* g_card_textures;
extern SDL_Texture* g_card_back_texture;

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
        int j = rand() % (i + 1);
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

    modal->title = d_InitString();
    if (!modal->title) {
        free(modal);
        d_LogError("Failed to allocate CardGridModal title");
        return NULL;
    }
    d_SetString(modal->title, title ? title : "Cards", 0);

    modal->cards = cards;  // NOT owned - just a pointer
    modal->is_visible = false;
    modal->should_shuffle_display = should_shuffle_display;
    modal->scroll_offset = 0;
    modal->max_scroll = 0;
    modal->dragging_scrollbar = false;
    modal->drag_start_y = 0;
    modal->drag_start_scroll = 0;
    modal->shuffled_indices = NULL;

    d_LogInfo("CardGridModal created");
    return modal;
}

void DestroyCardGridModal(CardGridModal_t** modal) {
    if (!modal || !*modal) return;

    CardGridModal_t* m = *modal;

    if (m->title) {
        d_DestroyString(m->title);
    }

    if (m->shuffled_indices) {
        free(m->shuffled_indices);
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

    int modal_x = (SCREEN_WIDTH - MODAL_WIDTH) / 2;
    int modal_y = (SCREEN_HEIGHT - MODAL_HEIGHT) / 2;

    // ESC key closes modal
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;
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
        // Note: Archimedes doesn't expose mouse wheel directly, but we can add it if needed
        // For now, draggable scrollbar only
    }

    return false;  // Don't close
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderCardGridModal(CardGridModal_t* modal) {
    if (!modal || !modal->is_visible) return;

    // Full-screen overlay
    a_DrawFilledRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT,
                     COLOR_OVERLAY.r, COLOR_OVERLAY.g, COLOR_OVERLAY.b, COLOR_OVERLAY.a);

    // Centered modal panel
    int modal_x = (SCREEN_WIDTH - MODAL_WIDTH) / 2;
    int modal_y = (SCREEN_HEIGHT - MODAL_HEIGHT) / 2;

    // Draw panel body
    int panel_body_y = modal_y + MODAL_HEADER_HEIGHT;
    int panel_body_height = MODAL_HEIGHT - MODAL_HEADER_HEIGHT;
    a_DrawFilledRect(modal_x, panel_body_y, MODAL_WIDTH, panel_body_height,
                     COLOR_PANEL_BG.r, COLOR_PANEL_BG.g, COLOR_PANEL_BG.b, COLOR_PANEL_BG.a);

    // Draw header
    a_DrawFilledRect(modal_x, modal_y, MODAL_WIDTH, MODAL_HEADER_HEIGHT,
                     COLOR_HEADER_BG.r, COLOR_HEADER_BG.g, COLOR_HEADER_BG.b, COLOR_HEADER_BG.a);
    a_DrawRect(modal_x, modal_y, MODAL_WIDTH, MODAL_HEADER_HEIGHT,
               COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, COLOR_HEADER_BORDER.a);

    // Draw title
    const char* title_str = d_PeekString(modal->title);
    a_DrawText((char*)title_str, SCREEN_WIDTH / 2, modal_y + 12,
               COLOR_HEADER_TEXT.r, COLOR_HEADER_TEXT.g, COLOR_HEADER_TEXT.b,
               FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw X button (centered in header)
    int close_button_size = 30;
    int close_button_x = modal_x + MODAL_WIDTH - close_button_size - 10;
    int close_button_y = modal_y + (MODAL_HEADER_HEIGHT - close_button_size) / 2;

    bool mouse_over_close = (app.mouse.x >= close_button_x &&
                             app.mouse.x <= close_button_x + close_button_size &&
                             app.mouse.y >= close_button_y &&
                             app.mouse.y <= close_button_y + close_button_size);

    aColor_t close_color = mouse_over_close ? COLOR_CLOSE_HOVER : COLOR_CLOSE_BUTTON;
    a_DrawFilledRect(close_button_x, close_button_y, close_button_size, close_button_size,
                     close_color.r, close_color.g, close_color.b, close_color.a);
    a_DrawText("X", close_button_x + close_button_size / 2, close_button_y + 5,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // Draw card grid with clipping
    if (modal->cards && modal->cards->count > 0) {
        int grid_x = modal_x + CARD_GRID_PADDING;
        int grid_y = panel_body_y + CARD_GRID_PADDING;
        int visible_height = panel_body_height - CARD_GRID_PADDING * 2;

        // Enable scissor for clipping
        SDL_Rect clip_rect = {
            modal_x,
            panel_body_y,
            MODAL_WIDTH - SCROLLBAR_WIDTH - 20,
            panel_body_height
        };
        SDL_RenderSetClipRect(app.renderer, &clip_rect);

        int card_count = (int)modal->cards->count;
        for (int i = 0; i < card_count; i++) {
            // Get actual card index (shuffled or ordered)
            int card_idx = modal->should_shuffle_display ? modal->shuffled_indices[i] : i;
            Card_t* card = (Card_t*)d_IndexDataFromArray(modal->cards, card_idx);
            if (!card) continue;

            // Ensure texture is loaded (lazy loading)
            if (!card->texture) {
                SDL_Texture** tex_ptr = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &card->card_id);
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
            SDL_Rect dst = {x, y, CARD_GRID_CARD_WIDTH, CARD_GRID_CARD_HEIGHT};
            if (card->texture) {
                // Always render the card face texture in modal view
                SDL_RenderCopy(app.renderer, card->texture, NULL, &dst);
            } else {
                // Fallback
                a_DrawFilledRect(x, y, CARD_GRID_CARD_WIDTH, CARD_GRID_CARD_HEIGHT, 200, 200, 200, 255);
                a_DrawRect(x, y, CARD_GRID_CARD_WIDTH, CARD_GRID_CARD_HEIGHT, 100, 100, 100, 255);
            }
        }

        // Disable scissor
        SDL_RenderSetClipRect(app.renderer, NULL);

        // Draw scrollbar if needed
        if (modal->max_scroll > 0) {
            int scrollbar_x = modal_x + MODAL_WIDTH - SCROLLBAR_WIDTH - 10;
            int scrollbar_y = panel_body_y + 10;
            int scrollbar_height = panel_body_height - 20;

            // Background
            a_DrawFilledRect(scrollbar_x, scrollbar_y, SCROLLBAR_WIDTH, scrollbar_height,
                             COLOR_SCROLLBAR_BG.r, COLOR_SCROLLBAR_BG.g, COLOR_SCROLLBAR_BG.b, COLOR_SCROLLBAR_BG.a);

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
            a_DrawFilledRect(scrollbar_x, handle_y, SCROLLBAR_WIDTH, handle_height,
                             handle_color.r, handle_color.g, handle_color.b, handle_color.a);
        }
    } else {
        // No cards message
        a_DrawText("No cards to display", SCREEN_WIDTH / 2, modal_y + MODAL_HEIGHT / 2,
                   200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    }
}
