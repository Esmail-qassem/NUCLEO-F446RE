/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : IWDG                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Register base ────────────────────────────────────────────────── */
constexpr uint32 IWDG_BASE_ADDR = 0x40003000UL;

#define IWDG_KR   (*reinterpret_cast<volatile uint32*>(IWDG_BASE_ADDR + 0x00U))
#define IWDG_PR   (*reinterpret_cast<volatile uint32*>(IWDG_BASE_ADDR + 0x04U))
#define IWDG_RLR  (*reinterpret_cast<volatile uint32*>(IWDG_BASE_ADDR + 0x08U))
#define IWDG_SR   (*reinterpret_cast<volatile uint32*>(IWDG_BASE_ADDR + 0x0CU))

/* ── Key register magic values ────────────────────────────────────── */
constexpr uint16 IWDG_KEY_RELOAD = 0xAAAAU;
constexpr uint16 IWDG_KEY_START  = 0xCCCCU;
constexpr uint16 IWDG_KEY_UNLOCK = 0x5555U;

/* ── Status register bits ─────────────────────────────────────────── */
constexpr uint32 IWDG_SR_PVU = (1U << 0U);
constexpr uint32 IWDG_SR_RVU = (1U << 1U);

/* ── Prescaler enumeration ────────────────────────────────────────── */
enum class IWDG_Prescaler : uint8
{
    PRE_4   = 0,
    PRE_8   = 1,
    PRE_16  = 2,
    PRE_32  = 3,
    PRE_64  = 4,
    PRE_128 = 5,
    PRE_256 = 6
};

/* ── IWDG Driver Class ────────────────────────────────────────────── */
class IWDG
{
public:
    static void   Init        (IWDG_Prescaler prescaler, uint16 reload);
    static void   Refresh     (void);
    static uint16 CalcReload  (uint32 timeout_ms, IWDG_Prescaler prescaler, uint32 lsi_hz);

private:
    IWDG() = delete;
};
