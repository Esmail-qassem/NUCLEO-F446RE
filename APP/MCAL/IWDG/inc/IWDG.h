#ifndef IWDG_H_
#define IWDG_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  IWDG — Independent Watchdog
 *
 *  Clock source : LSI (~32 kHz, independent of SYSCLK — keeps
 *                 running in Stop and Standby modes)
 *  Counter      : 12-bit down counter (0 – 4095)
 *  Once started : CANNOT be stopped except by a system reset.
 *
 *  Timeout formula:
 *      T_out = (Prescaler_Div × (Reload + 1)) / LSI_Hz
 *
 *  Quick timeout reference (LSI = 32 kHz):
 *  ┌────────────┬──────────┬──────────────┐
 *  │  Prescaler │  T/count │   Max T_out  │
 *  ├────────────┼──────────┼──────────────┤
 *  │    /4      │  125 µs  │   512 ms     │
 *  │    /8      │  250 µs  │  1024 ms     │
 *  │   /16      │  500 µs  │  2048 ms     │
 *  │   /32      │    1 ms  │  4096 ms     │
 *  │   /64      │    2 ms  │  8192 ms     │
 *  │  /128      │    4 ms  │ 16384 ms     │
 *  │  /256      │    8 ms  │ 32768 ms     │
 *  └────────────┴──────────┴──────────────┘
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 *  Register base and access
 *------------------------------------------------------------------*/
#define IWDG_BASE   0x40003000U

#define IWDG_KR     (*((volatile uint32*)(IWDG_BASE + 0x00U)))  /* Key     */
#define IWDG_PR     (*((volatile uint32*)(IWDG_BASE + 0x04U)))  /* Prescaler */
#define IWDG_RLR    (*((volatile uint32*)(IWDG_BASE + 0x08U)))  /* Reload  */
#define IWDG_SR     (*((volatile uint32*)(IWDG_BASE + 0x0CU)))  /* Status  */

/*------------------------------------------------------------------
 *  Key register magic values
 *------------------------------------------------------------------*/
#define IWDG_KEY_RELOAD   0xAAAAU   /* Refresh the counter        */
#define IWDG_KEY_START    0xCCCCU   /* Start the watchdog         */
#define IWDG_KEY_UNLOCK   0x5555U   /* Unlock PR and RLR for write*/

/*------------------------------------------------------------------
 *  Status register bits
 *------------------------------------------------------------------*/
#define IWDG_SR_PVU   (1U << 0)   /* Prescaler value update in progress */
#define IWDG_SR_RVU   (1U << 1)   /* Reload value update in progress    */

/*------------------------------------------------------------------
 *  Prescaler enumeration (maps to PR register bits [2:0])
 *------------------------------------------------------------------*/
typedef enum {
    IWDG_PRE_4   = 0,   /* LSI / 4   */
    IWDG_PRE_8   = 1,   /* LSI / 8   */
    IWDG_PRE_16  = 2,   /* LSI / 16  */
    IWDG_PRE_32  = 3,   /* LSI / 32  */
    IWDG_PRE_64  = 4,   /* LSI / 64  */
    IWDG_PRE_128 = 5,   /* LSI / 128 */
    IWDG_PRE_256 = 6    /* LSI / 256 */
} IWDG_Prescaler_t;

/*------------------------------------------------------------------
 *  Public API
 *------------------------------------------------------------------*/

/**
 * IWDG_Init — Configure prescaler and reload, then start the IWDG.
 *
 * @param prescaler  One of IWDG_PRE_4 … IWDG_PRE_256
 * @param reload     12-bit reload value (0 – 4095)
 *
 * Call IWDG_CalcReload() to compute the reload from a timeout in ms.
 * After this call the watchdog is running — call IWDG_Refresh()
 * regularly before the timeout expires.
 */
void IWDG_Init(IWDG_Prescaler_t prescaler, uint16 reload);

/**
 * IWDG_Refresh — Feed the watchdog (write IWDG_KEY_RELOAD).
 * Must be called more often than the configured timeout.
 */
void IWDG_Refresh(void);

/**
 * IWDG_CalcReload — Compute the reload register value for a given
 * timeout and prescaler selection.
 *
 * @param timeout_ms   Desired watchdog timeout in milliseconds
 * @param prescaler    Prescaler that will be used in IWDG_Init()
 * @param lsi_hz       Actual LSI frequency in Hz (use 32000 if unknown)
 * @return             Reload value to pass to IWDG_Init() (clamped to 4095)
 */
uint16 IWDG_CalcReload(uint32 timeout_ms, IWDG_Prescaler_t prescaler,
                        uint32 lsi_hz);

#endif /* IWDG_H_ */
