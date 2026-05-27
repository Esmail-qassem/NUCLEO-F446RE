/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : WWDG                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "WWDG.hpp"

/* ── Init ─────────────────────────────────────────────────────────── */
void WWDG::Init(const WWDG_Config_t &config)
{
    uint32 cfr = 0U;
    cfr |= (static_cast<uint32>(config.prescaler) << WWDG_CFR_WDGTB_Pos) & WWDG_CFR_WDGTB_Mask;
    cfr |= static_cast<uint32>(config.window) & WWDG_CFR_W_Mask;
    if (config.ewi_enable) cfr |= WWDG_CFR_EWI;
    WWDG_CFR = cfr;

    WWDG_CR = WWDG_CR_WDGA | (static_cast<uint32>(config.counter) & WWDG_CR_T_Mask);
}

/* ── Refresh ──────────────────────────────────────────────────────── */
void WWDG::Refresh(uint8 counter)
{
    WWDG_CR = WWDG_CR_WDGA | (static_cast<uint32>(counter) & WWDG_CR_T_Mask);
}

/* ── ClearFlag ────────────────────────────────────────────────────── */
void WWDG::ClearFlag(void)
{
    WWDG_SR &= ~WWDG_SR_EWIF;
}

/* ── GetFlag ──────────────────────────────────────────────────────── */
uint8 WWDG::GetFlag(void)
{
    return (WWDG_SR & WWDG_SR_EWIF) ? 1U : 0U;
}

/* ── CalcCounter ──────────────────────────────────────────────────── */
uint8 WWDG::CalcCounter(uint32 timeout_ms, WWDG_Prescaler prescaler, uint32 pclk1_hz)
{
    if (pclk1_hz == 0U) return WWDG_COUNTER_MAX;

    uint32 prescaler_div = 1U << static_cast<uint32>(prescaler);
    uint32 t_tick_us = (4096U * prescaler_div * 1000000U) / pclk1_hz;
    if (t_tick_us == 0U) return WWDG_COUNTER_MAX;

    uint32 ticks = (timeout_ms * 1000U) / t_tick_us;
    if (ticks > 63U) ticks = 63U;

    return static_cast<uint8>(WWDG_COUNTER_MIN + ticks);
}
