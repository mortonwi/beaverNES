#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_ttf.h>

int main(int argc, char *argv[]) {
    // start SDL and SDL_ttf
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() != 0) {
        printf("TTF_Init Error: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }

    // event to close the window
    bool running = true;
    bool menuVisible = false;
    SDL_Event event;

    // create main emulator window
    SDL_Window *window = SDL_CreateWindow(
        "beaverNES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800,
        600,
        0
    );

    // create window objects
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Rect gameView = {144, 44, 512, 512};
    SDL_Rect menuButton = {700, 550, 80, 30};

    // load font w/ size
    TTF_Font *font = TTF_OpenFont("C:/Users/willm/OneDrive/Documents/CS 46X/beaverNES/beaverNES/fonts/Rubik/Rubik-Bold.ttf", 20);
    SDL_Color blackTextColor = {0, 0, 0, 255};

    // main loop
    while(running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }
            if (event.type == SDL_MOUSEBUTTONDOWN) {
                int x = event.button.x, y = event.button.y;
                if (x >= menuButton.x && x <= menuButton.x + menuButton.w &&
                    y >= menuButton.y && y <= menuButton.y + menuButton.h) {
                    menuVisible = !menuVisible;
                }
            }
        }

        // clear
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);

        // draw game view
        SDL_SetRenderDrawColor(renderer, 60, 60, 200, 255);
        SDL_RenderFillRect(renderer, &gameView);

        // draw menu button
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_RenderFillRect(renderer, &menuButton);

        // draw text on menu button
        SDL_Surface *textSurface = TTF_RenderText_Solid(font, "Menu", blackTextColor);
        SDL_Texture *textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);

        // center button text
        SDL_Rect textRect;
        textRect.w = textSurface->w;
        textRect.h = textSurface->h;
        textRect.x = menuButton.x + (menuButton.w - textRect.w) / 2;
        textRect.y = menuButton.y + (menuButton.h - textRect.h) / 2;

        SDL_RenderCopy(renderer, textTexture, NULL, &textRect);

        SDL_FreeSurface(textSurface);
        SDL_DestroyTexture(textTexture);

        // draw menu overlay if active
        if (menuVisible) {
            SDL_SetRenderDrawColor(renderer, 164, 164, 164, 180); 
            SDL_Rect overlay = {200, 150, 400, 300};
            SDL_RenderFillRect(renderer, &overlay);
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // about 60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}