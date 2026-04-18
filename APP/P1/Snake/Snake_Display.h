/***********************************************************************
 * File        : Snake_Display.h
 * Module      : Snake / Display manager
 * Description : Rendering primitives built on top of the OLED driver.
 *               Provides cell-level drawing for the play area, a small
 *               5x7 pixel font for status/score text and the three
 *               dedicated screens (welcome, playing, game-over).
 ***********************************************************************/
#ifndef SNAKE_DISPLAY_H_
#define SNAKE_DISPLAY_H_

#include "STD_TYPES.h"
#include "Snake_Game.h"

/**
 * @brief  Clear OLED frame buffer. Does not push to the panel.
 */
void SnakeDisplay_Clear(void);

/**
 * @brief  Push the current frame buffer to the OLED.
 */
void SnakeDisplay_Flush(void);

/**
 * @brief  Draw a solid 4x4 pixel block (snake body / head).
 */
void SnakeDisplay_DrawCell(uint8 grid_x, uint8 grid_y);

/**
 * @brief  Draw a food marker in a 4x4 cell. Visually distinct from
 *         the snake body (a centred 2x2 dot).
 */
void SnakeDisplay_DrawFood(uint8 grid_x, uint8 grid_y);

/**
 * @brief  Draw the "magical dot" marker in a 4x4 cell, rendered as
 *         a hollow 4x4 ring so it is visually distinct from both
 *         the snake body (solid) and regular food (centred 2x2).
 */
void SnakeDisplay_DrawMagicDot(uint8 grid_x, uint8 grid_y);

/**
 * @brief  Draw the top status bar with current score and high score.
 */
void SnakeDisplay_DrawStatusBar(uint16 score, uint16 high_score);

/**
 * @brief  Render the WELCOME screen.
 */
void SnakeDisplay_ShowWelcome(uint16 high_score);

/**
 * @brief  Render the GAMEOVER screen.
 */
void SnakeDisplay_ShowGameOver(uint16 score, uint16 high_score);

/**
 * @brief  Draw a null-terminated ASCII string at the given pixel
 *         coordinates using the built-in 5x7 font.
 */
void SnakeDisplay_DrawText(uint8 x, uint8 y, const char *str);

#endif /* SNAKE_DISPLAY_H_ */
