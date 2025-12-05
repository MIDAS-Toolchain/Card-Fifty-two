/*
 * Card Tags System Implementation
 * Constitutional pattern: dTable_t for metadata storage, dArray_t for tag lists
 * Tag metadata loaded from DUF (ADR-019 + ADR-021)
 */

#include "cardTags.h"
#include "card.h"
#include "game.h"
#include "player.h"
#include "enemy.h"
#include "stats.h"
#include "scenes/sceneBlackjack.h"  // For GetVisualEffects, TweenEnemyHP, TriggerEnemyDamageEffect
#include "scenes/components/visualEffects.h"
#include "tween/tween.h"
#include "loaders/cardTagLoader.h"  // For DUF-based tag metadata

// ============================================================================
// GLOBAL REGISTRY
// ============================================================================

dTable_t* g_card_metadata = NULL;

// ============================================================================
// INTERNAL HELPERS
// ============================================================================

/**
 * GetOrCreateMetadata - Get existing metadata or create new entry
 *
 * @param card_id - Card ID (0-51)
 * @return CardMetadata_t* - Never NULL (creates if needed)
 */
static CardMetadata_t* GetOrCreateMetadata(int card_id) {
    if (!g_card_metadata) {
        d_LogFatal("GetOrCreateMetadata: g_card_metadata not initialized");
        return NULL;
    }

    // Try to get existing metadata
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (meta_ptr) {
        return *meta_ptr;
    }

    // Create new metadata entry
    CardMetadata_t* meta = malloc(sizeof(CardMetadata_t));
    if (!meta) {
        d_LogFatal("GetOrCreateMetadata: Failed to allocate CardMetadata_t");
        return NULL;
    }

    meta->card_id = card_id;
    // d_ArrayInit(capacity, element_size) - capacity FIRST!
    meta->tags = d_ArrayInit(4, sizeof(CardTag_t));  // Start with 4 tags capacity
    meta->rarity = CARD_RARITY_COMMON;
    meta->flavor_text = d_StringInit();

    // Store in table
    d_TableSet(g_card_metadata, &card_id, &meta);

    return meta;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

void InitCardMetadata(void) {
    if (g_card_metadata) {
        d_LogWarning("InitCardMetadata: Already initialized");
        return;
    }

    // Create metadata table (key: int card_id, value: CardMetadata_t*)
    g_card_metadata = d_TableInit(sizeof(int), sizeof(CardMetadata_t*),
                                   d_HashInt, d_CompareInt, 52);
    if (!g_card_metadata) {
        d_LogFatal("InitCardMetadata: Failed to create g_card_metadata table");
        return;
    }

    d_LogInfo("Card metadata system initialized");
}

void CleanupCardMetadata(void) {
    if (!g_card_metadata) {
        d_LogWarning("CleanupCardMetadata: Not initialized");
        return;
    }

    // Manually free all card metadata (0-51)
    for (int card_id = 0; card_id < 52; card_id++) {
        CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
        if (!meta_ptr || !*meta_ptr) {
            continue;
        }

        CardMetadata_t* meta = *meta_ptr;

        // Free internal resources
        if (meta->tags) {
            d_ArrayDestroy(meta->tags);
            meta->tags = NULL;
        }
        if (meta->flavor_text) {
            d_StringDestroy(meta->flavor_text);
            meta->flavor_text = NULL;
        }

        // Free metadata struct
        free(meta);
    }

    // Destroy table
    d_TableDestroy(&g_card_metadata);

    d_LogInfo("Card metadata system cleaned up");
}

// ============================================================================
// TAG MANAGEMENT
// ============================================================================

void AddCardTag(int card_id, CardTag_t tag) {
    CardMetadata_t* meta = GetOrCreateMetadata(card_id);
    if (!meta) return;

    // Check if tag already exists
    for (size_t i = 0; i < meta->tags->count; i++) {
        CardTag_t* existing = (CardTag_t*)d_ArrayGet(meta->tags, i);
        if (*existing == tag) {
            return;  // Already has this tag
        }
    }

    // Add new tag
    d_ArrayAppend(meta->tags, &tag);
    d_LogInfoF("Card %d: Added tag %s", card_id, GetCardTagName(tag));
}

void RemoveCardTag(int card_id, CardTag_t tag) {
    // Defensive: Check if metadata system initialized
    if (!g_card_metadata) {
        return;
    }

    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return;  // No metadata for this card
    }

    CardMetadata_t* meta = *meta_ptr;

    // Find and remove tag
    for (size_t i = 0; i < meta->tags->count; i++) {
        CardTag_t* existing = (CardTag_t*)d_ArrayGet(meta->tags, i);
        if (*existing == tag) {
            d_ArrayRemove(meta->tags, i);
            d_LogInfoF("Card %d: Removed tag %s", card_id, GetCardTagName(tag));
            return;
        }
    }
}

bool HasCardTag(int card_id, CardTag_t tag) {
    // Defensive: Check if metadata system initialized
    if (!g_card_metadata) {
        return false;
    }

    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return false;  // No metadata for this card
    }

    CardMetadata_t* meta = *meta_ptr;

    // Search for tag
    for (size_t i = 0; i < meta->tags->count; i++) {
        CardTag_t* existing = (CardTag_t*)d_ArrayGet(meta->tags, i);
        if (*existing == tag) {
            return true;
        }
    }

    return false;
}

const dArray_t* GetCardTags(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return NULL;
    }

    return (*meta_ptr)->tags;
}

void ClearCardTags(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return;
    }

    CardMetadata_t* meta = *meta_ptr;
    d_ArrayClear(meta->tags);
    d_LogInfoF("Card %d: Cleared all tags", card_id);
}

// ============================================================================
// RARITY MANAGEMENT
// ============================================================================

void SetCardRarity(int card_id, CardRarity_t rarity) {
    CardMetadata_t* meta = GetOrCreateMetadata(card_id);
    if (!meta) return;

    meta->rarity = rarity;
    d_LogInfoF("Card %d: Set rarity to %s", card_id, GetCardRarityName(rarity));
}

CardRarity_t GetCardRarity(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return CARD_RARITY_COMMON;  // Default rarity
    }

    return (*meta_ptr)->rarity;
}

// ============================================================================
// FLAVOR TEXT
// ============================================================================

void SetCardFlavorText(int card_id, const char* text) {
    CardMetadata_t* meta = GetOrCreateMetadata(card_id);
    if (!meta) return;

    d_StringSet(meta->flavor_text, text);
}

const char* GetCardFlavorText(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_TableGet(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return "";  // Empty string if no metadata
    }

    return d_StringPeek((*meta_ptr)->flavor_text);
}

// ============================================================================
// UTILITIES
// ============================================================================

const char* GetCardTagName(CardTag_t tag) {
    // DUF-based implementation (ADR-021)
    return GetTagDisplayName(tag);
}

CardTag_t CardTagFromString(const char* str) {
    if (!str) return CARD_TAG_VICIOUS;  // Default fallback

    if (strcmp(str, "VICIOUS") == 0) return CARD_TAG_VICIOUS;
    if (strcmp(str, "VAMPIRIC") == 0) return CARD_TAG_VAMPIRIC;
    if (strcmp(str, "LUCKY") == 0) return CARD_TAG_LUCKY;
    if (strcmp(str, "JAGGED") == 0) return CARD_TAG_JAGGED;
    if (strcmp(str, "DOUBLED") == 0) return CARD_TAG_DOUBLED;
    if (strcmp(str, "SHARP") == 0) return CARD_TAG_SHARP;

    d_LogWarningF("Unknown card tag: %s (defaulting to VICIOUS)", str);
    return CARD_TAG_VICIOUS;
}

const char* GetCardRarityName(CardRarity_t rarity) {
    switch (rarity) {
        case CARD_RARITY_COMMON:    return "Common";
        case CARD_RARITY_UNCOMMON:  return "Uncommon";
        case CARD_RARITY_RARE:      return "Rare";
        case CARD_RARITY_LEGENDARY: return "Legendary";
        default:                    return "Unknown";
    }
}

void GetCardTagColor(CardTag_t tag, int* out_r, int* out_g, int* out_b) {
    // DUF-based implementation (ADR-021)
    GetTagColor(tag, out_r, out_g, out_b);
}

void GetCardRarityColor(CardRarity_t rarity, int* out_r, int* out_g, int* out_b) {
    switch (rarity) {
        case CARD_RARITY_COMMON:    // Gray
            *out_r = 200; *out_g = 200; *out_b = 200;
            break;
        case CARD_RARITY_UNCOMMON:  // Green
            *out_r = 50; *out_g = 205; *out_b = 50;
            break;
        case CARD_RARITY_RARE:      // Blue
            *out_r = 65; *out_g = 105; *out_b = 225;
            break;
        case CARD_RARITY_LEGENDARY: // Gold
            *out_r = 255; *out_g = 215; *out_b = 0;
            break;
        default:  // White
            *out_r = 255; *out_g = 255; *out_b = 255;
            break;
    }
}

const char* GetCardTagDescription(CardTag_t tag) {
    // DUF-based implementation (ADR-021)
    return GetTagDescription(tag);
}

// ============================================================================
// TAG EFFECT PROCESSING
// ============================================================================

void ProcessCardTagEffects(const Card_t* card, GameContext_t* game, Player_t* drawer) {
    if (!card || !game || !drawer) {
        d_LogInfo("ProcessCardTagEffects: NULL parameter detected");
        return;
    }

    // Only trigger in combat mode
    if (!game->is_combat_mode || !game->current_enemy) {
        d_LogInfoF("ProcessCardTagEffects: Skipping - combat_mode=%d", game->is_combat_mode);
        return;
    }

    d_LogInfoF("ProcessCardTagEffects: Checking card %d for tags...", card->card_id);

    // Check for VICIOUS tag: 10 damage to enemy
    if (HasCardTag(card->card_id, CARD_TAG_VICIOUS)) {
        int base_damage = 10;

        // Apply ALL damage modifiers (ADR-010: Universal damage modifier)
        bool is_crit = false;
        int damage = ApplyPlayerDamageModifiers(drawer, base_damage, &is_crit);

        // Apply damage using existing funnel
        TakeDamage(game->current_enemy, damage);

        // Track stats
        Stats_RecordDamage(DAMAGE_SOURCE_CARD_TAG, damage);

        // Visual feedback (matches Degenerate's Gambit pattern)
        TweenEnemyHP(game->current_enemy);  // Smooth HP bar animation
        TweenManager_t* tween_mgr = GetTweenManager();
        if (tween_mgr) {
            TriggerEnemyDamageEffect(game->current_enemy, tween_mgr);  // Enemy shake + red flash
        }

        // Screen shake and damage number via visual effects component
        VisualEffects_t* vfx = GetVisualEffects();
        if (vfx) {
            VFX_TriggerScreenShake(vfx, 15.0f, 0.4f);  // 15px intensity, 0.4s duration
            VFX_SpawnDamageNumber(vfx, damage,
                                 GetGameAreaX() + (GetGameAreaWidth() / 2) + ENEMY_PORTRAIT_X_OFFSET,
                                 GetEnemyHealthBarY() - DAMAGE_NUMBER_Y_OFFSET,
                                 false, is_crit, false);  // Pass crit flag, not rake
        }

        // Fire tag-specific event
        Game_TriggerEvent(game, GAME_EVENT_CARD_TAG_VICIOUS);

        d_LogInfoF("💀 VICIOUS tag! %s of %s dealt %d damage to %s",
                   GetRankString(card->rank), GetSuitString(card->suit),
                   damage, game->current_enemy->name);
    }

    // Check for VAMPIRIC tag: 5 damage + 5 chips
    if (HasCardTag(card->card_id, CARD_TAG_VAMPIRIC)) {
        int base_damage = 5;
        int chip_gain = 5;

        // Apply ALL damage modifiers (ADR-010: Universal damage modifier)
        bool is_crit = false;
        int damage = ApplyPlayerDamageModifiers(drawer, base_damage, &is_crit);

        // Apply damage using existing funnel
        TakeDamage(game->current_enemy, damage);

        // Track stats
        Stats_RecordDamage(DAMAGE_SOURCE_CARD_TAG, damage);

        // Grant chips to drawer
        drawer->chips += chip_gain;

        // Visual feedback (matches Degenerate's Gambit pattern)
        TweenEnemyHP(game->current_enemy);  // Smooth HP bar animation
        TweenManager_t* tween_mgr = GetTweenManager();
        if (tween_mgr) {
            TriggerEnemyDamageEffect(game->current_enemy, tween_mgr);  // Enemy shake + red flash
        }

        // Screen shake and damage numbers via visual effects component
        VisualEffects_t* vfx = GetVisualEffects();
        if (vfx) {
            VFX_TriggerScreenShake(vfx, 10.0f, 0.3f);  // 10px intensity, 0.3s duration (lighter than CURSED)

            // Spawn damage number (red, on enemy)
            VFX_SpawnDamageNumber(vfx, damage,
                                 GetGameAreaX() + (GetGameAreaWidth() / 2) + ENEMY_PORTRAIT_X_OFFSET,
                                 GetEnemyHealthBarY() - DAMAGE_NUMBER_Y_OFFSET,
                                 false, is_crit, false);  // Pass crit flag, not rake

            // Spawn chip gain number (green, near chips display on left sidebar)
            // Left sidebar: x=0, width=280, center=140
            // Chips display is at top of sidebar (approx y=110 based on layout)
            VFX_SpawnDamageNumber(vfx, chip_gain,
                                 140,   // Center of left sidebar
                                 110,   // Near "Betting Power" / chips display
                                 true, false, false); // healing/positive (green), no crit, not rake
        }

        // Fire tag-specific event
        Game_TriggerEvent(game, GAME_EVENT_CARD_TAG_VAMPIRIC);

        d_LogInfoF("🩸 VAMPIRIC tag! %s of %s dealt %d damage and gained %d chips",
                   GetRankString(card->rank), GetSuitString(card->suit),
                   damage, chip_gain);
    }
}

// ============================================================================
// DATA-DRIVEN PASSIVE TAG SYSTEM
// ============================================================================

/**
 * GetPassiveTagBonuses - Collect all passive bonuses from tagged cards in play
 *
 * Reads effect definitions from DUF (data-driven).
 * Scans ALL players' hands (global scope) for passive tags.
 *
 * @param player - Player context (unused, but kept for API consistency)
 * @param out_flat_damage - Output: flat damage bonus
 * @param out_percent_damage - Output: percent damage bonus
 * @param out_percent_crit - Output: percent crit chance bonus
 */
void GetPassiveTagBonuses(Player_t* player, int* out_flat_damage, int* out_percent_damage, int* out_percent_crit) {
    // Initialize outputs
    *out_flat_damage = 0;
    *out_percent_damage = 0;
    *out_percent_crit = 0;

    if (!player || !g_card_metadata) return;

    // Scan all players (0=dealer, 1=human) for GLOBAL passive tags
    extern dTable_t* g_players;
    if (!g_players) {
        d_LogWarning("GetPassiveTagBonuses: g_players table is NULL");
        return;
    }

    for (int player_id = 0; player_id <= 1; player_id++) {
        Player_t* p = (Player_t*)d_TableGet(g_players, &player_id);
        if (!p || !p->hand.cards) continue;

        // Scan all FACE-UP cards in this player's hand
        for (size_t i = 0; i < p->hand.cards->count; i++) {
            const Card_t* card = (const Card_t*)d_ArrayGet(p->hand.cards, i);
            if (!card || !card->face_up) continue;  // Skip face-down cards

            const dArray_t* tags = GetCardTags(card->card_id);
            if (!tags) continue;

            // Process each tag on this card
            for (size_t t = 0; t < tags->count; t++) {
                CardTag_t* tag_ptr = (CardTag_t*)d_ArrayGet(tags, t);
                if (!tag_ptr) continue;
                CardTag_t tag = *tag_ptr;

                // Only process passive tags
                const char* trigger_type = GetTagTriggerType(tag);
                if (!trigger_type || strcmp(trigger_type, "passive") != 0) continue;

                // Process effects from DUF
                dDUFValue_t* effects = GetTagEffects(tag);
                if (!effects) continue;

                dDUFValue_t* effect = effects->child;
                while (effect) {
                    dDUFValue_t* type_val = d_DUFGetObjectItem(effect, "type");
                    dDUFValue_t* value_val = d_DUFGetObjectItem(effect, "value");

                    if (type_val && type_val->value_string && value_val) {
                        const char* effect_type = type_val->value_string;
                        int value = (int)value_val->value_int;

                        if (strcmp(effect_type, "add_flat_damage") == 0) {
                            *out_flat_damage += value;
                            d_LogInfoF("  Found %s tag (card %d): +%d flat damage",
                                     GetCardTagName(tag), card->card_id, value);
                        } else if (strcmp(effect_type, "add_damage_percent") == 0) {
                            *out_percent_damage += value;
                            d_LogInfoF("  Found %s tag (card %d): +%d%% damage",
                                     GetCardTagName(tag), card->card_id, value);
                        } else if (strcmp(effect_type, "add_crit_percent") == 0) {
                            *out_percent_crit += value;
                            d_LogInfoF("  Found %s tag (card %d): +%d%% crit",
                                     GetCardTagName(tag), card->card_id, value);
                        }
                    }
                    effect = effect->next;
                }
            }
        }
    }

    d_LogInfoF("Passive tag bonuses: flat_damage=%d, damage_percent=%d%%, crit_chance=%d%%",
               *out_flat_damage, *out_percent_damage, *out_percent_crit);
}
