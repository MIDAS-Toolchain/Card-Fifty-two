/*
 * Audio Helper - UI Sound Channel Management
 *
 * Uses dedicated SDL_mixer channels with interrupt flag to prevent queuing.
 * Archimedes v2 provides proper channel-based audio via SDL_mixer.
 *
 * Channel Assignments:
 * - AUDIO_CHANNEL_UI_HOVER (0) - UI hover sounds
 * - AUDIO_CHANNEL_UI_CLICK (1) - UI click sounds
 *
 * When .interrupt = 1, the channel is halted before playing new sound.
 * This eliminates sound queuing without timestamp debouncing.
 */

#include "../include/audioHelper.h"

// ============================================================================
// INITIALIZATION
// ============================================================================

void InitUIAudioChannels(void) {
    // No-op: Channel reservation happens in main.c via a_AudioReserveChannels(2)
    // Kept for API compatibility
    d_LogInfo("UI audio channels initialized (using SDL_mixer channels 0-1)");
}

// ============================================================================
// UI SOUND PLAYBACK (Channel-Based Stop-and-Replace)
// ============================================================================

void PlayUIHoverSound(void) {
    // Play on AUDIO_CHANNEL_UI_HOVER with interrupt flag
    // This halts any previous hover sound, preventing queuing
    aAudioOptions_t opts = {
        .channel = AUDIO_CHANNEL_UI_HOVER,  // Channel 0
        .volume = -1,                       // Use sound's default volume
        .loops = 0,                         // Play once
        .fade_ms = 0,                       // No fade
        .interrupt = 1                      // KEY: Stop previous sound
    };

    a_AudioPlaySound(&g_ui_hover_sound, &opts);
}

void PlayUIClickSound(void) {
    // Play on AUDIO_CHANNEL_UI_CLICK with interrupt flag
    // This halts any previous click sound, preventing queuing
    aAudioOptions_t opts = {
        .channel = AUDIO_CHANNEL_UI_CLICK,  // Channel 1
        .volume = -1,                       // Use sound's default volume
        .loops = 0,                         // Play once
        .fade_ms = 0,                       // No fade
        .interrupt = 1                      // KEY: Stop previous sound
    };

    a_AudioPlaySound(&g_ui_click_sound, &opts);
}

// ============================================================================
// GAME SOUND EFFECTS
// ============================================================================

void PlayCardSlideSound(void) {
    // No-repeat random selection: avoid playing same sound twice in a row
    int selected_index;
    do {
        selected_index = rand() % CARD_SLIDE_SOUND_COUNT;
    } while (selected_index == g_last_card_slide_index && CARD_SLIDE_SOUND_COUNT > 1);

    g_last_card_slide_index = selected_index;

    // Play on auto-allocated channel (channels 2-15)
    // This allows multiple card sounds to overlap (simultaneous draws)
    aAudioOptions_t opts = {
        .channel = -1,       // Auto-allocate channel
        .volume = -1,        // Use sound's default volume (controlled by Effect Volume setting)
        .loops = 0,          // Play once
        .fade_ms = 0,        // No fade
        .interrupt = 0       // Allow overlapping sounds
    };

    a_AudioPlaySound(&g_card_slide_sounds[selected_index], &opts);
}
