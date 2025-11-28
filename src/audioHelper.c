/*
 * Audio Helper - UI Sound Debouncing
 *
 * Implements timestamp-based debouncing for UI sounds.
 * Prevents rapid sound triggering when hovering/clicking quickly.
 *
 * NOTE: Archimedes uses SDL_QueueAudio (raw audio), not SDL_mixer channels.
 * We cannot use Mix_HaltChannel. Instead, we use a cooldown timer approach.
 */

#include "../include/audioHelper.h"
#include <SDL2/SDL.h>

// Cooldown timers (milliseconds since last play)
static Uint32 g_last_hover_time = 0;
static Uint32 g_last_click_time = 0;

// Cooldown durations (tunable)
#define HOVER_COOLDOWN_MS  50   // 50ms between hover sounds (20 per second max)
#define CLICK_COOLDOWN_MS  100  // 100ms between click sounds (10 per second max)

// ============================================================================
// INITIALIZATION
// ============================================================================

void InitUIAudioChannels(void) {
    // Initialize timers
    g_last_hover_time = 0;
    g_last_click_time = 0;

    d_LogInfo("UI audio debouncing initialized (Hover: 50ms, Click: 100ms cooldown)");
}

// ============================================================================
// UI SOUND PLAYBACK (Timestamp-Based Debouncing)
// ============================================================================

void PlayUIHoverSound(void) {
    Uint32 now = SDL_GetTicks();

    // Check if enough time has passed since last hover sound
    if (now - g_last_hover_time < HOVER_COOLDOWN_MS) {
        return;  // Too soon, skip this sound
    }

    // Play sound and update timestamp
    a_AudioPlayEffect(&g_ui_hover_sound);
    g_last_hover_time = now;
}

void PlayUIClickSound(void) {
    Uint32 now = SDL_GetTicks();

    // Check if enough time has passed since last click sound
    if (now - g_last_click_time < CLICK_COOLDOWN_MS) {
        return;  // Too soon, skip this sound
    }

    // Play sound and update timestamp
    a_AudioPlayEffect(&g_ui_click_sound);
    g_last_click_time = now;
}
