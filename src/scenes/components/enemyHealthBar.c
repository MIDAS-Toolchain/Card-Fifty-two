/*
 * Enemy Health Bar Component Implementation
 * Displays enemy HP with filled bar and HP text
 */

#include "../../../include/scenes/components/enemyHealthBar.h"

// Color constants
#define COLOR_BG      ((aColor_t){40, 40, 40, 255})      // #282828 - dark gray
#define COLOR_FILL    ((aColor_t){165, 48, 48, 255})     // #a53030 - red
#define COLOR_BORDER  ((aColor_t){235, 237, 233, 255})   // #ebede9 - off-white
#define COLOR_TEXT    ((aColor_t){235, 237, 233, 255})   // #ebede9 - off-white

// ============================================================================
// LIFECYCLE
// ============================================================================

EnemyHealthBar_t* CreateEnemyHealthBar(int x, int y, Enemy_t* enemy) {
    EnemyHealthBar_t* bar = malloc(sizeof(EnemyHealthBar_t));
    if (!bar) {
        d_LogFatal("Failed to allocate EnemyHealthBar");
        return NULL;
    }

    bar->x = x;
    bar->y = y;
    bar->w = ENEMY_HP_BAR_WIDTH;
    bar->h = ENEMY_HP_BAR_HEIGHT;
    bar->enemy = enemy;  // Reference only, not owned (can be NULL initially)

    d_LogInfo("EnemyHealthBar created");
    return bar;
}

void DestroyEnemyHealthBar(EnemyHealthBar_t** bar) {
    if (!bar || !*bar) return;

    free(*bar);
    *bar = NULL;
    d_LogInfo("EnemyHealthBar destroyed");
}

// ============================================================================
// SETTERS
// ============================================================================

void SetEnemyHealthBarPosition(EnemyHealthBar_t* bar, int x, int y) {
    if (!bar) return;
    bar->x = x;
    bar->y = y;
}

void SetEnemyHealthBarEnemy(EnemyHealthBar_t* bar, Enemy_t* enemy) {
    if (!bar) return;
    bar->enemy = enemy;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderEnemyHealthBar(const EnemyHealthBar_t* bar) {
    if (!bar || !bar->enemy) return;

    // Calculate HP percentage using display_hp (smoothly tweened value)
    float hp_percent = bar->enemy->display_hp / (float)bar->enemy->max_hp;
    if (hp_percent < 0.0f) hp_percent = 0.0f;
    if (hp_percent > 1.0f) hp_percent = 1.0f;

    int filled_width = (int)(bar->w * hp_percent);

    // Draw background (dark gray)
    a_DrawFilledRect((aRectf_t){bar->x, bar->y, bar->w, bar->h}, COLOR_BG);

    // Draw filled portion (red) - animates smoothly
    a_DrawFilledRect((aRectf_t){bar->x, bar->y, filled_width, bar->h}, COLOR_FILL);

    // Draw green flash overlay when enemy is healed
    float green_alpha = GetEnemyGreenFlashAlpha(bar->enemy);
    if (green_alpha > 0.0f) {
        Uint8 flash = (Uint8)(green_alpha * 255.0f);
        a_DrawFilledRect((aRectf_t){bar->x, bar->y, filled_width, bar->h},
                        (aColor_t){100, 255, 100, flash});
    }

    // Draw border (off-white)
    a_DrawRect((aRectf_t){bar->x, bar->y, bar->w, bar->h}, COLOR_BORDER);

    // Draw HP text to the right of the bar (show actual HP, not display_hp)
    dString_t* hp_text = d_StringInit();
    d_StringFormat(hp_text, "%d/%d", bar->enemy->current_hp, bar->enemy->max_hp);

    int text_x = bar->x + bar->w + 10;
    int text_y = bar->y;

    a_DrawText((char*)d_StringPeek(hp_text), text_x, text_y,
                   (aTextStyle_t){.type=FONT_ENTER_COMMAND, .fg={COLOR_TEXT.r,COLOR_TEXT.g,COLOR_TEXT.b,255}, .bg={0,0,0,0}, .align=TEXT_ALIGN_LEFT, .wrap_width=0, .scale=1.0f, .padding=0});

    d_StringDestroy(hp_text);
}
