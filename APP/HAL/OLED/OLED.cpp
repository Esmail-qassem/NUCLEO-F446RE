/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : OLED HAL (SSD1306)                                     */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "OLED.hpp"

static uint8 OLED_Buffer[OLED_WIDTH * OLED_PAGES];
constexpr uint8 OLED_COLUMN_OFFSET = 2U;

/* ── SendCommand ──────────────────────────────────────────────────── */
void OLED::SendCommand(I2C_Port port, uint8 cmd)
{
    uint8 data[2];
    data[0] = 0x00U;
    data[1] = cmd;
    I2C::MasterTransmit(port, OLED_I2C_ADDR, data, 2U, 0U);
}

/* ── SendData ─────────────────────────────────────────────────────── */
void OLED::SendData(I2C_Port port, uint8 dataByte)
{
    uint8 data[2];
    data[0] = 0x40U;
    data[1] = dataByte;
    I2C::MasterTransmit(port, OLED_I2C_ADDR, data, 2U, 0U);
}

/* ── Init ─────────────────────────────────────────────────────────── */
void OLED::Init(I2C_Port port)
{
    for (volatile uint32 i = 0U; i < 100000U; i++) {}

    SendCommand(port, 0xAEU);
    SendCommand(port, 0xD5U);
    SendCommand(port, 0x80U);
    SendCommand(port, 0xA8U);
    SendCommand(port, 0x3FU);
    SendCommand(port, 0xD3U);
    SendCommand(port, 0x00U);
    SendCommand(port, 0x40U);
    SendCommand(port, 0xADU);
    SendCommand(port, 0x8BU);
    SendCommand(port, 0xA1U);
    SendCommand(port, 0xC8U);
    SendCommand(port, 0xDAU);
    SendCommand(port, 0x12U);
    SendCommand(port, 0x81U);
    SendCommand(port, 0x80U);
    SendCommand(port, 0xD9U);
    SendCommand(port, 0x1FU);
    SendCommand(port, 0xDBU);
    SendCommand(port, 0x40U);
    SendCommand(port, 0xA4U);
    SendCommand(port, 0xA6U);

    Clear();
    UpdateScreen(port);

    SendCommand(port, 0xAFU);
}

/* ── Clear ────────────────────────────────────────────────────────── */
void OLED::Clear(void)
{
    for (uint16 i = 0U; i < static_cast<uint16>(sizeof(OLED_Buffer)); i++)
        OLED_Buffer[i] = 0x00U;
}

/* ── DrawPixel ────────────────────────────────────────────────────── */
void OLED::DrawPixel(uint8 x, uint8 y, OLED_Color color)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT) return;
    uint16 index = static_cast<uint16>(x) + static_cast<uint16>(y / 8U) * OLED_WIDTH;
    if (color == OLED_Color::WHITE)
        OLED_Buffer[index] |= static_cast<uint8>(1U << (y % 8U));
    else
        OLED_Buffer[index] &= static_cast<uint8>(~(1U << (y % 8U)));
}

/* ── UpdateScreen ─────────────────────────────────────────────────── */
void OLED::UpdateScreen(I2C_Port port)
{
    for (uint8 page = 0U; page < OLED_PAGES; page++)
    {
        SendCommand(port, static_cast<uint8>(0xB0U + page));
        SendCommand(port, static_cast<uint8>(0x00U + (OLED_COLUMN_OFFSET & 0x0FU)));
        SendCommand(port, static_cast<uint8>(0x10U + ((OLED_COLUMN_OFFSET >> 4U) & 0x0FU)));

        uint8 data[1U + OLED_WIDTH];
        data[0] = 0x40U;
        for (uint8 col = 0U; col < OLED_WIDTH; col++)
            data[1U + col] = OLED_Buffer[static_cast<uint16>(page) * OLED_WIDTH + col];

        I2C::MasterTransmit(port, OLED_I2C_ADDR, data, static_cast<uint16>(sizeof(data)), 0U);
    }
}

/* ── APP ──────────────────────────────────────────────────────────── */
void OLED::APP(void)
{
    static uint8 i = 0U;
    static uint8 j = 0U;
    i = static_cast<uint8>(i + 4U);
    if (i == 128U)
    {
        i = 0U;
        j = static_cast<uint8>(j + 3U);
        if (j == 63U) j = 0U;
    }
    OLED::DrawPixel(i, j, OLED_Color::WHITE);
    OLED::UpdateScreen(I2C_Port::I2C1);
    OLED::Clear();
}
