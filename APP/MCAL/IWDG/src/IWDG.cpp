/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : IWDG                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "IWDG.hpp"

/* ── Prescaler divider lookup ─────────────────────────────────────── */
static constexpr uint16 IWDG_PrescalerDiv[7] = { 4U, 8U, 16U, 32U, 64U, 128U, 256U };

/* ── Init ─────────────────────────────────────────────────────────── */
void IWDG::Init(IWDG_Prescaler prescaler, uint16 reload)
{
    IWDG_KR = static_cast<uint32>(IWDG_KEY_UNLOCK);

    while (IWDG_SR & IWDG_SR_PVU) {}
    IWDG_PR = static_cast<uint32>(prescaler) & 0x07U;

    while (IWDG_SR & IWDG_SR_RVU) {}
    IWDG_RLR = static_cast<uint32>(reload) & 0x0FFFU;

    IWDG_KR = static_cast<uint32>(IWDG_KEY_RELOAD);
    IWDG_KR = static_cast<uint32>(IWDG_KEY_START);
}

/* ── Refresh ──────────────────────────────────────────────────────── */
void IWDG::Refresh(void)
{
    IWDG_KR = static_cast<uint32>(IWDG_KEY_RELOAD);
}

/* ── CalcReload ───────────────────────────────────────────────────── */
uint16 IWDG::CalcReload(uint32 timeout_ms, IWDG_Prescaler prescaler, uint32 lsi_hz)
{
    if (static_cast<uint8>(prescaler) >= 7U || lsi_hz == 0U) return 0xFFFU;

    uint32 div    = static_cast<uint32>(IWDG_PrescalerDiv[static_cast<uint8>(prescaler)]);
    uint32 reload = (timeout_ms * (lsi_hz / div)) / 1000U;

    if (reload > 0U) reload -= 1U;
    if (reload > 0x0FFFU) reload = 0x0FFFU;

    return static_cast<uint16>(reload);
}
