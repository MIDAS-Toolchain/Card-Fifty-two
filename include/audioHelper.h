#ifndef AUDIO_HELPER_H
#define AUDIO_HELPER_H

#include "common.h"

// ============================================================================
// UI AUDIO CHANNEL MANAGEMENT
// ============================================================================

/**
 * UI Audio Debouncing Architecture
 *
 * Uses timestamp-based cooldowns to prevent rapid sound triggering.
 * When a UI sound is requested, it only plays if enough time has passed
 * since the last instance of that sound.
 *
 * NOTE: Archimedes uses SDL_QueueAudio (raw audio buffers), not SDL_mixer.
 * We cannot halt/interrupt sounds, so we use cooldown timers instead.
 *
 * This ensures:
 * - No rapid-fire sounds when hovering/clicking quickly
 * - Smooth audio experience (max 20 hovers/sec, 10 clicks/sec)
 * - Clean mixing (sounds don't overlap excessively)
 *
 * Cooldowns (tunable in audioHelper.c):
 * - Hover: 50ms cooldown (20 per second max)
 * - Click: 100ms cooldown (10 per second max)
 */

// ============================================================================
// INITIALIZATION
// ============================================================================

/**
 * InitUIAudioChannels - Initialize UI audio debouncing timers
 *
 * Call this AFTER a_InitAudio() in main.c Init().
 *
 * Resets cooldown timers to allow immediate sound playback on startup.
 */
void InitUIAudioChannels(void);

// ============================================================================
// UI SOUND PLAYBACK (Stop-and-Replace Pattern)
// ============================================================================

/**
 * PlayUIHoverSound - Play hover sound effect
 *
 * Stops any currently playing hover sound and immediately starts a new one.
 * Use for:
 * - Mouse entering buttons/menu items
 * - Keyboard navigation (UP/DOWN arrows)
 */
void PlayUIHoverSound(void);

/**
 * PlayUIClickSound - Play click sound effect
 *
 * Stops any currently playing click sound and immediately starts a new one.
 * Use for:
 * - Button clicks
 * - Menu activation (ENTER/SPACE)
 * - Toggle switches
 */
void PlayUIClickSound(void);

#endif // AUDIO_HELPER_H
