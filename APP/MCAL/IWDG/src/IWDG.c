#include "IWDG.h"

/*
 * Usage example — 2-second watchdog on a 16 MHz HSI system:
 *
 *   uint16 rl = IWDG_CalcReload(2000, IWDG_PRE_64, 32000);
 *   IWDG_Init(IWDG_PRE_64, rl);
 *
 *   while (1) {
 *       do_work();
 *       IWDG_Refresh();   // must call before 2 s elapses
 *   }
 *
 * Note: No RCC clock enable needed — the IWDG runs from LSI which
 *       is started automatically when IWDG_KEY_START is written.
 */

/*------------------------------------------------------------------
 *  Prescaler divider lookup (index = IWDG_Prescaler_t value)
 *------------------------------------------------------------------*/
static const uint16 IWDG_PrescalerDiv[7] = { 4, 8, 16, 32, 64, 128, 256 };

/*------------------------------------------------------------------
 *  IWDG_Init
 *------------------------------------------------------------------*/
void IWDG_Init(IWDG_Prescaler_t prescaler, uint16 reload)
{
    /* 1. Unlock write access to PR and RLR */
    IWDG_KR = IWDG_KEY_UNLOCK;

    /* 2. Set prescaler — wait for any previous update to finish */
    while (IWDG_SR & IWDG_SR_PVU) {}
    IWDG_PR = (uint32)prescaler & 0x07U;

    /* 3. Set reload — wait for any previous update to finish */
    while (IWDG_SR & IWDG_SR_RVU) {}
    IWDG_RLR = (uint32)reload & 0x0FFFU;

    /* 4. Refresh counter to load the new reload value */
    IWDG_KR = IWDG_KEY_RELOAD;

    /* 5. Start the watchdog (also locks PR and RLR again) */
    IWDG_KR = IWDG_KEY_START;
}

/*------------------------------------------------------------------
 *  IWDG_Refresh
 *------------------------------------------------------------------*/
void IWDG_Refresh(void)
{
    IWDG_KR = IWDG_KEY_RELOAD;
}

/*------------------------------------------------------------------
 *  IWDG_CalcReload
 *------------------------------------------------------------------*/
uint16 IWDG_CalcReload(uint32 timeout_ms, IWDG_Prescaler_t prescaler,
                        uint32 lsi_hz)
{
    if ((uint8)prescaler >= 7U || lsi_hz == 0U) return 0xFFFU;

    uint32 div = (uint32)IWDG_PrescalerDiv[(uint8)prescaler];

    /* reload = (timeout_ms * lsi_hz) / (1000 * div) - 1 */
    uint32 reload = (timeout_ms * (lsi_hz / div)) / 1000U;

    if (reload > 0U) reload -= 1U;
    if (reload > 0x0FFFU) reload = 0x0FFFU;   /* clamp to 12-bit max */

    return (uint16)reload;
}
