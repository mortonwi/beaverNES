#include "input.h"

#include <SDL2/SDL.h>
#include <stdint.h>

static SDL_Window* g_window = NULL;

int input_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }

    // Tiny window so SDL can receive keyboard events.
    // (You can replace this with your real window later.)
    g_window = SDL_CreateWindow(
        "beaverNES input test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        320, 240,
        0
    );

    if (!g_window) {
        SDL_Quit();
        return 1;
    }

    return 0;
}

static uint8_t build_buttons_from_keyboard(void) {
    const Uint8* k = SDL_GetKeyboardState(NULL);
    uint8_t b = 0;

    // Common NES-ish mapping:
    // X = A, Z = B, RSHIFT = Select, Enter = Start, arrows = d-pad
    if (k[SDL_SCANCODE_X])       b |= (1u << BTN_A);
    if (k[SDL_SCANCODE_Z])       b |= (1u << BTN_B);
    if (k[SDL_SCANCODE_RSHIFT])  b |= (1u << BTN_SELECT);
    if (k[SDL_SCANCODE_RETURN])  b |= (1u << BTN_START);

    if (k[SDL_SCANCODE_UP])      b |= (1u << BTN_UP);
    if (k[SDL_SCANCODE_DOWN])    b |= (1u << BTN_DOWN);
    if (k[SDL_SCANCODE_LEFT])    b |= (1u << BTN_LEFT);
    if (k[SDL_SCANCODE_RIGHT])   b |= (1u << BTN_RIGHT);

    return b;
}

int input_update(Controller* c) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return 1;

        if (e.type == SDL_KEYDOWN &&
            e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            return 1;
        }
    }

    // SDL_GetKeyboardState needs SDL_PumpEvents; PollEvent already pumps,
    // but this makes it safe if no events happened.
    SDL_PumpEvents();

    uint8_t buttons = build_buttons_from_keyboard();
    controller_set_state(c, buttons);

    return 0;
}

void input_shutdown(void) {
    if (g_window) SDL_DestroyWindow(g_window);
    g_window = NULL;
    SDL_Quit();
}