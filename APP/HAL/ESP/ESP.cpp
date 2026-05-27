/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : ESP HAL                                                */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "ESP.hpp"

static const uint8 *ESP_Commands[] =
{
    reinterpret_cast<const uint8*>("AT\r\n"),
    reinterpret_cast<const uint8*>("AT+CWMODE=1\r\n"),
    reinterpret_cast<const uint8*>("AT+CWJAP=\"ZzZz\",\"Esmail_122001\"\r\n"),
    reinterpret_cast<const uint8*>("AT+CIFSR\r\n"),
    reinterpret_cast<const uint8*>("AT+PING=\"8.8.8.8\"\r\n"),
    reinterpret_cast<const uint8*>("AT+CIPSTART=\"TCP\",\"example.com\",80\r\n"),
    reinterpret_cast<const uint8*>("AT+CIPSEND=18\r\n"),
    reinterpret_cast<const uint8*>("GET / HTTP/1.1\r\n\r\n")
};

static uint8 ESP_APPLICATION_FLAG = 0U;

static uint32 my_strlen(const uint8 *str)
{
    uint32 len = 0U;
    while (str[len] != '\0') len++;
    return len;
}

/* ── MainFunction ─────────────────────────────────────────────────── */
void ESP::MainFunction(void)
{
    static uint8 ESP_Commands_Counter = 0U;
    constexpr uint8 CMD_COUNT = static_cast<uint8>(sizeof(ESP_Commands) / sizeof(ESP_Commands[0]));

    if (ESP_Commands_Counter < CMD_COUNT)
    {
        UART::SendSyncBuffer(UART_HardWare::UART1,
                             ESP_Commands[ESP_Commands_Counter],
                             static_cast<uint8>(my_strlen(ESP_Commands[ESP_Commands_Counter])));
        ESP_Commands_Counter++;
    }

    if (ESP_Commands_Counter == CMD_COUNT)
    {
        ESP_APPLICATION_FLAG = 1U;
        ESP_Commands_Counter++;
    }
}
