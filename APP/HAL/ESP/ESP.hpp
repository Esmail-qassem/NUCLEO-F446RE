/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : ESP HAL                                                */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "UART.hpp"

/* ── ESP Driver Class ─────────────────────────────────────────────── */
class ESP
{
public:
    static void MainFunction(void);

private:
    ESP() = delete;
};
