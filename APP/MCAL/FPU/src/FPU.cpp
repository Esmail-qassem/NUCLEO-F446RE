/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : FPU                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "FPU.hpp"

/* ── GetFPSCR / SetFPSCR ──────────────────────────────────────────── */
uint32 FPU::GetFPSCR(void)
{
    uint32 val;
    __asm volatile ("VMRS %0, FPSCR" : "=r"(val));
    return val;
}

void FPU::SetFPSCR(uint32 value)
{
    __asm volatile ("VMSR FPSCR, %0" :: "r"(value));
}

/* ── Enable ───────────────────────────────────────────────────────── */
void FPU::Enable(void)
{
    FPU_CPACR |= (static_cast<uint32>(FPU_ACCESS_FULL) << FPU_CPACR_CP10_Pos)
              |  (static_cast<uint32>(FPU_ACCESS_FULL) << FPU_CPACR_CP11_Pos);
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
    FPU_FPCCR |= (FPU_FPCCR_ASPEN | FPU_FPCCR_LSPEN);
}

/* ── Disable ──────────────────────────────────────────────────────── */
void FPU::Disable(void)
{
    FPU_CPACR &= ~((static_cast<uint32>(3U) << FPU_CPACR_CP10_Pos)
               |   (static_cast<uint32>(3U) << FPU_CPACR_CP11_Pos));
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}

/* ── SetAccess ────────────────────────────────────────────────────── */
void FPU::SetAccess(uint8 level)
{
    uint32 mask = (static_cast<uint32>(3U) << FPU_CPACR_CP10_Pos)
                | (static_cast<uint32>(3U) << FPU_CPACR_CP11_Pos);
    uint32 val  = (static_cast<uint32>(level & 0x3U) << FPU_CPACR_CP10_Pos)
                | (static_cast<uint32>(level & 0x3U) << FPU_CPACR_CP11_Pos);
    FPU_CPACR = (FPU_CPACR & ~mask) | val;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}

/* ── EnableLazyStacking ───────────────────────────────────────────── */
void FPU::EnableLazyStacking(void)
{
    FPU_FPCCR |= (FPU_FPCCR_ASPEN | FPU_FPCCR_LSPEN);
}

/* ── DisableLazyStacking ──────────────────────────────────────────── */
void FPU::DisableLazyStacking(void)
{
    FPU_FPCCR |=  FPU_FPCCR_ASPEN;
    FPU_FPCCR &= ~FPU_FPCCR_LSPEN;
}

/* ── SetRoundMode ─────────────────────────────────────────────────── */
void FPU::SetRoundMode(FPU_RoundMode mode)
{
    uint32 fpscr = GetFPSCR();
    fpscr &= ~(static_cast<uint32>(3U) << FPU_FPSCR_RMode_Pos);
    fpscr |=  (static_cast<uint32>(mode) & 0x3U) << FPU_FPSCR_RMode_Pos;
    SetFPSCR(fpscr);
}

/* ── GetRoundMode ─────────────────────────────────────────────────── */
FPU_RoundMode FPU::GetRoundMode(void)
{
    return static_cast<FPU_RoundMode>((GetFPSCR() >> FPU_FPSCR_RMode_Pos) & 0x3U);
}

/* ── SetFlushToZero ───────────────────────────────────────────────── */
void FPU::SetFlushToZero(uint8 enable)
{
    uint32 fpscr = GetFPSCR();
    if (enable)
        fpscr |=  FPU_FPSCR_FZ;
    else
        fpscr &= ~FPU_FPSCR_FZ;
    SetFPSCR(fpscr);
}

/* ── SetDefaultNaN ────────────────────────────────────────────────── */
void FPU::SetDefaultNaN(uint8 enable)
{
    uint32 fpscr = GetFPSCR();
    if (enable)
        fpscr |=  FPU_FPSCR_DN;
    else
        fpscr &= ~FPU_FPSCR_DN;
    SetFPSCR(fpscr);
}

/* ── GetExceptions ────────────────────────────────────────────────── */
uint32 FPU::GetExceptions(void)
{
    return GetFPSCR() & FPU_EX_ALL;
}

/* ── ClearExceptions ──────────────────────────────────────────────── */
void FPU::ClearExceptions(uint32 mask)
{
    uint32 fpscr = GetFPSCR();
    fpscr &= ~(mask & FPU_EX_ALL);
    SetFPSCR(fpscr);
}
