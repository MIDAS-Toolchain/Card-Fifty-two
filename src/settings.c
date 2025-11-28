/*
 * Card Fifty-Two - Settings System
 *
 * Global game settings with DUF persistence and immediate-apply controls
 */

#include "../include/common.h"
#include "../include/settings.h"
#include <SDL2/SDL_mixer.h>
#include <string.h>

// ============================================================================
// RESOLUTION TABLE
// ============================================================================

const Resolution_t AVAILABLE_RESOLUTIONS[] = {
    {1280, 720,  "1280x720 (HD)"},
    {1920, 1080, "1920x1080 (Full HD)"},
    {2560, 1440, "2560x1440 (QHD)"},
    {3840, 2160, "3840x2160 (4K)"}
};

const int RESOLUTION_COUNT = sizeof(AVAILABLE_RESOLUTIONS) / sizeof(Resolution_t);

// ============================================================================
// DEFAULTS
// ============================================================================

static void ApplyDefaultSettings(Settings_t* settings) {
    if (!settings) return;

    // Audio tab
    settings->sound_volume = 50;
    settings->music_volume = 50;
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
    settings->fullscreen = false;
    settings->vsync = true;
    settings->resolution_index = 0;  // 1280x720
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
        else if (strcmp(key, "fullscreen") == 0) {
            settings->fullscreen = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "vsync") == 0) {
            settings->vsync = (strcmp(value, "true") == 0);
        } else if (strcmp(key, "resolution_index") == 0) {
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
    d_StringFormat(buffer, "fullscreen=%s\n", settings->fullscreen ? "true" : "false");
    d_StringFormat(buffer, "vsync=%s\n", settings->vsync ? "true" : "false");
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

    // Apply audio settings
    if (settings->sound_enabled) {
        int sdl_volume = (int)(MIX_MAX_VOLUME * (settings->sound_volume / 100.0f));
        Mix_Volume(-1, sdl_volume);  // -1 = all channels
    } else {
        Mix_Volume(-1, 0);
    }

    if (settings->music_enabled) {
        int sdl_volume = (int)(MIX_MAX_VOLUME * (settings->music_volume / 100.0f));
        Mix_VolumeMusic(sdl_volume);
    } else {
        Mix_VolumeMusic(0);
    }

    // Apply graphics settings
    const Resolution_t* res = Settings_GetCurrentResolution(settings);
    if (res) {
        SDL_SetWindowSize(app.window, res->width, res->height);
        SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    }

    // Apply fullscreen
    Uint32 fullscreen_flag = settings->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(app.window, fullscreen_flag);

    // Apply V-Sync
    SDL_RenderSetVSync(app.renderer, settings->vsync ? 1 : 0);

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

    // Apply immediately
    if (settings->sound_enabled) {
        int sdl_volume = (int)(MIX_MAX_VOLUME * (volume / 100.0f));
        Mix_Volume(-1, sdl_volume);
    }
}

void Settings_SetMusicVolume(Settings_t* settings, int volume) {
    if (!settings) return;

    // Clamp to 0-100
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    settings->music_volume = volume;

    // Apply immediately
    if (settings->music_enabled) {
        int sdl_volume = (int)(MIX_MAX_VOLUME * (volume / 100.0f));
        Mix_VolumeMusic(sdl_volume);
    }
}

void Settings_SetResolution(Settings_t* settings, int index) {
    if (!settings) return;

    // Clamp to valid range
    if (index < 0) index = 0;
    if (index >= RESOLUTION_COUNT) index = RESOLUTION_COUNT - 1;

    settings->resolution_index = index;

    // Apply immediately
    const Resolution_t* res = &AVAILABLE_RESOLUTIONS[index];
    SDL_SetWindowSize(app.window, res->width, res->height);
    SDL_SetWindowPosition(app.window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    d_LogInfoF("Resolution changed to %s", res->label);
}

void Settings_SetFullscreen(Settings_t* settings, bool enabled) {
    if (!settings) return;

    settings->fullscreen = enabled;

    // Apply immediately
    Uint32 fullscreen_flag = enabled ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;
    SDL_SetWindowFullscreen(app.window, fullscreen_flag);

    d_LogInfoF("Fullscreen: %s", enabled ? "ON" : "OFF");
}

void Settings_SetVSync(Settings_t* settings, bool enabled) {
    if (!settings) return;

    settings->vsync = enabled;

    // Apply immediately
    SDL_RenderSetVSync(app.renderer, enabled ? 1 : 0);

    d_LogInfoF("V-Sync: %s", enabled ? "ON" : "OFF");
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
