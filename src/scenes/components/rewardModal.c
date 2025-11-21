/*
 * Reward Modal Component Implementation
 * Constitutional pattern: Modal overlay, parallel arrays for card+tag combos
 * Design: Matches cardGridModal palette and structure
 */

#include "../../../include/scenes/components/rewardModal.h"
#include "../../../include/card.h"
#include "../../../include/cardTags.h"
#include "../../../include/random.h"
#include "../../../include/tween/tween.h"

// External globals
extern dTable_t* g_card_textures;  // Card textures table (from sceneBlackjack.c)
extern TweenManager_t g_tween_manager;  // Global tween manager (from sceneBlackjack.c)

// Color constants (matching cardGridModal palette)
#define COLOR_OVERLAY           ((aColor_t){9, 10, 20, 180})     // #090a14 - almost black overlay
#define COLOR_PANEL_BG          ((aColor_t){9, 10, 20, 240})     // #090a14 - almost black
#define COLOR_HEADER_BG         ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy blue
#define COLOR_HEADER_TEXT       ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream
#define COLOR_HEADER_BORDER     ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue
#define COLOR_CLOSE_BUTTON      ((aColor_t){207, 87, 60, 255})   // #cf573c - red-orange
#define COLOR_CLOSE_HOVER       ((aColor_t){222, 158, 65, 255})  // #de9e41 - orange
#define COLOR_INFO_TEXT         ((aColor_t){168, 181, 178, 255}) // #a8b5b2 - light gray
#define COLOR_TAG_GOLD          ((aColor_t){232, 193, 112, 255}) // #e8c170 - gold
#define COLOR_SKIP_BG           ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy
#define COLOR_SKIP_HOVER        ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue


// ============================================================================
// LIFECYCLE
// ============================================================================

RewardModal_t* CreateRewardModal(void) {
    RewardModal_t* modal = malloc(sizeof(RewardModal_t));
    if (!modal) {
        d_LogFatal("CreateRewardModal: Failed to allocate RewardModal_t");
        return NULL;
    }

    modal->is_visible = false;
    modal->card_ids[0] = -1;
    modal->card_ids[1] = -1;
    modal->card_ids[2] = -1;
    modal->tags[0] = CARD_TAG_MAX;
    modal->tags[1] = CARD_TAG_MAX;
    modal->tags[2] = CARD_TAG_MAX;
    modal->selected_index = -1;
    modal->hovered_index = -1;
    modal->key_held_index = -1;
    modal->reward_taken = false;
    modal->result_timer = 0.0f;

    // Initialize animation state
    modal->anim_stage = ANIM_NONE;
    modal->fade_out_alpha = 1.0f;
    modal->card_scale = 1.0f;
    modal->tag_badge_alpha = 0.0f;

    // Calculate layout positions (shifted 96px right from center)
    int modal_x = ((SCREEN_WIDTH - REWARD_MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - REWARD_MODAL_HEIGHT) / 2;
    int panel_body_y = modal_y + REWARD_MODAL_HEADER_HEIGHT;

    // Create FlexBox for info text (vertical)
    modal->info_layout = a_FlexBoxCreate(
        modal_x + REWARD_MODAL_PADDING,
        panel_body_y + 20,
        REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2),
        60
    );
    a_FlexConfigure(modal->info_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, 10);
    a_FlexAddItem(modal->info_layout, REWARD_MODAL_WIDTH, 20, NULL);  // Info line 1
    a_FlexAddItem(modal->info_layout, REWARD_MODAL_WIDTH, 20, NULL);  // Info line 2
    a_FlexLayout(modal->info_layout);

    // Create FlexBox for horizontal card layout (top section)
    int cards_area_y = panel_body_y + 100;
    modal->card_layout = a_FlexBoxCreate(
        modal_x + REWARD_MODAL_PADDING,
        cards_area_y,
        REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2),
        CARD_HEIGHT + 20
    );
    a_FlexConfigure(modal->card_layout, FLEX_DIR_ROW, FLEX_JUSTIFY_CENTER, REWARD_CARD_SPACING);

    // Add 3 card slots
    for (int i = 0; i < 3; i++) {
        a_FlexAddItem(modal->card_layout, CARD_WIDTH, CARD_HEIGHT, NULL);
    }
    a_FlexLayout(modal->card_layout);

    // Create FlexBox for tag list (bottom section)
    int list_area_y = cards_area_y + CARD_HEIGHT + 40;
    int list_width = REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2);

    modal->list_layout = a_FlexBoxCreate(
        modal_x + REWARD_MODAL_PADDING,
        list_area_y,
        list_width,
        (REWARD_LIST_ITEM_HEIGHT * 3) + (REWARD_LIST_ITEM_SPACING * 2)
    );
    a_FlexConfigure(modal->list_layout, FLEX_DIR_COLUMN, FLEX_JUSTIFY_START, REWARD_LIST_ITEM_SPACING);

    // Add 3 list items
    for (int i = 0; i < 3; i++) {
        a_FlexAddItem(modal->list_layout, list_width, REWARD_LIST_ITEM_HEIGHT, NULL);
    }
    a_FlexLayout(modal->list_layout);

    d_LogInfo("Reward modal created with FlexBox layouts");
    return modal;
}

void DestroyRewardModal(RewardModal_t** modal) {
    if (!modal || !*modal) return;

    if ((*modal)->card_layout) {
        a_FlexBoxDestroy(&(*modal)->card_layout);
    }
    if ((*modal)->info_layout) {
        a_FlexBoxDestroy(&(*modal)->info_layout);
    }
    if ((*modal)->list_layout) {
        a_FlexBoxDestroy(&(*modal)->list_layout);
    }

    free(*modal);
    *modal = NULL;

    d_LogInfo("Reward modal destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

bool ShowRewardModal(RewardModal_t* modal) {
    if (!modal) return false;

    // Find pool of cards that can accept new tags (0 or 1 tag, max 2 tags per card)
    int eligible_cards[52];
    int count = 0;

    for (int card_id = 0; card_id < 52; card_id++) {
        const dArray_t* tags = GetCardTags(card_id);
        int tag_count = tags ? (int)tags->count : 0;

        // Card can accept tags if it has 0 or 1 tag (max 2 total)
        if (tag_count < 2) {
            eligible_cards[count++] = card_id;
        }
    }

    // Need at least 3 eligible cards
    if (count < 3) {
        d_LogWarning("ShowRewardModal: Not enough eligible cards (need 3, all cards have 2 tags)");
        return false;
    }

    // Pick 3 random eligible cards (no duplicates)
    for (int i = 0; i < 3; i++) {
        int random_idx = GetRandomInt(0, count - 1);
        modal->card_ids[i] = eligible_cards[random_idx];

        // Remove from pool to avoid duplicates
        eligible_cards[random_idx] = eligible_cards[count - 1];
        count--;
    }

    // All available tags
    CardTag_t available_tags[] = {
        CARD_TAG_CURSED,   // 10 damage to enemy when drawn
        CARD_TAG_VAMPIRIC, // 5 damage + 5 chips when drawn
        CARD_TAG_LUCKY,    // +10% crit while in any hand
        CARD_TAG_BRUTAL    // +10% damage while in any hand
    };
    int all_tags_count = sizeof(available_tags) / sizeof(available_tags[0]);

    // Assign random tags to each card, excluding tags they already have
    for (int i = 0; i < 3; i++) {
        int card_id = modal->card_ids[i];
        const dArray_t* existing_tags = GetCardTags(card_id);

        // Build list of tags this card doesn't have
        CardTag_t valid_tags[4];  // Max 4 tag types
        int valid_count = 0;

        for (int t = 0; t < all_tags_count; t++) {
            CardTag_t tag = available_tags[t];

            // Check if card already has this tag
            bool already_has = false;
            if (existing_tags) {
                for (size_t e = 0; e < existing_tags->count; e++) {
                    CardTag_t* existing = (CardTag_t*)d_IndexDataFromArray((dArray_t*)existing_tags, e);
                    if (existing && *existing == tag) {
                        already_has = true;
                        break;
                    }
                }
            }

            if (!already_has) {
                valid_tags[valid_count++] = tag;
            }
        }

        // Pick random tag from valid options
        if (valid_count > 0) {
            int tag_idx = GetRandomInt(0, valid_count - 1);
            modal->tags[i] = valid_tags[tag_idx];
            d_LogInfoF("Card %d: Offering tag %d (has %zu existing tags, %d valid options)",
                      card_id, modal->tags[i],
                      existing_tags ? existing_tags->count : 0, valid_count);
        } else {
            // Fallback (shouldn't happen if eligible_cards logic is correct)
            d_LogWarning("Card has no valid tag options (shouldn't happen)");
            modal->tags[i] = CARD_TAG_LUCKY;
        }
    }

    // Reset state
    modal->selected_index = -1;
    modal->hovered_index = -1;
    modal->key_held_index = -1;
    modal->reward_taken = false;
    modal->result_timer = 0.0f;

    // Reset animation state
    modal->anim_stage = ANIM_NONE;
    modal->fade_out_alpha = 1.0f;
    modal->card_scale = 1.0f;
    modal->tag_badge_alpha = 0.0f;

    modal->is_visible = true;

    d_LogInfoF("Showing reward modal - 3 cards: %d, %d, %d",
              modal->card_ids[0], modal->card_ids[1], modal->card_ids[2]);
    return true;
}

void HideRewardModal(RewardModal_t* modal) {
    if (!modal) return;

    modal->is_visible = false;
    d_LogInfo("Reward modal hidden");
}

bool IsRewardModalVisible(const RewardModal_t* modal) {
    return modal && modal->is_visible;
}

// ============================================================================
// INPUT & UPDATE
// ============================================================================

bool HandleRewardModalInput(RewardModal_t* modal, float dt) {
    if (!modal || !modal->is_visible) return false;

    // If reward taken, handle multi-stage animation
    if (modal->reward_taken) {
        switch (modal->anim_stage) {
            case ANIM_FADE_OUT:
                // Wait for fade-out to complete
                if (modal->fade_out_alpha <= 0.01f) {
                    // Stage 1 complete, start Stage 2 (scale up card)
                    modal->anim_stage = ANIM_SCALE_CARD;
                    modal->card_scale = 1.0f;
                    TweenFloat(&g_tween_manager, &modal->card_scale, 1.5f,
                              0.5f, TWEEN_EASE_OUT_CUBIC);
                    d_LogInfo("Reward animation: Starting card scale");
                }
                break;

            case ANIM_SCALE_CARD:
                // Wait for scale animation to complete
                if (modal->card_scale >= 1.49f) {
                    // Stage 2 complete, start Stage 3 (fade in tag badge)
                    modal->anim_stage = ANIM_FADE_IN_TAG;
                    modal->tag_badge_alpha = 0.0f;
                    TweenFloat(&g_tween_manager, &modal->tag_badge_alpha, 1.0f,
                              0.3f, TWEEN_EASE_IN_QUAD);
                    d_LogInfo("Reward animation: Fading in tag badge");
                }
                break;

            case ANIM_FADE_IN_TAG:
                // Wait for tag badge fade-in to complete
                if (modal->tag_badge_alpha >= 0.99f) {
                    // All animations complete, brief pause before closing
                    modal->anim_stage = ANIM_COMPLETE;
                    modal->result_timer = 0.0f;
                    d_LogInfo("Reward animation: Complete");
                }
                break;

            case ANIM_COMPLETE:
                // Brief pause, then close
                modal->result_timer += dt;
                if (modal->result_timer >= 0.5f) {
                    return true;  // Close modal
                }
                break;

            case ANIM_NONE:
                // Should not happen, but handle gracefully
                return true;
        }

        return false;  // Still animating
    }

    int mx = app.mouse.x;
    int my = app.mouse.y;

    // Calculate modal positions (shifted 96px right from center)
    int modal_x = ((SCREEN_WIDTH - REWARD_MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - REWARD_MODAL_HEIGHT) / 2;

    // Update hovered index based on list item hover OR card hover
    modal->hovered_index = -1;

    // Check card hover first
    for (int i = 0; i < 3; i++) {
        const FlexItem_t* card_item = a_FlexGetItem(modal->card_layout, i);
        if (!card_item) continue;

        int card_x = card_item->calc_x;
        int card_y = card_item->calc_y + 32;  // Match rendering offset

        if (mx >= card_x && mx <= card_x + CARD_WIDTH &&
            my >= card_y && my <= card_y + CARD_HEIGHT) {
            modal->hovered_index = i;
            break;
        }
    }

    // Check list item hover if no card hovered
    if (modal->hovered_index == -1) {
        for (int i = 0; i < 3; i++) {
            const FlexItem_t* list_item = a_FlexGetItem(modal->list_layout, i);
            if (!list_item) continue;

            int item_x = list_item->calc_x;
            int item_y = list_item->calc_y;
            int item_w = REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2);
            int item_h = REWARD_LIST_ITEM_HEIGHT;

            if (mx >= item_x && mx <= item_x + item_w &&
                my >= item_y && my <= item_y + item_h) {
                modal->hovered_index = i;
                break;
            }
        }
    }

    // Handle keyboard hotkeys (1, 2, 3) - hold to preview, release to confirm
    int prev_key_held = modal->key_held_index;

    // Check which key is currently held
    modal->key_held_index = -1;
    if (app.keyboard[SDL_SCANCODE_1]) modal->key_held_index = 0;
    else if (app.keyboard[SDL_SCANCODE_2]) modal->key_held_index = 1;
    else if (app.keyboard[SDL_SCANCODE_3]) modal->key_held_index = 2;

    // Trigger on key release (was held, now not held)
    if (prev_key_held != -1 && modal->key_held_index == -1) {
        int choice = prev_key_held;
        modal->selected_index = choice;
        AddCardTag(modal->card_ids[choice], modal->tags[choice]);
        modal->reward_taken = true;
        modal->result_timer = 0.0f;
        modal->anim_stage = ANIM_FADE_OUT;
        modal->fade_out_alpha = 1.0f;
        TweenFloat(&g_tween_manager, &modal->fade_out_alpha, 0.0f, 0.4f, TWEEN_EASE_OUT_QUAD);
        d_LogInfoF("Hotkey %d released: Applied tag %s to card %d",
                   choice + 1, GetCardTagName(modal->tags[choice]), modal->card_ids[choice]);
        return false;
    }

    // Handle input (matching cardGridModal pattern)
    if (app.mouse.pressed) {
        // Check card click first
        for (int i = 0; i < 3; i++) {
            const FlexItem_t* card_item = a_FlexGetItem(modal->card_layout, i);
            if (!card_item) continue;

            int card_x = card_item->calc_x;
            int card_y = card_item->calc_y + 32;  // Match rendering offset

            if (mx >= card_x && mx <= card_x + CARD_WIDTH &&
                my >= card_y && my <= card_y + CARD_HEIGHT) {

                // Selected this card+tag combo
                modal->selected_index = i;

                // Apply tag to this card
                AddCardTag(modal->card_ids[i], modal->tags[i]);

                modal->reward_taken = true;
                modal->result_timer = 0.0f;

                // Start fade-out animation (Stage 1)
                modal->anim_stage = ANIM_FADE_OUT;
                modal->fade_out_alpha = 1.0f;
                TweenFloat(&g_tween_manager, &modal->fade_out_alpha, 0.0f,
                          0.4f, TWEEN_EASE_OUT_QUAD);

                d_LogInfoF("Clicked card %d - Applied tag %s - starting animation",
                           modal->card_ids[i], GetCardTagName(modal->tags[i]));

                return false;  // Don't close yet, show animation
            }
        }

        // Check list item click
        for (int i = 0; i < 3; i++) {
            const FlexItem_t* list_item = a_FlexGetItem(modal->list_layout, i);
            if (!list_item) continue;

            int item_x = list_item->calc_x;
            int item_y = list_item->calc_y;
            int item_w = REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2);
            int item_h = REWARD_LIST_ITEM_HEIGHT;

            if (mx >= item_x && mx <= item_x + item_w &&
                my >= item_y && my <= item_y + item_h) {

                // Selected this card+tag combo
                modal->selected_index = i;

                // Apply tag to this card
                AddCardTag(modal->card_ids[i], modal->tags[i]);

                modal->reward_taken = true;
                modal->result_timer = 0.0f;

                // Start fade-out animation (Stage 1)
                modal->anim_stage = ANIM_FADE_OUT;
                modal->fade_out_alpha = 1.0f;
                TweenFloat(&g_tween_manager, &modal->fade_out_alpha, 0.0f,
                          0.4f, TWEEN_EASE_OUT_QUAD);

                d_LogInfoF("Applied tag %s to card %d - starting animation",
                           GetCardTagName(modal->tags[i]), modal->card_ids[i]);

                return false;  // Don't close yet, show animation
            }
        }

        // Check [Skip All] button
        int skip_w = 120;
        int skip_h = 40;
        int skip_x = modal_x + (REWARD_MODAL_WIDTH - skip_w) / 2;
        int skip_y = modal_y + REWARD_MODAL_HEIGHT - skip_h - 20;

        if (mx >= skip_x && mx <= skip_x + skip_w &&
            my >= skip_y && my <= skip_y + skip_h) {

            d_LogInfo("Player skipped reward");
            return true;  // Close immediately
        }
    }

    // ESC key closes modal (skip reward)
    if (app.keyboard[SDL_SCANCODE_ESCAPE]) {
        app.keyboard[SDL_SCANCODE_ESCAPE] = 0;  // Clear key
        d_LogInfo("ESC pressed - skipping reward");
        return true;  // Close immediately
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderRewardModal(const RewardModal_t* modal) {
    if (!modal || !modal->is_visible) return;

    // Full-screen overlay (matching cardGridModal)
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, COLOR_OVERLAY);

    // Modal panel (shifted 96px right from center)
    int modal_x = ((SCREEN_WIDTH - REWARD_MODAL_WIDTH) / 2) + 96;
    int modal_y = (SCREEN_HEIGHT - REWARD_MODAL_HEIGHT) / 2;

    // Draw panel body
    int panel_body_y = modal_y + REWARD_MODAL_HEADER_HEIGHT;
    int panel_body_height = REWARD_MODAL_HEIGHT - REWARD_MODAL_HEADER_HEIGHT;
    a_DrawFilledRect((aRectf_t){modal_x, panel_body_y, REWARD_MODAL_WIDTH, panel_body_height}, COLOR_PANEL_BG);

    // Draw header
    a_DrawFilledRect((aRectf_t){modal_x, modal_y, REWARD_MODAL_WIDTH, REWARD_MODAL_HEADER_HEIGHT}, COLOR_HEADER_BG);
    a_DrawRect((aRectf_t){modal_x, modal_y, REWARD_MODAL_WIDTH, REWARD_MODAL_HEADER_HEIGHT}, COLOR_HEADER_BORDER);

    // Draw title in header (centered on modal panel, not screen)
    int title_center_x = modal_x + (REWARD_MODAL_WIDTH / 2);
    a_DrawText("CARD UPGRADE - Choose One Card", title_center_x, modal_y + 10,
               (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_HEADER_TEXT.r,COLOR_HEADER_TEXT.g,COLOR_HEADER_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});


    // Informational text using FlexBox positions
    const FlexItem_t* info1 = a_FlexGetItem(modal->info_layout, 0);
    const FlexItem_t* info2 = a_FlexGetItem(modal->info_layout, 1);

    // Text centered on modal panel (not screen)
    int text_center_x = modal_x + (REWARD_MODAL_WIDTH / 2);

    if (info1) {
        a_DrawText("You always have 52 cards, choose one to add a tag to.",
                   text_center_x, info1->calc_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_INFO_TEXT.r,COLOR_INFO_TEXT.g,COLOR_INFO_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }
    if (info2) {
        a_DrawText("Tags are permanent modifiers to card behavior.",
                   text_center_x, info2->calc_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_INFO_TEXT.r,COLOR_INFO_TEXT.g,COLOR_INFO_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    if (!modal->reward_taken) {
        // Draw 3 cards using FlexBox positions (top section)
        for (int i = 0; i < 3; i++) {
            const FlexItem_t* card_item = a_FlexGetItem(modal->card_layout, i);
            if (!card_item) continue;

            int card_x = card_item->calc_x;
            int card_y = card_item->calc_y + 32;

            // Get card data
            CardSuit_t suit;
            CardRank_t rank;
            IDToCard(modal->card_ids[i], &suit, &rank);

            // Draw card texture
            SDL_Texture** tex_ptr = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &modal->card_ids[i]);

            if (tex_ptr && *tex_ptr) {
                SDL_Rect card_rect = {card_x, card_y, CARD_WIDTH, CARD_HEIGHT};
                SDL_RenderCopy(app.renderer, *tex_ptr, NULL, &card_rect);
            } else {
                // Fallback: gray placeholder
                a_DrawFilledRect((aRectf_t){card_x, card_y, CARD_WIDTH, CARD_HEIGHT}, (aColor_t){50, 50, 50, 255});
                a_DrawRect((aRectf_t){card_x, card_y, CARD_WIDTH, CARD_HEIGHT}, (aColor_t){100, 100, 100, 255});
            }

            // Highlight card if corresponding list item is hovered OR key held
            bool is_card_highlighted = (modal->hovered_index == i || modal->key_held_index == i);
            if (is_card_highlighted) {
                a_DrawRect((aRectf_t){card_x - 5, card_y - 5, CARD_WIDTH + 10, CARD_HEIGHT + 10}, (aColor_t){COLOR_TAG_GOLD.r, COLOR_TAG_GOLD.g, COLOR_TAG_GOLD.b, 255});
            }

            // Tag name above card with colored background badge
            int r, g, b;
            GetCardTagColor(modal->tags[i], &r, &g, &b);

            const char* tag_name = GetCardTagName(modal->tags[i]);
            int badge_w = 125;
            int badge_h = 35;
            int badge_x = card_x + (CARD_WIDTH - badge_w) / 2;
            int badge_y = card_y - 42;

            // Badge background (60% opacity if not hovered/key-held, full if highlighted)
            Uint8 badge_alpha = is_card_highlighted ? 255 : (Uint8)(255 * 0.4);

            a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){r, g, b, badge_alpha});
            a_DrawRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){0, 0, 0, badge_alpha});

            // Tag name (black text on colored background)
            a_DrawText((char*)tag_name,
                      card_x + CARD_WIDTH / 2, badge_y - 6,
                      (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={0,0,0,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
        }

        // Draw tag list (bottom section)
        for (int i = 0; i < 3; i++) {
            const FlexItem_t* list_item = a_FlexGetItem(modal->list_layout, i);
            if (!list_item) continue;

            int item_x = list_item->calc_x;
            int item_y = list_item->calc_y;
            int item_w = REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2);
            int item_h = REWARD_LIST_ITEM_HEIGHT;

            // Background color (highlight on hover OR key held)
            bool is_hovered = (modal->hovered_index == i || modal->key_held_index == i);
            aColor_t bg_color = is_hovered ? COLOR_SKIP_HOVER : COLOR_SKIP_BG;

            a_DrawFilledRect((aRectf_t){item_x, item_y, item_w, item_h}, bg_color);
            a_DrawRect((aRectf_t){item_x, item_y, item_w, item_h}, (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, 255});

            // Big number hotkey (left side, no background)
            int number_x = item_x + 25;
            int number_y = item_y + (item_h / 2) - 20;

            // Big number text (1, 2, 3) - white 60% transparent
            char number_str[2];
            snprintf(number_str, sizeof(number_str), "%d", i + 1);

            // Use styled text config for larger size
            a_DrawText(number_str, number_x, number_y,
                      (aTextStyle_t){.type=FONT_GAME, .fg={255,255,255,(Uint8)(255 * 0.6)}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=3.0f, .padding=0});

            // Tag name with colored badge background (shifted right after number)
            int text_padding = 75;  // Shifted right to make room for number
            int r, g, b;
            GetCardTagColor(modal->tags[i], &r, &g, &b);

            int tag_badge_w = 125;
            int tag_badge_h = 35;
            int tag_badge_x = item_x + text_padding;
            int tag_badge_y = item_y + 8;

            // Draw colored badge for tag name
            a_DrawFilledRect((aRectf_t){tag_badge_x, tag_badge_y, tag_badge_w, tag_badge_h}, (aColor_t){r, g, b, 255});
            a_DrawRect((aRectf_t){tag_badge_x, tag_badge_y, tag_badge_w, tag_badge_h}, (aColor_t){0, 0, 0, 255});

            // Tag name (black text on colored background)
            a_DrawText((char*)GetCardTagName(modal->tags[i]),
                      tag_badge_x + tag_badge_w / 2, tag_badge_y - 6,
                      (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={0,0,0,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

            // Card name (e.g., "Queen of Hearts")
            CardSuit_t suit;
            CardRank_t rank;
            IDToCard(modal->card_ids[i], &suit, &rank);

            dString_t* card_name = d_StringInit();
            d_StringFormat(card_name, "%s of %s", GetRankString(rank), GetSuitString(suit));

            a_DrawText((char*)d_StringPeek(card_name),
                      item_x + 250, item_y + 10,
                      (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,255,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});

            d_StringDestroy(card_name);

            // Tag description (gray color)
            a_DrawText((char*)GetCardTagDescription(modal->tags[i]),
                      item_x + text_padding, item_y + 50,
                      (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_INFO_TEXT.r,COLOR_INFO_TEXT.g,COLOR_INFO_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
        }

        // [Skip All] button (red background)
        int skip_w = 120;
        int skip_h = 46;
        int skip_x = modal_x + (REWARD_MODAL_WIDTH - skip_w) / 2;  // Centered horizontally
        int skip_y = modal_y + REWARD_MODAL_HEIGHT - skip_h - 20;   // Bottom with padding

        bool skip_hovered = (app.mouse.x >= skip_x && app.mouse.x <= skip_x + skip_w &&
                            app.mouse.y >= skip_y && app.mouse.y <= skip_y + skip_h);

        aColor_t skip_bg = skip_hovered ? COLOR_CLOSE_HOVER : COLOR_CLOSE_BUTTON;

        a_DrawFilledRect((aRectf_t){skip_x, skip_y, skip_w, skip_h}, skip_bg);
        a_DrawRect((aRectf_t){skip_x, skip_y, skip_w, skip_h}, (aColor_t){COLOR_CLOSE_BUTTON.r, COLOR_CLOSE_BUTTON.g, COLOR_CLOSE_BUTTON.b, 255});

        a_DrawText("Skip", skip_x + skip_w / 2, skip_y + 6,
                  (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={255,255,255,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});

    } else {
        // Animation in progress - render based on stage
        int selected = modal->selected_index;

        // Stage 1: Fade out unselected elements while keeping selected card
        if (modal->anim_stage == ANIM_FADE_OUT) {
            // Draw ALL cards, but fade unselected ones
            for (int i = 0; i < 3; i++) {
                const FlexItem_t* card_item = a_FlexGetItem(modal->card_layout, i);
                if (!card_item) continue;

                int card_x = card_item->calc_x;
                int card_y = card_item->calc_y;

                // Get card texture
                SDL_Texture** tex_ptr = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &modal->card_ids[i]);
                if (tex_ptr && *tex_ptr) {
                    // Set alpha based on selection
                    Uint8 alpha = (i == selected) ? 255 : (Uint8)(modal->fade_out_alpha * 255);
                    SDL_SetTextureAlphaMod(*tex_ptr, alpha);

                    SDL_Rect card_rect = {card_x, card_y, CARD_WIDTH, CARD_HEIGHT};
                    SDL_RenderCopy(app.renderer, *tex_ptr, NULL, &card_rect);

                    // Reset alpha
                    SDL_SetTextureAlphaMod(*tex_ptr, 255);
                }
            }

            // Fade out list items (including borders)
            Uint8 list_alpha = (Uint8)(modal->fade_out_alpha * 255);
            for (int i = 0; i < 3; i++) {
                const FlexItem_t* list_item = a_FlexGetItem(modal->list_layout, i);
                if (!list_item) continue;

                int item_x = list_item->calc_x;
                int item_y = list_item->calc_y;
                int item_w = REWARD_MODAL_WIDTH - (REWARD_MODAL_PADDING * 2);
                int item_h = REWARD_LIST_ITEM_HEIGHT;

                aColor_t bg = (aColor_t){COLOR_SKIP_BG.r, COLOR_SKIP_BG.g, COLOR_SKIP_BG.b, list_alpha};
                a_DrawFilledRect((aRectf_t){item_x, item_y, item_w, item_h}, bg);
                // Fade border with same alpha
                a_DrawRect((aRectf_t){item_x, item_y, item_w, item_h},
                          (aColor_t){COLOR_HEADER_BORDER.r, COLOR_HEADER_BORDER.g, COLOR_HEADER_BORDER.b, list_alpha});
            }
        }

        // Stage 2 & 3: Show scaled card with tag badge
        if (modal->anim_stage == ANIM_SCALE_CARD ||
            modal->anim_stage == ANIM_FADE_IN_TAG ||
            modal->anim_stage == ANIM_COMPLETE) {

            // Get card texture
            SDL_Texture** tex_ptr = (SDL_Texture**)d_GetDataFromTable(g_card_textures, &modal->card_ids[selected]);

            if (tex_ptr && *tex_ptr) {
                // Calculate scaled card dimensions (shifted 96px right from center)
                int scaled_w = (int)(CARD_WIDTH * modal->card_scale);
                int scaled_h = (int)(CARD_HEIGHT * modal->card_scale);
                int centered_x = ((SCREEN_WIDTH - scaled_w) / 2) + 96;
                int centered_y = (SCREEN_HEIGHT - scaled_h) / 2;

                // Render scaled card
                SDL_Rect dest = {centered_x, centered_y, scaled_w, scaled_h};
                SDL_RenderCopy(app.renderer, *tex_ptr, NULL, &dest);

                // Draw tag badge (top-right corner) - only during fade-in stage
                // Match the style of badges above cards
                if (modal->anim_stage >= ANIM_FADE_IN_TAG) {
                    int r, g, b;
                    GetCardTagColor(modal->tags[selected], &r, &g, &b);

                    int badge_w = 110;
                    int badge_h = 35;
                    int badge_x = centered_x + scaled_w - badge_w;
                    int badge_y = centered_y;

                    Uint8 badge_alpha = (Uint8)(modal->tag_badge_alpha * 255);

                    // Badge background (matching card badge style)
                    a_DrawFilledRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){r, g, b, badge_alpha});
                    a_DrawRect((aRectf_t){badge_x, badge_y, badge_w, badge_h}, (aColor_t){0, 0, 0, badge_alpha});

                    // Tag name (full name, black text)
                    const char* tag_name = GetCardTagName(modal->tags[selected]);

                    a_DrawText((char*)tag_name,
                              badge_x + badge_w / 2,
                              badge_y,
                              (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={0,0,0,badge_alpha}, .bg={0,0,0,0}, .align=TEXT_ALIGN_CENTER, .wrap_width=0, .scale=1.0f, .padding=0});
                }
            }
        }
    }
}

// ============================================================================
// QUERIES
// ============================================================================

CardTag_t GetSelectedTag(const RewardModal_t* modal) {
    if (!modal || modal->selected_index < 0 || modal->selected_index >= 3) {
        return CARD_TAG_MAX;  // Invalid/none
    }

    return modal->tags[modal->selected_index];
}

int GetTargetCardID(const RewardModal_t* modal) {
    if (!modal || modal->selected_index < 0 || modal->selected_index >= 3) {
        return -1;
    }

    return modal->card_ids[modal->selected_index];
}
