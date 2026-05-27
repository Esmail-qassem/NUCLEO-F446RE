/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : LowPower (PwrMD)                                       */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "LowPower.hpp"

/* ── Internal WFI / WFE ──────────────────────────────────────────── */
static inline void exec_wfi(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("wfi"     ::: "memory");
}

static inline void exec_wfe(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("wfe"     ::: "memory");
}

/* ── EnterSleep ───────────────────────────────────────────────────── */
void LowPower::EnterSleep(LP_Entry entry)
{
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    if (entry == LP_Entry::WFI)
        exec_wfi();
    else
        exec_wfe();
}

/* ── EnableSleepOnExit ────────────────────────────────────────────── */
void LowPower::EnableSleepOnExit(void)
{
    SCB_SCR |= SCB_SCR_SLEEPONEXIT;
}

/* ── DisableSleepOnExit ───────────────────────────────────────────── */
void LowPower::DisableSleepOnExit(void)
{
    SCB_SCR &= ~SCB_SCR_SLEEPONEXIT;
}

/* ── EnterStop ────────────────────────────────────────────────────── */
void LowPower::EnterStop(LP_Regulator regulator, uint8 flash_pd, LP_Entry entry)
{
    PWR_CR &= ~PWR_CR_PDDS;

    if (regulator == LP_Regulator::LOW_POWER)
        PWR_CR |=  PWR_CR_LPDS;
    else
        PWR_CR &= ~PWR_CR_LPDS;

    if (flash_pd)
        PWR_CR |=  PWR_CR_FPDS;
    else
        PWR_CR &= ~PWR_CR_FPDS;

    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    if (entry == LP_Entry::WFI)
        exec_wfi();
    else
        exec_wfe();

    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
}

/* ── EnterStandby ─────────────────────────────────────────────────── */
void LowPower::EnterStandby(void)
{
    PWR_CR |= PWR_CR_PDDS;
    PWR_CR |= (PWR_CR_CWUF | PWR_CR_CSBF);
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    exec_wfi();
    while (1) {}
}

/* ── EnableWakeupPin ──────────────────────────────────────────────── */
void LowPower::EnableWakeupPin(uint8 pin)
{
    if (pin == 1U)
        PWR_CSR |= PWR_CSR_EWUP1;
    else if (pin == 2U)
        PWR_CSR |= PWR_CSR_EWUP2;
}

/* ── DisableWakeupPin ─────────────────────────────────────────────── */
void LowPower::DisableWakeupPin(uint8 pin)
{
    if (pin == 1U)
        PWR_CSR &= ~PWR_CSR_EWUP1;
    else if (pin == 2U)
        PWR_CSR &= ~PWR_CSR_EWUP2;
}

/* ── IsWakeFromStandby ────────────────────────────────────────────── */
uint8 LowPower::IsWakeFromStandby(void)
{
    return (PWR_CSR & PWR_CSR_SBF) ? 1U : 0U;
}

/* ── IsWakeupPinEvent ─────────────────────────────────────────────── */
uint8 LowPower::IsWakeupPinEvent(void)
{
    return (PWR_CSR & PWR_CSR_WUF) ? 1U : 0U;
}

/* ── ClearFlags ───────────────────────────────────────────────────── */
void LowPower::ClearFlags(void)
{
    PWR_CR |= (PWR_CR_CWUF | PWR_CR_CSBF);
}

/* ── EnablePVD ────────────────────────────────────────────────────── */
void LowPower::EnablePVD(LP_PVDLevel level)
{
    uint32 cr = PWR_CR;
    cr &= ~(static_cast<uint32>(7U) << PWR_CR_PLS_Pos);
    cr |=  (static_cast<uint32>(level) << PWR_CR_PLS_Pos);
    cr |=  PWR_CR_PVDE;
    PWR_CR = cr;
}

/* ── DisablePVD ───────────────────────────────────────────────────── */
void LowPower::DisablePVD(void)
{
    PWR_CR &= ~PWR_CR_PVDE;
}

/* ── GetPVDOutput ─────────────────────────────────────────────────── */
uint8 LowPower::GetPVDOutput(void)
{
    return (PWR_CSR & PWR_CSR_PVDO) ? 1U : 0U;
}
