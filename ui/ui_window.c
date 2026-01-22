#include "ui_window.h"

UiWindow ui_window_create(int x, int y, int w, int h,
                          bool visible, 
                          SDL_Color bg_color,
                          SDL_Color text_color) {
    UiWindow window;
    
    window.base.rect = (SDL_Rect){x, y, w, h};
    window.base.visible = visible;

    window.bg_color = bg_color;
    window.text_color = bg_color;

    return window;
}

void ui_window_toggle(UiWindow* window) {
    window->base.visible = !window->base.visible;
}

void ui_window_render(
    SDL_Renderer* renderer,
    TTF_Font* font,
    UiWindow* window
) {
    if (!window->base.visible) return;

    SDL_SetRenderDrawColor(
        renderer,
        window->bg_color.r,
        window->bg_color.g,
        window->bg_color.b,
        window->bg_color.a
    );
    SDL_RenderFillRect(renderer, &window->base.rect);
}
