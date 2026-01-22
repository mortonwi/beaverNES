#include "ui_text.h"

SDL_Rect ui_text_draw(SDL_Renderer* renderer, TTF_Font* font, const char* text, int x, int y, SDL_Color color) {
    SDL_Rect rect = {0,0,0,0};

    if (!font || !text) return rect;

    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return rect;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return rect;
    }

    rect.w = surface->w;
    rect.h = surface->h;
    rect.x = x;
    rect.y = y;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    return rect;
}

SDL_Rect ui_text_draw_centered(SDL_Renderer* renderer, TTF_Font* font,
                               const char* text, int centerX, int centerY,
                               SDL_Color color) {
    if (!font || !text) return (SDL_Rect){0,0,0,0};

    SDL_Surface* surface = TTF_RenderText_Solid(font, text, color);
    if (!surface) return (SDL_Rect){0,0,0,0};

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (!texture) {
        SDL_FreeSurface(surface);
        return (SDL_Rect){0,0,0,0};
    }

    SDL_Rect rect;
    rect.w = surface->w;
    rect.h = surface->h;
    rect.x = centerX - rect.w / 2;
    rect.y = centerY - rect.h / 2;

    SDL_RenderCopy(renderer, texture, NULL, &rect);

    SDL_DestroyTexture(texture);
    SDL_FreeSurface(surface);

    return rect;
}
