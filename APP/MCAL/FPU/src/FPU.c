#include "FPU.h"

/*
 * Usage — call once before any float operation, typically in SystemInit():
 *
 *   FPU_Enable();                           // full access, lazy stacking
 *   FPU_SetRoundMode(FPU_ROUND_NEAREST);    // IEEE-754 default
 *   FPU_SetFlushToZero(0);                  // strict IEEE-754
 *   FPU_SetDefaultNaN(1);                   // simplify NaN checks
 *
 * Using float in RTOS tasks:
 *   The RTOS context switch must save/restore FP registers.
 *   With lazy stacking enabled the hardware does this automatically
 *   when an ISR uses FP instructions — no extra RTOS code needed.
 *
 * Checking for FP exceptions after a calculation:
 *   FPU_ClearExceptions(FPU_EX_ALL);
 *   float result = a / b;
 *   if (FPU_GetExceptions() & FPU_EX_DIVZERO) { handle_divzero(); }
 */

/*------------------------------------------------------------------
 *  VMRS / VMSR wrappers to read/write FPSCR
 *  (FPSCR is a special register — not memory-mapped)
 *------------------------------------------------------------------*/
uint32 FPU_GetFPSCR(void)
{
    uint32 val;
    __asm volatile ("VMRS %0, FPSCR" : "=r"(val));
    return val;
}

void FPU_SetFPSCR(uint32 value)
{
    __asm volatile ("VMSR FPSCR, %0" :: "r"(value));
}

/*------------------------------------------------------------------
 *  FPU_Enable
 *------------------------------------------------------------------*/
void FPU_Enable(void)
{
    /* Grant full access to CP10 and CP11 (both must be set identically) */
    FPU_CPACR |= ((uint32)FPU_ACCESS_FULL << FPU_CPACR_CP10_Pos)
              |  ((uint32)FPU_ACCESS_FULL << FPU_CPACR_CP11_Pos);

    /* DSB + ISB to ensure the CPACR write takes effect before the
       first FP instruction executes.                              */
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    /* Enable automatic + lazy FP context preservation (recommended) */
    FPU_FPCCR |= (FPU_FPCCR_ASPEN | FPU_FPCCR_LSPEN);
}

/*------------------------------------------------------------------
 *  FPU_Disable
 *------------------------------------------------------------------*/
void FPU_Disable(void)
{
    FPU_CPACR &= ~(((uint32)3U << FPU_CPACR_CP10_Pos)
                |  ((uint32)3U << FPU_CPACR_CP11_Pos));
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}

/*------------------------------------------------------------------
 *  FPU_SetAccess
 *------------------------------------------------------------------*/
void FPU_SetAccess(uint8 level)
{
    uint32 mask = ((uint32)3U << FPU_CPACR_CP10_Pos)
                | ((uint32)3U << FPU_CPACR_CP11_Pos);
    uint32 val  = ((uint32)(level & 0x3U) << FPU_CPACR_CP10_Pos)
                | ((uint32)(level & 0x3U) << FPU_CPACR_CP11_Pos);
    FPU_CPACR = (FPU_CPACR & ~mask) | val;
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");
}

/*------------------------------------------------------------------
 *  FPU_EnableLazyStacking / FPU_DisableLazyStacking
 *------------------------------------------------------------------*/
void FPU_EnableLazyStacking(void)
{
    FPU_FPCCR |= (FPU_FPCCR_ASPEN | FPU_FPCCR_LSPEN);
}

void FPU_DisableLazyStacking(void)
{
    /* ASPEN=1, LSPEN=0 → always save FP context on exception entry */
    FPU_FPCCR |=  FPU_FPCCR_ASPEN;
    FPU_FPCCR &= ~FPU_FPCCR_LSPEN;
}

/*------------------------------------------------------------------
 *  FPU_SetRoundMode
 *------------------------------------------------------------------*/
void FPU_SetRoundMode(FPU_RoundMode_t mode)
{
    uint32 fpscr = FPU_GetFPSCR();
    fpscr &= ~(3U << FPU_FPSCR_RMode_Pos);
    fpscr |=  ((uint32)mode & 0x3U) << FPU_FPSCR_RMode_Pos;
    FPU_SetFPSCR(fpscr);
}

FPU_RoundMode_t FPU_GetRoundMode(void)
{
    return (FPU_RoundMode_t)((FPU_GetFPSCR() >> FPU_FPSCR_RMode_Pos) & 0x3U);
}

/*------------------------------------------------------------------
 *  FPU_SetFlushToZero
 *------------------------------------------------------------------*/
void FPU_SetFlushToZero(uint8 enable)
{
    uint32 fpscr = FPU_GetFPSCR();
    if (enable)
        fpscr |=  FPU_FPSCR_FZ;
    else
        fpscr &= ~FPU_FPSCR_FZ;
    FPU_SetFPSCR(fpscr);
}

/*------------------------------------------------------------------
 *  FPU_SetDefaultNaN
 *------------------------------------------------------------------*/
void FPU_SetDefaultNaN(uint8 enable)
{
    uint32 fpscr = FPU_GetFPSCR();
    if (enable)
        fpscr |=  FPU_FPSCR_DN;
    else
        fpscr &= ~FPU_FPSCR_DN;
    FPU_SetFPSCR(fpscr);
}

/*------------------------------------------------------------------
 *  Exception flags
 *------------------------------------------------------------------*/
uint32 FPU_GetExceptions(void)
{
    return FPU_GetFPSCR() & (uint32)FPU_EX_ALL;
}

void FPU_ClearExceptions(uint32 mask)
{
    uint32 fpscr = FPU_GetFPSCR();
    fpscr &= ~(mask & (uint32)FPU_EX_ALL);
    FPU_SetFPSCR(fpscr);
}
