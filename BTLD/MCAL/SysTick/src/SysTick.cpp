/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : SysTick                                                */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "SysTick.hpp"

static void (*s_callback)(void) = nullptr;

void SysTick::Init(void)
{
    if (SYSTICK_CLK_SOURCE == SYSTICK_CLK_AHB) SET_BIT(SYSTICK_REGS->CTRL, 2);
    else CLEAR_BIT(SYSTICK_REGS->CTRL, 2);
}

void SysTick::BusyWait(uint32 ms)
{
    SYSTICK_REGS->VAL = 0u; SYSTICK_REGS->LOAD = ms * TICKS_PER_MS;
    SET_BIT(SYSTICK_REGS->CTRL, 0);
    while (!GET_BIT(SYSTICK_REGS->CTRL, 16)) {}
    StopTimer();
}

Status_t SysTick::SetIntervalSingle(uint32 ticks, void(*fn)(void))
{
    if (!fn) return E_Null_Pointer;
    s_callback = fn; SYSTICK_REGS->LOAD = ticks;
    SET_BIT(SYSTICK_REGS->CTRL, 1); SET_BIT(SYSTICK_REGS->CTRL, 0);
    return E_Ok;
}

Status_t SysTick::SetIntervalPeriodic(uint32 ticks, void(*fn)(void))
{
    if (!fn) return E_Null_Pointer;
    s_callback = fn; SYSTICK_REGS->LOAD = ticks - 1u; SYSTICK_REGS->VAL = 0u;
    SET_BIT(SYSTICK_REGS->CTRL, 1); SET_BIT(SYSTICK_REGS->CTRL, 0);
    return E_Ok;
}

void   SysTick::StopTimer(void)      { CLEAR_BIT(SYSTICK_REGS->CTRL, 0); }
uint32 SysTick::GetElapsedTime(void) { return SYSTICK_REGS->LOAD - SYSTICK_REGS->VAL; }
uint32 SysTick::GetRemainingTime(void){ return SYSTICK_REGS->VAL; }

extern "C" void SysTick_Handler(void) { if (s_callback) s_callback(); }
