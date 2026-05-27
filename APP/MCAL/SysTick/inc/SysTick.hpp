/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : SysTick                                                */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Base address ─────────────────────────────────────────────────── */
constexpr uint32 SYSTICK_BASE_ADDR = 0xE000E010UL;

/* ── Register layout ──────────────────────────────────────────────── */
struct SysTick_RegDef_t
{
    volatile uint32 CTRL;   /* 0x00 */
    volatile uint32 LOAD;   /* 0x04 */
    volatile uint32 VAL;    /* 0x08 */
    volatile uint32 CALIB;  /* 0x0C */
};

#define SYSTICK_REGS (reinterpret_cast<SysTick_RegDef_t*>(SYSTICK_BASE_ADDR))

/* ── Clock source options ─────────────────────────────────────────── */
constexpr uint8 SYSTICK_CLK_AHB_DIV8 = 0u;
constexpr uint8 SYSTICK_CLK_AHB      = 1u;

/* ── Config ───────────────────────────────────────────────────────── */
constexpr uint32 SYSTICK_SYSTEM_CLOCK_HZ = 16000000UL;
constexpr uint8  SYSTICK_CLK_SOURCE      = SYSTICK_CLK_AHB_DIV8;
constexpr uint32 SYSTICK_CLOCK           = (SYSTICK_CLK_SOURCE == SYSTICK_CLK_AHB)
                                              ? SYSTICK_SYSTEM_CLOCK_HZ
                                              : (SYSTICK_SYSTEM_CLOCK_HZ / 8UL);
constexpr uint32 TICKS_PER_MS            = SYSTICK_CLOCK / 1000UL;

/* ── ISR ──────────────────────────────────────────────────────────── */
extern "C" {
    void SysTick_Handler(void);
}

/* ── SysTick Driver Class ─────────────────────────────────────────── */
class SysTick
{
public:
    static void     Init                (void);
    static void     BusyWait            (uint32 ms);
    static Status_t SetIntervalSingle   (uint32 ticks, void(*fn)(void));
    static Status_t SetIntervalPeriodic (uint32 ticks, void(*fn)(void));
    static void     StopTimer           (void);
    static uint32   GetElapsedTime      (void);
    static uint32   GetRemainingTime    (void);

private:
    SysTick() = delete;
};
