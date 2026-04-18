/***********************************************************************
 * File        : Snake_Game.h
 * Module      : Snake / Game logic
 * Target      : STM32 NUCLEO-F446RE + 128x64 SSD1306 OLED (I2C)
 * Description : Public API of the Snake game. The module owns the
 *               finite-state machine (WELCOME -> PLAYING -> GAMEOVER),
 *               the snake body, food placement and collision detection.
 *
 * Notes on geometry:
 *   - Each logical cell is 4 x 4 OLED pixels.
 *   - The play area is 128 x 56 px, which yields a 32 x 14 cell grid.
 *   - The top 8 px of the display are reserved for the status bar.
 *
 * MISRA-C notes:
 *   - All types are fixed-width (STD_TYPES.h).
 *   - No dynamic allocation, no recursion, no variadic functions.
 *   - Every control path has explicit braces.
 ***********************************************************************/
#ifndef SNAKE_GAME_H_
#define SNAKE_GAME_H_

#include "STD_TYPES.h"

/*-----------------------------------------------------------------*/
/*  Grid / display geometry                                        */
/*-----------------------------------------------------------------*/
#define SNAKE_CELL_PX          (4U)                            /* pixels per cell     */
#define SNAKE_STATUS_BAR_PX    (8U)                            /* top bar height (px) */
#define SNAKE_PLAY_AREA_W_PX   (128U)                          /* play area width     */
#define SNAKE_PLAY_AREA_H_PX   (56U)                           /* play area height    */
#define SNAKE_GRID_W           (SNAKE_PLAY_AREA_W_PX / SNAKE_CELL_PX)  /* 32 cells */
#define SNAKE_GRID_H           (SNAKE_PLAY_AREA_H_PX / SNAKE_CELL_PX)  /* 14 cells */
#define SNAKE_MAX_LENGTH       ((uint16)(SNAKE_GRID_W * SNAKE_GRID_H)) /* 448 cells */

/*-----------------------------------------------------------------*/
/*  Gameplay parameters                                            */
/*-----------------------------------------------------------------*/
#define SNAKE_INIT_LENGTH      (3U)        /* starting body size                     */

/* Adaptive-difficulty timing.
 * The game step period starts at SNAKE_TICK_MS_INIT (slow) and is
 * shortened by SNAKE_TICK_MS_STEP every SNAKE_SPEEDUP_EVERY foods
 * eaten, down to SNAKE_TICK_MS_MIN. All values must be multiples
 * of the 20 ms scheduler slot -- the implementation divides by 20
 * to get a call-count divider.
 */
#define SNAKE_TICK_MS_INIT     (200U)      /* initial step period in ms              */
#define SNAKE_TICK_MS_MIN      (60U)       /* fastest step period we allow (ms)      */
#define SNAKE_TICK_MS_STEP     (20U)       /* reduction per speed-up bracket (ms)    */
#define SNAKE_SPEEDUP_EVERY    (5U)        /* foods eaten per speed-up bracket       */

/*-----------------------------------------------------------------*/
/*  Magical-dot / bonus-mode parameters                            */
/*-----------------------------------------------------------------*/
/*
 * A "magical dot" appears at a random free cell after a random
 * cooldown. It stays visible for SNAKE_MAGIC_TTL_MS; if the snake
 * eats it in time, the game enters BONUS mode: SNAKE_BONUS_FOODS
 * food items are placed at once, and the snake has
 * SNAKE_BONUS_DURATION_MS to eat as many as it can before the game
 * returns to normal (single-food) play. Eating the magic dot itself
 * does NOT grow the snake or change the score -- only the bonus
 * foods do (each counts as a normal +1).
 */
#define SNAKE_MAGIC_TTL_MS              (5000U)   /* magic-dot lifetime       */
#define SNAKE_BONUS_DURATION_MS         (10000U)  /* bonus-mode duration      */
#define SNAKE_BONUS_FOODS               (5U)      /* foods placed in bonus    */
#define SNAKE_MAGIC_COOLDOWN_MIN_MS     (10000U)  /* min wait between spawns  */
#define SNAKE_MAGIC_COOLDOWN_RANGE_MS   (10000U)  /* random extra (=> 10-20s) */

/*-----------------------------------------------------------------*/
/*  Public types                                                   */
/*-----------------------------------------------------------------*/
typedef enum
{
    SNAKE_STATE_WELCOME  = 0,
    SNAKE_STATE_PLAYING  = 1,
    SNAKE_STATE_GAMEOVER = 2
} Snake_State_t;

typedef enum
{
    SNAKE_DIR_UP    = 0,
    SNAKE_DIR_DOWN  = 1,
    SNAKE_DIR_LEFT  = 2,
    SNAKE_DIR_RIGHT = 3,
    SNAKE_DIR_NONE  = 4     /* used as "any key" request */
} Snake_Dir_t;

/*-----------------------------------------------------------------*/
/*  Public API                                                     */
/*-----------------------------------------------------------------*/

/**
 * @brief  One-shot initialisation of the game subsystem.
 *         Must be called once after the OLED, GPIO, I2C and flash
 *         drivers have been brought up.
 */
void Snake_Init(void);

/**
 * @brief  Cyclic hook. Call this every 20 ms from the scheduler.
 *         Internally it divides the rate for input polling, snake
 *         stepping and display refresh. The step period starts at
 *         SNAKE_TICK_MS_INIT and shortens with the score (see the
 *         SNAKE_TICK_MS_* / SNAKE_SPEEDUP_EVERY macros).
 */
void Snake_Tick(void);

/**
 * @brief  Current high-level state (useful for tests / telemetry).
 */
Snake_State_t Snake_GetState(void);

#endif /* SNAKE_GAME_H_ */
