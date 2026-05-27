/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : Bootloader Parser                                      */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "UART.hpp"
#include "FLASH.hpp"

constexpr uint32 SCB_AIRCR_ADDR_BTLD = 0xE000ED0CUL;
#define SCB_AIRCR_BTLD (*reinterpret_cast<volatile uint32*>(SCB_AIRCR_ADDR_BTLD))

constexpr uint32 MAX_LINE_LENGTH = 64U;

/* BIN mode */
#define BIN_MODE

enum class Buffer_State : uint8
{
    EMPTY      = 0,
    FILLING    = 1,
    READY      = 2,
    PROCESSING = 3
};

/* ISR-called bootloader handler — extern "C" so it can be registered as a C callback */
extern "C" {
    void BootLoader_Handler(uint8 data);
}

uint8  parseByte      (uint8 high, uint8 low);
uint8  processRecord  (uint8 *recordBuffer);
void   CRC32_Init     (void);
void   BootLoader_MainFunction(void);
