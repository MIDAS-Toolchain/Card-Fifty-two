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

// Texture cache: Key = int (card_id), Value = SDL_Texture*
extern dTable_t* g_card_textures;

// Portrait texture cache: Key = int (player_id), Value = SDL_Texture*
extern dTable_t* g_portraits;

// Card back texture
extern SDL_Texture* g_card_back_texture;

// ============================================================================
// GLOBAL FONT STYLES (defined in main.c)
// ============================================================================

extern aFontConfig_t FONT_STYLE_TITLE;       // Large centered white text
extern aFontConfig_t FONT_STYLE_BODY;        // Default body text
extern aFontConfig_t FONT_STYLE_CHIP_COUNT;  // Chip count display
extern aFontConfig_t FONT_STYLE_DEBUG;       // FPS and debug info
extern aFontConfig_t FONT_STYLE_DAMAGE;      // Damage numbers

#endif // COMMON_H
