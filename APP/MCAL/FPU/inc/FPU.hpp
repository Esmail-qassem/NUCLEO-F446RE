/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : FPU                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  Coprocessor Access Control Register
 *------------------------------------------------------------------*/
constexpr uint32 FPU_CPACR_ADDR     = 0xE000ED88UL;
constexpr uint32 FPU_FPCCR_ADDR     = 0xE000EF34UL;
constexpr uint32 FPU_FPCAR_ADDR     = 0xE000EF38UL;
constexpr uint32 FPU_FPDSCR_ADDR    = 0xE000EF3CUL;

#define FPU_CPACR   (*reinterpret_cast<volatile uint32*>(FPU_CPACR_ADDR))
#define FPU_FPCCR   (*reinterpret_cast<volatile uint32*>(FPU_FPCCR_ADDR))
#define FPU_FPCAR   (*reinterpret_cast<volatile uint32*>(FPU_FPCAR_ADDR))
#define FPU_FPDSCR  (*reinterpret_cast<volatile uint32*>(FPU_FPDSCR_ADDR))

constexpr uint8  FPU_CPACR_CP10_Pos = 20U;
constexpr uint8  FPU_CPACR_CP11_Pos = 22U;
constexpr uint8  FPU_ACCESS_DENIED     = 0U;
constexpr uint8  FPU_ACCESS_PRIVILEGED = 1U;
constexpr uint8  FPU_ACCESS_FULL       = 3U;

constexpr uint32 FPU_FPCCR_ASPEN = (1U << 31U);
constexpr uint32 FPU_FPCCR_LSPEN = (1U << 30U);

constexpr uint32 FPU_FPSCR_IOC      = (1U <<  0U);
constexpr uint32 FPU_FPSCR_DZC      = (1U <<  1U);
constexpr uint32 FPU_FPSCR_OFC      = (1U <<  2U);
constexpr uint32 FPU_FPSCR_UFC      = (1U <<  3U);
constexpr uint32 FPU_FPSCR_IXC      = (1U <<  4U);
constexpr uint32 FPU_FPSCR_IDC      = (1U <<  7U);
constexpr uint32 FPU_FPSCR_FZ       = (1U << 24U);
constexpr uint32 FPU_FPSCR_DN       = (1U << 25U);
constexpr uint8  FPU_FPSCR_RMode_Pos = 22U;
constexpr uint32 FPU_EX_ALL         = 0x9FU;

/* ── Enumerations ─────────────────────────────────────────────────── */
enum class FPU_RoundMode : uint8
{
    NEAREST   = 0,
    PLUS_INF  = 1,
    MINUS_INF = 2,
    ZERO      = 3
};

enum class FPU_Exception : uint32
{
    INVALID  = 1U <<  0U,
    DIVZERO  = 1U <<  1U,
    OVERFLOW = 1U <<  2U,
    UNDERFLOW= 1U <<  3U,
    INEXACT  = 1U <<  4U,
    DENORMAL = 1U <<  7U,
    ALL      = 0x9FU
};

/* ── FPU Driver Class ─────────────────────────────────────────────── */
class FPU
{
public:
    static void         Enable              (void);
    static void         Disable             (void);
    static void         SetAccess           (uint8 level);
    static void         EnableLazyStacking  (void);
    static void         DisableLazyStacking (void);
    static void         SetRoundMode        (FPU_RoundMode mode);
    static FPU_RoundMode GetRoundMode       (void);
    static void         SetFlushToZero      (uint8 enable);
    static void         SetDefaultNaN       (uint8 enable);
    static uint32       GetExceptions       (void);
    static void         ClearExceptions     (uint32 mask);
    static uint32       GetFPSCR            (void);
    static void         SetFPSCR            (uint32 value);

private:
    FPU() = delete;
};
