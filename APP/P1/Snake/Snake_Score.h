/***********************************************************************
 * File        : Snake_Score.h
 * Module      : Snake / Score manager
 * Description : Tracks the current score and the persistent high
 *               score. The high score lives in sector 7 of the
 *               internal flash (128 KB @ 0x08060000). That sector is
 *               reserved in LinkerScript.ld (FLASH LENGTH was shrunk
 *               from 480 K to 352 K) so it is never used by code or
 *               rodata.
 *
 * Wear-levelled layout (why):
 *   A full 128 KB sector erase on F446 can take up to ~3 s, far above
 *   the 150 ms IWDG timeout. To avoid ever hitting the erase path
 *   during normal gameplay we append 8-byte records to the sector
 *   and only erase when it fills up. One sector holds 16384 records,
 *   so in practice the erase path is hit once in thousands of new
 *   high scores -- effectively never for a single device.
 *
 * Record layout (8 bytes, two aligned 32-bit words):
 *   offset 0 : uint32 magic            (0xA5D5F00D when valid)
 *   offset 4 : uint16 high_score
 *   offset 6 : uint16 high_score_inv   (bitwise complement of value)
 *
 * For the rare "sector full" case, erase is performed by a local
 * routine that refreshes the IWDG inside its polling loop, so even
 * a multi-second erase cannot cause a watchdog reset.
 ***********************************************************************/
#ifndef SNAKE_SCORE_H_
#define SNAKE_SCORE_H_

#include "STD_TYPES.h"

/**
 * @brief  Load the high score from flash. If no valid record is found
 *         the high score is initialised to 0 (no flash write is
 *         performed yet; it only happens when a new high score is
 *         achieved). Must be called once at startup.
 */
void SnakeScore_Init(void);

/**
 * @brief  Reset the current score to 0.
 */
void SnakeScore_Reset(void);

/**
 * @brief  Increase current score by 1 unit (one food eaten).
 *         If the new value exceeds the stored high score, the high
 *         score is updated in RAM (not yet flushed to flash).
 */
void SnakeScore_Increment(void);

/**
 * @brief  Persist the high score to flash. Call this when entering
 *         the GAMEOVER state.
 */
void SnakeScore_Persist(void);

/**
 * @brief  Return the current score (current run).
 */
uint16 SnakeScore_GetCurrent(void);

/**
 * @brief  Return the highest score seen so far (flash-backed).
 */
uint16 SnakeScore_GetHigh(void);

#endif /* SNAKE_SCORE_H_ */
