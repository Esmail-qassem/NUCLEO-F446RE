/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : ADC                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

/* ── Base Addresses ───────────────────────────────────────────────── */
constexpr uint32 ADC1_BASE_ADDR   = 0x40012000UL;
constexpr uint32 ADC_COMMON_BASE  = 0x40012300UL;

/* ── Register layout ──────────────────────────────────────────────── */
struct ADC_RegDef_t
{
    volatile uint32 SR;     /* 0x00 */
    volatile uint32 CR1;    /* 0x04 */
    volatile uint32 CR2;    /* 0x08 */
    volatile uint32 SMPR1;  /* 0x0C */
    volatile uint32 SMPR2;  /* 0x10 */
    uint32 _r0[4];
    volatile uint32 SQR1;   /* 0x2C */
    volatile uint32 SQR2;   /* 0x30 */
    volatile uint32 SQR3;   /* 0x34 */
    uint32 _r1[5];
    volatile uint32 DR;     /* 0x4C */
};

struct ADC_Common_RegDef_t
{
    volatile uint32 CSR;    /* 0x00 */
    volatile uint32 CCR;    /* 0x04 */
};

#define ADC1_REGS        (reinterpret_cast<ADC_RegDef_t*>(ADC1_BASE_ADDR))
#define ADC_COMMON_REGS  (reinterpret_cast<ADC_Common_RegDef_t*>(ADC_COMMON_BASE))

/* ── Sample-time enums ────────────────────────────────────────────── */
enum class ADC_SampleTime : uint8
{
    CYCLES_3   = 0,
    CYCLES_15  = 1,
    CYCLES_28  = 2,
    CYCLES_56  = 3,
    CYCLES_84  = 4,
    CYCLES_112 = 5,
    CYCLES_144 = 6,
    CYCLES_480 = 7
};

/* ── Channel constants ────────────────────────────────────────────── */
constexpr uint8 ADC_CHANNEL_0    =  0u;
constexpr uint8 ADC_CHANNEL_1    =  1u;
constexpr uint8 ADC_CHANNEL_2    =  2u;
constexpr uint8 ADC_CHANNEL_3    =  3u;
constexpr uint8 ADC_CHANNEL_4    =  4u;
constexpr uint8 ADC_CHANNEL_5    =  5u;
constexpr uint8 ADC_CHANNEL_6    =  6u;
constexpr uint8 ADC_CHANNEL_7    =  7u;
constexpr uint8 ADC_CHANNEL_8    =  8u;
constexpr uint8 ADC_CHANNEL_9    =  9u;
constexpr uint8 ADC_CHANNEL_10   = 10u;
constexpr uint8 ADC_CHANNEL_11   = 11u;
constexpr uint8 ADC_CHANNEL_12   = 12u;
constexpr uint8 ADC_CHANNEL_13   = 13u;
constexpr uint8 ADC_CHANNEL_14   = 14u;
constexpr uint8 ADC_CHANNEL_15   = 15u;
constexpr uint8 ADC_CHANNEL_TEMP = 16u;
constexpr uint8 ADC_CHANNEL_VREF = 17u;
constexpr uint8 ADC_CHANNEL_VBAT = 18u;
constexpr uint8 ADC_AVERAGE_SAMPLES = 64u;

/* ── ADC Driver Class ─────────────────────────────────────────────── */
class ADC
{
public:
    static void   Init            (void);
    static uint16 Read            (uint8 channel);
    static uint16 ReadAveraged    (uint8 channel);
    static uint32 ReadVoltage_mV  (uint8 channel);
    static void   SetSampleTime   (uint8 channel, ADC_SampleTime st);

private:
    static void SetChannelSampleTime(uint8 channel, ADC_SampleTime st);
    ADC() = delete;
};
