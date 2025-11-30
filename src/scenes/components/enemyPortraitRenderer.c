/*
 * Enemy Portrait Renderer Component Implementation
 * Renders enemy portrait with shake, red flash, and defeat fade effects
 */

#include "../../../include/scenes/components/enemyPortraitRenderer.h"
#include "../../../include/tween/tween.h"

// ============================================================================
// LIFECYCLE
// ============================================================================

EnemyPortraitRenderer_t* CreateEnemyPortraitRenderer(int x, int y, int target_w, int target_h, Enemy_t* enemy) {
    EnemyPortraitRenderer_t* renderer = malloc(sizeof(EnemyPortraitRenderer_t));
    if (!renderer) {
        d_LogFatal("Failed to allocate EnemyPortraitRenderer");
        return NULL;
    }

    renderer->x = x;
    renderer->y = y;
    renderer->target_w = target_w;
    renderer->target_h = target_h;
    renderer->enemy = enemy;  // Reference only, not owned (can be NULL initially)

    d_LogInfo("EnemyPortraitRenderer created");
    return renderer;
}

void DestroyEnemyPortraitRenderer(EnemyPortraitRenderer_t** renderer) {
    if (!renderer || !*renderer) return;

    free(*renderer);
    *renderer = NULL;
    d_LogInfo("EnemyPortraitRenderer destroyed");
}

// ============================================================================
// SETTERS
// ============================================================================

void SetEnemyPortraitEnemy(EnemyPortraitRenderer_t* renderer, Enemy_t* enemy) {
    if (!renderer) return;
    renderer->enemy = enemy;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEnemyPortrait(const EnemyPortraitRenderer_t* renderer) {
    if (!renderer || !renderer->enemy) return;

    // Get portrait texture (auto-converts from surface if dirty)
    SDL_Texture* portrait = GetEnemyPortraitTexture(renderer->enemy);
    if (!portrait) return;

    // Get shake offset for damage feedback
    float shake_x, shake_y;
    GetEnemyShakeOffset(renderer->enemy, &shake_x, &shake_y);

    // Get flash alphas for damage/heal feedback
    float red_alpha = GetEnemyRedFlashAlpha(renderer->enemy);
    float green_alpha = GetEnemyGreenFlashAlpha(renderer->enemy);

    // Get defeat animation state (fade + zoom out)
    float defeat_alpha = GetEnemyDefeatAlpha(renderer->enemy);
    float defeat_scale = GetEnemyDefeatScale(renderer->enemy);

    // Apply defeat scale to dimensions
    int final_w = (int)(renderer->target_w * defeat_scale);
    int final_h = (int)(renderer->target_h * defeat_scale);

    // Center the scaled portrait (so zoom-out is centered, not top-left)
    int scale_offset_x = 0;  // No horizontal drift during fade
    int scale_offset_y = (renderer->target_h - final_h) / 2;

    // Calculate final position with all offsets
    int x = renderer->x + (int)shake_x + scale_offset_x;
    int y = renderer->y + (int)shake_y + scale_offset_y;

    // Apply color flash effects (red for damage, green for heal)
    if (red_alpha > 0) {
        SDL_SetTextureColorMod(portrait, 255, (Uint8)(255 * (1.0f - red_alpha)), (Uint8)(255 * (1.0f - red_alpha)));
    } else if (green_alpha > 0) {
        SDL_SetTextureColorMod(portrait, (Uint8)(255 * (1.0f - green_alpha)), 255, (Uint8)(255 * (1.0f - green_alpha)));
    } else {
        SDL_SetTextureColorMod(portrait, 255, 255, 255);
    }

    // Apply defeat fade effect
    SDL_SetTextureAlphaMod(portrait, (Uint8)(255 * defeat_alpha));

    // Render texture using SDL_RenderCopy (scales automatically to fit dest rect)
    SDL_Rect dest = {(int)x, (int)y, (int)final_w, (int)final_h};
    SDL_RenderCopy(app.renderer, portrait, NULL, &dest);

    // Reset texture mods for next frame
    SDL_SetTextureColorMod(portrait, 255, 255, 255);
    SDL_SetTextureAlphaMod(portrait, 255);
}
