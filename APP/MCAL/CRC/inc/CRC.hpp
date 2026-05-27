/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : CRC                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"

constexpr uint32 CRC_BASE_ADDR = 0x40023000UL;

struct CRC_RegDef_t
{
    volatile uint32 DR;  /* 0x00 */
    volatile uint8  IDR; /* 0x04 — 8-bit scratch */
    uint8  _r0[3];
    volatile uint32 CR;  /* 0x08 */
};

#define CRC_REGS (reinterpret_cast<CRC_RegDef_t*>(CRC_BASE_ADDR))

constexpr uint8 CRC_OK  = 0u;
constexpr uint8 CRC_ERR = 1u;

class CRC
{
public:
    static void   Init           (void);
    static void   Reset          (void);
    static uint32 AccumulateWord (uint32 word);
    static uint32 Calculate      (const uint8 *buf, uint32 len);
    static uint8  Verify         (const uint8 *buf, uint32 total_len);
    static uint32 GetResult      (void);
private:
    CRC() = delete;
};
