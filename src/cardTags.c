/*
 * Card Tags System Implementation
 * Constitutional pattern: dTable_t for metadata storage, dArray_t for tag lists
 */

#include "cardTags.h"

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
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
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
    // d_InitArray(capacity, element_size) - capacity FIRST!
    meta->tags = d_InitArray(4, sizeof(CardTag_t));  // Start with 4 tags capacity
    meta->rarity = CARD_RARITY_COMMON;
    meta->flavor_text = d_StringInit();

    // Store in table
    d_SetDataInTable(g_card_metadata, &card_id, &meta);

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
    g_card_metadata = d_InitTable(sizeof(int), sizeof(CardMetadata_t*),
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
        CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
        if (!meta_ptr || !*meta_ptr) {
            continue;
        }

        CardMetadata_t* meta = *meta_ptr;

        // Free internal resources
        if (meta->tags) {
            d_DestroyArray(meta->tags);
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
    d_DestroyTable(&g_card_metadata);

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
        CardTag_t* existing = (CardTag_t*)d_IndexDataFromArray(meta->tags, i);
        if (*existing == tag) {
            return;  // Already has this tag
        }
    }

    // Add new tag
    d_AppendDataToArray(meta->tags, &tag);
    d_LogInfoF("Card %d: Added tag %s", card_id, GetCardTagName(tag));
}

void RemoveCardTag(int card_id, CardTag_t tag) {
    // Defensive: Check if metadata system initialized
    if (!g_card_metadata) {
        return;
    }

    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return;  // No metadata for this card
    }

    CardMetadata_t* meta = *meta_ptr;

    // Find and remove tag
    for (size_t i = 0; i < meta->tags->count; i++) {
        CardTag_t* existing = (CardTag_t*)d_IndexDataFromArray(meta->tags, i);
        if (*existing == tag) {
            d_RemoveDataFromArray(meta->tags, i);
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

    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return false;  // No metadata for this card
    }

    CardMetadata_t* meta = *meta_ptr;

    // Search for tag
    for (size_t i = 0; i < meta->tags->count; i++) {
        CardTag_t* existing = (CardTag_t*)d_IndexDataFromArray(meta->tags, i);
        if (*existing == tag) {
            return true;
        }
    }

    return false;
}

const dArray_t* GetCardTags(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return NULL;
    }

    return (*meta_ptr)->tags;
}

void ClearCardTags(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return;
    }

    CardMetadata_t* meta = *meta_ptr;
    d_ClearArray(meta->tags);
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
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
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

    d_StringSet(meta->flavor_text, text, 0);
}

const char* GetCardFlavorText(int card_id) {
    CardMetadata_t** meta_ptr = (CardMetadata_t**)d_GetDataFromTable(g_card_metadata, &card_id);
    if (!meta_ptr || !*meta_ptr) {
        return "";  // Empty string if no metadata
    }

    return d_StringPeek((*meta_ptr)->flavor_text);
}

// ============================================================================
// UTILITIES
// ============================================================================

const char* GetCardTagName(CardTag_t tag) {
    switch (tag) {
        case CARD_TAG_CURSED:   return "Cursed";
        case CARD_TAG_VAMPIRIC: return "Vampiric";
        case CARD_TAG_LUCKY:    return "Lucky";
        case CARD_TAG_BRUTAL:   return "Brutal";
        case CARD_TAG_DOUBLED:  return "Doubled";
        default:                return "Unknown";
    }
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
    switch (tag) {
        case CARD_TAG_CURSED:   // #a53030 - red from palette
            *out_r = 165; *out_g = 48; *out_b = 48;
            break;
        case CARD_TAG_VAMPIRIC: // #cf573c - red-orange from palette
            *out_r = 207; *out_g = 87; *out_b = 60;
            break;
        case CARD_TAG_LUCKY:    // #75a743 - green from palette
            *out_r = 117; *out_g = 167; *out_b = 67;
            break;
        case CARD_TAG_BRUTAL:   // #de9e41 - orange from palette
            *out_r = 222; *out_g = 158; *out_b = 65;
            break;
        case CARD_TAG_DOUBLED:  // #e8c170 - gold from palette
            *out_r = 232; *out_g = 193; *out_b = 112;
            break;
        default:  // #577277 - gray from palette
            *out_r = 87; *out_g = 114; *out_b = 119;
            break;
    }
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
    switch (tag) {
        case CARD_TAG_CURSED:
            return "10 damage to enemy when drawn";
        case CARD_TAG_VAMPIRIC:
            return "5 damage + 5 chips when drawn";
        case CARD_TAG_LUCKY:
            return "+10% crit while in any hand";
        case CARD_TAG_BRUTAL:
            return "+10% damage while in any hand";
        case CARD_TAG_DOUBLED:
            return "Counts twice for hand value";
        default:
            return "Unknown tag";
    }
}
