#ifndef UI_TEXT_H
#define UI_TEXT_H

#include <SDL.h>
#include <SDL_ttf.h>

/**
 * @brief Draws a string of text at a specific position.
 *
 * @param renderer Pointer to SDL_Renderer.
 * @param font Pointer to TTF_Font to use.
 * @param text Text string to render.
 * @param x X position on screen.
 * @param y Y position on screen.
 * @param color SDL_Color for the text.
 * 
 * @return SDL_Rect of the rendered text.
 */
SDL_Rect ui_text_draw(SDL_Renderer* renderer, 
                      TTF_Font* font, 
                      const char* text, 
                      int x, int y, 
                      SDL_Color color);

/**
 * @brief Renders text centered at a specific position.
 *
 * @param renderer Valid SDL_Renderer to draw to.
 * @param font Valid TTF_Font loaded in memory.
 * @param text Null-terminated string to render.
 * @param centerX X coordinate for the center of the text.
 * @param centerY Y coordinate for the center of the text.
 * @param color SDL_Color to render the text with.
 * 
 * @return SDL_Rect containing the position and size of the rendered text.
 */
SDL_Rect ui_text_draw_centered(SDL_Renderer* renderer,
                               TTF_Font* font,
                               const char* text,
                               int centerX,
                               int centerY,
                               SDL_Color color);

#endif