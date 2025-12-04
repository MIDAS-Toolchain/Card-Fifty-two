#ifndef STRUCTS_H
#define STRUCTS_H

#include "defs.h"
#include "stateStorage.h"
#include <SDL2/SDL.h>
#include <Archimedes.h>  // MUST come before using aImage_t!
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
    aImage_t* texture;     // Cached Archimedes image pointer (not owned by card) - MUST be first for alignment
    int card_id;           // Unique identifier: 0-51 for standard deck
    int x;                 // Screen X coordinate for rendering
    int y;                 // Screen Y coordinate for rendering
    CardSuit_t suit;       // 0-3: Hearts, Diamonds, Clubs, Spades
    CardRank_t rank;       // 1-13: Ace through King
    bool face_up;          // Visibility flag (true = show face, false = show back)
    char _padding[3];      // Explicit padding to 32 bytes - ensures consistent layout
} Card_t;

// Compile-time verification of struct size
// NOTE: Size varies by platform (32 bytes on 64-bit native, 28 bytes on WASM32)
// This is expected due to pointer size differences (8 bytes vs 4 bytes)
#ifdef __EMSCRIPTEN__
_Static_assert(sizeof(Card_t) == 28, "Card_t must be exactly 28 bytes on WASM32");
#else
_Static_assert(sizeof(Card_t) == 32, "Card_t must be exactly 32 bytes on native 64-bit");
#endif

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
    TRINKET_RARITY_COMMON,      // Common (white #C8C8C8)
    TRINKET_RARITY_UNCOMMON,    // Uncommon (green #64FF64)
    TRINKET_RARITY_RARE,        // Rare (blue #6496FF)
    TRINKET_RARITY_LEGENDARY,   // Legendary (gold #FFD700)
    TRINKET_RARITY_EVENT,       // Event (teal #64FFFF - event-only, not combat drops)
    TRINKET_RARITY_CLASS,       // Class trinket (purple #B464FF - hardcoded, cannot sell)
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
// TRINKET DUF SYSTEM (Affix-based loot drops)
// ============================================================================

/**
 * TrinketEffectType_t - Types of effects trinkets can have
 *
 * Used for data-driven trinkets loaded from DUF files.
 * Each effect type has specific execution logic in ExecuteTrinketEffect().
 */
typedef enum {
    TRINKET_EFFECT_NONE,
    TRINKET_EFFECT_ADD_CHIPS,              // Flat chip gain (value = amount)
    TRINKET_EFFECT_ADD_CHIPS_PERCENT,      // Percentage chip gain (value = percent of winnings)
    TRINKET_EFFECT_LOSE_CHIPS,             // Flat chip loss (value = amount)
    TRINKET_EFFECT_APPLY_STATUS,           // Apply status effect (status_key, stacks)
    TRINKET_EFFECT_CLEAR_STATUS,           // Remove status effect (status_key)
    TRINKET_EFFECT_TRINKET_STACK,          // Increment trinket's internal stack counter
    TRINKET_EFFECT_TRINKET_STACK_RESET,    // Reset trinket's stacks to 0 (Streak Counter on loss)
    TRINKET_EFFECT_REFUND_CHIPS_PERCENT,   // Refund % of bet on loss (value = percent)
    TRINKET_EFFECT_ADD_DAMAGE_FLAT,        // Add flat damage this combat (value = amount)
    TRINKET_EFFECT_DAMAGE_MULTIPLIER,      // Multiply damage (value = percent, e.g., 200 = 2x)
    TRINKET_EFFECT_ADD_TAG_TO_CARDS,       // Add card tag to N random cards (on equip)
    TRINKET_EFFECT_BUFF_TAG_DAMAGE,        // Increase damage from tagged cards (value = bonus)
    TRINKET_EFFECT_PUSH_DAMAGE_PERCENT,    // Deal damage on push (value = percent of normal)
    TRINKET_EFFECT_BLOCK_DEBUFF,           // Block N debuffs this combat (value = count, Warded Charm)
    TRINKET_EFFECT_PUNISH_HEAL,            // Punish enemy heals (deal damage equal to heal, Bleeding Heart)
} TrinketEffectType_t;

/**
 * TrinketAffix_t - Single affix on a trinket instance
 *
 * Affixes are stat bonuses rolled from affix pool based on trinket rarity/act.
 * Each trinket can have 1-3 affixes (act 1/2/3).
 */
typedef struct {
    dString_t* stat_key;     // "damage_bonus_percent", "crit_chance", etc
    int rolled_value;        // Rolled value within affix min/max range
} TrinketAffix_t;

/**
 * AffixTemplate_t - Affix definition from DUF
 *
 * Loaded from data/affixes/combat_affixes.duf
 * Describes possible stat bonuses that can roll on trinkets.
 */
typedef struct {
    dString_t* stat_key;      // Nametag from DUF (@damage_bonus_percent)
    dString_t* name;          // Display name ("Violent")
    dString_t* description;   // Template with {value} placeholder
    int min_value;            // Minimum roll
    int max_value;            // Maximum roll
    int weight;               // Weighted selection (higher = more common)
} AffixTemplate_t;

/**
 * TrinketTemplate_t - Base trinket archetype from DUF
 *
 * Loaded from data/trinkets/combat_trinkets.duf
 * Defines trinket behavior without random affixes.
 */
typedef struct {
    dString_t* trinket_key;           // Nametag from DUF (@lucky_chip)
    dString_t* name;                  // Display name ("Lucky Chip")
    dString_t* flavor;                // Flavor text
    TrinketRarity_t rarity;           // Base rarity tier
    int base_value;                   // Base sell value (before rarity/tier scaling)

    // Primary passive effect
    GameEvent_t passive_trigger;      // When does it trigger?
    TrinketEffectType_t passive_effect_type;  // What does it do?
    int passive_effect_value;         // Numeric value (if applicable)
    dString_t* passive_status_key;    // Status effect key (for APPLY_STATUS/CLEAR_STATUS)
    int passive_status_stacks;        // Status stacks to apply

    // Trinket stack system (Broken Watch, Iron Knuckles, Streak Counter)
    dString_t* passive_stack_stat;    // Stat affected by stacks ("damage_bonus_percent")
    int passive_stack_value;          // Value per stack (+2% per stack)
    int passive_stack_max;            // Max stacks (12 stacks = 24% max, 0 = infinite)
    dString_t* passive_stack_on_max;  // Behavior on reaching max ("reset_to_one" or NULL)

    // Tag system (Cursed Skull)
    dString_t* passive_tag;           // Tag to add ("CURSED")
    int passive_tag_count;            // Number of cards to tag (4)
    int passive_tag_buff_value;       // Damage buff to tagged cards (+5)

    // Secondary passive (optional - for dual-trigger trinkets like Lucky Chip)
    GameEvent_t passive_trigger_2;
    TrinketEffectType_t passive_effect_type_2;
    int passive_effect_value_2;
    dString_t* passive_status_key_2;
    int passive_status_stacks_2;
    dString_t* passive_tag_2;

    // Optional condition
    int passive_condition_bet_gte;    // Only trigger if bet >= this (0 = no condition)
} TrinketTemplate_t;

/**
 * TrinketStatType_t - Enum for data-driven stat tracking
 *
 * Each stat type has corresponding metadata (display name, color).
 * Add new stats by: (1) adding enum value, (2) adding metadata entry.
 */
typedef enum {
    TRINKET_STAT_DAMAGE_DEALT = 0,
    TRINKET_STAT_BONUS_CHIPS,
    TRINKET_STAT_REFUNDED_CHIPS,
    TRINKET_STAT_HIGHEST_STREAK,
    TRINKET_STAT_DEBUFFS_BLOCKED,
    TRINKET_STAT_HEAL_DAMAGE_DEALT,
    TRINKET_STAT_COUNT
} TrinketStatType_t;

/**
 * TrinketStatMetadata_t - Display metadata for trinket stats
 *
 * Used by rendering code to show stat names and colors.
 */
typedef struct {
    const char* display_name;
    aColor_t text_color;
} TrinketStatMetadata_t;

/**
 * TrinketInstance_t - Runtime trinket with rolled affixes
 *
 * Generated when trinket drops from combat.
 * Combines base template + random affixes + persistent state.
 *
 * Constitutional pattern: VALUE TYPE stored in Player_t.trinket_slots[6]
 */
typedef struct {
    dString_t* base_trinket_key;  // Reference to template (@lucky_chip)
    TrinketRarity_t rarity;       // Rolled rarity (may upgrade from base via pity)
    int tier;                     // Act number when dropped (1/2/3)
    int sell_value;               // Calculated sell value

    // Affixes (1-3 based on tier)
    TrinketAffix_t affixes[3];
    int affix_count;

    // Trinket-specific stacks (persists across combats)
    int trinket_stacks;              // Current stacks (Broken Watch: 0-12)
    int trinket_stack_max;           // Max stacks
    dString_t* trinket_stack_stat;   // Stat affected by stacks
    int trinket_stack_value;         // Value per stack

    // Tag buff tracking (Cursed Skull)
    int buffed_tag;                  // Which tag is buffed (CardTag_t stored as int)
    int tag_buff_value;              // Damage bonus for buffed tag (+5)

    // Combat charges (per-trinket counters, reset each combat)
    int heal_punishes_remaining;     // Enemy heals to punish (Bleeding Heart)

    // Stats tracking (data-driven array replaces individual fields)
    int tracked_stats[TRINKET_STAT_COUNT];  // Indexed by TrinketStatType_t

    // Animation state (shake/flash on trigger)
    float shake_offset_x;
    float shake_offset_y;
    float flash_alpha;
} TrinketInstance_t;

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
    int debuff_blocks_remaining;            // Number of debuffs to block this combat (Warded Charm trinket)

    // Class system
    PlayerClass_t class;           // Character class (Degenerate, Dealer, Detective, Dreamer)
    Trinket_t class_trinket;       // Class-specific trinket (VALUE - hardcoded in C, has active ability)
    bool has_class_trinket;        // true if class trinket is equipped

    // Trinket system (DUF-based loot drops with affixes)
    TrinketInstance_t trinket_slots[6];  // VALUE ARRAY (6 slots, each is own copy)
    bool trinket_slot_occupied[6];       // Track which slots have trinkets

    // Combat stats (calculated from tags/trinkets) - ADR-09: Dirty-flag aggregation
    int damage_flat;               // Flat damage bonus (added to base damage)
    int damage_percent;            // Percentage damage increase (multiplies base damage)
    int crit_chance;               // % chance to crit (0-100, LUCKY tag gives +10% per card)
    int crit_bonus;                // % bonus damage on crit (e.g., 50 = +50% damage)
    bool combat_stats_dirty;       // true = recalculate stats from hand tags

    // Defensive stats (chip economy modifiers from trinkets/affixes) - ADR-09 pattern
    int win_bonus_percent;         // % bonus chips on win (Elite Membership passive + affixes)
    int loss_refund_percent;       // % refund chips on loss (Elite Membership passive + affixes)
    int push_damage_percent;       // % of normal damage dealt on push (default 0, Pusher's Pebble = 50)
    int flat_chips_on_win;         // Flat chip bonus on win (Prosperous affix)
} Player_t;

// ============================================================================
// TRINKET STAT ACCESSOR MACROS
// ============================================================================

/**
 * Data-driven stat access macros for TrinketInstance_t.tracked_stats[]
 *
 * Usage:
 *   int dmg = TRINKET_GET_STAT(inst, TRINKET_STAT_DAMAGE_DEALT);
 *   TRINKET_INC_STAT(inst, TRINKET_STAT_BONUS_CHIPS);
 *   TRINKET_ADD_STAT(inst, TRINKET_STAT_REFUNDED_CHIPS, 50);
 */
#define TRINKET_GET_STAT(inst, stat_type) ((inst)->tracked_stats[stat_type])
#define TRINKET_SET_STAT(inst, stat_type, value) ((inst)->tracked_stats[stat_type] = (value))
#define TRINKET_INC_STAT(inst, stat_type) ((inst)->tracked_stats[stat_type]++)
#define TRINKET_ADD_STAT(inst, stat_type, amount) ((inst)->tracked_stats[stat_type] += (amount))

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

    // Trinket loot system (pity counters for rarity upgrades)
    int normal_enemy_pity;          // Kills since last uncommon drop (5 = force uncommon)
    int elite_enemy_pity;           // Kills since last legendary drop (5 = force legendary)

    // Game over system
    bool player_defeated;           // true = player reached 0 chips (triggers STATE_GAME_OVER)
} GameContext_t;

#endif // STRUCTS_H
