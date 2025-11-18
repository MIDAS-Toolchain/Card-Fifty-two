/*
 * Title Section Implementation
 */

#include "../../../include/scenes/sections/titleSection.h"
#include "../../../include/scenes/sceneBlackjack.h"
#include "../../../include/state.h"

// ============================================================================
// TITLE SECTION LIFECYCLE
// ============================================================================

TitleSection_t* CreateTitleSection(void) {
    TitleSection_t* section = malloc(sizeof(TitleSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate TitleSection");
        return NULL;
    }

    section->layout = NULL;  // Reserved for future FlexBox refinement

    d_LogInfo("TitleSection created");
    return section;
}

void DestroyTitleSection(TitleSection_t** section) {
    if (!section || !*section) return;

    if ((*section)->layout) {
        a_DestroyFlexBox(&(*section)->layout);
    }

    free(*section);
    *section = NULL;
    d_LogInfo("TitleSection destroyed");
}

// ============================================================================
// TITLE SECTION RENDERING
// ============================================================================

void RenderTitleSection(TitleSection_t* section, const GameContext_t* game, int y) {
    if (!section || !game) return;

    // Title at top of section
    a_DrawText("Blackjack", SCREEN_WIDTH / 2, y + TITLE_TEXT_Y_OFFSET,
               255, 255, 255, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);

    // State info below title
    dString_t* state_str = d_StringInit();
    d_StringFormat(state_str, "State: %s | Round: %d",
                   State_ToString(game->current_state), game->round_number);
    a_DrawText((char*)d_StringPeek(state_str), SCREEN_WIDTH / 2, y + STATE_TEXT_Y_OFFSET,
               200, 200, 200, FONT_ENTER_COMMAND, TEXT_ALIGN_CENTER, 0);
    d_StringDestroy(state_str);
}
