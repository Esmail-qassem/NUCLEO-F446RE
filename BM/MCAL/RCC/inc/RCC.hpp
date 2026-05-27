/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : RCC                                                    */
/* Version    : V3.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "BIT_MATH.h"

/* ── Hardware Register Map ────────────────────────────────────────── */
struct RCC_RegDef_t
{
    volatile uint32 CR;          /* 0x00 */
    volatile uint32 PLLCFGR;    /* 0x04 */
    volatile uint32 CFGR;       /* 0x08 */
    volatile uint32 CIR;        /* 0x0C */
    volatile uint32 AHB1RSTR;   /* 0x10 */
    volatile uint32 AHB2RSTR;   /* 0x14 */
    volatile uint32 AHB3RSTR;   /* 0x18 */
    uint32          _res0;       /* 0x1C */
    volatile uint32 APB1RSTR;   /* 0x20 */
    volatile uint32 APB2RSTR;   /* 0x24 */
    uint32          _res1[2];   /* 0x28-0x2C */
    volatile uint32 AHB1ENR;    /* 0x30 */
    volatile uint32 AHB2ENR;    /* 0x34 */
    volatile uint32 AHB3ENR;    /* 0x38 */
    uint32          _res2;       /* 0x3C */
    volatile uint32 APB1ENR;    /* 0x40 */
    volatile uint32 APB2ENR;    /* 0x44 */
};

constexpr uint32 RCC_BASE_ADDR = 0x40023800UL;
#define RCC_REGS (reinterpret_cast<RCC_RegDef_t*>(RCC_BASE_ADDR))

/* ── RCC_CSR bit positions (reset cause detection) ────────────────── */
constexpr uint8 RCC_CSR_LSION    =  0;
constexpr uint8 RCC_CSR_LSIRDY   =  1;
constexpr uint8 RCC_CSR_RMVF     = 24;
constexpr uint8 RCC_CSR_BOR_RSTF = 25;
constexpr uint8 RCC_CSR_PIN_RSTF = 26;
constexpr uint8 RCC_CSR_POR_RSTF = 27;
constexpr uint8 RCC_CSR_SFT_RSTF = 28;
constexpr uint8 RCC_CSR_IWDGRSTF = 29;
constexpr uint8 RCC_CSR_WWDGRSTF = 30;
constexpr uint8 RCC_CSR_LPWRRSTF = 31;

/* ── Peripheral bit-position IDs (same value = same bit, diff bus) ── */
enum class RCC_Peripheral : uint8
{
    /* AHB1 – pass with RCC_Bus::AHB1 */
    GPIOA =  0, GPIOB  =  1, GPIOC  =  2, GPIOD  =  3,
    GPIOE =  4, GPIOF  =  5, GPIOG  =  6, GPIOH  =  7,
    CRC   = 12, DMA1   = 21, DMA2   = 22,

    /* AHB2 – pass with RCC_Bus::AHB2 */
    OTGFS =  7,

    /* APB1 – pass with RCC_Bus::APB1 */
    TIM2  =  0, TIM3   =  1, TIM4   =  2, TIM5   =  3,
    TIM6  =  4, TIM7   =  5,
    WWDG  = 11,
    SPI2  = 14, SPI3   = 15,
    USART2= 17, USART3 = 18, USART4 = 19, USART5 = 20,
    I2C1  = 21, I2C2   = 22, I2C3   = 23,
    PWR   = 28,

    /* APB2 – pass with RCC_Bus::APB2 */
    TIM1  =  0,
    USART1=  4, USART6 =  5,
    ADC1  =  8,
    SDIO  = 11, SPI1   = 12, SPI4   = 13,
    SYSCFG= 14,
    TIM9  = 16, TIM10  = 17, TIM11  = 18
};

/* ── Bus ──────────────────────────────────────────────────────────── */
enum class RCC_Bus : uint8
{
    AHB1, AHB2, AHB3, APB1, APB2
};

/* ── Clock source ─────────────────────────────────────────────────── */
enum class RCC_ClockSrc : uint8
{
    HSI = 0x00,
    HSE = 0x01,
    PLL = 0x02
};

enum class RCC_PLLSrc : uint8
{
    HSI = 0,
    HSE = 1
};

/* ── Prescalers ───────────────────────────────────────────────────── */
enum class RCC_AHBPre : uint8
{
    DIV1   = 0x0, DIV2   = 0x8, DIV4   = 0x9,
    DIV8   = 0xA, DIV16  = 0xB, DIV64  = 0xC,
    DIV128 = 0xD, DIV256 = 0xE, DIV512 = 0xF
};

enum class RCC_APBPre : uint8
{
    DIV1  = 0x0,
    DIV2  = 0x4,
    DIV4  = 0x5,
    DIV8  = 0x6,
    DIV16 = 0x7
};

/* ── Config structures ────────────────────────────────────────────── */
struct RCC_PLLConfig_t
{
    RCC_PLLSrc Source;
    uint8      M;   /* Input divider   (2-63)    */
    uint16     N;   /* VCO multiplier  (50-432)  */
    uint8      P;   /* System divider  (2,4,6,8) */
    uint8      Q;   /* USB/SDIO divider (2-15)   */
};

struct RCC_Config_t
{
    RCC_ClockSrc    ClockSource;
    RCC_PLLConfig_t PLL;
    RCC_AHBPre      AHB_Prescaler;
    RCC_APBPre      APB1_Prescaler;
    RCC_APBPre      APB2_Prescaler;
};

/* ── RCC Driver Class ─────────────────────────────────────────────── */
class RCC
{
public:
    static void Init        (const RCC_Config_t& cfg);
    static void EnableClock (RCC_Bus bus, RCC_Peripheral periph);
    static void DisableClock(RCC_Bus bus, RCC_Peripheral periph);

private:
    static void EnableHSI ();
    static void EnableHSE ();
    static void EnablePLL ();
    static void DisablePLL();
    static void SelectClock(RCC_ClockSrc src);
    static void ConfigPLL  (const RCC_PLLConfig_t& pll);
    static void SetAHBPre  (RCC_AHBPre pre);
    static void SetAPB1Pre (RCC_APBPre pre);
    static void SetAPB2Pre (RCC_APBPre pre);

    RCC() = delete;
};
