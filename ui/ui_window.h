#ifndef UI_WINDOW_H_
#define UI_WINDOW_H_

#include <SDL.h>
#include <SDL_ttf.h>
#include "ui_element.h"
#include "ui_button.h"

// predefined arbitrary limits
#define UI_WINDOW_MAX_BUTTONS 8
#define UI_WINDOW_MAX_LABELS  8

// label struct for adding text to a window
typedef struct {
    const char* text;
    SDL_Point offset;    // relative to window origin
} UiWindowLabel;

// struct defining UiWindow
typedef struct {
    UiElement base;
    SDL_Color bg_color;
    SDL_Color text_color;

    UiButton* buttons[UI_WINDOW_MAX_BUTTONS];
    int button_count;

    UiWindowLabel labels[UI_WINDOW_MAX_LABELS];
    int label_count;
} UiWindow;

/**
 * @brief Creates and returns a UiWindow.
 *
 * @param x X coordinate for the window relative to main window.
 * @param y Y coordinate for the window relative to main window.
 * @param w Width value for the window.
 * @param h Height value for the window.
 * @param visible Determines whether the window is drawn or not.
 * @param bg_color Color for the background.
 * @param text_color Color for any child text/label.
 * 
 * @return A fully initialized UiWindow.
 */
UiWindow ui_window_create(int x, int y, int w, int h,
                          bool visibile,
                          SDL_Color bg_color,
                          SDL_Color text_color);

/**
 * @brief Toggles the visibility of a window.
 *
 * If the window is currently visible, it will be hidden.
 * If it is hidden, it will be shown.
 *
 * @param window Pointer to the UiWindow to modify.
 */
void ui_window_toggle(UiWindow* window);

/**
 * @brief Renders a UiWindow.
 *
 * Renders a UiWindow with all of the appropriate parameters.
 *
 * @param renderer Pointer to a valid SDL_Renderer.
 * @param font Pointer to a valid TTF_Font used for rendering button text.
 * @param window Pointer to the UiWindow to render.
 */
void ui_window_render(SDL_Renderer* renderer, TTF_Font* font, UiWindow* window);

// unfinished
void ui_window_add_button(UiWindow* window, UiButton* button);
void ui_window_add_label(UiWindow* window, const char* text, int offset_x, int offset_y);

#endif
