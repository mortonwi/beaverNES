#include "ui_button.h"

UiButton ui_button_create(int x, int y, int w, int h,
                          const char* text,
                          SDL_Color bg_color,
                          SDL_Color text_color,
                          void (*on_click)(void*),
                          void* context) {  // pass context here
    UiButton button;
    button.base.rect = (SDL_Rect){x, y, w, h};
    button.base.visible = true;
    button.text = text;
    button.bg_color = bg_color;
    button.text_color = text_color;
    button.on_click = on_click;
    button.context = context; // store context
    return button;
}

void ui_button_handle_event(UiButton* button, const SDL_Event* event) {
    if (!button->base.visible) return;

    if (event->type == SDL_MOUSEBUTTONDOWN) {
        int x = event->button.x;
        int y = event->button.y;
        SDL_Rect r = button->base.rect;

        if (x >= r.x && x <= r.x + r.w &&
            y >= r.y && y <= r.y + r.h) {
            if (button->on_click) {
                button->on_click(button->context);
            }
        }
    }
}

void ui_button_render(SDL_Renderer* renderer, TTF_Font* font, UiButton* button) {
    if (!button->base.visible) return;

    SDL_SetRenderDrawColor(renderer,
        button->bg_color.r,
        button->bg_color.g,
        button->bg_color.b,
        255);
    SDL_RenderFillRect(renderer, &button->base.rect);

    SDL_Surface* surface =
        TTF_RenderText_Solid(font, button->text, button->text_color);
    SDL_Texture* texture =
        SDL_CreateTextureFromSurface(renderer, surface);

    SDL_Rect text_rect = {
        button->base.rect.x + (button->base.rect.w - surface->w) / 2,
        button->base.rect.y + (button->base.rect.h - surface->h) / 2,
        surface->w,
        surface->h
    };

    SDL_RenderCopy(renderer, texture, NULL, &text_rect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);
}
