#ifndef UI_BUTTON_H_
#define UI_BUTTON_H_

#include <SDL.h>
#include <SDL_ttf.h>
#include "ui_element.h"

// Struct defining the elements of the button
typedef struct {
    UiElement base;
    const char* text;
    SDL_Color bg_color;
    SDL_Color text_color;
    void (*on_click)(void*);
    void* context;
} UiButton;

/**
 * @brief Creates and returns a UiButton.
 *
 * @param x Relative x coordinate for the button.
 * @param y Relative y coordinate for the button.
 * @param w Width value for the button.
 * @param h Height value for the button.
 * @param bg_color Color for the background.
 * @param text_color Color for any text rendered onto the button. Default format is centered.
 * @param on_click Function pointer invoked when the button is clicked.
 * @param context User-defined context passed to the on_click function.
 * 
 * @return A fully initialized UiButton.
 */
UiButton ui_button_create(int x, int y, int w, int h,
                          const char* text,
                          SDL_Color bg_color,
                          SDL_Color text_color,
                          void (*on_click)(void*),
                          void* context);

/**
 * @brief Processes an SDL event and triggers the button callback if activated.
 *
 * @param button Pointer to the UiButton to process.
 * @param event Pointer to the SDL_Event to evaluate.
 */
void ui_button_handle_event(UiButton* button, const SDL_Event* event);

/**
 * @brief Renders a UiButton.
 * 
 * @param renderer Pointer to a valid SDL_Renderer.
 * @param font Pointer to a valid TTF_Font used for rendering button text.
 * @param button Pointer to the UiButton to render.
 */
void ui_button_render(SDL_Renderer* renderer, TTF_Font* font, UiButton* button);

#endif
