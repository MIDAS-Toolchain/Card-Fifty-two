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

// Card back texture
extern SDL_Texture* g_card_back_texture;

#endif // COMMON_H
