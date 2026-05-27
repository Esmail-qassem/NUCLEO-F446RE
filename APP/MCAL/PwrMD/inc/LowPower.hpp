/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : LowPower (PwrMD)                                       */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── PWR registers ────────────────────────────────────────────────── */
constexpr uint32 PWR_BASE_ADDR = 0x40007000UL;

#define PWR_CR   (*reinterpret_cast<volatile uint32*>(PWR_BASE_ADDR + 0x00U))
#define PWR_CSR  (*reinterpret_cast<volatile uint32*>(PWR_BASE_ADDR + 0x04U))

/* ── PWR_CR bit masks ─────────────────────────────────────────────── */
constexpr uint32 PWR_CR_LPDS   = (1U <<  0U);
constexpr uint32 PWR_CR_PDDS   = (1U <<  1U);
constexpr uint32 PWR_CR_CWUF   = (1U <<  2U);
constexpr uint32 PWR_CR_CSBF   = (1U <<  3U);
constexpr uint32 PWR_CR_PVDE   = (1U <<  4U);
constexpr uint8  PWR_CR_PLS_Pos = 5U;
constexpr uint32 PWR_CR_DBP    = (1U <<  8U);
constexpr uint32 PWR_CR_FPDS   = (1U <<  9U);
constexpr uint32 PWR_CR_ODEN   = (1U << 16U);
constexpr uint32 PWR_CR_ODSWEN = (1U << 17U);

/* ── PWR_CSR bit masks ────────────────────────────────────────────── */
constexpr uint32 PWR_CSR_WUF   = (1U <<  0U);
constexpr uint32 PWR_CSR_SBF   = (1U <<  1U);
constexpr uint32 PWR_CSR_PVDO  = (1U <<  2U);
constexpr uint32 PWR_CSR_BRR   = (1U <<  3U);
constexpr uint32 PWR_CSR_EWUP1 = (1U <<  8U);
constexpr uint32 PWR_CSR_EWUP2 = (1U <<  9U);
constexpr uint32 PWR_CSR_BRE   = (1U << 10U);

/* ── SCB System Control Register ─────────────────────────────────── */
constexpr uint32 SCB_SCR_ADDR = 0xE000ED10UL;
#define SCB_SCR  (*reinterpret_cast<volatile uint32*>(SCB_SCR_ADDR))
constexpr uint32 SCB_SCR_SLEEPONEXIT = (1U << 1U);
constexpr uint32 SCB_SCR_SLEEPDEEP  = (1U << 2U);
constexpr uint32 SCB_SCR_SEVONPEND  = (1U << 4U);

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class LP_Entry : uint8
{
    WFI = 0,
    WFE = 1
};

enum class LP_Regulator : uint8
{
    ON         = 0,
    LOW_POWER  = 1
};

enum class LP_PVDLevel : uint8
{
    PVD_2V0 = 0,
    PVD_2V1 = 1,
    PVD_2V3 = 2,
    PVD_2V5 = 3,
    PVD_2V6 = 4,
    PVD_2V7 = 5,
    PVD_2V8 = 6,
    PVD_2V9 = 7
};

/* ── LowPower Driver Class ────────────────────────────────────────── */
class LowPower
{
public:
    static void  EnterSleep          (LP_Entry entry);
    static void  EnableSleepOnExit   (void);
    static void  DisableSleepOnExit  (void);

    static void  EnterStop           (LP_Regulator regulator, uint8 flash_pd, LP_Entry entry);

    static void  EnterStandby        (void);
    static void  EnableWakeupPin     (uint8 pin);
    static void  DisableWakeupPin    (uint8 pin);

    static uint8 IsWakeFromStandby   (void);
    static uint8 IsWakeupPinEvent    (void);
    static void  ClearFlags          (void);

    static void  EnablePVD           (LP_PVDLevel level);
    static void  DisablePVD          (void);
    static uint8 GetPVDOutput        (void);

private:
    LowPower() = delete;
};
