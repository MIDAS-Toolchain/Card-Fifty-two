/*
 * Discard Pile Modal Section Implementation
 */

#include "../../../include/scenes/sections/discardPileModalSection.h"

// ============================================================================
// LIFECYCLE
// ============================================================================

DiscardPileModalSection_t* CreateDiscardPileModalSection(Deck_t* deck) {
    if (!deck) {
        d_LogError("CreateDiscardPileModalSection: Invalid deck");
        return NULL;
    }

    DiscardPileModalSection_t* section = malloc(sizeof(DiscardPileModalSection_t));
    if (!section) {
        d_LogFatal("Failed to allocate DiscardPileModalSection");
        return NULL;
    }

    section->deck = deck;

    // Create modal with ordered display
    section->modal = CreateCardGridModal("Discard Pile", deck->discard_pile, false);
    if (!section->modal) {
        free(section);
        d_LogError("Failed to create CardGridModal for discard pile");
        return NULL;
    }

    d_LogInfo("DiscardPileModalSection created");
    return section;
}

void DestroyDiscardPileModalSection(DiscardPileModalSection_t** section) {
    if (!section || !*section) return;

    DiscardPileModalSection_t* s = *section;

    if (s->modal) {
        DestroyCardGridModal(&s->modal);
    }

    free(s);
    *section = NULL;
    d_LogInfo("DiscardPileModalSection destroyed");
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowDiscardPileModal(DiscardPileModalSection_t* section) {
    if (!section || !section->modal) return;
    ShowCardGridModal(section->modal);
}

void HideDiscardPileModal(DiscardPileModalSection_t* section) {
    if (!section || !section->modal) return;
    HideCardGridModal(section->modal);
}

bool IsDiscardPileModalVisible(const DiscardPileModalSection_t* section) {
    if (!section || !section->modal) return false;
    return section->modal->is_visible;
}

// ============================================================================
// INPUT AND RENDERING
// ============================================================================

void HandleDiscardPileModalInput(DiscardPileModalSection_t* section) {
    if (!section || !section->modal) return;

    if (HandleCardGridModalInput(section->modal)) {
        HideDiscardPileModal(section);
    }
}

void RenderDiscardPileModalSection(DiscardPileModalSection_t* section) {
    if (!section || !section->modal) return;
    RenderCardGridModal(section->modal);
}
