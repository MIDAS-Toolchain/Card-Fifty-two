#ifndef AUDIO_HELPER_H
#define AUDIO_HELPER_H

#include "common.h"

// ============================================================================
// UI AUDIO CHANNEL MANAGEMENT
// ============================================================================

/**
 * UI Audio Channel Architecture (SDL_mixer v2)
 *
 * Uses dedicated channels with interrupt flag to prevent sound queuing.
 * Archimedes v2 uses SDL_mixer for proper channel-based mixing.
 *
 * Channel Assignments:
 * - AUDIO_CHANNEL_UI_HOVER (0) - Reserved for UI hover sounds
 * - AUDIO_CHANNEL_UI_CLICK (1) - Reserved for UI click sounds
 *
 * When a UI sound plays with .interrupt = 1, it immediately halts any
 * previous sound on that channel, preventing queuing when hovering/clicking
 * rapidly.
 *
 * Benefits:
 * - No sound queuing (instant stop-and-replace)
 * - No timestamp debouncing needed
 * - Clean channel isolation (hover won't interrupt click)
 * - Industry-standard SDL_mixer API
 */

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * InitUIAudioChannels - Initialize UI audio system
 *
 * Call this AFTER a_InitAudio() and a_AudioReserveChannels(2) in main.c.
 *
 * Note: This function is now a no-op (reserved channels handle everything).
 * Kept for API compatibility.
 */
void InitUIAudioChannels(void);

// ============================================================================
// UI SOUND PLAYBACK (Stop-and-Replace Pattern)
// ============================================================================

/**
 * PlayUIHoverSound - Play hover sound effect
 *
 * Plays hover sound on AUDIO_CHANNEL_UI_HOVER with interrupt flag.
 * Automatically stops any previous hover sound (no queuing).
 *
 * Use for:
 * - Mouse entering buttons/menu items
 * - Keyboard navigation (UP/DOWN arrows)
 */
void PlayUIHoverSound(void);

/**
 * PlayUIClickSound - Play click sound effect
 *
 * Plays click sound on AUDIO_CHANNEL_UI_CLICK with interrupt flag.
 * Automatically stops any previous click sound (no queuing).
 *
 * Use for:
 * - Button clicks
 * - Menu activation (ENTER/SPACE)
 * - Toggle switches
 */
void PlayUIClickSound(void);

// ============================================================================
// GAME SOUND EFFECTS
// ============================================================================

/**
 * PlayCardSlideSound - Play random card slide sound (with no-repeat)
 *
 * Plays a random card slide sound (1-8 variants) with no-repeat logic.
 * Uses auto-allocated channels (2-15) so multiple cards can slide simultaneously.
 * Controlled by Effect Volume setting (sound_volume).
 *
 * Use for:
 * - Every card draw (player, dealer, initial deal)
 * - Triggered by GAME_EVENT_CARD_DRAWN
 */
void PlayCardSlideSound(void);

#endif // AUDIO_HELPER_H
