#ifndef STRUCTS_H
#define STRUCTS_H

#include "defs.h"
#include <SDL2/SDL.h>
#include <Daedalus.h>
#include <stdbool.h>
#include <stddef.h>

// ============================================================================
// CARD STRUCTURE
// ============================================================================

/**
 * Card_t - Value type representing a playing card
 *
 * Constitutional pattern: Card_t is a value type, stored directly in dArray_t
 * Copying by value is intentional - cards are lightweight (32 bytes)
 *
 * IMPORTANT: Fields ordered for optimal alignment on 64-bit systems:
 * - 8-byte pointer first (8-byte aligned)
 * - 4-byte ints next (4-byte aligned)
 * - Enums and bools last
 * - EXPLICIT padding to ensure consistent layout across compilation units
 *
 * Memory layout (32 bytes total):
 *   0-7:   texture pointer (8 bytes)
 *   8-11:  card_id (4 bytes)
 *   12-15: x (4 bytes)
 *   16-19: y (4 bytes)
 *   20-23: suit (4 bytes, enum)
 *   24-27: rank (4 bytes, enum)
 *   28:    face_up (1 byte, bool)
 *   29-31: _padding (3 bytes, explicit)
 */
typedef struct Card {
    SDL_Texture* texture;  // Cached texture pointer (not owned by card) - MUST be first for alignment
    int card_id;           // Unique identifier: 0-51 for standard deck
    int x;                 // Screen X coordinate for rendering
    int y;                 // Screen Y coordinate for rendering
    CardSuit_t suit;       // 0-3: Hearts, Diamonds, Clubs, Spades
    CardRank_t rank;       // 1-13: Ace through King
    bool face_up;          // Visibility flag (true = show face, false = show back)
    char _padding[3];      // Explicit padding to 32 bytes - ensures consistent layout
} Card_t;

// Compile-time verification of struct size
_Static_assert(sizeof(Card_t) == 32, "Card_t must be exactly 32 bytes");

// ============================================================================
// DECK STRUCTURE
// ============================================================================

/**
 * Deck_t - Dynamic deck of cards
 *
 * Uses dArray_t for card storage - no manual malloc/free
 * Multiple decks supported (casino games often use 6-8 decks)
 */
typedef struct Deck {
    dArray_t* cards;        // dArray of Card_t (value types)
    dArray_t* discard_pile; // dArray of Card_t (discarded cards)
    size_t cards_remaining; // Quick count (avoids iterating array)
} Deck_t;

// ============================================================================
// HAND STRUCTURE
// ============================================================================

/**
 * Hand_t - A collection of cards held by a player
 *
 * Cards are stored as value types in dArray_t
 * Only the dArray_t* cards is heap-allocated (managed by Daedalus)
 */
typedef struct Hand {
    dArray_t* cards;      // dArray of Card_t (value types, not pointers)
    int total_value;      // Cached hand value (Blackjack scoring)
    bool is_bust;         // true if total_value > 21
    bool is_blackjack;    // true if natural 21 (Ace + 10-value card)
} Hand_t;

// ============================================================================
// PLAYER STRUCTURE
// ============================================================================

/**
 * Player_t - Represents a player or dealer
 *
 * Constitutional pattern: Hand_t is embedded (value type), not pointer
 */
typedef struct Player {
    dString_t* name;        // Player name (Constitutional: dString_t, not char[])
    int player_id;          // Unique ID (0 = dealer, 1+ = players)
    Hand_t hand;            // Current hand (VALUE TYPE - embedded, not pointer)
    int chips;              // Available chips (also used as HP in combat)
    float display_chips;    // Displayed chips (tweened for smooth HP bar drain)
    int current_bet;        // Amount bet this round
    bool is_dealer;         // true if dealer
    bool is_ai;             // true if AI-controlled
    PlayerState_t state;    // Current player state

    // Portrait system (hybrid for dynamic effects)
    SDL_Surface* portrait_surface;  // Source pixel data (owned, for manipulation)
    SDL_Texture* portrait_texture;  // Cached GPU texture (owned, for rendering)
    bool portrait_dirty;            // true if surface changed, needs texture rebuild

    int sanity;             // Mental state (0-100)
    int max_sanity;         // Max sanity value
} Player_t;

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// Forward declare Enemy_t (defined in enemy.h)
typedef struct Enemy Enemy_t;

// ============================================================================
// CARD HOVER STATE (shared between dealer and player sections)
// ============================================================================

/**
 * CardHoverState_t - Tracks hover animation state for cards
 *
 * Used for smooth hover effects (scale + lift) on cards
 */
typedef struct CardHoverState {
    int hovered_card_index;   // Currently hovered card index (-1 = none)
    float hover_amount;       // 0.0 to 1.0 (tweened for smooth transition)
} CardHoverState_t;

// ============================================================================
// GAME CONTEXT STRUCTURE
// ============================================================================

/**
 * GameContext_t - Global game state and state machine
 *
 * Constitutional pattern: Uses separate typed dTable_t for each state variable type,
 * dArray_t for active player list. No void* chaos - explicit types only.
 */
typedef struct GameContext {
    GameState_t current_state;      // Current state in state machine
    GameState_t previous_state;     // Previous state (for transitions)
    dTable_t* bool_flags;           // Boolean flags (key: char*, value: bool)
    dTable_t* int_values;           // Integer values (key: char*, value: int)
    dTable_t* dealer_phase;         // Dealer phase (key: char*, value: DealerPhase_t)
    dArray_t* active_players;       // Array of int (player IDs in turn order)
    int current_player_index;       // Index into active_players for current turn
    Deck_t* deck;                   // Pointer to game deck
    float state_timer;              // Delta time accumulator for state transitions
    int round_number;               // Current round counter

    // Combat system
    Enemy_t* current_enemy;         // Current combat enemy (NULL if not in combat)
    bool is_combat_mode;            // true if currently in combat encounter
} GameContext_t;

#endif // STRUCTS_H
