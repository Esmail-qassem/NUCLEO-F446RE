#ifndef WWDG_H_
#define WWDG_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  WWDG — Window Watchdog
 *
 *  Clock source : APB1 (PCLK1), divided by a fixed factor of 4096
 *                 and then by the WDGTB prescaler.
 *
 *  Counter      : 7-bit down counter T[6:0] in CR.
 *                 Bit T[6] must stay 1 while running; when it flips
 *                 to 0 (counter reaches 0x3F = 63) the MCU resets.
 *
 *  Window       : Counter can be refreshed ONLY while
 *                     0x40 ≤ T[6:0] ≤ W[6:0]
 *                 Writing too early (T > W) → immediate reset.
 *                 Writing too late  (T < 0x40) → reset already happened.
 *
 *  EWI (Early Wakeup Interrupt): fires one clock period before reset,
 *  i.e. when the counter reaches 0x40. Use it to save state or log.
 *
 *  Timeout formula (one counter tick):
 *      T_tick = Tpclk1 × 4096 × 2^WDGTB
 *
 *  Full window timeout (from T_start down to 0x40):
 *      T_window = T_tick × (T[5:0] + 1)
 *
 *  Closed window (forbidden zone, too early to refresh):
 *      T_closed = T_tick × (T[5:0] - W[5:0])
 *
 *  Prescaler reference (PCLK1 = 16 MHz → Tpclk1 = 62.5 ns):
 *  ┌────────┬──────────┬──────────────────────────────────────┐
 *  │ WDGTB  │  T_tick  │  Max window (T=0x7F, W=0x40)        │
 *  ├────────┼──────────┼──────────────────────────────────────┤
 *  │   /1   │  256 µs  │  ~16.4 ms (64 ticks)                │
 *  │   /2   │  512 µs  │  ~32.8 ms                           │
 *  │   /4   │ 1024 µs  │  ~65.5 ms                           │
 *  │   /8   │ 2048 µs  │  ~131 ms                            │
 *  └────────┴──────────┴──────────────────────────────────────┘
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 *  Register base and access
 *------------------------------------------------------------------*/
#define WWDG_BASE   0x40002C00U

#define WWDG_CR     (*((volatile uint32*)(WWDG_BASE + 0x00U)))  /* Control */
#define WWDG_CFR    (*((volatile uint32*)(WWDG_BASE + 0x04U)))  /* Config  */
#define WWDG_SR     (*((volatile uint32*)(WWDG_BASE + 0x08U)))  /* Status  */

/*------------------------------------------------------------------
 *  CR bit masks
 *------------------------------------------------------------------*/
#define WWDG_CR_T_Mask  0x7FU           /* Counter bits [6:0]   */
#define WWDG_CR_WDGA    (1U << 7)       /* Activate (write-once)*/

/*------------------------------------------------------------------
 *  CFR bit masks
 *------------------------------------------------------------------*/
#define WWDG_CFR_W_Mask   0x7FU         /* Window bits [6:0]    */
#define WWDG_CFR_WDGTB_Pos  7U          /* Prescaler position   */
#define WWDG_CFR_WDGTB_Mask (3U << 7)   /* Prescaler mask       */
#define WWDG_CFR_EWI        (1U << 9)   /* Early wakeup int     */

/*------------------------------------------------------------------
 *  SR bit mask
 *------------------------------------------------------------------*/
#define WWDG_SR_EWIF    (1U << 0)       /* Early wakeup int flag*/

/*------------------------------------------------------------------
 *  Counter limits
 *------------------------------------------------------------------*/
#define WWDG_COUNTER_MIN   0x40U   /* Minimum valid counter value */
#define WWDG_COUNTER_MAX   0x7FU   /* Maximum valid counter value */

/*------------------------------------------------------------------
 *  Prescaler enumeration
 *------------------------------------------------------------------*/
typedef enum {
    WWDG_PRE_1 = 0,   /* PCLK1 / 4096 / 1 */
    WWDG_PRE_2 = 1,   /* PCLK1 / 4096 / 2 */
    WWDG_PRE_4 = 2,   /* PCLK1 / 4096 / 4 */
    WWDG_PRE_8 = 3    /* PCLK1 / 4096 / 8 */
} WWDG_Prescaler_t;

/*------------------------------------------------------------------
 *  Configuration structure
 *------------------------------------------------------------------*/
typedef struct {
    WWDG_Prescaler_t prescaler;   /* Clock prescaler                      */
    uint8            window;      /* Window value W[6:0] (0x40 – 0x7F)   */
    uint8            counter;     /* Initial counter T[6:0] (0x40 – 0x7F)*/
    uint8            ewi_enable;  /* 1 = enable early wakeup interrupt    */
} WWDG_Config_t;

/*------------------------------------------------------------------
 *  Public API
 *------------------------------------------------------------------*/

/**
 * WWDG_Init — Configure and start the WWDG.
 *  Requires APB1 WWDG clock enabled via RCC before calling.
 *  Note: WDGA is a write-once bit; once set only a reset clears it.
 */
void WWDG_Init(const WWDG_Config_t *config);

/**
 * WWDG_Refresh — Reload the counter.
 *  @param counter  New counter value (must be 0x40 – 0x7F and ≤ window)
 *  Call this within the open window (when current counter ≤ W and > 0x3F).
 */
void WWDG_Refresh(uint8 counter);

/**
 * WWDG_ClearFlag — Clear the EWI flag in SR (call inside WWDG_IRQHandler).
 */
void WWDG_ClearFlag(void);

/**
 * WWDG_GetFlag — Return non-zero if EWI flag is set.
 */
uint8 WWDG_GetFlag(void);

/**
 * WWDG_CalcCounter — Compute an initial counter / window value that
 * gives approximately the requested timeout at the given PCLK1.
 *
 * @param timeout_ms   Desired timeout in milliseconds
 * @param prescaler    The prescaler that will be used
 * @param pclk1_hz     APB1 clock in Hz (e.g. 16000000)
 * @return             7-bit counter value (0x40 – 0x7F), or 0x7F if clamped
 */
uint8 WWDG_CalcCounter(uint32 timeout_ms, WWDG_Prescaler_t prescaler,
                        uint32 pclk1_hz);

#endif /* WWDG_H_ */
