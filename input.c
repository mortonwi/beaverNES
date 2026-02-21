#include "input.h"

#include <SDL2/SDL.h>

static int key_down(const uint8_t* keys, SDL_Scancode sc) {
    return keys[sc] != 0;
}

void input_init(InputState* in) {
    in->quit = 0;
}

void input_update(InputState* in, Controller* c) {
    // Process window/events (close button, etc.)
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            in->quit = 1;
        }
    }

    // Read keyboard state
    const uint8_t* keys = SDL_GetKeyboardState(NULL);

    // Optional: allow Esc to quit
    if (key_down(keys, SDL_SCANCODE_ESCAPE)) {
        in->quit = 1;
    }

    // Build a full 8-bit controller state (bit0=A ... bit7=Right)
    uint8_t buttons = 0;

    if (key_down(keys, SDL_SCANCODE_Z))          buttons |= (1 << BTN_A);
    if (key_down(keys, SDL_SCANCODE_X))          buttons |= (1 << BTN_B);
    if (key_down(keys, SDL_SCANCODE_RSHIFT))     buttons |= (1 << BTN_SELECT);
    if (key_down(keys, SDL_SCANCODE_RETURN))     buttons |= (1 << BTN_START);

    if (key_down(keys, SDL_SCANCODE_UP))         buttons |= (1 << BTN_UP);
    if (key_down(keys, SDL_SCANCODE_DOWN))       buttons |= (1 << BTN_DOWN);
    if (key_down(keys, SDL_SCANCODE_LEFT))       buttons |= (1 << BTN_LEFT);
    if (key_down(keys, SDL_SCANCODE_RIGHT))      buttons |= (1 << BTN_RIGHT);

    // Update controller in one shot
    controller_set_state(c, buttons);
}

void input_shutdown(InputState* in) {
    (void)in;
}