#ifndef DEFS_H
#define DEFS_H

// ============================================================================
// SCREEN CONFIGURATION
// ============================================================================

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

// ============================================================================
// CARD CONFIGURATION
// ============================================================================

#define CARD_WIDTH 100
#define CARD_HEIGHT 140

// ============================================================================
// GAME CONSTANTS
// ============================================================================

#define MAX_PLAYERS 6
#define STARTING_CHIPS 100

// ============================================================================
// ENUMS
// ============================================================================

// Card suits
typedef enum {
    SUIT_HEARTS = 0,    // ♥
    SUIT_DIAMONDS,      // ♦
    SUIT_CLUBS,         // ♣
    SUIT_SPADES,        // ♠
    SUIT_MAX
} CardSuit_t;

// Card ranks
typedef enum {
    RANK_ACE = 1,
    RANK_TWO, RANK_THREE, RANK_FOUR, RANK_FIVE,
    RANK_SIX, RANK_SEVEN, RANK_EIGHT, RANK_NINE,
    RANK_TEN, RANK_JACK, RANK_QUEEN, RANK_KING,
    RANK_MAX
} CardRank_t;

// Game states
typedef enum {
    STATE_MENU,          // Main menu
    STATE_BETTING,       // Players place bets
    STATE_DEALING,       // Initial card deal
    STATE_PLAYER_TURN,   // Player actions (hit/stand)
    STATE_DEALER_TURN,   // Dealer plays by rules
    STATE_SHOWDOWN,      // Compare hands, payout
    STATE_ROUND_END,     // Display results
    STATE_COMBAT_VICTORY // Enemy defeated, show victory screen
} GameState_t;

// Player states
typedef enum {
    PLAYER_STATE_WAITING,
    PLAYER_STATE_BETTING,
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_STAND,
    PLAYER_STATE_BUST,
    PLAYER_STATE_BLACKJACK,
    PLAYER_STATE_WON,
    PLAYER_STATE_LOST,
    PLAYER_STATE_PUSH
} PlayerState_t;

// Player actions
typedef enum {
    ACTION_HIT,      // Take another card
    ACTION_STAND,    // End turn
    ACTION_DOUBLE,   // Double bet, take one card, end turn
    ACTION_SPLIT     // Split pairs (future)
} PlayerAction_t;

// Dealer turn phases
typedef enum {
    DEALER_PHASE_CHECK_REVEAL,  // Check if we should reveal (player stood?)
    DEALER_PHASE_DECIDE,        // Make hit/stand decision
    DEALER_PHASE_ACTION,        // Execute action (hit or stand-with-reveal)
    DEALER_PHASE_WAIT           // Wait 0.5s before next decision
} DealerPhase_t;

#endif // DEFS_H
