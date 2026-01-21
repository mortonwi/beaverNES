#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include "ui/ui_button.h"

// context for buttons
typedef struct {
    bool* menuVisible;
} MenuContext;

// toggle menu visibility
void menuButtonClicked(void* ctx) {
    MenuContext* context = (MenuContext*)ctx;
    *(context->menuVisible) = !*(context->menuVisible);
}

int main(int argc, char *argv[]) {
    // initialize SDL and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // create windows related events
    bool running = true;
    SDL_Event event;

    // colors
    SDL_Color blackTextColor = {0, 0, 0, 255};
    SDL_Color menuButtonColor = {200, 200, 200, 255};

    // Setup context for menu button
    bool menuVisible = false;
    MenuContext menuCtx = { .menuVisible = &menuVisible };

    // create main emulator window
    SDL_Window *window = SDL_CreateWindow(
        "beaverNES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800,
        600,
        0
    );

    // create window elements
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // load font w/ size (Runs from build directory)
    TTF_Font *font = TTF_OpenFont("../fonts/Rubik/Rubik-Bold.ttf", 20);

    SDL_Rect gameView = {144, 44, 512, 512};
    UiButton menuButton = ui_button_create(700, 550, 80, 30, "Menu", menuButtonColor, blackTextColor, menuButtonClicked, &menuCtx);

    // main loop
    while(running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }

            ui_button_handle_event(&menuButton, &event);
        }

        // clear
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);

        // draw game view
        SDL_SetRenderDrawColor(renderer, 60, 60, 200, 255);
        SDL_RenderFillRect(renderer, &gameView);

        // render menu button
        ui_button_render(renderer, font, &menuButton);

        // game view text
        SDL_Surface *gameTextSurface = TTF_RenderText_Solid(font, "Game View", blackTextColor);
        SDL_Texture *gameTextTexture = SDL_CreateTextureFromSurface(renderer, gameTextSurface);

        // center gameview text
        SDL_Rect gameTextRect;
        gameTextRect.w = gameTextSurface->w;
        gameTextRect.h = gameTextSurface->h;
        gameTextRect.x = gameView.x + (gameView.w - gameTextRect.w) / 2;
        gameTextRect.y = gameView.y + (gameView.h - gameTextRect.h) / 2;

        SDL_RenderCopy(renderer, gameTextTexture, NULL, &gameTextRect);
        SDL_FreeSurface(gameTextSurface);
        SDL_DestroyTexture(gameTextTexture);

        // border text
        SDL_Surface *borderTextSurface = TTF_RenderText_Solid(font, "beaverNES", blackTextColor);
        SDL_Texture *borderTextTexture = SDL_CreateTextureFromSurface(renderer, borderTextSurface);

        // border text
        SDL_Rect borderTextRect;
        borderTextRect.w = borderTextSurface->w;
        borderTextRect.h = borderTextSurface->h;
        borderTextRect.x = 10;
        borderTextRect.y = 10;

        SDL_RenderCopy(renderer, borderTextTexture, NULL, &borderTextRect);
        SDL_FreeSurface(borderTextSurface);
        SDL_DestroyTexture(borderTextTexture);

        // draw menu overlay if active
        if (menuVisible) {
            SDL_SetRenderDrawColor(renderer, 164, 164, 164, 180); 
            SDL_Rect overlay = {200, 150, 400, 300};
            SDL_RenderFillRect(renderer, &overlay);

            // menu window text
            SDL_Surface *menuTextSurface = TTF_RenderText_Solid(font, "Menu Screen", blackTextColor);
            SDL_Texture *menuTextTexture = SDL_CreateTextureFromSurface(renderer, menuTextSurface);

            // menu window text
            SDL_Rect menuTextRect;
            menuTextRect.w = menuTextSurface->w;
            menuTextRect.h = menuTextSurface->h;
            menuTextRect.x = overlay.x + (overlay.w - menuTextRect.w) / 2;
            menuTextRect.y = overlay.y + 20;

            SDL_RenderCopy(renderer, menuTextTexture, NULL, &menuTextRect);
            SDL_FreeSurface(menuTextSurface);
            SDL_DestroyTexture(menuTextTexture);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // about 60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
