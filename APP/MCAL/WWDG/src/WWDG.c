#include "WWDG.h"

/*
 * Usage example — ~50 ms watchdog window on 16 MHz APB1:
 *
 *   // Enable WWDG clock first:
 *   RCC_EnableClock(RCC_APB1, APB1_WWDG);
 *
 *   // Counter for ~50 ms with /8 prescaler:
 *   //   T_tick = 1/16e6 * 4096 * 8 = 2.048 ms per count
 *   //   50 ms / 2.048 ms ≈ 24 ticks → counter = 0x40 + 24 = 0x58
 *   uint8 cnt = WWDG_CalcCounter(50, WWDG_PRE_8, 16000000);
 *
 *   WWDG_Config_t cfg = {
 *       .prescaler  = WWDG_PRE_8,
 *       .window     = cnt,         // refresh only allowed below this
 *       .counter    = cnt,         // start counter here
 *       .ewi_enable = 1,           // warn us one tick before reset
 *   };
 *   WWDG_Init(&cfg);
 *   NVIC_EnableInterrupt(WWDG_IRQ_NUM);   // enable EWI in NVIC
 *
 *   // In the main loop — call inside the open window:
 *   WWDG_Refresh(cnt);
 *
 *   // EWI handler (last resort — save state):
 *   void WWDG_IRQHandler(void) {
 *       WWDG_ClearFlag();
 *       save_critical_state();
 *   }
 */

/*------------------------------------------------------------------
 *  WWDG_Init
 *------------------------------------------------------------------*/
void WWDG_Init(const WWDG_Config_t *config)
{
    if (!config) return;

    /* 1. Set prescaler and window in CFR (before enabling in CR) */
    uint32 cfr = 0U;
    cfr |= ((uint32)config->prescaler << WWDG_CFR_WDGTB_Pos) & WWDG_CFR_WDGTB_Mask;
    cfr |= (uint32)config->window & WWDG_CFR_W_Mask;
    if (config->ewi_enable) cfr |= WWDG_CFR_EWI;
    WWDG_CFR = cfr;

    /* 2. Load counter and activate (WDGA is write-once) */
    WWDG_CR = WWDG_CR_WDGA | ((uint32)config->counter & WWDG_CR_T_Mask);
}

/*------------------------------------------------------------------
 *  WWDG_Refresh
 *------------------------------------------------------------------*/
void WWDG_Refresh(uint8 counter)
{
    /* Write the new counter value; WDGA stays set since we OR it in.
       The counter must satisfy: 0x40 ≤ counter ≤ W (window value)   */
    WWDG_CR = WWDG_CR_WDGA | ((uint32)counter & WWDG_CR_T_Mask);
}

/*------------------------------------------------------------------
 *  WWDG_ClearFlag
 *------------------------------------------------------------------*/
void WWDG_ClearFlag(void)
{
    WWDG_SR &= ~WWDG_SR_EWIF;
}

/*------------------------------------------------------------------
 *  WWDG_GetFlag
 *------------------------------------------------------------------*/
uint8 WWDG_GetFlag(void)
{
    return (WWDG_SR & WWDG_SR_EWIF) ? 1U : 0U;
}

/*------------------------------------------------------------------
 *  WWDG_CalcCounter
 *
 *  T_tick = (4096 × 2^WDGTB) / PCLK1
 *  ticks  = timeout_ms / (T_tick * 1000)
 *  counter = 0x40 + ticks  (must not exceed 0x7F = 63 ticks max)
 *------------------------------------------------------------------*/
uint8 WWDG_CalcCounter(uint32 timeout_ms, WWDG_Prescaler_t prescaler,
                        uint32 pclk1_hz)
{
    if (pclk1_hz == 0U) return WWDG_COUNTER_MAX;

    uint32 prescaler_div = 1U << (uint32)prescaler;   /* 1, 2, 4, or 8 */

    /* T_tick_us = (4096 * prescaler_div * 1000000) / pclk1_hz  [µs] */
    /* ticks = timeout_ms * 1000 / T_tick_us                          */
    uint32 t_tick_us = (4096U * prescaler_div * 1000000U) / pclk1_hz;
    if (t_tick_us == 0U) return WWDG_COUNTER_MAX;

    uint32 ticks = (timeout_ms * 1000U) / t_tick_us;

    /* Counter = 0x40 + ticks, clamped to 7-bit max (63 ticks above 0x40) */
    if (ticks > 63U) ticks = 63U;

    return (uint8)(WWDG_COUNTER_MIN + ticks);
}
