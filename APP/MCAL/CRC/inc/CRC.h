#ifndef CRC_H_
#define CRC_H_

#include "STD_TYPES.h"

/*===========================================================================
  STM32F446RE — Hardware CRC Unit
  Base : 0x40023000  (AHB1, bit 12 of AHB1ENR)
  Polynomial : 0x04C11DB7  (CRC-32 / ISO 3309 / Ethernet / ZIP)
  Input  : 32-bit words, MSB first
  Reset value : 0xFFFFFFFF
===========================================================================*/

/* ── Registers ─────────────────────────────────────────────────────────── */
#define CRC_BASE    0x40023000UL
#define CRC_DR      (*(volatile uint32*)(CRC_BASE + 0x00U))  /* data register   */
#define CRC_IDR     (*(volatile uint8* )(CRC_BASE + 0x04U))  /* 8-bit scratch   */
#define CRC_CR      (*(volatile uint32*)(CRC_BASE + 0x08U))  /* control register */

#define CRC_CR_RESET_BIT  0U   /* write 1 → resets CRC_DR to 0xFFFFFFFF */

/* ── Return codes ───────────────────────────────────────────────────────── */
#define CRC_OK      0U
#define CRC_ERR     1U

/* ── API ────────────────────────────────────────────────────────────────── */

/**
 * CRC_Init
 * Enables AHB1 clock for the CRC unit.
 * Call once in SystemInit or APP_init.
 */
void CRC_Init(void);

/**
 * CRC_Reset
 * Resets the internal accumulator to 0xFFFFFFFF without touching the clock.
 * Call before starting a new computation.
 */
void CRC_Reset(void);

/**
 * CRC_AccumulateWord
 * Feeds one 32-bit word into the hardware and returns the running CRC.
 * The hardware automatically accumulates — no need to XOR manually.
 */
uint32 CRC_AccumulateWord(uint32 word);

/**
 * CRC_Calculate
 * Computes CRC-32 over a byte buffer.
 * Handles buffers that are NOT a multiple of 4 bytes (pads last word with 0xFF).
 *
 * @param buf   pointer to data
 * @param len   number of bytes
 * @return      32-bit CRC
 */
uint32 CRC_Calculate(const uint8* buf, uint32 len);

/**
 * CRC_Verify
 * Checks a buffer whose last 4 bytes are the expected CRC (big-endian, appended).
 * Used by the bootloader after receiving firmware over UART.
 *
 * Layout: [N bytes firmware][4 bytes CRC32 big-endian]
 *
 * @param buf       pointer to full received buffer (firmware + CRC)
 * @param total_len total bytes including the 4-byte CRC trailer
 * @return          CRC_OK (0) if valid, CRC_ERR (1) if corrupted
 */
uint8 CRC_Verify(const uint8* buf, uint32 total_len);

/**
 * CRC_GetResult
 * Returns the current accumulator value without feeding new data.
 */
uint32 CRC_GetResult(void);

#endif /* CRC_H_ */
