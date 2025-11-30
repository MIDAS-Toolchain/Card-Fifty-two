/*
 * Card Fifty-Two - Settings System
 *
 * Global game settings with DUF persistence and immediate-apply controls
 */

#include "../include/common.h"
#include "../include/settings.h"
#include <string.h>

// ============================================================================
// RESOLUTION TABLE
// ============================================================================

const Resolution_t AVAILABLE_RESOLUTIONS[] = {
    {1280, 720,  "1280x720"},
    {1366, 768,  "1366x768"},
    {1600, 900,  "1600x900"}
};

const int RESOLUTION_COUNT = sizeof(AVAILABLE_RESOLUTIONS) / sizeof(Resolution_t);

// ============================================================================
// DISPLAY DETECTION (Prevent window teleportation!)
// ============================================================================

/**
 * GetMaxSafeResolution - Get maximum safe resolution for primary display
 *
 * Returns 95% of primary display bounds to account for taskbars/decorations.
 * This prevents SDL from teleporting the window to a secondary monitor.
 */
static void GetMaxSafeResolution(int* out_width, int* out_height) {
    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(0, &bounds) == 0) {  // 0 = primary display
        // Use full display bounds (SDL handles window decorations automatically)
        *out_width = bounds.w;
        *out_height = bounds.h;
        d_LogInfoF("Primary display resolution: %dx%d", *out_width, *out_height);
    } else {
        // Fallback if SDL fails
        *out_width = 1920;
        *out_height = 1080;
        d_LogWarning("Failed to query primary display, using fallback 1920x1080");
    }
}

/**
 * IsResolutionValidForDisplay - Check if resolution fits on primary display
 *
 * @param width: Resolution width
 * @param height: Resolution height
 * @return: true if resolution fits safely on primary display
 */
bool IsResolutionValidForDisplay(int width, int height) {
    int max_w, max_h;
    GetMaxSafeResolution(&max_w, &max_h);
    return (width <= max_w && height <= max_h);
}

// ============================================================================
// DEFAULTS
// ============================================================================

static void ApplyDefaultSettings(Settings_t* settings) {
    if (!settings) return;

    // Audio tab
    settings->sound_volume = 50;
    settings->music_volume = 50;
    settings->ui_volume = 75;
    settings->sound_enabled = true;
    settings->music_enabled = true;

    // Gameplay tab
    settings->show_damage_numbers = true;
    settings->auto_advance_dialogue = false;
    settings->tutorial_hints = 2;  // Detailed

    // UI tab
    settings->show_fps = false;
    settings->screen_shake = true;
    settings->ui_scale = 0;  // 100%

    // Graphics tab
    settings->resolution_index = 1;  // 1366x768 - index 1
}

// ============================================================================
// LIFECYCLE
// ============================================================================

Settings_t* Settings_Init(void) {
    Settings_t* settings = malloc(sizeof(Settings_t));
    if (!settings) {
        d_LogFatal("Failed to allocate Settings_t");
        return NULL;
    }

    ApplyDefaultSettings(settings);
    d_LogInfo("Settings initialized with defaults");
    return settings;
}

void Settings_Destroy(Settings_t** settings) {
    if (!settings || !*settings) return;

    free(*settings);
    *settings = NULL;
    d_LogInfo("Settings destroyed");
}

// ============================================================================
// PERSISTENCE (DUF FORMAT)
// ============================================================================

void Settings_Load(Settings_t* settings) {
    if (!settings) return;

    // Try to load settings.duf from user config directory
    // TODO: Implement DUF parsing once we have path resolver
    // For now, check if file exists - if not, use defaults

    const char* settings_path = "settings.duf";  // TODO: Use proper config path
    SDL_RWops* file = SDL_RWFromFile(settings_path, "rb");

    if (!file) {
        d_LogInfo("No settings.duf found, using defaults");
        ApplyDefaultSettings(settings);
        return;
    }

    // Read file size
    Sint64 file_size = SDL_RWsize(file);
    if (file_size <= 0) {
        d_LogError("settings.duf is empty, using defaults");
        SDL_RWclose(file);
        ApplyDefaultSettings(settings);
        return;
    }

    // Read entire file into dString_t buffer (ADR-003: No raw malloc!)
    dString_t* buffer = d_StringInit();
    if (!buffer) {
        d_LogError("Failed to allocate dString_t for settings.duf");
        SDL_RWclose(file);
        ApplyDefaultSettings(settings);
        return;
    }

    // Read file into temporary C buffer, then copy to dString_t
    char* temp_buf = SDL_malloc(file_size + 1);
    if (!temp_buf) {
        d_LogError("Failed to allocate temp buffer for settings.duf");
        d_StringDestroy(buffer);
        SDL_RWclose(file);
        ApplyDefaultSettings(settings);
        return;
    }

    SDL_RWread(file, temp_buf, 1, file_size);
    temp_buf[file_size] = '\0';
    SDL_RWclose(file);

    d_StringSet(buffer, temp_buf, 0);  // 0 = replace entire string
    SDL_free(temp_buf);

    // Parse DUF format (simple key=value pairs)
    // Format:
    // sound_volume=50
    // music_volume=50
    // sound_enabled=true
    // ...

    // Get mutable copy for strtok (d_StringPeek is const)
    const char* buffer_content = d_StringPeek(buffer);
    size_t buffer_len = strlen(buffer_content);
    char* mutable_buffer = SDL_malloc(buffer_len + 1);
    strcpy(mutable_buffer, buffer_content);
    if (!mutable_buffer) {
        d_LogError("Failed to duplicate buffer for parsing");
        d_StringDestroy(buffer);
        ApplyDefaultSettings(settings);
        return;
    }

    char* line = strtok(mutable_buffer, "\n");
    while (line) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\0') {
            line = strtok(NULL, "\n");
            continue;
        }

        // Parse key=value
        char* equals = strchr(line, '=');
        if (!equals) {
            line = strtok(NULL, "\n");
            continue;
        }

        *equals = '\0';  // Split into key and value
        const char* key = line;
        const char* value = equals + 1;

        // Audio tab
        if (strcmp(key, "sound_volume") == 0) {
            settings->sound_volume = atoi(value);
        } else if (strcmp(key, "music_volume") == 0) {
            settings->music_volume = atoi(value);
        } else if (strcmp(key, "ui_volume") == 0) {
            settings->ui_volume = atoi(value);
        } else if (strcmp(key, "sound_enabled") == 0) {
            settings->sound_enabled = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "music_enabled") == 0) {
            settings->music_enabled = (strcmp(value, "true") == 0);
        }
        // Gameplay tab
        else if (strcmp(key, "show_damage_numbers") == 0) {
            settings->show_damage_numbers = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "auto_advance_dialogue") == 0) {
            settings->auto_advance_dialogue = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "tutorial_hints") == 0) {
            settings->tutorial_hints = atoi(value);
        }
        // UI tab
        else if (strcmp(key, "show_fps") == 0) {
            settings->show_fps = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "screen_shake") == 0) {
            settings->screen_shake = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "ui_scale") == 0) {
            settings->ui_scale = atoi(value);
        }
        // Graphics tab
        else if (strcmp(key, "resolution_index") == 0) {
            settings->resolution_index = atoi(value);
        }

        line = strtok(NULL, "\n");
    }

    SDL_free(mutable_buffer);
    d_StringDestroy(buffer);
    d_LogInfo("Settings loaded from settings.duf");
}

void Settings_Save(const Settings_t* settings) {
    if (!settings) return;

    const char* settings_path = "settings.duf";  // TODO: Use proper config path
    SDL_RWops* file = SDL_RWFromFile(settings_path, "wb");

    if (!file) {
        d_LogError("Failed to open settings.duf for writing");
        return;
    }

    // Write DUF format (key=value pairs) using d_StringFormat only
    dString_t* buffer = d_StringInit();

    d_StringFormat(buffer, "# Card Fifty-Two Settings\n");
    d_StringFormat(buffer, "# Auto-generated - do not edit while game is running\n\n");

    // Audio tab
    d_StringFormat(buffer, "# Audio Settings\n");
    d_StringFormat(buffer, "sound_volume=%d\n", settings->sound_volume);
    d_StringFormat(buffer, "music_volume=%d\n", settings->music_volume);
    d_StringFormat(buffer, "ui_volume=%d\n", settings->ui_volume);
    d_StringFormat(buffer, "sound_enabled=%s\n", settings->sound_enabled ? "true" : "false");
    d_StringFormat(buffer, "music_enabled=%s\n\n", settings->music_enabled ? "true" : "false");

    // Gameplay tab
    d_StringFormat(buffer, "# Gameplay Settings\n");
    d_StringFormat(buffer, "show_damage_numbers=%s\n", settings->show_damage_numbers ? "true" : "false");
    d_StringFormat(buffer, "auto_advance_dialogue=%s\n", settings->auto_advance_dialogue ? "true" : "false");
    d_StringFormat(buffer, "tutorial_hints=%d\n\n", settings->tutorial_hints);

    // UI tab
    d_StringFormat(buffer, "# UI Settings\n");
    d_StringFormat(buffer, "show_fps=%s\n", settings->show_fps ? "true" : "false");
    d_StringFormat(buffer, "screen_shake=%s\n", settings->screen_shake ? "true" : "false");
    d_StringFormat(buffer, "ui_scale=%d\n\n", settings->ui_scale);

    // Graphics tab
    d_StringFormat(buffer, "# Graphics Settings\n");
    d_StringFormat(buffer, "resolution_index=%d\n", settings->resolution_index);

    // Write to file
    const char* content = d_StringPeek(buffer);
    SDL_RWwrite(file, content, 1, strlen(content));

    SDL_RWclose(file);
    d_StringDestroy(buffer);

    d_LogInfo("Settings saved to settings.duf");
}

// ============================================================================
// APPLY SETTINGS
// ============================================================================

void Settings_Apply(const Settings_t* settings) {
    if (!settings) return;

    // Apply audio settings using Archimedes API
    if (settings->sound_enabled) {
        int sdl_volume = (int)(128 * (settings->sound_volume / 100.0f));  // 0-128 (MIX_MAX_VOLUME)
        a_AudioSetChannelVolume(-1, sdl_volume);  // -1 = all channels
    } else {
        a_AudioSetChannelVolume(-1, 0);
    }

    // Apply UI volume separately (channels 0 and 1)
    if (settings->sound_enabled) {
        int ui_sdl_volume = (int)(128 * (settings->ui_volume / 100.0f));
        a_AudioSetChannelVolume(AUDIO_CHANNEL_UI_HOVER, ui_sdl_volume);
        a_AudioSetChannelVolume(AUDIO_CHANNEL_UI_CLICK, ui_sdl_volume);
    } else {
        a_AudioSetChannelVolume(AUDIO_CHANNEL_UI_HOVER, 0);
        a_AudioSetChannelVolume(AUDIO_CHANNEL_UI_CLICK, 0);
    }

    if (settings->music_enabled) {
        int sdl_volume = (int)(128 * (settings->music_volume / 100.0f));
        a_AudioSetMusicVolume(sdl_volume);
    } else {
        a_AudioSetMusicVolume(0);
    }

    // Apply graphics settings (just resolution now)
    const Resolution_t* res = Settings_GetCurrentResolution(settings);
    if (res) {
        SDL_SetWindowSize(app.window, res->width, res->height);
        // Center on primary display (0) to prevent teleporting to secondary monitor
        SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0));
    }

    d_LogInfo("Settings applied to game engine");
}

// ============================================================================
// SETTERS (with immediate apply)
// ============================================================================

void Settings_SetSoundVolume(Settings_t* settings, int volume) {
    if (!settings) return;

    // Clamp to 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    settings->sound_volume = volume;

    // Apply immediately using Archimedes API
    if (settings->sound_enabled) {
        int sdl_volume = (int)(128 * (volume / 100.0f));  // 0-128 (MIX_MAX_VOLUME)
        a_AudioSetChannelVolume(-1, sdl_volume);
    }
}

void Settings_SetMusicVolume(Settings_t* settings, int volume) {
    if (!settings) return;

    // Clamp to 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    settings->music_volume = volume;

    // Apply immediately using Archimedes API
    if (settings->music_enabled) {
        int sdl_volume = (int)(128 * (volume / 100.0f));  // 0-128 (MIX_MAX_VOLUME)
        a_AudioSetMusicVolume(sdl_volume);
    }
}

void Settings_SetUIVolume(Settings_t* settings, int volume) {
    if (!settings) return;

    // Clamp to 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    settings->ui_volume = volume;

    // Apply immediately to UI channels (0 and 1)
    if (settings->sound_enabled) {
        int sdl_volume = (int)(128 * (volume / 100.0f));  // 0-128 (MIX_MAX_VOLUME)
        a_AudioSetChannelVolume(AUDIO_CHANNEL_UI_HOVER, sdl_volume);
        a_AudioSetChannelVolume(AUDIO_CHANNEL_UI_CLICK, sdl_volume);
    }
}

void Settings_SetResolution(Settings_t* settings, int index) {
    if (!settings) return;

    // Clamp to valid range
    if (index < 0) index = 0;
    if (index >= RESOLUTION_COUNT) index = RESOLUTION_COUNT - 1;

    const Resolution_t* res = &AVAILABLE_RESOLUTIONS[index];

    // Check if this resolution is too big for the primary display
    if (!IsResolutionValidForDisplay(res->width, res->height)) {
        d_LogWarningF("Resolution %s is too large for primary display, finding largest valid option...",
                      res->label);

        // Find largest resolution that DOES fit
        bool found_valid = false;
        for (int i = RESOLUTION_COUNT - 1; i >= 0; i--) {
            const Resolution_t* candidate = &AVAILABLE_RESOLUTIONS[i];
            if (IsResolutionValidForDisplay(candidate->width, candidate->height)) {
                index = i;
                res = candidate;
                found_valid = true;
                d_LogInfoF("Clamped to largest valid resolution: %s", res->label);
                break;
            }
        }

        // If somehow NO resolutions fit (shouldn't happen), use smallest
        if (!found_valid) {
            d_LogWarning("No valid resolutions found! Using smallest (1280x720)");
            index = 0;
            res = &AVAILABLE_RESOLUTIONS[0];
        }
    }

    settings->resolution_index = index;

    // Apply immediately
    SDL_SetWindowSize(app.window, res->width, res->height);
    // Center on primary display (0) to prevent teleporting to secondary monitor
    SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED_DISPLAY(0), SDL_WINDOWPOS_CENTERED_DISPLAY(0));

    d_LogInfoF("Resolution changed to %s", res->label);
}


// ============================================================================
// GETTERS
// ============================================================================

const Resolution_t* Settings_GetCurrentResolution(const Settings_t* settings) {
    if (!settings) return NULL;

    int index = settings->resolution_index;
    if (index < 0 || index >= RESOLUTION_COUNT) {
        return &AVAILABLE_RESOLUTIONS[0];  // Fallback to default
    }

    return &AVAILABLE_RESOLUTIONS[index];
}
