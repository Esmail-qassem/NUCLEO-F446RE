/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : OLED HAL (SSD1306)                                     */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#pragma once

#include "STD_TYPES.h"
#include "I2C.hpp"

constexpr uint8  OLED_I2C_ADDR = 0x3CU;
constexpr uint8  OLED_WIDTH    = 128U;
constexpr uint8  OLED_HEIGHT   = 64U;
constexpr uint8  OLED_PAGES    = OLED_HEIGHT / 8U;

enum class OLED_Color : uint8
{
    BLACK = 0x00,
    WHITE = 0x01
};

/* ── OLED Driver Class ────────────────────────────────────────────── */
class OLED
{
public:
    static void Init         (I2C_Port port);
    static void SendCommand  (I2C_Port port, uint8 cmd);
    static void SendData     (I2C_Port port, uint8 data);
    static void Clear        (void);
    static void DrawPixel    (uint8 x, uint8 y, OLED_Color color);
    static void UpdateScreen (I2C_Port port);
    static void APP          (void);

private:
    OLED() = delete;
};
