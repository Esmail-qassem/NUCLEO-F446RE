/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : NVIC                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 NVIC_BASE_ADDR = 0xE000E100UL;
constexpr uint32 SCB_AIRCR_ADDR = 0xE000ED0CUL;

/* ── Priority grouping presets ────────────────────────────────────── */
enum class NVIC_PriorityGroup : uint8
{
    G16_S0  = 0b011,  /* 16 groups, 0 sub-groups  */
    G8_S2   = 0b100,  /* 8 groups,  2 sub-groups  */
    G4_S4   = 0b101,  /* 4 groups,  4 sub-groups  */
    G2_S8   = 0b110,  /* 2 groups,  8 sub-groups  */
    G0_S16  = 0b111   /* 0 groups, 16 sub-groups  */
};

/* ── Status ───────────────────────────────────────────────────────── */
enum class NVIC_Status : uint8
{
    OK  = 0,
    NOK = 1
};

/* ── NVIC Driver Class ────────────────────────────────────────────── */
class NVIC
{
public:
    static NVIC_Status EnableInterrupt  (uint8 irqNum);
    static NVIC_Status DisableInterrupt (uint8 irqNum);
    static NVIC_Status SetPendingFlag   (uint8 irqNum);
    static NVIC_Status ClearPendingFlag (uint8 irqNum);
    static NVIC_Status GetActiveFlag    (uint8 irqNum, uint8 &out);
    static void        SetPriority      (sint8 irqId, uint8 group, uint8 sub);

private:
    NVIC() = delete;
};
