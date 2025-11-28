#ifndef TRINKET_DROP_MODAL_H
#define TRINKET_DROP_MODAL_H

#include "common.h"
#include "structs.h"

// ============================================================================
// TRINKET DROP MODAL COMPONENT
// ============================================================================

// Layout constants
#define TRINKET_DROP_MODAL_WIDTH          700
#define TRINKET_DROP_MODAL_HEIGHT         600
#define TRINKET_DROP_MODAL_PADDING        40
#define TRINKET_DROP_BUTTON_WIDTH         200
#define TRINKET_DROP_BUTTON_HEIGHT        50
#define TRINKET_DROP_BUTTON_SPACING       20

// Animation stages (multi-stage pattern like rewardModal - ADR-12)
typedef enum {
    TRINKET_ANIM_NONE,
    TRINKET_ANIM_FADE_IN,         // On show
    TRINKET_ANIM_FADE_OUT,        // 0.2s - Modal fades to black
    TRINKET_ANIM_FLY_TO_SLOT,     // 0.6s - Trinket flies from center to slot (equip path)
    TRINKET_ANIM_SLOT_HIGHLIGHT,  // 0.8s - Slot flashes/bounces (equip path)
    TRINKET_ANIM_FLY_TO_CHIPS,    // 0.6s - Trinket flies to chips display (sell path)
    TRINKET_ANIM_COIN_BURST,      // 0.5s - Gold coin particles burst from chips (sell path)
    TRINKET_ANIM_FLASH_CHIPS,     // 0.3s - Flash chips gained text (sell path)
    TRINKET_ANIM_COMPLETE         // Animation complete
} TrinketAnimStage_t;

// Coin particle for sell animation
typedef struct CoinParticle {
    float x, y;           // Position
    float vx, vy;         // Velocity
    float lifetime;       // Time alive (0.0 → max_lifetime)
    float max_lifetime;   // Total lifetime (0.5s - 1.0s random)
    float rotation;       // Spin angle
    float rotation_speed; // Spin speed
    float scale;          // Size (0.5 - 1.0)
} CoinParticle_t;

/**
 * TrinketDropModal_t - Modal for trinket drop (Equip/Sell choice)
 *
 * Shows when player defeats enemy and trinket drops.
 * Player can choose to:
 * - **Equip**: Select which slot (0-5) to equip trinket to
 * - **Sell**: Convert to chips immediately
 *
 * Constitutional pattern:
 * - Modal component (matches design palette - ADR-008)
 * - TrinketInstance_t stored BY VALUE (not pointer)
 * - Renders on top of blackjack scene during STATE_TRINKET_DROP
 * - Multi-stage animations (fade, zoom, flash)
 * - Hotkey support (E, S, 1-6, ESC)
 */
typedef struct TrinketDropModal {
    bool is_visible;                  // true = shown, false = hidden
    TrinketInstance_t trinket_drop;   // Trinket that dropped (stored by value)
    const TrinketTemplate_t* template; // Template ref (lookup from trinket_drop.base_trinket_key)

    // UI state
    bool choosing_slot;               // true = showing slot selection, false = showing Equip/Sell buttons
    int hovered_button;               // 0 = Equip, 1 = Sell, -1 = none
    int hovered_slot;                 // 0-5 = slot index, -1 = none (only when choosing_slot)
    bool confirmed;                   // true = player made choice, ready to exit

    // Animation state (ADR-12 pattern: multi-stage like card rewards)
    TrinketAnimStage_t anim_stage;    // Current animation stage
    float fade_alpha;                 // Modal fade (0.0 = transparent, 1.0 = opaque)
    float trinket_pos_x;              // Trinket X position during fly animation
    float trinket_pos_y;              // Trinket Y position during fly animation
    float trinket_scale;              // Trinket scale during fly (2.0 → 1.0)
    float slot_flash_alpha;           // Gold flash on target slot (0.0 → 1.0 → 0.0)
    float slot_scale;                 // Slot bounce effect (1.0 → 1.2 → 1.0)
    float chip_flash_alpha;           // Gold flash on sell (0.0 = none, 1.0 = full)
    float result_timer;               // Timer for pause before closing

    // Coin particle system (for sell animation)
    CoinParticle_t particles[20];     // Max 20 coin particles
    int particle_count;               // Active particle count

    // Button hover/press state
    float equip_button_scale;         // Scale for hover/press feedback (1.0 normal, 1.05 hover, 0.95 press)
    float sell_button_scale;          // Scale for hover/press feedback
    bool equip_button_pressed;        // Mouse down on equip button
    bool sell_button_pressed;         // Mouse down on sell button
    int key_held_button;              // Track which button key is held: 0=E (Equip), 1=S (Sell), -1=none

    // Result
    bool was_equipped;                // true = equipped, false = sold
    int equipped_slot;                // Slot index if equipped, -1 if sold
    int chips_gained;                 // Chips from sell (0 if equipped)
    bool should_equip_now;            // Signal to sceneBlackjack to equip trinket during animation

    // Cached affix templates (loaded once in Show, freed in Hide)
    AffixTemplate_t* cached_affix_templates[3];  // Max 3 affixes per trinket
    int cached_affix_count;                      // Number of cached templates
} TrinketDropModal_t;

// ============================================================================
// LIFECYCLE
// ============================================================================

/**
 * CreateTrinketDropModal - Initialize trinket drop modal
 *
 * @return TrinketDropModal_t* - Heap-allocated modal, or NULL on failure
 *
 * Modal starts hidden (is_visible = false).
 * Call ShowTrinketDropModal() to display it.
 */
TrinketDropModal_t* CreateTrinketDropModal(void);

/**
 * DestroyTrinketDropModal - Free trinket drop modal
 *
 * @param modal - Pointer to modal pointer (double pointer for nulling)
 *
 * Cleans up trinket instance dStrings before freeing.
 */
void DestroyTrinketDropModal(TrinketDropModal_t** modal);

// ============================================================================
// VISIBILITY
// ============================================================================

/**
 * ShowTrinketDropModal - Display the modal with dropped trinket
 *
 * @param modal - Modal to show
 * @param trinket_drop - Trinket that dropped (copied by value into modal)
 * @param template - Trinket template (looked up from trinket_drop.base_trinket_key)
 *
 * Copies trinket_drop into modal storage (deep copy of dStrings).
 */
void ShowTrinketDropModal(TrinketDropModal_t* modal, const TrinketInstance_t* trinket_drop, const TrinketTemplate_t* template);

/**
 * HideTrinketDropModal - Hide the modal
 *
 * @param modal - Modal to hide
 */
void HideTrinketDropModal(TrinketDropModal_t* modal);

/**
 * IsTrinketDropModalVisible - Check if modal is visible
 *
 * @param modal - Modal to check
 * @return bool - true if visible
 */
bool IsTrinketDropModalVisible(const TrinketDropModal_t* modal);

// ============================================================================
// INPUT & UPDATE
// ============================================================================

/**
 * HandleTrinketDropModalInput - Process input and update animations
 *
 * @param modal - Modal to update
 * @param player - Player (for checking slot availability)
 * @param dt - Delta time (for animation timers)
 * @return bool - true if modal wants to close (choice confirmed + animations complete)
 *
 * Handles:
 * - Equip button → show slot selection (0-5)
 * - Sell button → add chips, close modal
 * - Slot click → equip to slot, close modal
 * - Hotkeys (E, S, 1-6, ESC)
 * - Hover/press effects (button scaling)
 * - Multi-stage animations (fade, zoom, flash)
 */
bool HandleTrinketDropModalInput(TrinketDropModal_t* modal, Player_t* player, float dt);

// ============================================================================
// RENDERING
// ============================================================================

/**
 * RenderTrinketDropModal - Draw the modal overlay
 *
 * @param modal - Modal to render
 * @param player - Player (for showing slot availability)
 *
 * Draws:
 * - Dark overlay
 * - Trinket name, rarity, flavor
 * - Passive effect description
 * - All rolled affixes
 * - Sell value
 * - Equip/Sell buttons OR slot selection grid
 *
 * Only renders if is_visible = true.
 */
void RenderTrinketDropModal(const TrinketDropModal_t* modal, const Player_t* player);

// ============================================================================
// QUERIES
// ============================================================================

/**
 * WasTrinketEquipped - Check if trinket was equipped
 *
 * @param modal - Modal to query
 * @return bool - true if equipped, false if sold
 */
bool WasTrinketEquipped(const TrinketDropModal_t* modal);

/**
 * GetEquippedSlot - Get slot trinket was equipped to
 *
 * @param modal - Modal to query
 * @return int - Slot index (0-5), or -1 if sold
 */
int GetEquippedSlot(const TrinketDropModal_t* modal);

/**
 * GetChipsGained - Get chips gained from sell
 *
 * @param modal - Modal to query
 * @return int - Chips gained (0 if equipped)
 */
int GetChipsGained(const TrinketDropModal_t* modal);

#endif // TRINKET_DROP_MODAL_H
