#include "config.h"
#include "controller.h" // for BTN_A, BTN_B, etc.

#include <SDL.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_FILENAME "beaverNES.cfg"
#define APP_ORG         "beaverNES"
#define APP_NAME        "beaverNES"

// Key names must match exactly what config_save writes
static const char *key_names[NES_BUTTON_COUNT] = {
    "key_a", "key_b", "key_select", "key_start",
    "key_up", "key_down", "key_left", "key_right"
};

static const char *pad_names[NES_BUTTON_COUNT] = {
    "pad_a", "pad_b", "pad_select", "pad_start",
    "pad_up", "pad_down", "pad_left", "pad_right"
};

void config_defaults(EmulatorConfig *cfg)
{
    cfg->volume = 0.5f;

    cfg->keybinds[BTN_A]      = SDL_SCANCODE_X;
    cfg->keybinds[BTN_B]      = SDL_SCANCODE_Z;
    cfg->keybinds[BTN_SELECT] = SDL_SCANCODE_RSHIFT;
    cfg->keybinds[BTN_START]  = SDL_SCANCODE_RETURN;
    cfg->keybinds[BTN_UP]     = SDL_SCANCODE_UP;
    cfg->keybinds[BTN_DOWN]   = SDL_SCANCODE_DOWN;
    cfg->keybinds[BTN_LEFT]   = SDL_SCANCODE_LEFT;
    cfg->keybinds[BTN_RIGHT]  = SDL_SCANCODE_RIGHT;

    cfg->padbinds[BTN_A]      = SDL_CONTROLLER_BUTTON_A;
    cfg->padbinds[BTN_B]      = SDL_CONTROLLER_BUTTON_B;
    cfg->padbinds[BTN_SELECT] = SDL_CONTROLLER_BUTTON_BACK;
    cfg->padbinds[BTN_START]  = SDL_CONTROLLER_BUTTON_START;
    cfg->padbinds[BTN_UP]     = SDL_CONTROLLER_BUTTON_DPAD_UP;
    cfg->padbinds[BTN_DOWN]   = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    cfg->padbinds[BTN_LEFT]   = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    cfg->padbinds[BTN_RIGHT]  = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
}

// Returns a malloc'd path to the config file, or NULL on failure.
// Caller is responsible for SDL_free'ing it.
static char *get_config_path(void)
{
    char *pref_path = SDL_GetPrefPath(APP_ORG, APP_NAME);
    if (!pref_path) {
        printf("config: SDL_GetPrefPath failed: %s\n", SDL_GetError());
        return NULL;
    }

    size_t len = strlen(pref_path) + strlen(CONFIG_FILENAME) + 1;
    char *full_path = SDL_malloc(len);
    if (!full_path) {
        SDL_free(pref_path);
        return NULL;
    }

    snprintf(full_path, len, "%s%s", pref_path, CONFIG_FILENAME);
    SDL_free(pref_path);
    return full_path;
}

bool config_load(EmulatorConfig *cfg)
{
    // Always start from defaults so missing keys are handled gracefully
    config_defaults(cfg);

    char *path = get_config_path();
    if (!path) return false;

    FILE *f = fopen(path, "r");
    SDL_free(path);

    if (!f) {
        // First launch — no config file yet, defaults are fine
        return false;
    }

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Strip trailing newline
        line[strcspn(line, "\r\n")] = '\0';

        // Skip blank lines and comments
        if (line[0] == '\0' || line[0] == '#') continue;

        char key[64];
        int  value;
        float fvalue;

        // Try float first (for volume), then int for everything else
        if (sscanf(line, "%63[^=]=%f", key, &fvalue) == 2) {
            if (strcmp(key, "volume") == 0) {
                // Clamp to [0.0, 1.0]
                if (fvalue < 0.0f) fvalue = 0.0f;
                if (fvalue > 1.0f) fvalue = 1.0f;
                cfg->volume = fvalue;
                continue;
            }
        }

        if (sscanf(line, "%63[^=]=%d", key, &value) != 2) continue;

        for (int i = 0; i < NES_BUTTON_COUNT; i++) {
            if (strcmp(key, key_names[i]) == 0) {
                cfg->keybinds[i] = (SDL_Scancode)value;
                break;
            }
            if (strcmp(key, pad_names[i]) == 0) {
                cfg->padbinds[i] = (SDL_GameControllerButton)value;
                break;
            }
        }
    }

    fclose(f);
    return true;
}

bool config_save(const EmulatorConfig *cfg)
{
    char *path = get_config_path();
    if (!path) return false;

    FILE *f = fopen(path, "w");
    SDL_free(path);

    if (!f) {
        printf("config: failed to open config file for writing\n");
        return false;
    }

    fprintf(f, "# beaverNES configuration\n\n");
    fprintf(f, "volume=%.4f\n\n", cfg->volume);

    fprintf(f, "# Keyboard bindings (SDL scancode integers)\n");
    for (int i = 0; i < NES_BUTTON_COUNT; i++) {
        fprintf(f, "%s=%d\n", key_names[i], (int)cfg->keybinds[i]);
    }

    fprintf(f, "\n# Controller bindings (SDL_GameControllerButton integers)\n");
    for (int i = 0; i < NES_BUTTON_COUNT; i++) {
        fprintf(f, "%s=%d\n", pad_names[i], (int)cfg->padbinds[i]);
    }

    fclose(f);
    return true;
}
