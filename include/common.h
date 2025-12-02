#ifndef COMMON_H
#define COMMON_H

// ============================================================================
// EXTERNAL LIBRARIES
// ============================================================================

#include <Archimedes.h>
#include <Daedalus.h>

// ============================================================================
// STANDARD C LIBRARIES
// ============================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// ============================================================================
// CROSS-PLATFORM SUPPORT
// ============================================================================

#ifdef __EMSCRIPTEN__
    #include <emscripten.h>
#endif

// ============================================================================
// PROJECT HEADERS
// ============================================================================

#include "defs.h"
#include "structs.h"

// ============================================================================
// GLOBAL VARIABLES (declared here, defined in main.c)
// ============================================================================

// Player registry: Key = int (player_id), Value = Player_t*
extern dTable_t* g_players;

// Archimedes image cache: Key = int (card_id), Value = aImage_t* (managed by Archimedes, auto-cleanup)
extern dTable_t* g_card_textures;

// Portrait surface cache: Key = int (player_id), Value = SDL_Surface*
extern dTable_t* g_portraits;

// Card back surface
extern aImage_t* g_card_back_texture;

// Ability icon textures: Key = int (EnemyAbility_t), Value = SDL_Texture*
// Falls back to text abbreviations if texture is NULL
extern dTable_t* g_ability_icons;

// Enemy database: Parsed DUF file containing all enemy definitions
// Loaded at startup, used to create enemies by key (e.g., "didact", "daemon")
extern dDUFValue_t* g_enemies_db;

// Global settings: Loaded at startup, persisted to settings.duf
// Accessible by all systems (audio, visual effects, UI, etc.)
// Forward declaration (full definition in settings.h)
typedef struct Settings_t Settings_t;
extern Settings_t* g_settings;

// ============================================================================
// WINDOW SIZE HELPERS (for resolution-independent UI)
// ============================================================================

/**
 * Get current window width (runtime, not compile-time constant)
 * Use this instead of SCREEN_WIDTH for resolution-independent layouts
 */
int GetWindowWidth(void);

/**
 * Get current window height (runtime, not compile-time constant)
 * Use this instead of SCREEN_HEIGHT for resolution-independent layouts
 */
int GetWindowHeight(void);

/**
 * Get UI scale multiplier based on current settings
 * Returns: 1.0f (100%), 1.25f (125%), or 1.5f (150%)
 * Use this to scale text and UI elements for better readability
 */
float GetUIScale(void);

/**
 * Get card scale multiplier based on resolution
 * Returns: 1.0f for 720p/768p, 1.2f for 900p (20% bigger cards)
 * Use this to scale card rendering on larger screens
 */
float GetCardScale(void);

// ============================================================================
// GLOBAL UI SOUND EFFECTS (defined in main.c)
// ============================================================================

extern aSoundEffect_t g_ui_hover_sound;   // Button hover sound
extern aSoundEffect_t g_ui_click_sound;   // Button click sound

// Card slide sound effects (8 variants with no-repeat system)
extern aSoundEffect_t g_card_slide_sounds[CARD_SLIDE_SOUND_COUNT];
extern int g_last_card_slide_index;  // Track last played sound for no-repeat

// ============================================================================
// GLOBAL FONT STYLES (defined in main.c)
// ============================================================================

extern aTextStyle_t FONT_STYLE_TITLE;       // Large centered white text
extern aTextStyle_t FONT_STYLE_BODY;        // Default body text
extern aTextStyle_t FONT_STYLE_CHIP_COUNT;  // Chip count display
extern aTextStyle_t FONT_STYLE_DEBUG;       // FPS and debug info
extern aTextStyle_t FONT_STYLE_DAMAGE;      // Damage numbers

#endif // COMMON_H
