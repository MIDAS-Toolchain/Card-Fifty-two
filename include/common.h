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

// Surface cache: Key = int (card_id), Value = SDL_Surface*
extern dTable_t* g_card_textures;

// Portrait surface cache: Key = int (player_id), Value = SDL_Surface*
extern dTable_t* g_portraits;

// Card back surface
extern SDL_Surface* g_card_back_texture;

// Ability icon textures: Key = int (EnemyAbility_t), Value = SDL_Texture*
// Falls back to text abbreviations if texture is NULL
extern dTable_t* g_ability_icons;

// Enemy database: Parsed DUF file containing all enemy definitions
// Loaded at startup, used to create enemies by key (e.g., "didact", "daemon")
extern dDUFValue_t* g_enemies_db;

// Global settings: Loaded at startup, persisted to settings.duf
// Accessible by all systems (audio, visual effects, UI, etc.)
// Forward declaration (full definition in settings.h)
struct Settings_t;
extern struct Settings_t* g_settings;

// ============================================================================
// GLOBAL UI SOUND EFFECTS (defined in main.c)
// ============================================================================

extern aAudioClip_t g_ui_hover_sound;   // Button hover sound
extern aAudioClip_t g_ui_click_sound;   // Button click sound

// ============================================================================
// GLOBAL FONT STYLES (defined in main.c)
// ============================================================================

extern aTextStyle_t FONT_STYLE_TITLE;       // Large centered white text
extern aTextStyle_t FONT_STYLE_BODY;        // Default body text
extern aTextStyle_t FONT_STYLE_CHIP_COUNT;  // Chip count display
extern aTextStyle_t FONT_STYLE_DEBUG;       // FPS and debug info
extern aTextStyle_t FONT_STYLE_DAMAGE;      // Damage numbers

#endif // COMMON_H
