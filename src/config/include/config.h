#pragma once

#include <SDL.h>
#include <stdbool.h>

#define NES_BUTTON_COUNT 8

typedef struct {
    float volume;
    SDL_Scancode keybinds[NES_BUTTON_COUNT];
    SDL_GameControllerButton padbinds[NES_BUTTON_COUNT];
} EmulatorConfig;

// Fill cfg with hardcoded defaults
void config_defaults(EmulatorConfig *cfg);

// Load config from disk into cfg. Returns true on success.
// Falls back to defaults for any missing/invalid keys.
bool config_load(EmulatorConfig *cfg);

// Write cfg to disk. Returns true on success.
bool config_save(const EmulatorConfig *cfg);
