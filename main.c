#include <stdio.h>
#include <stdbool.h>
#include <SDL.h>
#include <SDL_ttf.h>

#include "ui/ui_button.h"
#include "ui/ui_window.h"
#include "ui/ui_text.h"

// toggle menu visibility
void menuButtonClicked(void* ctx) {
    UiWindow* menuWindow = (UiWindow*)ctx;
    ui_window_toggle(menuWindow);
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
    SDL_Color menuBgColor     = {164, 164, 164, 180};

    // create main emulator window
    SDL_Window *window = SDL_CreateWindow(
        "beaverNES",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        800,
        600,
        0
    );

    // create renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    // load font w/ size (Runs from build directory)
    TTF_Font *font = TTF_OpenFont("../fonts/Rubik/Rubik-Bold.ttf", 20);

    if (!font) {
        printf("Font load failed: %s\n", TTF_GetError());
    }

    // set gameview area
    SDL_Rect gameView = {144, 44, 512, 512};

    // create ui elements
    UiWindow menuWindow = ui_window_create(200, 150, 400, 300, false, menuBgColor, blackTextColor);
    UiButton menuButton = ui_button_create(700, 550, 80, 30, "Menu", menuButtonColor, blackTextColor, menuButtonClicked, &menuWindow);

    // main loop
    while(running) {
        while(SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = 0;
            }

            // check for button clicks
            ui_button_handle_event(&menuButton, &event);
        }

        // clear
        SDL_SetRenderDrawColor(renderer, 128, 128, 128, 255);
        SDL_RenderClear(renderer);

        // draw game view
        SDL_SetRenderDrawColor(renderer, 60, 60, 200, 255);
        SDL_RenderFillRect(renderer, &gameView);

        // render text using ui helper
        ui_text_draw_centered(renderer, font, "Game View",
                            gameView.x + gameView.w / 2,
                            gameView.y + gameView.h / 2,
                            blackTextColor);

        ui_text_draw(renderer, font, "beaverNES", 10, 10, blackTextColor);

        // draw ui elements
        ui_button_render(renderer, font, &menuButton);
        ui_window_render(renderer, font, &menuWindow);

        SDL_RenderPresent(renderer);
        SDL_Delay(16); // about 60 FPS
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
