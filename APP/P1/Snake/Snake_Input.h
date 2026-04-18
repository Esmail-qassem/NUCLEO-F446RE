/***********************************************************************
 * File        : Snake_Input.h
 * Module      : Snake / Input handling
 * Description : Four momentary push buttons (UP / DOWN / LEFT / RIGHT)
 *               are polled from GPIOC. Buttons are active-LOW (internal
 *               pull-up). The module exposes edge-triggered events so
 *               that holding a button only produces a single press.
 *
 * Pin mapping (active LOW, pull-up internal):
 *   UP    -> PC0
 *   DOWN  -> PC1
 *   LEFT  -> PC2
 *   RIGHT -> PC4     (PC3 is the bootloader trigger and must be kept
 *                     free -> we skip it)
 ***********************************************************************/
#ifndef SNAKE_INPUT_H_
#define SNAKE_INPUT_H_

#include "STD_TYPES.h"
#include "Snake_Game.h"   /* Snake_Dir_t */

/**
 * @brief  Configure the four button GPIOs as input with pull-up.
 */
void SnakeInput_Init(void);

/**
 * @brief  Sample the buttons. Should be called every 20 ms so that
 *         the built-in debounce (2 consecutive equal samples) works.
 */
void SnakeInput_Poll(void);

/**
 * @brief  Non-blocking read of the most recent directional press.
 * @param  dir [out] direction that was pressed
 * @return 1 if a new press is available, 0 otherwise
 */
uint8 SnakeInput_GetDirection(Snake_Dir_t *dir);

/**
 * @brief  Consumes any pending press (any of the 4 buttons) without
 *         caring which direction. Used by WELCOME / GAMEOVER states
 *         where "press any key" is the trigger.
 * @return 1 if a press is pending, 0 otherwise
 */
uint8 SnakeInput_AnyKeyPressed(void);

#endif /* SNAKE_INPUT_H_ */
