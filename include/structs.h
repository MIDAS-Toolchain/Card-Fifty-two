#ifndef STRUCTS_H
#define STRUCTS_H

#include "defs.h"
#include "stateStorage.h"
#include <SDL2/SDL.h>
#include <Daedalus.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations (for circular dependencies)
typedef struct Player Player_t;
typedef struct GameContext GameContext_t;
typedef struct Card Card_t;
typedef struct Hand Hand_t;
typedef struct Deck Deck_t;

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

// Now that Card_t, Hand_t, Deck_t are defined, we can include game.h for GameEvent_t
#include "game.h"

// ============================================================================
// FORWARD DECLARATIONS (needed before Player_t)
// ============================================================================

// Forward declare Enemy_t (defined in enemy.h)
typedef struct Enemy Enemy_t;

// Forward declare StatusEffectManager_t (defined in statusEffects.h)
typedef struct StatusEffectManager StatusEffectManager_t;

// Forward declare Act_t (defined in act.h)
typedef struct Act Act_t;

// ============================================================================
// TRINKET ENUMS AND STRUCTURE (moved from trinket.h to resolve circular dependency)
// ============================================================================

/**
 * TrinketRarity_t - Trinket rarity tiers
 */
typedef enum {
    TRINKET_RARITY_COMMON,      // Common (white)
    TRINKET_RARITY_UNCOMMON,    // Uncommon (green)
    TRINKET_RARITY_RARE,        // Rare (blue)
    TRINKET_RARITY_LEGENDARY,   // Legendary (gold)
    TRINKET_RARITY_CLASS,       // Class trinket (purple - unique to character class)
} TrinketRarity_t;

/**
 * TrinketTargetType_t - What can trinket actives target?
 */
typedef enum {
    TRINKET_TARGET_NONE,        // No targeting (self-buff)
    TRINKET_TARGET_CARD,        // Target a card (player or dealer)
    TRINKET_TARGET_ENEMY,       // Target the enemy
    TRINKET_TARGET_HAND,        // Target entire hand (yours or dealer's)
} TrinketTargetType_t;

/**
 * Trinket_t - Equipment with passive and active effects
 *
 * Constitutional Amendment #1 (2025-11-14): VALUE TYPE (like Card_t, StatusEffectInstance_t)
 * - Global registry (g_trinket_templates) stores templates as VALUES
 * - Players store copies in trinket_slots (dArray_t of Trinket_t)
 * - Equipping COPIES template → player slot (each player has own instance)
 * - Passives triggered by GameEvent_t (reuse existing system)
 * - Actives manually triggered with targeting
 * - Tweening uses TweenFloatInArray() with slot_index (NO dangling pointers!)
 */
typedef struct Trinket {
    int trinket_id;                    // Unique ID (0-N)
    dString_t* name;                   // "Degenerate's Gambit"
    dString_t* description;            // Full description

    // Passive (event-triggered)
    GameEvent_t passive_trigger;       // Which event triggers passive?
    void (*passive_effect)(struct Player* player, GameContext_t* game, struct Trinket* self, size_t slot_index);
    dString_t* passive_description;    // "When you HIT on 15+: Deal X damage"

    // Active (player-activated)
    TrinketTargetType_t active_target_type;
    void (*active_effect)(struct Player* player, GameContext_t* game, void* target, struct Trinket* self, size_t slot_index);
    int active_cooldown_max;           // Turns until reusable
    int active_cooldown_current;       // Current cooldown (0 = ready)
    dString_t* active_description;     // "Double a card ≤5"

    // State tracking (per-trinket scaling)
    int passive_damage_bonus;          // For Degenerate: +5 per active use
    int total_damage_dealt;            // Total damage dealt this combat (for stats)

    // Animation state (for shake/flash on proc - matches status effects/abilities)
    float shake_offset_x;              // X shake offset (tweened)
    float shake_offset_y;              // Y shake offset (tweened)
    float flash_alpha;                 // Red flash alpha (tweened, 0-255)

    // Additional fields (added at END to preserve binary compatibility)
    int total_bonus_chips;             // For Elite Membership: Chips won via win bonus
    int total_refunded_chips;          // For Elite Membership: Chips refunded via loss protection
    TrinketRarity_t rarity;            // Rarity tier (common/uncommon/rare/legendary)
    // Future: Add more state fields HERE (at end) to avoid breaking offsets

} Trinket_t;

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

    // Status effects system (token manipulation debuffs)
    StatusEffectManager_t* status_effects;  // Heap-allocated status effect manager

    // Class system
    PlayerClass_t class;           // Character class (Degenerate, Dealer, Detective, Dreamer)
    Trinket_t class_trinket;       // Class-specific trinket (VALUE - own copy of template)
    bool has_class_trinket;        // true if class trinket is equipped
    dArray_t* trinket_slots;       // Array of Trinket_t VALUES (6 slots max, each is own copy)

    // Combat stats (calculated from tags/trinkets) - ADR-09: Dirty-flag aggregation
    int damage_flat;               // Flat damage bonus (added to base damage)
    int damage_percent;            // Percentage damage increase (multiplies base damage)
    int crit_chance;               // % chance to crit (0-100, LUCKY tag gives +10% per card)
    int crit_bonus;                // % bonus damage on crit (e.g., 50 = +50% damage)
    bool combat_stats_dirty;       // true = recalculate stats from hand tags
} Player_t;

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
 * Constitutional pattern: Composition over inheritance
 * - Embeds GameStateData_t for typed state storage
 * - Uses dArray_t for active player list
 * - No void* chaos - explicit types only
 */
typedef struct GameContext {
    GameState_t current_state;      // Current state in state machine
    GameState_t previous_state;     // Previous state (for transitions)
    GameStateData_t state_data;     // State variable storage (embedded)
    dArray_t* active_players;       // Array of int (player IDs in turn order)
    int current_player_index;       // Index into active_players for current turn
    Deck_t* deck;                   // Pointer to game deck
    float state_timer;              // Delta time accumulator for state transitions
    int round_number;               // Current round counter

    // Combat system
    Enemy_t* current_enemy;         // Current combat enemy (NULL if not in combat)
    bool is_combat_mode;            // true if currently in combat encounter
    float next_enemy_hp_multiplier; // HP multiplier for next spawned enemy (1.0 = normal, set by event consequences)

    // Act progression system (roguelike structure)
    Act_t* current_act;             // Current act with encounter sequence (NULL if not started)

    // Event preview system (reroll mechanic)
    int event_reroll_base_cost;     // Base reroll cost (50 chips, configurable)
    int event_reroll_cost;          // Current reroll cost (doubles each use: 50→100→200→400)
    int event_rerolls_used;         // Reroll count for this preview (stats tracking)
    float event_preview_timer;      // 3.0 → 0.0 countdown (auto-proceed when 0)

    // Combat preview system (elite/boss warning)
    float combat_preview_timer;     // 3.0 → 0.0 countdown (auto-proceed when 0)
} GameContext_t;

#endif // STRUCTS_H
