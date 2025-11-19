#ifndef CARD_TAGS_H
#define CARD_TAGS_H

#include "common.h"

// ============================================================================
// CARD TAG SYSTEM
// ============================================================================

/**
 * CardTag_t - Tags that modify card behavior
 *
 * Tags are applied to cards to give them special effects during gameplay.
 * Multiple tags can be applied to a single card.
 *
 * Examples:
 * - CARD_TAG_LUCKY: Increases blackjack chance
 * - CARD_TAG_CURSED: Drains chips when drawn
 * - CARD_TAG_HEAVY: Deals more damage on win
 */
typedef enum {
    CARD_TAG_CURSED,     // 10 damage to enemy when drawn
    CARD_TAG_VAMPIRIC,   // 5 damage + 5 chips when drawn
    CARD_TAG_LUCKY,      // +10% crit while in any hand (global passive)
    CARD_TAG_BRUTAL,     // +10% damage while in any hand (global passive)
    CARD_TAG_DOUBLED,    // Value doubled this hand (one-time, removed after calculation)
    CARD_TAG_MAX
} CardTag_t;

/**
 * CardRarity_t - Rarity tier for cards
 *
 * Used for reward generation and visual effects.
 * Higher rarity = more powerful tags.
 */
typedef enum {
    CARD_RARITY_COMMON,
    CARD_RARITY_UNCOMMON,
    CARD_RARITY_RARE,
    CARD_RARITY_LEGENDARY
} CardRarity_t;

/**
 * CardMetadata_t - Extended metadata for a card
 *
 * Constitutional pattern:
 * - Heap-allocated (pointer type)
 * - Stored in global g_card_metadata table (card_id → CardMetadata_t*)
 * - Uses dArray_t for tags (not raw array)
 * - Uses dString_t for flavor text (not char[])
 */
typedef struct CardMetadata {
    int card_id;                 // 0-51 standard card ID
    dArray_t* tags;              // Array of CardTag_t (value types)
    CardRarity_t rarity;         // Card rarity tier
    dString_t* flavor_text;      // Description of tags/effects
} CardMetadata_t;

// ============================================================================
// GLOBAL REGISTRY
// ============================================================================

/**
 * g_card_metadata - Global table mapping card_id → CardMetadata_t*
 *
 * Constitutional pattern: dTable_t for O(1) lookup
 * Initialized in InitCardMetadata(), destroyed in CleanupCardMetadata()
 */
extern dTable_t* g_card_metadata;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * InitCardMetadata - Initialize card metadata system
 *
 * Creates g_card_metadata table and initializes all 52 cards with:
 * - Empty tag arrays
 * - COMMON rarity by default
 * - Empty flavor text
 *
 * Call once during game initialization (main.c).
 */
void InitCardMetadata(void);

/**
 * CleanupCardMetadata - Destroy card metadata system
 *
 * Frees all CardMetadata_t* entries and destroys g_card_metadata table.
 * Call once during game shutdown (main.c).
 */
void CleanupCardMetadata(void);

// ============================================================================
// TAG MANAGEMENT
// ============================================================================

/**
 * AddCardTag - Add a tag to a card
 *
 * @param card_id - Card ID (0-51)
 * @param tag - Tag to add
 *
 * Does nothing if tag already exists on card.
 * Creates metadata entry if card not yet initialized.
 */
void AddCardTag(int card_id, CardTag_t tag);

/**
 * RemoveCardTag - Remove a tag from a card
 *
 * @param card_id - Card ID (0-51)
 * @param tag - Tag to remove
 *
 * Does nothing if tag doesn't exist on card.
 */
void RemoveCardTag(int card_id, CardTag_t tag);

/**
 * HasCardTag - Check if card has a specific tag
 *
 * @param card_id - Card ID (0-51)
 * @param tag - Tag to check
 * @return true if card has tag, false otherwise
 */
bool HasCardTag(int card_id, CardTag_t tag);

/**
 * GetCardTags - Get all tags for a card
 *
 * @param card_id - Card ID (0-51)
 * @return dArray_t* of CardTag_t, or NULL if card not initialized
 *
 * Constitutional pattern: Returns pointer to internal array (don't modify!)
 */
const dArray_t* GetCardTags(int card_id);

/**
 * ClearCardTags - Remove all tags from a card
 *
 * @param card_id - Card ID (0-51)
 */
void ClearCardTags(int card_id);

// ============================================================================
// RARITY MANAGEMENT
// ============================================================================

/**
 * SetCardRarity - Set rarity tier for a card
 *
 * @param card_id - Card ID (0-51)
 * @param rarity - Rarity tier
 */
void SetCardRarity(int card_id, CardRarity_t rarity);

/**
 * GetCardRarity - Get rarity tier for a card
 *
 * @param card_id - Card ID (0-51)
 * @return CardRarity_t (defaults to COMMON if not set)
 */
CardRarity_t GetCardRarity(int card_id);

// ============================================================================
// FLAVOR TEXT
// ============================================================================

/**
 * SetCardFlavorText - Set flavor text for a card
 *
 * @param card_id - Card ID (0-51)
 * @param text - Flavor text (copied internally)
 *
 * Constitutional pattern: Uses dString_t internally, not char[]
 */
void SetCardFlavorText(int card_id, const char* text);

/**
 * GetCardFlavorText - Get flavor text for a card
 *
 * @param card_id - Card ID (0-51)
 * @return const char* flavor text, or empty string if not set
 */
const char* GetCardFlavorText(int card_id);

// ============================================================================
// UTILITIES
// ============================================================================

/**
 * GetCardTagName - Convert tag enum to string
 *
 * @param tag - Tag enum
 * @return const char* tag name ("Cursed", "Blessed", etc.)
 */
const char* GetCardTagName(CardTag_t tag);

/**
 * GetCardRarityName - Convert rarity enum to string
 *
 * @param rarity - Rarity enum
 * @return const char* rarity name ("Common", "Rare", etc.)
 */
const char* GetCardRarityName(CardRarity_t rarity);

/**
 * GetCardTagColor - Get UI color for tag
 *
 * @param tag - Tag enum
 * @param out_r - Red channel (0-255)
 * @param out_g - Green channel (0-255)
 * @param out_b - Blue channel (0-255)
 *
 * Used for rendering tag badges on card UI.
 */
void GetCardTagColor(CardTag_t tag, int* out_r, int* out_g, int* out_b);

/**
 * GetCardRarityColor - Get UI color for rarity
 *
 * @param rarity - Rarity enum
 * @param out_r - Red channel (0-255)
 * @param out_g - Green channel (0-255)
 * @param out_b - Blue channel (0-255)
 *
 * Used for rendering card borders/backgrounds.
 */
void GetCardRarityColor(CardRarity_t rarity, int* out_r, int* out_g, int* out_b);

/**
 * GetCardTagDescription - Get short description of tag effect
 *
 * @param tag - Tag enum
 * @return const char* - Description text (for reward screen tooltips)
 *
 * Examples:
 * - CURSED: "Drains chips when drawn"
 * - LUCKY: "Increased blackjack chance"
 */
const char* GetCardTagDescription(CardTag_t tag);

// ============================================================================
// TAG EFFECT PROCESSING
// ============================================================================

// Forward declarations
struct Card;
struct GameContext;
struct Player;

/**
 * ProcessCardTagEffects - Trigger immediate tag effects (CURSED, VAMPIRIC)
 *
 * @param card - Card that was drawn or flipped face-up
 * @param game - Game context (for enemy damage)
 * @param drawer - Player who drew/owns the card (for chip gain)
 *
 * Called when:
 * - Card is drawn from deck (face-up)
 * - Card is flipped from face-down to face-up
 *
 * Effects:
 * - CURSED: 10 damage to enemy + visual feedback
 * - VAMPIRIC: 5 damage to enemy + 5 chips to drawer + visual feedback
 *
 * Pattern: Uses existing TakeDamage() funnel, SpawnDamageNumber(), TweenEnemyHP()
 */
void ProcessCardTagEffects(const struct Card* card, struct GameContext* game, struct Player* drawer);

#endif // CARD_TAGS_H
