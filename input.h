#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "controller.h"

typedef struct {
    int quit;               // set to 1 when user closes window / presses Esc
} InputState;

// Call once at startup (after SDL_Init in your main, or you can move SDL_Init in here)
void input_init(InputState* in);

// Call every frame to update Controller button bits from keyboard state
void input_update(InputState* in, Controller* c);

// Optional cleanup (does nothing right now, but nice to have)
void input_shutdown(InputState* in);

#endif