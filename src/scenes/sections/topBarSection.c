/*
 * Top Bar Section Implementation
 */

#include "../../../include/scenes/sections/topBarSection.h"
#include "../../../include/scenes/sceneBlackjack.h"

// ============================================================================
// TOP BAR SECTION LIFECYCLE
// ============================================================================

TopBarSection_t* CreateTopBarSection(void) {
    TopBarSection_t* section = malloc(sizeof(TopBarSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate TopBarSection");
        return NULL;
    }

    // Create settings button (positioned at far right)
    // Calculate X position: right-align the button's LEFT edge, not its center
    int button_x = SCREEN_WIDTH - SETTINGS_BUTTON_MARGIN - SETTINGS_BUTTON_WIDTH;
    section->settings_button = CreateButton(button_x, 0,
                                             SETTINGS_BUTTON_WIDTH,
                                             SETTINGS_BUTTON_HEIGHT,
                                             "[S]");

    if (!section->settings_button) {
        free(section);
        d_LogFatal("Failed to create settings button");
        return NULL;
    }

    // Initialize combat showcase
    section->showcase_text = d_StringInit();
    if (!section->showcase_text) {
        DestroyButton(&section->settings_button);
        free(section);
        d_LogFatal("Failed to allocate showcase text");
        return NULL;
    }

    // Initialize versus word pool
    section->versus_words[0] = "versus";
    section->versus_words[1] = "fights";
    section->versus_words[2] = "faces";
    section->versus_words[3] = "against";
    section->versus_words[4] = "battles";

    section->current_versus_index = 0;
    section->versus_timer = 0.0f;

    // Create HP bars (will be positioned in render function)
    section->player_hp_bar = CreateEnemyHealthBar(0, 0, NULL);
    if (!section->player_hp_bar) {
        d_StringDestroy(section->showcase_text);
        DestroyButton(&section->settings_button);
        free(section);
        d_LogFatal("Failed to create player HP bar");
        return NULL;
    }

    section->enemy_hp_bar = CreateEnemyHealthBar(0, 0, NULL);
    if (!section->enemy_hp_bar) {
        DestroyEnemyHealthBar(&section->player_hp_bar);
        d_StringDestroy(section->showcase_text);
        DestroyButton(&section->settings_button);
        free(section);
        d_LogFatal("Failed to create enemy HP bar");
        return NULL;
    }

    d_LogInfo("TopBarSection created");
    return section;
}

void DestroyTopBarSection(TopBarSection_t** section) {
    if (!section || !*section) return;

    TopBarSection_t* bar = *section;

    if (bar->settings_button) {
        DestroyButton(&bar->settings_button);
    }

    if (bar->showcase_text) {
        d_StringDestroy(bar->showcase_text);
    }

    if (bar->player_hp_bar) {
        DestroyEnemyHealthBar(&bar->player_hp_bar);
    }

    if (bar->enemy_hp_bar) {
        DestroyEnemyHealthBar(&bar->enemy_hp_bar);
    }

    free(bar);
    *section = NULL;
    d_LogInfo("TopBarSection destroyed");
}

// ============================================================================
// TOP BAR SECTION UPDATE
// ============================================================================

void UpdateTopBarShowcase(TopBarSection_t* section, float dt,
                          const Player_t* player, const Enemy_t* enemy) {
    if (!section || !section->showcase_text) return;

    // If no enemy, clear showcase and return
    if (!enemy) {
        d_StringClear(section->showcase_text);
        return;
    }

    // Update versus word rotation timer
    section->versus_timer += dt;
    if (section->versus_timer >= VERSUS_WORD_ROTATION_TIME) {
        section->versus_timer = 0.0f;
        section->current_versus_index = (section->current_versus_index + 1) % VERSUS_WORD_COUNT;
    }

    // Rebuild showcase text: "[Player Name] [versus word] [Enemy Name]"
    d_StringClear(section->showcase_text);

    // Append player name
    if (player) {
        const char* player_name = GetPlayerName(player);
        d_StringAppend(section->showcase_text, player_name, strlen(player_name));
    } else {
        d_StringAppend(section->showcase_text, "Unknown", 7);
    }

    // Append space + versus word + space
    d_StringAppend(section->showcase_text, " ", 1);
    const char* versus_word = section->versus_words[section->current_versus_index];
    d_StringAppend(section->showcase_text, versus_word, strlen(versus_word));
    d_StringAppend(section->showcase_text, " ", 1);

    // Append enemy name
    const char* enemy_name = GetEnemyName(enemy);
    d_StringAppend(section->showcase_text, enemy_name, strlen(enemy_name));
}

// ============================================================================
// TOP BAR SECTION RENDERING
// ============================================================================

void RenderTopBarSection(TopBarSection_t* section, const GameContext_t* game, int y) {
    if (!section) return;

    // Draw top bar background
    a_DrawFilledRect((aRectf_t){0, y, SCREEN_WIDTH, TOP_BAR_HEIGHT},
                     (aColor_t){TOP_BAR_BG.r, TOP_BAR_BG.g, TOP_BAR_BG.b, TOP_BAR_BG.a});

    // Draw combat showcase text (left side)
    if (game && game->current_enemy && game->is_combat_mode && section->showcase_text && d_StringGetLength(section->showcase_text) > 0) {
        int text_x = SHOWCASE_TEXT_X_MARGIN;
        int text_y = y + SHOWCASE_TEXT_Y_OFFSET;

        // Draw showcase text "Player faces The Broken Dealer"
        a_DrawText((char*)d_StringPeek(section->showcase_text), text_x, text_y,
                   (aTextStyle_t){.type=// Gold color
                   FONT_ENTER_COMMAND, .fg={232,193,112,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});
    }

    // Update settings button Y position (centered in top bar)
    if (section->settings_button) {
        SetButtonPosition(section->settings_button,
                         section->settings_button->x,
                         y + SETTINGS_BUTTON_Y_OFFSET);
        RenderButton(section->settings_button);
    }
}

// ============================================================================
// TOP BAR SECTION INTERACTION
// ============================================================================

bool IsTopBarSettingsClicked(TopBarSection_t* section) {
    if (!section || !section->settings_button) return false;
    return IsButtonClicked(section->settings_button);
}
