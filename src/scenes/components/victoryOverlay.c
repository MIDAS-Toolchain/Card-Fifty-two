#include "../../../include/scenes/components/victoryOverlay.h"
#include "../../../include/enemy.h"

// ============================================================================
// LIFECYCLE
// ============================================================================

VictoryOverlay_t* CreateVictoryOverlay(void) {
    VictoryOverlay_t* overlay = malloc(sizeof(VictoryOverlay_t));
    if (!overlay) {
        d_LogError("Failed to allocate VictoryOverlay_t");
        return NULL;
    }

    overlay->visible = false;
    overlay->enemy_name = d_StringInit();

    d_LogInfo("Victory overlay created");
    return overlay;
}

void DestroyVictoryOverlay(VictoryOverlay_t** overlay) {
    if (!overlay || !*overlay) return;

    d_StringDestroy((*overlay)->enemy_name);
    free(*overlay);
    *overlay = NULL;

    d_LogInfo("Victory overlay destroyed");
}

// ============================================================================
// DISPLAY
// ============================================================================

void ShowVictoryOverlay(VictoryOverlay_t* overlay, Enemy_t* defeated_enemy) {
    if (!overlay) return;

    overlay->visible = true;

    // Cache enemy name to avoid repeated lookups
    if (defeated_enemy) {
        const char* enemy_name = GetEnemyName(defeated_enemy);
        d_StringClear(overlay->enemy_name);
        d_StringFormat(overlay->enemy_name, "%s", enemy_name);
        d_LogInfoF("Victory overlay shown for enemy: %s", enemy_name);
    } else {
        d_StringClear(overlay->enemy_name);
        d_StringFormat(overlay->enemy_name, "the enemy");
        d_LogWarning("ShowVictoryOverlay called with NULL enemy - using default name");
    }
}

void HideVictoryOverlay(VictoryOverlay_t* overlay) {
    if (!overlay) return;
    overlay->visible = false;
    d_LogInfo("Victory overlay hidden");
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderVictoryOverlay(const VictoryOverlay_t* overlay) {
    if (!overlay || !overlay->visible) return;

    // Dark fullscreen overlay
    a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, (aColor_t){0, 0, 0, 200});

    // Calculate centered positions
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2 - 50;

    // "VICTORY!" title (gold, large, centered)
    aTextStyle_t gold_title = {
        .type = FONT_ENTER_COMMAND,
        .fg = {232, 193, 112, 255},  // Gold
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.5f
    };
    a_DrawText("VICTORY!", center_x, center_y, gold_title);

    // "You defeated [Enemy]!" message (off-white, centered)
    dString_t* victory_msg = d_StringInit();
    d_StringFormat(victory_msg, "You defeated %s!", (char*)d_StringPeek(overlay->enemy_name));

    aTextStyle_t off_white = {
        .type = FONT_ENTER_COMMAND,
        .fg = {235, 237, 233, 255},  // Off-white
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText((char*)d_StringPeek(victory_msg), center_x, center_y + 50, off_white);
    d_StringDestroy(victory_msg);

    // Note: FlexBox result screen effects (Win, Cleansed!) are rendered
    // by resultScreen.c, not here. This component only handles the
    // victory celebration overlay (title + defeat message).
}

// ============================================================================
// STATE
// ============================================================================

bool IsVictoryOverlayVisible(const VictoryOverlay_t* overlay) {
    return overlay && overlay->visible;
}
