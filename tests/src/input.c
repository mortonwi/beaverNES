#include "input.h"

#include <SDL2/SDL.h>
#include <stdint.h>

static SDL_Window* g_window = NULL;

/*
    Customizable keyboard mapping for NES controls.
    These values can later be changed by your UI/settings menu.
*/
typedef struct {
    SDL_Scancode a;
    SDL_Scancode b;
    SDL_Scancode select;
    SDL_Scancode start;

    SDL_Scancode up;
    SDL_Scancode down;
    SDL_Scancode left;
    SDL_Scancode right;
} InputMapping;

/*
    Default keyboard controls:
    X       = A
    Z       = B
    RSHIFT  = Select
    ENTER   = Start
    Arrows  = D-pad
*/
static InputMapping controls = {
    SDL_SCANCODE_X,
    SDL_SCANCODE_Z,
    SDL_SCANCODE_RSHIFT,
    SDL_SCANCODE_RETURN,

    SDL_SCANCODE_UP,
    SDL_SCANCODE_DOWN,
    SDL_SCANCODE_LEFT,
    SDL_SCANCODE_RIGHT
};

int input_init(void) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        return 1;
    }

    // Tiny window so SDL can receive keyboard events.
    g_window = SDL_CreateWindow(
        "beaverNES input test",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        320,
        240,
        0
    );

    if (!g_window) {
        SDL_Quit();
        return 1;
    }

    return 0;
}

// Build the button state bitfield from the current keyboard state.
static uint8_t build_buttons_from_keyboard(void) {
    const Uint8* k = SDL_GetKeyboardState(NULL);
    uint8_t b = 0;

    // Face buttons
    if (k[controls.a])       b |= (1u << BTN_A);
    if (k[controls.b])       b |= (1u << BTN_B);

    // System buttons
    if (k[controls.select])  b |= (1u << BTN_SELECT);
    if (k[controls.start])   b |= (1u << BTN_START);

    // D-pad
    if (k[controls.up])      b |= (1u << BTN_UP);
    if (k[controls.down])    b |= (1u << BTN_DOWN);
    if (k[controls.left])    b |= (1u << BTN_LEFT);
    if (k[controls.right])   b |= (1u << BTN_RIGHT);

    return b;
}

// Poll for input events and update the controller state.
// Returns 1 if the user requested to quit, 0 otherwise.
int input_update(Controller* c) {
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            return 1;
        }

        if (e.type == SDL_KEYDOWN &&
            e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
            return 1;
        }
    }

    // SDL_GetKeyboardState needs SDL_PumpEvents
    SDL_PumpEvents();

    uint8_t buttons = build_buttons_from_keyboard();
    controller_set_state(c, buttons);

    return 0;
}

// Clean up SDL resources.
void input_shutdown(void) {
    if (g_window) {
        SDL_DestroyWindow(g_window);
    }

    g_window = NULL;
    SDL_Quit();
}