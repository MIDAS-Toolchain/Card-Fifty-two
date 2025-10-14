/*
 * Draw Pile Modal Section Implementation
 */

#include "../../../include/scenes/sections/drawPileModalSection.h"

// ============================================================================
// LIFECYCLE
// ============================================================================

DrawPileModalSection_t* CreateDrawPileModalSection(Deck_t* deck) {
    if (!deck) {
        d_LogError("CreateDrawPileModalSection: Invalid deck");
        return NULL;
    }

    DrawPileModalSection_t* section = malloc(sizeof(DrawPileModalSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate DrawPileModalSection");
        return NULL;
    }

    section->deck = deck;

    // Create modal with shuffled display
    section->modal = CreateCardGridModal("Draw Pile (Randomized)", deck->cards, true);
    if (!section->modal) {
        free(section);
        d_LogError("Failed to create CardGridModal for draw pile");
        return NULL;
    }

    d_LogInfo("DrawPileModalSection created");
    return section;
}

void DestroyDrawPileModalSection(DrawPileModalSection_t** section) {
    if (!section || !*section) return;

    DrawPileModalSection_t* s = *section;

    if (s->modal) {
        DestroyCardGridModal(&s->modal);
    }

    free(s);
    *section = NULL;
    d_LogInfo("DrawPileModalSection destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowDrawPileModal(DrawPileModalSection_t* section) {
    if (!section || !section->modal) return;
    ShowCardGridModal(section->modal);
}

void HideDrawPileModal(DrawPileModalSection_t* section) {
    if (!section || !section->modal) return;
    HideCardGridModal(section->modal);
}

bool IsDrawPileModalVisible(const DrawPileModalSection_t* section) {
    if (!section || !section->modal) return false;
    return section->modal->is_visible;
}

// ============================================================================
// INPUT AND RENDERING
// ============================================================================

void HandleDrawPileModalInput(DrawPileModalSection_t* section) {
    if (!section || !section->modal) return;

    if (HandleCardGridModalInput(section->modal)) {
        HideDrawPileModal(section);
    }
}

void RenderDrawPileModalSection(DrawPileModalSection_t* section) {
    if (!section || !section->modal) return;
    RenderCardGridModal(section->modal);
}
