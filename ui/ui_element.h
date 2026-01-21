#ifndef UI_ELEMENT_H_
#define UI_ELEMENT_H_

#include <stdbool.h>
#include <SDL.h>

typedef struct {
    SDL_Rect rect;
    bool visible;
} UiElement;

#endif
