/*
 * Trinket Drop Modal Implementation
 *
 * Displays trinket drop after combat victory with Equip/Sell choice.
 */

#include "../../../include/scenes/components/trinketDropModal.h"
#include "../../../include/scenes/sceneBlackjack.h"  // For TRINKET_UI_X, TRINKET_UI_Y, TRINKET_SLOT_SIZE, TRINKET_SLOT_GAP
#include "../../../include/trinket.h"
#include "../../../include/loaders/trinketLoader.h"
#include "../../../include/loaders/affixLoader.h"
#include "../../../include/common.h"
#include "../../../include/tween/tween.h"
#include <stdlib.h>
#include <math.h>

// External globals (defined in common.h/defs.h)
// app is already declared in common.h, no need to extern
extern TweenManager_t g_tween_manager;  // Tween manager for animations

// ============================================================================
// COLOR PALETTE (ADR-008: Modal Design Consistency)
// ============================================================================

#define COLOR_OVERLAY           ((aColor_t){9, 10, 20, 180})     // #090a14 - almost black overlay
#define COLOR_PANEL_BG          ((aColor_t){9, 10, 20, 240})     // #090a14 - almost black
#define COLOR_HEADER_BG         ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy blue
#define COLOR_HEADER_TEXT       ((aColor_t){231, 213, 179, 255}) // #e7d5b3 - cream
#define COLOR_HEADER_BORDER     ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue
#define COLOR_INFO_TEXT         ((aColor_t){168, 181, 178, 255}) // #a8b5b2 - light gray
#define COLOR_TAG_GOLD          ((aColor_t){232, 193, 112, 255}) // #e8c170 - gold
#define COLOR_BUTTON_BG         ((aColor_t){37, 58, 94, 255})    // #253a5e - dark navy
#define COLOR_BUTTON_HOVER      ((aColor_t){60, 94, 139, 255})   // #3c5e8b - medium blue
#define COLOR_BUTTON_PRESSED    ((aColor_t){30, 47, 75, 255})    // Darker than BG
#define COLOR_EQUIP_ACCENT      ((aColor_t){168, 202, 88, 255})  // Green accent
#define COLOR_SELL_ACCENT       ((aColor_t){207, 87, 60, 255})   // Red-orange accent

// ============================================================================
// FORWARD DECLARATIONS
// ============================================================================

// FormatTrinketPassive from trinketUI.c (will extract to shared utils later)
static dString_t* FormatTrinketPassive(const TrinketTemplate_t* template, const TrinketInstance_t* instance, bool secondary);

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * CleanupTrinketInstanceValue - Free dStrings in TrinketInstance_t
 *
 * Does NOT free the struct itself (value type).
 */
static void CleanupTrinketInstanceValue(TrinketInstance_t* instance) {
    if (!instance) return;

    if (instance->base_trinket_key) {
        d_StringDestroy(instance->base_trinket_key);
        instance->base_trinket_key = NULL;
    }

    if (instance->trinket_stack_stat) {
        d_StringDestroy(instance->trinket_stack_stat);
        instance->trinket_stack_stat = NULL;
    }

    // Clean up affixes
    for (int i = 0; i < instance->affix_count; i++) {
        if (instance->affixes[i].stat_key) {
            d_StringDestroy(instance->affixes[i].stat_key);
            instance->affixes[i].stat_key = NULL;
        }
    }
}

/**
 * CopyTrinketInstanceValue - Deep copy TrinketInstance_t
 *
 * Copies all dStrings (new allocations).
 */
static void CopyTrinketInstanceValue(TrinketInstance_t* dest, const TrinketInstance_t* src) {
    if (!dest || !src) return;

    // Copy primitive fields
    dest->rarity = src->rarity;
    dest->tier = src->tier;
    dest->sell_value = src->sell_value;
    dest->affix_count = src->affix_count;
    dest->trinket_stacks = src->trinket_stacks;
    dest->trinket_stack_max = src->trinket_stack_max;
    dest->trinket_stack_value = src->trinket_stack_value;
    dest->buffed_tag = src->buffed_tag;
    dest->tag_buff_value = src->tag_buff_value;
    dest->total_damage_dealt = src->total_damage_dealt;
    dest->total_bonus_chips = src->total_bonus_chips;
    dest->total_refunded_chips = src->total_refunded_chips;
    dest->shake_offset_x = src->shake_offset_x;
    dest->shake_offset_y = src->shake_offset_y;
    dest->flash_alpha = src->flash_alpha;

    // Deep copy dStrings
    if (src->base_trinket_key) {
        const char* key_str = d_StringPeek(src->base_trinket_key);
        dest->base_trinket_key = d_StringInit();
        d_StringAppend(dest->base_trinket_key, key_str, strlen(key_str));
    }

    if (src->trinket_stack_stat) {
        const char* stat_str = d_StringPeek(src->trinket_stack_stat);
        dest->trinket_stack_stat = d_StringInit();
        d_StringAppend(dest->trinket_stack_stat, stat_str, strlen(stat_str));
    }

    // Deep copy affixes
    for (int i = 0; i < src->affix_count; i++) {
        dest->affixes[i].rolled_value = src->affixes[i].rolled_value;
        if (src->affixes[i].stat_key) {
            const char* affix_str = d_StringPeek(src->affixes[i].stat_key);
            dest->affixes[i].stat_key = d_StringInit();
            d_StringAppend(dest->affixes[i].stat_key, affix_str, strlen(affix_str));
        }
    }
}

/**
 * FormatTrinketPassive - Format passive description for display
 *
 * @param template - Trinket template
 * @param instance - Trinket instance (for stack tracking)
 * @param secondary - true for secondary passive (trigger_2), false for primary
 * @return dString_t* - Formatted passive string (caller must destroy)
 */
static dString_t* FormatTrinketPassive(const TrinketTemplate_t* template, const TrinketInstance_t* instance, bool secondary) {
    dString_t* passive_text = d_StringInit();
    if (!passive_text) return NULL;

    // Select which trigger/effect to format
    GameEvent_t trigger = secondary ? template->passive_trigger_2 : template->passive_trigger;
    TrinketEffectType_t effect_type = secondary ? template->passive_effect_type_2 : template->passive_effect_type;
    int effect_value = secondary ? template->passive_effect_value_2 : template->passive_effect_value;

    // Skip if no trigger set
    if (trigger == 0 && effect_type == TRINKET_EFFECT_NONE) {
        d_StringSet(passive_text, "", 0);
        return passive_text;
    }

    // 1. Format trigger ("On Win:", "On Blackjack:", etc.)
    const char* trigger_str = NULL;
    switch (trigger) {
        case GAME_EVENT_COMBAT_START:     trigger_str = "On Combat Start"; break;
        case GAME_EVENT_PLAYER_WIN:       trigger_str = "On Win"; break;
        case GAME_EVENT_PLAYER_LOSS:      trigger_str = "On Loss"; break;
        case GAME_EVENT_PLAYER_BUST:      trigger_str = "On Bust"; break;
        case GAME_EVENT_PLAYER_BLACKJACK: trigger_str = "On Blackjack"; break;
        case GAME_EVENT_PLAYER_PUSH:      trigger_str = "On Push"; break;
        case GAME_EVENT_CARD_DRAWN:       trigger_str = "On Card Drawn"; break;
        case 999:                         trigger_str = "On Equip"; break; // Special ON_EQUIP
        default:                          trigger_str = "Passive"; break;
    }

    // 2. Format effect ("Gain 5 chips", "Apply 1 LUCKY_STREAK", etc.)
    switch (effect_type) {
        case TRINKET_EFFECT_ADD_CHIPS:
            d_StringFormat(passive_text, "%s: Gain %d chips", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_LOSE_CHIPS:
            d_StringFormat(passive_text, "%s: Lose %d chips", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_REFUND_CHIPS_PERCENT:
            d_StringFormat(passive_text, "%s: Refund %d%% of bet", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_APPLY_STATUS: {
            const char* status_key = secondary ?
                (template->passive_status_key_2 ? d_StringPeek(template->passive_status_key_2) : "???") :
                (template->passive_status_key ? d_StringPeek(template->passive_status_key) : "???");
            int status_stacks = secondary ? template->passive_status_stacks_2 : template->passive_status_stacks;
            d_StringFormat(passive_text, "%s: Gain %d %s", trigger_str, status_stacks, status_key);
            break;
        }

        case TRINKET_EFFECT_CLEAR_STATUS: {
            const char* status_key = secondary ?
                (template->passive_status_key_2 ? d_StringPeek(template->passive_status_key_2) : "???") :
                (template->passive_status_key ? d_StringPeek(template->passive_status_key) : "???");
            d_StringFormat(passive_text, "%s: Clear %s", trigger_str, status_key);
            break;
        }

        case TRINKET_EFFECT_TRINKET_STACK: {
            const char* stat = template->passive_stack_stat ? d_StringPeek(template->passive_stack_stat) : "???";
            // Show current stacks / max stacks if instance exists
            if (instance && instance->trinket_stacks > 0) {
                d_StringFormat(passive_text, "%s: +%d%% %s (%d/%d stacks)",
                              trigger_str, template->passive_stack_value, stat,
                              instance->trinket_stacks, template->passive_stack_max);
            } else {
                d_StringFormat(passive_text, "%s: +%d%% %s (max %d stacks)",
                              trigger_str, template->passive_stack_value, stat,
                              template->passive_stack_max);
            }
            break;
        }

        case TRINKET_EFFECT_ADD_DAMAGE_FLAT:
            d_StringFormat(passive_text, "%s: +%d damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_DAMAGE_MULTIPLIER:
            d_StringFormat(passive_text, "%s: ×%d%% damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_PUSH_DAMAGE_PERCENT:
            d_StringFormat(passive_text, "%s: Deal %d%% damage", trigger_str, effect_value);
            break;

        case TRINKET_EFFECT_ADD_TAG_TO_CARDS: {
            const char* tag = template->passive_tag ? d_StringPeek(template->passive_tag) : "???";
            d_StringFormat(passive_text, "%s: Add %s tag to %d cards",
                          trigger_str, tag, template->passive_tag_count);
            break;
        }

        case TRINKET_EFFECT_BUFF_TAG_DAMAGE: {
            const char* tag = template->passive_tag ? d_StringPeek(template->passive_tag) : "???";
            d_StringFormat(passive_text, "%s: %s cards deal +%d damage",
                          trigger_str, tag, template->passive_tag_buff_value);
            break;
        }

        case TRINKET_EFFECT_NONE:
        default:
            d_StringFormat(passive_text, "%s: ???", trigger_str);
            break;
    }

    return passive_text;
}

// ============================================================================
// LIFECYCLE
// ============================================================================

TrinketDropModal_t* CreateTrinketDropModal(void) {
    TrinketDropModal_t* modal = malloc(sizeof(TrinketDropModal_t));
    if (!modal) {
        d_LogError("Failed to allocate TrinketDropModal_t");
        return NULL;
    }

    modal->is_visible = false;
    modal->template = NULL;
    modal->choosing_slot = false;
    modal->hovered_button = -1;
    modal->hovered_slot = -1;
    modal->confirmed = false;
    modal->was_equipped = false;
    modal->equipped_slot = -1;
    modal->chips_gained = 0;
    modal->should_equip_now = false;

    // Animation state (ADR-12 pattern)
    modal->anim_stage = TRINKET_ANIM_NONE;
    modal->fade_alpha = 1.0f;
    modal->trinket_pos_x = 0.0f;
    modal->trinket_pos_y = 0.0f;
    modal->trinket_scale = 1.0f;
    modal->slot_flash_alpha = 0.0f;
    modal->slot_scale = 1.0f;
    modal->chip_flash_alpha = 0.0f;
    modal->result_timer = 0.0f;

    // Coin particle system
    modal->particle_count = 0;
    memset(modal->particles, 0, sizeof(modal->particles));

    // Button hover/press state
    modal->equip_button_scale = 1.0f;
    modal->sell_button_scale = 1.0f;
    modal->equip_button_pressed = false;
    modal->sell_button_pressed = false;
    modal->key_held_button = -1;

    // Zero out trinket_drop (will be populated on Show)
    memset(&modal->trinket_drop, 0, sizeof(TrinketInstance_t));

    // Initialize cached affix templates
    modal->cached_affix_count = 0;
    for (int i = 0; i < 3; i++) {
        modal->cached_affix_templates[i] = NULL;
    }

    return modal;
}

void DestroyTrinketDropModal(TrinketDropModal_t** modal) {
    if (!modal || !*modal) return;

    // Clean up cached affix templates
    for (int i = 0; i < (*modal)->cached_affix_count; i++) {
        if ((*modal)->cached_affix_templates[i]) {
            CleanupAffixTemplate((*modal)->cached_affix_templates[i]);
            free((*modal)->cached_affix_templates[i]);
            (*modal)->cached_affix_templates[i] = NULL;
        }
    }

    // Clean up trinket instance dStrings
    CleanupTrinketInstanceValue(&(*modal)->trinket_drop);

    free(*modal);
    *modal = NULL;
}

// ============================================================================
// VISIBILITY
// ============================================================================

void ShowTrinketDropModal(TrinketDropModal_t* modal, const TrinketInstance_t* trinket_drop, const TrinketTemplate_t* template) {
    if (!modal || !trinket_drop || !template) {
        d_LogError("ShowTrinketDropModal: NULL parameter");
        return;
    }

    // Clean up old cached affix templates if any
    for (int i = 0; i < modal->cached_affix_count; i++) {
        if (modal->cached_affix_templates[i]) {
            CleanupAffixTemplate(modal->cached_affix_templates[i]);
            free(modal->cached_affix_templates[i]);
            modal->cached_affix_templates[i] = NULL;
        }
    }
    modal->cached_affix_count = 0;

    // Clean up old trinket if any
    CleanupTrinketInstanceValue(&modal->trinket_drop);

    // Deep copy trinket instance
    CopyTrinketInstanceValue(&modal->trinket_drop, trinket_drop);

    modal->template = template;
    modal->is_visible = true;
    modal->choosing_slot = false;
    modal->hovered_button = -1;
    modal->hovered_slot = -1;
    modal->confirmed = false;
    modal->was_equipped = false;
    modal->equipped_slot = -1;
    modal->chips_gained = 0;

    // Start with full opacity (no fade-in animation needed)
    modal->anim_stage = TRINKET_ANIM_NONE;
    modal->fade_alpha = 1.0f;
    modal->trinket_pos_x = 0.0f;
    modal->trinket_pos_y = 0.0f;
    modal->trinket_scale = 1.0f;
    modal->slot_flash_alpha = 0.0f;
    modal->slot_scale = 1.0f;
    modal->chip_flash_alpha = 0.0f;
    modal->result_timer = 0.0f;

    // Load and cache NEW affix templates (enemy pattern: on-demand)
    modal->cached_affix_count = trinket_drop->affix_count;
    for (int i = 0; i < trinket_drop->affix_count && i < 3; i++) {
        const char* stat_key = d_StringPeek(trinket_drop->affixes[i].stat_key);
        modal->cached_affix_templates[i] = LoadAffixTemplateFromDUF(stat_key);
        if (!modal->cached_affix_templates[i]) {
            d_LogWarningF("Failed to load affix template for: %s", stat_key);
        }
    }

    d_LogInfoF("Showing trinket drop modal: %s", d_StringPeek(template->name));
}

void HideTrinketDropModal(TrinketDropModal_t* modal) {
    if (!modal) return;

    modal->is_visible = false;

    // Clean up cached affix templates
    for (int i = 0; i < modal->cached_affix_count; i++) {
        if (modal->cached_affix_templates[i]) {
            CleanupAffixTemplate(modal->cached_affix_templates[i]);
            free(modal->cached_affix_templates[i]);
            modal->cached_affix_templates[i] = NULL;
        }
    }
    modal->cached_affix_count = 0;

    d_LogDebug("Hiding trinket drop modal");
}

bool IsTrinketDropModalVisible(const TrinketDropModal_t* modal) {
    return modal && modal->is_visible;
}

// ============================================================================
// INPUT & UPDATE
// ============================================================================

bool HandleTrinketDropModalInput(TrinketDropModal_t* modal, Player_t* player, float dt) {
    if (!modal || !player || !modal->is_visible) return false;

    // ========================================================================
    // ANIMATION STATE MACHINE (multi-stage like rewardModal)
    // ========================================================================
    if (modal->confirmed) {
        switch (modal->anim_stage) {
            case TRINKET_ANIM_FADE_OUT:
                // Stage 1: Wait for fade-out to complete (0.3s)
                if (modal->fade_alpha <= 0.01f) {
                    if (modal->was_equipped) {
                        // EQUIP PATH: Start flying trinket to slot
                        modal->anim_stage = TRINKET_ANIM_FLY_TO_SLOT;

                        // Start position: center screen
                        modal->trinket_pos_x = SCREEN_WIDTH / 2.0f;
                        modal->trinket_pos_y = SCREEN_HEIGHT / 2.0f;
                        modal->trinket_scale = 2.0f;  // Start big

                        // End position: target slot
                        int slot = modal->equipped_slot;
                        int col = slot % 3;
                        int row = slot / 3;
                        float target_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP) + TRINKET_SLOT_SIZE / 2.0f;
                        float target_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP) + TRINKET_SLOT_SIZE / 2.0f;

                        // Tween to target (0.6s)
                        TweenFloat(&g_tween_manager, &modal->trinket_pos_x, target_x, 0.6f, TWEEN_EASE_OUT_CUBIC);
                        TweenFloat(&g_tween_manager, &modal->trinket_pos_y, target_y, 0.6f, TWEEN_EASE_OUT_CUBIC);
                        TweenFloat(&g_tween_manager, &modal->trinket_scale, 1.0f, 0.6f, TWEEN_EASE_OUT_CUBIC);

                        d_LogDebug("Trinket animation: Flying trinket to slot");
                    } else {
                        // SELL PATH: Fly trinket to chips display (top-left)
                        modal->anim_stage = TRINKET_ANIM_FLY_TO_CHIPS;

                        // Start position: center screen
                        modal->trinket_pos_x = SCREEN_WIDTH / 2.0f;
                        modal->trinket_pos_y = SCREEN_HEIGHT / 2.0f;
                        modal->trinket_scale = 2.0f;  // Start big

                        // End position: chips display (top-left, estimated position)
                        float target_x = 120.0f;  // Top-left area (adjust based on actual chip UI)
                        float target_y = 40.0f;

                        // Tween to chips (0.6s, same as equip for consistency)
                        TweenFloat(&g_tween_manager, &modal->trinket_pos_x, target_x, 0.6f, TWEEN_EASE_OUT_CUBIC);
                        TweenFloat(&g_tween_manager, &modal->trinket_pos_y, target_y, 0.6f, TWEEN_EASE_OUT_CUBIC);
                        TweenFloat(&g_tween_manager, &modal->trinket_scale, 0.5f, 0.6f, TWEEN_EASE_OUT_CUBIC);  // Shrink to 0.5x

                        d_LogDebug("Trinket animation: Flying trinket to chips");
                    }
                }
                break;

            case TRINKET_ANIM_FLY_TO_SLOT:
                // Stage 1: Wait for fly animation to complete (1.0s)
                if (modal->trinket_scale <= 0.81f) {
                    // Start slot highlight (longer, more visible)
                    modal->anim_stage = TRINKET_ANIM_SLOT_HIGHLIGHT;
                    modal->slot_flash_alpha = 1.0f;  // Start at full brightness
                    modal->slot_scale = 1.0f;

                    // Signal sceneBlackjack to equip trinket NOW (during animation)
                    modal->should_equip_now = true;

                    // Flash stays bright longer
                    TweenFloat(&g_tween_manager, &modal->slot_flash_alpha, 0.0f, 0.8f, TWEEN_EASE_OUT_QUAD);
                    // Big bounce for visibility
                    TweenFloat(&g_tween_manager, &modal->slot_scale, 1.3f, 0.4f, TWEEN_EASE_OUT_BOUNCE);

                    d_LogDebug("Trinket animation: Highlighting slot");
                }
                break;

            case TRINKET_ANIM_SLOT_HIGHLIGHT:
                // Stage 2: Wait for highlight to complete (0.8s)
                if (modal->slot_flash_alpha <= 0.01f) {
                    modal->anim_stage = TRINKET_ANIM_COMPLETE;
                    modal->result_timer = 0.0f;
                    d_LogDebug("Trinket animation: Slot highlight complete");
                }
                break;

            case TRINKET_ANIM_FLY_TO_CHIPS:
                // Wait for fly-to-chips to complete (0.6s)
                if (modal->trinket_scale <= 0.51f) {
                    // Spawn coin particles
                    modal->anim_stage = TRINKET_ANIM_COIN_BURST;
                    modal->particle_count = 15;  // Spawn 15 coins

                    // Spawn position: where trinket landed (top-left chips area)
                    float spawn_x = modal->trinket_pos_x;
                    float spawn_y = modal->trinket_pos_y;

                    for (int i = 0; i < modal->particle_count; i++) {
                        CoinParticle_t* p = &modal->particles[i];

                        // Random velocity (burst outward)
                        float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;  // Random angle
                        float speed = 100.0f + ((float)rand() / RAND_MAX) * 150.0f;  // 100-250 px/s

                        p->x = spawn_x;
                        p->y = spawn_y;
                        p->vx = cosf(angle) * speed;
                        p->vy = sinf(angle) * speed - 50.0f;  // Bias upward
                        p->lifetime = 0.0f;
                        p->max_lifetime = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;  // 0.5-1.0s
                        p->rotation = ((float)rand() / RAND_MAX) * 360.0f;
                        p->rotation_speed = -200.0f + ((float)rand() / RAND_MAX) * 400.0f;  // -200 to +200 deg/s
                        p->scale = 0.5f + ((float)rand() / RAND_MAX) * 0.5f;  // 0.5-1.0
                    }

                    // Start chip flash text animation immediately (fade in during coin burst)
                    modal->chip_flash_alpha = 0.0f;
                    TweenFloat(&g_tween_manager, &modal->chip_flash_alpha, 1.0f, 0.8f, TWEEN_EASE_OUT_QUAD);

                    d_LogDebug("Trinket animation: Coin burst + text fade-in");
                }
                break;

            case TRINKET_ANIM_COIN_BURST:
                // Update particles (integrate with dt)
                {
                    for (int i = 0; i < modal->particle_count; i++) {
                        CoinParticle_t* p = &modal->particles[i];
                        if (p->lifetime < p->max_lifetime) {
                            // Update physics
                            p->x += p->vx * dt;
                            p->y += p->vy * dt;
                            p->vy += 300.0f * dt;  // Gravity
                            p->rotation += p->rotation_speed * dt;
                            p->lifetime += dt;
                        }
                    }

                    // Transition to chip flash after 0.8s (let particles play out)
                    modal->result_timer += dt;
                    if (modal->result_timer >= 0.8f) {
                        modal->anim_stage = TRINKET_ANIM_FLASH_CHIPS;
                        modal->chip_flash_alpha = 0.0f;
                        modal->result_timer = 0.0f;
                        TweenFloat(&g_tween_manager, &modal->chip_flash_alpha, 1.0f, 0.5f, TWEEN_EASE_OUT_QUAD);
                        d_LogDebug("Trinket animation: Flashing chips");
                    }
                }
                break;

            case TRINKET_ANIM_FLASH_CHIPS:
                // Wait for flash to complete, then fade out
                modal->result_timer += dt;
                if (modal->result_timer >= 1.3f) {  // 0.8s fade-in + 0.5s hold
                    // Fade out entire sell animation (0.2s)
                    modal->fade_alpha = 1.0f;
                    TweenFloat(&g_tween_manager, &modal->fade_alpha, 0.0f, 0.2f, TWEEN_EASE_OUT_QUAD);
                    modal->anim_stage = TRINKET_ANIM_COMPLETE;
                    modal->result_timer = 0.0f;
                    d_LogDebug("Trinket animation: Fading out sell animation");
                }
                break;

            case TRINKET_ANIM_COMPLETE:
                // Equip path: Close immediately (no fade-out)
                // Sell path: Wait for fade-out to finish (0.2s) before closing
                if (modal->was_equipped) {
                    return true;  // Equip animation done, close immediately
                } else {
                    // Sell animation: wait for fade
                    if (modal->fade_alpha <= 0.01f) {
                        return true;  // Fade complete, close modal
                    }
                }
                break;

            case TRINKET_ANIM_NONE:
            case TRINKET_ANIM_FADE_IN:
                // Should not happen during confirmed state
                return true;
        }

        return false;  // Still animating
    }

    int mouse_x = app.mouse.x;
    int mouse_y = app.mouse.y;
    bool clicked = app.mouse.pressed;

    // Calculate modal position (centered)
    int modal_x = (SCREEN_WIDTH - TRINKET_DROP_MODAL_WIDTH) / 2;
    int modal_y = (SCREEN_HEIGHT - TRINKET_DROP_MODAL_HEIGHT) / 2;

    // ========================================================================
    // KEYBOARD HOTKEYS (reward modal pattern)
    // ========================================================================
    int prev_key_held = modal->key_held_button;

    // Check which key is currently held (else if chain - only ONE key can be held)
    modal->key_held_button = -1;
    if (app.keyboard[SDL_SCANCODE_E]) modal->key_held_button = 0;       // E = Equip
    else if (app.keyboard[SDL_SCANCODE_S]) modal->key_held_button = 1;  // S = Sell

    // Trigger on key release (ANY key was held, now NO key is held)
    if (prev_key_held != -1 && modal->key_held_button == -1) {
        if (prev_key_held == 0) {
            // E released - equip to first available slot
            int slot = -1;
            for (int i = 0; i < 6; i++) {
                if (!player->trinket_slot_occupied[i]) {
                    slot = i;
                    break;
                }
            }

            if (slot >= 0) {
                modal->was_equipped = true;
                modal->equipped_slot = slot;
                modal->confirmed = true;

                // Start fade-out animation (0.3s to match ADR-12 timing)
                modal->anim_stage = TRINKET_ANIM_FADE_OUT;
                modal->fade_alpha = 1.0f;
                TweenFloat(&g_tween_manager, &modal->fade_alpha, 0.0f, 0.3f, TWEEN_EASE_OUT_QUAD);

                d_LogInfoF("Trinket equipped to slot %d (hotkey) - starting animation", slot);
                return false;  // Don't close yet, show animation
            } else {
                d_LogWarning("No available trinket slots!");
                return false;
            }
        } else if (prev_key_held == 1) {
            // S released - sell trinket
            modal->was_equipped = false;
            modal->equipped_slot = -1;
            modal->chips_gained = modal->trinket_drop.sell_value;
            modal->confirmed = true;

            // Start fade-out animation
            modal->anim_stage = TRINKET_ANIM_FADE_OUT;
            modal->fade_alpha = 1.0f;
            TweenFloat(&g_tween_manager, &modal->fade_alpha, 0.0f, 0.2f, TWEEN_EASE_OUT_QUAD);

            d_LogInfoF("Trinket sold for %d chips (hotkey) - starting animation", modal->chips_gained);
            return false;  // Don't close yet, show animation
        }
    }

    // ========================================================================
    // MOUSE INPUT
    // ========================================================================

    // Calculate button positions (centered at bottom)
    int button_y = modal_y + TRINKET_DROP_MODAL_HEIGHT - TRINKET_DROP_MODAL_PADDING - TRINKET_DROP_BUTTON_HEIGHT;
    int total_button_width = 2 * TRINKET_DROP_BUTTON_WIDTH + TRINKET_DROP_BUTTON_SPACING;
    int button_start_x = modal_x + (TRINKET_DROP_MODAL_WIDTH - total_button_width) / 2;

    int equip_button_x = button_start_x;
    int sell_button_x = button_start_x + TRINKET_DROP_BUTTON_WIDTH + TRINKET_DROP_BUTTON_SPACING;

    // Check Equip button hover/click
    bool equip_hovered = (mouse_x >= equip_button_x && mouse_x < equip_button_x + TRINKET_DROP_BUTTON_WIDTH &&
                          mouse_y >= button_y && mouse_y < button_y + TRINKET_DROP_BUTTON_HEIGHT);

    // Check Sell button hover/click
    bool sell_hovered = (mouse_x >= sell_button_x && mouse_x < sell_button_x + TRINKET_DROP_BUTTON_WIDTH &&
                         mouse_y >= button_y && mouse_y < button_y + TRINKET_DROP_BUTTON_HEIGHT);

    // Key held counts as "hovered" for visual feedback
    if (modal->key_held_button == 0) equip_hovered = true;
    if (modal->key_held_button == 1) sell_hovered = true;

    // Update hovered button index
    if (equip_hovered) modal->hovered_button = 0;
    else if (sell_hovered) modal->hovered_button = 1;
    else modal->hovered_button = -1;

    // Update button scale animations (smooth transitions)
    float target_equip_scale = equip_hovered ? 1.05f : 1.0f;
    float target_sell_scale = sell_hovered ? 1.05f : 1.0f;

    // Smooth interpolation (lerp)
    float lerp_speed = 10.0f;
    modal->equip_button_scale += (target_equip_scale - modal->equip_button_scale) * lerp_speed * dt;
    modal->sell_button_scale += (target_sell_scale - modal->sell_button_scale) * lerp_speed * dt;

    // Handle mouse clicks
    if (clicked) {
        if (equip_hovered) {
            // Equip to first available slot
            int slot = -1;
            for (int i = 0; i < 6; i++) {
                if (!player->trinket_slot_occupied[i]) {
                    slot = i;
                    break;
                }
            }

            if (slot >= 0) {
                modal->was_equipped = true;
                modal->equipped_slot = slot;
                modal->confirmed = true;

                // SKIP FADE - Go straight to fly animation (modal stays visible in background)
                modal->anim_stage = TRINKET_ANIM_FLY_TO_SLOT;

                // Start position: center screen
                modal->trinket_pos_x = SCREEN_WIDTH / 2.0f;
                modal->trinket_pos_y = SCREEN_HEIGHT / 2.0f;
                modal->trinket_scale = 2.5f;  // Start bigger for visibility

                // End position: target slot
                int col = slot % 3;
                int row = slot / 3;
                float target_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP) + TRINKET_SLOT_SIZE / 2.0f;
                float target_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP) + TRINKET_SLOT_SIZE / 2.0f;

                // Tween to target (1.0s - slow enough to see clearly)
                TweenFloat(&g_tween_manager, &modal->trinket_pos_x, target_x, 1.0f, TWEEN_EASE_IN_OUT_CUBIC);
                TweenFloat(&g_tween_manager, &modal->trinket_pos_y, target_y, 1.0f, TWEEN_EASE_IN_OUT_CUBIC);
                TweenFloat(&g_tween_manager, &modal->trinket_scale, 0.8f, 1.0f, TWEEN_EASE_IN_OUT_CUBIC);

                d_LogInfoF("Trinket equipped to slot %d (click) - starting fly animation", slot);
                return false;  // Don't close yet, show animation
            } else {
                d_LogWarning("No available trinket slots!");
                return false;
            }
        } else if (sell_hovered) {
            // Sell trinket for chips
            modal->was_equipped = false;
            modal->equipped_slot = -1;
            modal->chips_gained = modal->trinket_drop.sell_value;
            modal->confirmed = true;

            // Start fade-out animation
            modal->anim_stage = TRINKET_ANIM_FADE_OUT;
            modal->fade_alpha = 1.0f;
            TweenFloat(&g_tween_manager, &modal->fade_alpha, 0.0f, 0.2f, TWEEN_EASE_OUT_QUAD);

            d_LogInfoF("Trinket sold for %d chips (click) - starting animation", modal->chips_gained);
            return false;  // Don't close yet, show animation
        }
    }

    return false;
}

// ============================================================================
// RENDERING
// ============================================================================

void RenderTrinketDropModal(const TrinketDropModal_t* modal, const Player_t* player) {
    if (!modal || !modal->is_visible || !modal->template) return;

    // Calculate modal position (centered)
    int modal_x = (SCREEN_WIDTH - TRINKET_DROP_MODAL_WIDTH) / 2;
    int modal_y = (SCREEN_HEIGHT - TRINKET_DROP_MODAL_HEIGHT) / 2;

    // ========================================================================
    // ANIMATION CHECK - Complete stage shows overlay only
    // ========================================================================
    if (modal->anim_stage == TRINKET_ANIM_COMPLETE) {
        // Animation complete, still show dark overlay but skip modal
        a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, COLOR_OVERLAY);
        return;
    }

    // ========================================================================
    // NORMAL MODAL RENDERING (fade in/out or static)
    // ========================================================================

    // Calculate alpha for fade animations
    uint8_t alpha = 255;
    if (modal->anim_stage == TRINKET_ANIM_FADE_OUT) {
        alpha = (uint8_t)(modal->fade_alpha * 255.0f);
    }

    // Skip modal background during ALL animation stages (equip and sell)
    bool skip_modal_bg = (modal->anim_stage == TRINKET_ANIM_FLY_TO_SLOT ||
                          modal->anim_stage == TRINKET_ANIM_SLOT_HIGHLIGHT ||
                          modal->anim_stage == TRINKET_ANIM_FLY_TO_CHIPS ||
                          modal->anim_stage == TRINKET_ANIM_COIN_BURST ||
                          modal->anim_stage == TRINKET_ANIM_FLASH_CHIPS ||
                          modal->anim_stage == TRINKET_ANIM_COMPLETE);

    if (!skip_modal_bg) {
        // Dark overlay (ADR-008 color palette) - apply fade alpha
        aColor_t overlay_color = COLOR_OVERLAY;
        overlay_color.a = (uint8_t)((overlay_color.a / 255.0f) * alpha);
        a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, overlay_color);

        // Modal background (ADR-008 color palette) - apply fade alpha
        aColor_t bg_color = COLOR_PANEL_BG;
        bg_color.a = alpha;
        a_DrawFilledRect((aRectf_t){modal_x, modal_y, TRINKET_DROP_MODAL_WIDTH, TRINKET_DROP_MODAL_HEIGHT}, bg_color);

        // Modal border (ADR-008 color palette) - apply fade alpha
        aColor_t border_color = COLOR_HEADER_BORDER;
        border_color.a = alpha;
        a_DrawRect((aRectf_t){modal_x, modal_y, TRINKET_DROP_MODAL_WIDTH, TRINKET_DROP_MODAL_HEIGHT}, border_color);
    }

    // Content area (only used if not skipping modal)
    int content_x = modal_x + TRINKET_DROP_MODAL_PADDING;
    int content_y = modal_y + TRINKET_DROP_MODAL_PADDING;
    int content_width = TRINKET_DROP_MODAL_WIDTH - (TRINKET_DROP_MODAL_PADDING * 2);

    // Skip all modal content during equip animation stages
    if (!skip_modal_bg) {

    // ========================================================================
    // HEADER: "TRINKET DROP!"
    // ========================================================================
    aTextStyle_t header_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = COLOR_HEADER_TEXT,  // Cream (ADR-008)
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.2f
    };
    a_DrawText("TRINKET DROP!", modal_x + TRINKET_DROP_MODAL_WIDTH / 2, content_y, header_config);
    content_y += 40;

    content_y += 2;  // Top margin for trinket info row

    // ========================================================================
    // TRINKET INFO ROW: [Icon Container] [Name + Rarity Circle] - CENTERED
    // ========================================================================
    const char* trinket_name = d_StringPeek(modal->template->name);
    int rarity_r, rarity_g, rarity_b;
    GetTrinketRarityColor(modal->trinket_drop.rarity, &rarity_r, &rarity_g, &rarity_b);
    aColor_t rarity_color = {(uint8_t)rarity_r, (uint8_t)rarity_g, (uint8_t)rarity_b, 255};

    // Icon size and dimensions
    int icon_size = 64;
    int circle_radius = 8;
    int circle_diameter = circle_radius * 2;

    // Calculate actual trinket name width
    float name_w = 0, name_h = 0;
    a_CalcTextDimensions(trinket_name, FONT_ENTER_COMMAND, &name_w, &name_h);
    int name_width = (int)name_w + 10;  // Add 10px padding

    // Create flexbox for icon/name/circle row (centered)
    FlexBox_t* header_row = a_FlexBoxCreate(modal_x, content_y, TRINKET_DROP_MODAL_WIDTH, icon_size);
    if (!header_row) {
        content_y += icon_size + 10;
    } else {
        a_FlexConfigure(header_row, FLEX_DIR_ROW, FLEX_JUSTIFY_CENTER, 15);  // 15px gap

        // Add 3 items: icon (64px), name (calculated width), circle (16px diameter)
        a_FlexAddItem(header_row, icon_size, icon_size, NULL);
        a_FlexAddItem(header_row, name_width, icon_size, NULL);
        a_FlexAddItem(header_row, circle_diameter, icon_size, NULL);

        // Calculate layout
        a_FlexLayout(header_row);

        // Get item positions
        const FlexItem_t* icon_item = a_FlexGetItem(header_row, 0);
        const FlexItem_t* name_item = a_FlexGetItem(header_row, 1);
        const FlexItem_t* circle_item = a_FlexGetItem(header_row, 2);

        // Draw icon container
        if (icon_item) {
            int icon_x = icon_item->calc_x;
            int icon_y = icon_item->calc_y;
            a_DrawFilledRect((aRectf_t){icon_x, icon_y, icon_size, icon_size}, COLOR_PANEL_BG);
            // Draw rarity-colored border (double border for emphasis)
            a_DrawRect((aRectf_t){icon_x, icon_y, icon_size, icon_size}, rarity_color);
            a_DrawRect((aRectf_t){icon_x + 1, icon_y + 1, icon_size - 2, icon_size - 2}, rarity_color);
        }

        // Draw trinket name
        if (name_item) {
            int name_y = name_item->calc_y + 12;  // Vertically centered-ish
            aTextStyle_t name_config = {
                .type = FONT_ENTER_COMMAND,
                .fg = COLOR_TAG_GOLD,
                .align = TEXT_ALIGN_LEFT,
                .wrap_width = 0,
                .scale = 1.0f
            };
            a_DrawText((char*)trinket_name, name_item->calc_x, name_y, name_config);
        }

        // Draw rarity circle
        if (circle_item) {
            int circle_x = circle_item->calc_x + circle_radius;  // Center of circle space
            int circle_y = circle_item->calc_y + (icon_size / 2);  // Vertically centered
            a_DrawFilledCircle(circle_x, circle_y, circle_radius, rarity_color);
            // Draw darker border (same hue, darker)
            aColor_t circle_border_color = {
                (uint8_t)(rarity_r * 0.6f),
                (uint8_t)(rarity_g * 0.6f),
                (uint8_t)(rarity_b * 0.6f),
                255
            };
            a_DrawCircle(circle_x, circle_y, circle_radius, circle_border_color);
        }

        // Cleanup flexbox
        a_FlexBoxDestroy(&header_row);

        content_y += icon_size + 10;  // Move down past icon container
    }
    content_y += 2;  // Bottom margin for trinket info row

    // ========================================================================
    // FLAVOR TEXT (with dark background)
    // ========================================================================
    const char* flavor = d_StringPeek(modal->template->flavor);
    aTextStyle_t flavor_config = {
        .type = FONT_GAME,
        .fg = {150, 150, 150, 255},  // Dimmed
        .bg = {0, 0, 0, 255},  // Dark black background
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = content_width - 40,
        .scale = 1.0f,
        .padding = 8  // Add padding around text for background
    };
    int flavor_height = a_GetWrappedTextHeight((char*)flavor, FONT_GAME, content_width - 40);
    a_DrawText((char*)flavor, modal_x + TRINKET_DROP_MODAL_WIDTH / 2, content_y, flavor_config);
    content_y += flavor_height + 15;

    // Divider
    a_DrawFilledRect((aRectf_t){content_x, content_y, content_width, 2}, (aColor_t){100, 100, 100, 200});
    content_y += 15;

    // ========================================================================
    // PASSIVE DESCRIPTION
    // ========================================================================
    aTextStyle_t passive_label_config = {
        .type = FONT_GAME,
        .fg = COLOR_EQUIP_ACCENT,  // Green (ADR-008)
        .align = TEXT_ALIGN_LEFT,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText("Passive:", content_x, content_y, passive_label_config);
    content_y += 20;

    // Format primary passive
    dString_t* primary_passive = FormatTrinketPassive(modal->template, &modal->trinket_drop, false);
    if (primary_passive && d_StringGetLength(primary_passive) > 0) {
        aTextStyle_t passive_desc_config = {
            .type = FONT_GAME,
            .fg = COLOR_INFO_TEXT,  // Light gray (ADR-008)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width - 20,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(primary_passive), content_x + 10, content_y, passive_desc_config);
        content_y += 25;
    }
    if (primary_passive) d_StringDestroy(primary_passive);

    // Format secondary passive (if exists)
    dString_t* secondary_passive = FormatTrinketPassive(modal->template, &modal->trinket_drop, true);
    if (secondary_passive && d_StringGetLength(secondary_passive) > 0) {
        aTextStyle_t passive_desc_config = {
            .type = FONT_GAME,
            .fg = COLOR_INFO_TEXT,  // Light gray (ADR-008)
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = content_width - 20,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(secondary_passive), content_x + 10, content_y, passive_desc_config);
        content_y += 25;
    }
    if (secondary_passive) d_StringDestroy(secondary_passive);

    // ========================================================================
    // AFFIXES
    // ========================================================================
    if (modal->trinket_drop.affix_count > 0) {
        aTextStyle_t affix_label_config = {
            .type = FONT_GAME,
            .fg = {100, 150, 255, 255},  // Blue
            .align = TEXT_ALIGN_LEFT,
            .wrap_width = 0,
            .scale = 1.0f
        };
        a_DrawText("Affixes:", content_x, content_y, affix_label_config);
        content_y += 20;

        for (int i = 0; i < modal->trinket_drop.affix_count; i++) {
            int value = modal->trinket_drop.affixes[i].rolled_value;

            // Use cached affix template
            AffixTemplate_t* affix_template = modal->cached_affix_templates[i];
            if (!affix_template) {
                continue;
            }

            const char* affix_name = d_StringPeek(affix_template->name);
            int weight = affix_template->weight;

            // Calculate weight-based color gradient
            // Weight range: 25 (rare/orange) to 100 (common/white)
            // Normalized: 0.0 (rare) to 1.0 (common)
            float t = (weight - 25.0f) / 75.0f;  // 0.0 = rare, 1.0 = common
            t = fmaxf(0.0f, fminf(1.0f, t));     // Clamp to [0, 1]

            // Interpolate: Orange (255,140,0) -> White (200,200,200)
            uint8_t name_r = (uint8_t)(255.0f - (55.0f * t));   // 255 -> 200
            uint8_t name_g = (uint8_t)(140.0f + (60.0f * t));   // 140 -> 200
            uint8_t name_b = (uint8_t)(0.0f + (200.0f * t));    // 0 -> 200

            // Format description with rolled value
            const char* desc_template = d_StringPeek(affix_template->description);
            dString_t* temp_desc = d_StringInit();
            const char* desc_str = desc_template;
            bool replaced = false;
            for (size_t j = 0; desc_str[j] != '\0'; j++) {
                if (desc_str[j] == '{' && desc_str[j+1] == 'v' && desc_str[j+2] == 'a' &&
                    desc_str[j+3] == 'l' && desc_str[j+4] == 'u' && desc_str[j+5] == 'e' &&
                    desc_str[j+6] == '}' && !replaced) {
                    // Found {value}, replace with actual value
                    d_StringAppendInt(temp_desc, value);
                    j += 6;  // Skip past {value}
                    replaced = true;
                } else {
                    d_StringAppendChar(temp_desc, desc_str[j]);
                }
            }

            // Format roll range
            dString_t* range_text = d_StringInit();
            d_StringFormat(range_text, "(%d-%d)", affix_template->min_value, affix_template->max_value);

            // Calculate actual text widths for each column (with scale factors)
            float affix_name_w = 0, affix_name_h = 0;
            float desc_w = 0, desc_h = 0;
            float range_w = 0, range_h = 0;

            a_CalcTextDimensions(affix_name, FONT_ENTER_COMMAND, &affix_name_w, &affix_name_h);
            a_CalcTextDimensions(d_StringPeek(temp_desc), FONT_GAME, &desc_w, &desc_h);
            a_CalcTextDimensions(d_StringPeek(range_text), FONT_GAME, &range_w, &range_h);

            // Apply scale factors and add padding
            int name_col_width = (int)(affix_name_w * 1.1f) + 15;  // 1.1 scale + padding
            int desc_col_width = (int)(desc_w * 1.0f) + 15;        // 1.0 scale + padding
            int range_col_width = (int)(range_w * 0.9f) + 10;      // 0.9 scale + padding

            // Create FlexBox row layout for this affix line
            FlexBox_t* affix_row = a_FlexBoxCreate(content_x, content_y, content_width, 25);
            if (affix_row) {
                a_FlexConfigure(affix_row, FLEX_DIR_ROW, FLEX_JUSTIFY_START, 8);  // 8px gap

                // Add 3 items: name (calculated), description (calculated), roll_range (calculated)
                a_FlexAddItem(affix_row, name_col_width, 25, NULL);
                a_FlexAddItem(affix_row, desc_col_width, 25, NULL);
                a_FlexAddItem(affix_row, range_col_width, 25, NULL);

                // Calculate layout
                a_FlexLayout(affix_row);

                // Render name (bold, weight-colored, scale 1.1) - RAISE IT
                const FlexItem_t* name_item = a_FlexGetItem(affix_row, 0);
                if (name_item) {
                    aTextStyle_t name_config = {
                        .type = FONT_ENTER_COMMAND,
                        .fg = {name_r, name_g, name_b, 255},
                        .align = TEXT_ALIGN_LEFT,
                        .wrap_width = 0,
                        .scale = 1.1f
                    };
                    a_DrawText((char*)affix_name, name_item->calc_x, name_item->calc_y - 5, name_config);
                }

                // Calculate roll quality percentage
                int min_val = affix_template->min_value;
                int max_val = affix_template->max_value;
                float roll_range = (float)(max_val - min_val);
                float roll_quality = (roll_range > 0) ? ((float)(value - min_val) / roll_range) : 0.5f;
                roll_quality = fmaxf(0.0f, fminf(1.0f, roll_quality));  // Clamp [0, 1]

                // Color gradient based on roll quality:
                // 0-30%: Gray (160, 160, 160)
                // 31-70%: Gray -> White (160->235)
                // 71-100%: White -> Green (235,235,235 -> 168,202,88 from palette)
                uint8_t desc_r, desc_g, desc_b;
                uint8_t range_r, range_g, range_b;

                if (roll_quality <= 0.30f) {
                    // Low roll: Gray
                    desc_r = desc_g = desc_b = 160;
                } else if (roll_quality <= 0.70f) {
                    // Mid roll: Gray -> White
                    float t = (roll_quality - 0.30f) / 0.40f;  // 0.0 to 1.0
                    desc_r = desc_g = desc_b = (uint8_t)(160 + (75 * t));  // 160 -> 235
                } else {
                    // High roll: White -> Green
                    float t = (roll_quality - 0.70f) / 0.30f;  // 0.0 to 1.0
                    desc_r = (uint8_t)(235 - (67 * t));   // 235 -> 168 (palette green)
                    desc_g = (uint8_t)(235 - (33 * t));   // 235 -> 202 (palette green)
                    desc_b = (uint8_t)(235 - (147 * t));  // 235 -> 88 (palette green)
                }

                // Range text color (same gray unless perfect roll)
                if (roll_quality >= 0.99f) {
                    // Perfect roll: Green
                    range_r = 168;
                    range_g = 202;
                    range_b = 88;
                } else {
                    // Normal: Dim gray
                    range_r = range_g = range_b = 120;
                }

                // Render description with roll-quality color
                const FlexItem_t* desc_item = a_FlexGetItem(affix_row, 1);
                if (desc_item) {
                    aTextStyle_t desc_config = {
                        .type = FONT_GAME,
                        .fg = {desc_r, desc_g, desc_b, 255},
                        .align = TEXT_ALIGN_LEFT,
                        .wrap_width = 0,
                        .scale = 1.0f
                    };
                    a_DrawText((char*)d_StringPeek(temp_desc), desc_item->calc_x, desc_item->calc_y + 2, desc_config);
                }

                // Render roll range with color (green if perfect roll)
                const FlexItem_t* range_item = a_FlexGetItem(affix_row, 2);
                if (range_item) {
                    aTextStyle_t range_config = {
                        .type = FONT_GAME,
                        .fg = {range_r, range_g, range_b, 255},
                        .align = TEXT_ALIGN_LEFT,
                        .wrap_width = 0,
                        .scale = 0.9f
                    };
                    a_DrawText((char*)d_StringPeek(range_text), range_item->calc_x, range_item->calc_y + 1, range_config);
                }

                // Cleanup FlexBox
                a_FlexBoxDestroy(&affix_row);
            }

            // Cleanup strings
            d_StringDestroy(temp_desc);
            d_StringDestroy(range_text);

            content_y += 25;  // Move down for next affix (matches FlexBox height)
        }

        content_y += 5;
    }

    // ========================================================================
    // EQUIP/SELL BUTTONS (with labels above)
    // ========================================================================
    // Hide buttons during ALL animation stages (equip and sell)
    if (modal->anim_stage != TRINKET_ANIM_FLY_TO_SLOT &&
        modal->anim_stage != TRINKET_ANIM_SLOT_HIGHLIGHT &&
        modal->anim_stage != TRINKET_ANIM_FLY_TO_CHIPS &&
        modal->anim_stage != TRINKET_ANIM_COIN_BURST &&
        modal->anim_stage != TRINKET_ANIM_FLASH_CHIPS) {

    int button_y = modal_y + TRINKET_DROP_MODAL_HEIGHT - TRINKET_DROP_MODAL_PADDING - TRINKET_DROP_BUTTON_HEIGHT;
    int total_button_width = 2 * TRINKET_DROP_BUTTON_WIDTH + TRINKET_DROP_BUTTON_SPACING;
    int button_start_x = modal_x + (TRINKET_DROP_MODAL_WIDTH - total_button_width) / 2;

    int equip_button_x = button_start_x;
    int sell_button_x = button_start_x + TRINKET_DROP_BUTTON_WIDTH + TRINKET_DROP_BUTTON_SPACING;

    // ====================================================================
    // SLOT LABEL (above equip button, 70% opacity)
    // ====================================================================
    // Find first available slot
    int target_slot = -1;
    for (int i = 0; i < 6; i++) {
        if (!player->trinket_slot_occupied[i]) {
            target_slot = i;
            break;
        }
    }

    if (target_slot >= 0) {
        dString_t* slot_text = d_StringInit();
        d_StringFormat(slot_text, "Slot %d", target_slot);
        aTextStyle_t slot_label_config = {
            .type = FONT_GAME,
            .fg = {200, 200, 200, 179},  // Light gray at 70% opacity
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.0f
        };
        a_DrawText((char*)d_StringPeek(slot_text),
                   equip_button_x + TRINKET_DROP_BUTTON_WIDTH / 2,
                   button_y - 25,
                   slot_label_config);
        d_StringDestroy(slot_text);
    }

    // ====================================================================
    // EQUIP BUTTON (Green accent, ADR-008 colors)
    // ====================================================================
    bool equip_hovered = (modal->hovered_button == 0);
    bool equip_pressed = modal->equip_button_pressed;

    // Determine button color (normal → hover → pressed)
    aColor_t equip_bg_color = COLOR_BUTTON_BG;
    if (equip_pressed) {
        equip_bg_color = COLOR_BUTTON_PRESSED;
    } else if (equip_hovered) {
        equip_bg_color = COLOR_BUTTON_HOVER;
    }

    // Apply button scale (1.0 normal, 1.05 hover, 0.95 pressed)
    float equip_scale = modal->equip_button_scale;
    int scaled_width = (int)(TRINKET_DROP_BUTTON_WIDTH * equip_scale);
    int scaled_height = (int)(TRINKET_DROP_BUTTON_HEIGHT * equip_scale);
    int scaled_x = equip_button_x + (TRINKET_DROP_BUTTON_WIDTH - scaled_width) / 2;
    int scaled_y = button_y + (TRINKET_DROP_BUTTON_HEIGHT - scaled_height) / 2;

    a_DrawFilledRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, equip_bg_color);
    a_DrawRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, COLOR_EQUIP_ACCENT);

    aTextStyle_t equip_text_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, 255},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText("EQUIP (E)", equip_button_x + TRINKET_DROP_BUTTON_WIDTH / 2, button_y, equip_text_config);

    // ====================================================================
    // SELL VALUE LABEL (above sell button, 90% opacity)
    // ====================================================================
    dString_t* sell_text = d_StringInit();
    d_StringFormat(sell_text, "%d chips", modal->trinket_drop.sell_value);
    aTextStyle_t sell_label_config = {
        .type = FONT_GAME,
        .fg = {255, 215, 0, 230},  // Gold at 90% opacity
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText((char*)d_StringPeek(sell_text),
               sell_button_x + TRINKET_DROP_BUTTON_WIDTH / 2,
               button_y - 25,
               sell_label_config);
    d_StringDestroy(sell_text);

    // ====================================================================
    // SELL BUTTON (Red-orange accent, ADR-008 colors)
    // ====================================================================
    bool sell_hovered = (modal->hovered_button == 1);
    bool sell_pressed = modal->sell_button_pressed;

    // Determine button color (normal → hover → pressed)
    aColor_t sell_bg_color = COLOR_BUTTON_BG;
    if (sell_pressed) {
        sell_bg_color = COLOR_BUTTON_PRESSED;
    } else if (sell_hovered) {
        sell_bg_color = COLOR_BUTTON_HOVER;
    }

    // Apply button scale (1.0 normal, 1.05 hover, 0.95 pressed)
    float sell_scale = modal->sell_button_scale;
    scaled_width = (int)(TRINKET_DROP_BUTTON_WIDTH * sell_scale);
    scaled_height = (int)(TRINKET_DROP_BUTTON_HEIGHT * sell_scale);
    scaled_x = sell_button_x + (TRINKET_DROP_BUTTON_WIDTH - scaled_width) / 2;
    scaled_y = button_y + (TRINKET_DROP_BUTTON_HEIGHT - scaled_height) / 2;

    a_DrawFilledRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, sell_bg_color);
    a_DrawRect((aRectf_t){scaled_x, scaled_y, scaled_width, scaled_height}, COLOR_SELL_ACCENT);

    aTextStyle_t sell_text_config = {
        .type = FONT_ENTER_COMMAND,
        .fg = {255, 255, 255, 255},
        .align = TEXT_ALIGN_CENTER,
        .wrap_width = 0,
        .scale = 1.0f
    };
    a_DrawText("SELL (S)", sell_button_x + TRINKET_DROP_BUTTON_WIDTH / 2, button_y, sell_text_config);

    }  // End button rendering (hidden during animation)

    }  // End skip_modal_bg - only render modal content when not in fly/highlight animation

    // ========================================================================
    // SLOT HIGHLIGHT ANIMATION OVERLAY (stage 3) - renders on top of trinket UI
    // ========================================================================
    if (modal->anim_stage == TRINKET_ANIM_SLOT_HIGHLIGHT) {
        // Get slot position
        int slot = modal->equipped_slot;
        int col = slot % 3;
        int row = slot / 3;
        int slot_x = TRINKET_UI_X + col * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);
        int slot_y = TRINKET_UI_Y + row * (TRINKET_SLOT_SIZE + TRINKET_SLOT_GAP);

        // Get rarity color for flash
        aColor_t rarity_color;
        switch (modal->trinket_drop.rarity) {
            case TRINKET_RARITY_COMMON:     rarity_color = (aColor_t){168, 181, 178, 255}; break;
            case TRINKET_RARITY_UNCOMMON:   rarity_color = (aColor_t){168, 202, 88, 255};  break;
            case TRINKET_RARITY_RARE:       rarity_color = (aColor_t){100, 150, 255, 255}; break;
            case TRINKET_RARITY_LEGENDARY:  rarity_color = (aColor_t){232, 193, 112, 255}; break;
            default:                         rarity_color = (aColor_t){255, 255, 255, 255}; break;
        }

        // Draw flash border with alpha
        uint8_t flash_alpha = (uint8_t)(modal->slot_flash_alpha * 255.0f);
        aColor_t flash_color = {rarity_color.r, rarity_color.g, rarity_color.b, flash_alpha};

        // Draw outer glow (scaled slot)
        int scaled_size = (int)(TRINKET_SLOT_SIZE * modal->slot_scale);
        int offset = (scaled_size - TRINKET_SLOT_SIZE) / 2;
        a_DrawRect((aRectf_t){slot_x - offset, slot_y - offset, scaled_size, scaled_size}, flash_color);

        // Draw inner border (normal size, brighter)
        a_DrawRect((aRectf_t){slot_x, slot_y, TRINKET_SLOT_SIZE, TRINKET_SLOT_SIZE}, flash_color);
        a_DrawRect((aRectf_t){slot_x + 1, slot_y + 1, TRINKET_SLOT_SIZE - 2, TRINKET_SLOT_SIZE - 2}, flash_color);
    }

    // ========================================================================
    // FLYING TRINKET ICON (renders ON TOP of modal + trinket UI)
    // ========================================================================
    if (modal->anim_stage == TRINKET_ANIM_FLY_TO_SLOT || modal->anim_stage == TRINKET_ANIM_FLY_TO_CHIPS) {
        const char* trinket_name = d_StringPeek(modal->template->name);

        // Get rarity color using centralized function
        int rarity_r, rarity_g, rarity_b;
        GetTrinketRarityColor(modal->trinket_drop.rarity, &rarity_r, &rarity_g, &rarity_b);
        aColor_t rarity_color = {(uint8_t)rarity_r, (uint8_t)rarity_g, (uint8_t)rarity_b, 255};

        // Calculate icon box size (scales with trinket_scale)
        int icon_size = (int)(TRINKET_SLOT_SIZE * modal->trinket_scale);
        int icon_x = (int)(modal->trinket_pos_x - icon_size / 2);
        int icon_y = (int)(modal->trinket_pos_y - icon_size / 2);

        // Draw dark background box (trinket container)
        a_DrawFilledRect((aRectf_t){icon_x, icon_y, icon_size, icon_size}, COLOR_PANEL_BG);

        // Draw rarity-colored border
        a_DrawRect((aRectf_t){icon_x, icon_y, icon_size, icon_size}, rarity_color);
        a_DrawRect((aRectf_t){icon_x + 1, icon_y + 1, icon_size - 2, icon_size - 2}, rarity_color);

        // Draw trinket name centered in box
        aTextStyle_t icon_text_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = rarity_color,
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 0.7f * modal->trinket_scale  // Scale text with box
        };
        a_DrawText((char*)trinket_name, (int)modal->trinket_pos_x, (int)modal->trinket_pos_y - 8, icon_text_config);
    }

    // ========================================================================
    // SELL ANIMATION: Dark overlay for all sell stages
    // ========================================================================
    if (modal->anim_stage == TRINKET_ANIM_FLY_TO_CHIPS ||
        modal->anim_stage == TRINKET_ANIM_COIN_BURST ||
        modal->anim_stage == TRINKET_ANIM_FLASH_CHIPS ||
        modal->anim_stage == TRINKET_ANIM_COMPLETE) {
        aColor_t overlay = COLOR_OVERLAY;
        // Apply fade-out during COMPLETE stage
        if (modal->anim_stage == TRINKET_ANIM_COMPLETE) {
            overlay.a = (uint8_t)((overlay.a / 255.0f) * modal->fade_alpha * 255.0f);
        }
        a_DrawFilledRect((aRectf_t){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT}, overlay);
    }

    // ========================================================================
    // COIN PARTICLES (sell animation burst)
    // ========================================================================
    if (modal->anim_stage == TRINKET_ANIM_COIN_BURST ||
        modal->anim_stage == TRINKET_ANIM_FLASH_CHIPS ||
        modal->anim_stage == TRINKET_ANIM_COMPLETE) {
        for (int i = 0; i < modal->particle_count; i++) {
            const CoinParticle_t* p = &modal->particles[i];

            // Skip dead particles
            if (p->lifetime >= p->max_lifetime) continue;

            // Fade out alpha based on lifetime (1.0 → 0.0)
            float life_percent = p->lifetime / p->max_lifetime;
            float particle_alpha = (1.0f - life_percent);

            // Apply fade-out during COMPLETE stage
            if (modal->anim_stage == TRINKET_ANIM_COMPLETE) {
                particle_alpha *= modal->fade_alpha;
            }

            uint8_t alpha = (uint8_t)(particle_alpha * 255.0f);

            // Gold color with fade
            aColor_t coin_color = {255, 215, 0, alpha};  // Gold

            // Coin size (8x8 square, scaled by particle scale)
            int coin_size = (int)(8.0f * p->scale);
            int coin_x = (int)(p->x - coin_size / 2);
            int coin_y = (int)(p->y - coin_size / 2);

            // Draw coin (filled square with border)
            a_DrawFilledRect((aRectf_t){coin_x, coin_y, coin_size, coin_size}, coin_color);

            // Darker border
            aColor_t border_color = {200, 170, 0, alpha};
            a_DrawRect((aRectf_t){coin_x, coin_y, coin_size, coin_size}, border_color);
        }
    }

    // ========================================================================
    // CHIP FLASH TEXT (sell animation - renders during COIN_BURST + FLASH_CHIPS + COMPLETE)
    // ========================================================================
    if (modal->anim_stage == TRINKET_ANIM_COIN_BURST ||
        modal->anim_stage == TRINKET_ANIM_FLASH_CHIPS ||
        modal->anim_stage == TRINKET_ANIM_COMPLETE) {
        // Show chips gained with gold flash (fades in during COIN_BURST)
        dString_t* chip_text = d_StringInit();
        d_StringFormat(chip_text, "+%d CHIPS", modal->chips_gained);

        // Flash alpha animation (0.0 → 1.0), with fade-out during COMPLETE
        float text_alpha = modal->chip_flash_alpha;
        if (modal->anim_stage == TRINKET_ANIM_COMPLETE) {
            text_alpha *= modal->fade_alpha;
        }
        uint8_t flash_alpha = (uint8_t)(text_alpha * 255.0f);

        // Draw centered text with flash
        aTextStyle_t flash_config = {
            .type = FONT_ENTER_COMMAND,
            .fg = {255, 215, 0, flash_alpha},  // Gold with fade-in
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.5f  // Bigger for visibility
        };
        a_DrawText((char*)d_StringPeek(chip_text), SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 - 40, flash_config);

        // "SOLD!" label below
        aTextStyle_t sold_config = {
            .type = FONT_GAME,
            .fg = {(uint8_t)COLOR_SELL_ACCENT.r, (uint8_t)COLOR_SELL_ACCENT.g, (uint8_t)COLOR_SELL_ACCENT.b, flash_alpha},
            .align = TEXT_ALIGN_CENTER,
            .wrap_width = 0,
            .scale = 1.2f  // Bigger for visibility
        };
        a_DrawText("SOLD!", SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2 + 30, sold_config);

        d_StringDestroy(chip_text);
    }
}

// ============================================================================
// QUERIES
// ============================================================================

bool WasTrinketEquipped(const TrinketDropModal_t* modal) {
    return modal && modal->was_equipped;
}

int GetEquippedSlot(const TrinketDropModal_t* modal) {
    return modal ? modal->equipped_slot : -1;
}

int GetChipsGained(const TrinketDropModal_t* modal) {
    return modal ? modal->chips_gained : 0;
}
