/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : WWDG                                                   */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Register base ────────────────────────────────────────────────── */
constexpr uint32 WWDG_BASE_ADDR = 0x40002C00UL;

#define WWDG_CR   (*reinterpret_cast<volatile uint32*>(WWDG_BASE_ADDR + 0x00U))
#define WWDG_CFR  (*reinterpret_cast<volatile uint32*>(WWDG_BASE_ADDR + 0x04U))
#define WWDG_SR   (*reinterpret_cast<volatile uint32*>(WWDG_BASE_ADDR + 0x08U))

/* ── CR bit masks ─────────────────────────────────────────────────── */
constexpr uint32 WWDG_CR_T_Mask  = 0x7FU;
constexpr uint32 WWDG_CR_WDGA   = (1U << 7U);

/* ── CFR bit masks ────────────────────────────────────────────────── */
constexpr uint32 WWDG_CFR_W_Mask        = 0x7FU;
constexpr uint8  WWDG_CFR_WDGTB_Pos    = 7U;
constexpr uint32 WWDG_CFR_WDGTB_Mask   = (3U << 7U);
constexpr uint32 WWDG_CFR_EWI          = (1U << 9U);

/* ── SR bit masks ─────────────────────────────────────────────────── */
constexpr uint32 WWDG_SR_EWIF = (1U << 0U);

/* ── Counter limits ───────────────────────────────────────────────── */
constexpr uint8 WWDG_COUNTER_MIN = 0x40U;
constexpr uint8 WWDG_COUNTER_MAX = 0x7FU;

/* ── Prescaler enumeration ────────────────────────────────────────── */
enum class WWDG_Prescaler : uint8
{
    PRE_1 = 0,
    PRE_2 = 1,
    PRE_4 = 2,
    PRE_8 = 3
};

/* ── Configuration structure ──────────────────────────────────────── */
struct WWDG_Config_t
{
    WWDG_Prescaler prescaler;
    uint8          window;
    uint8          counter;
    uint8          ewi_enable;
};

/* ── WWDG Driver Class ────────────────────────────────────────────── */
class WWDG
{
public:
    static void  Init        (const WWDG_Config_t &config);
    static void  Refresh     (uint8 counter);
    static void  ClearFlag   (void);
    static uint8 GetFlag     (void);
    static uint8 CalcCounter (uint32 timeout_ms, WWDG_Prescaler prescaler, uint32 pclk1_hz);

private:
    WWDG() = delete;
};
