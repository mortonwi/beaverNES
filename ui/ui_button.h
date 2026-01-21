#ifndef UI_BUTTON_H_
#define UI_BUTTON_H_

#include <SDL.h>
#include <SDL_ttf.h>
#include "ui_element.h"

typedef struct {
    UiElement base;
    const char* text;
    SDL_Color bg_color;
    SDL_Color text_color;
    void (*on_click)(void*);
    void* context;
} UiButton;

UiButton ui_button_create(int x, int y, int w, int h,
                          const char* text,
                          SDL_Color bg_color,
                          SDL_Color text_color,
                          void (*on_click)(void*),
                          void* context);

void ui_button_handle_event(UiButton* button, const SDL_Event* event);
void ui_button_render(SDL_Renderer* renderer, TTF_Font* font, UiButton* button);

#endif
