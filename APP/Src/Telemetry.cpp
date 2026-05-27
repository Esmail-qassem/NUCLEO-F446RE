#include "Telemetry.hpp"
#include "App_Config.hpp"
#include "UART.hpp"
#include "RTC.h"
#include "ADC.hpp"
#include "RTOS.hpp"

#define RCC_CSR_REG      (*reinterpret_cast<volatile uint32*>(0x40023874u))
#define RCC_CSR_RMVF     (1u << 24u)
#define RCC_CSR_IWDGRSTF (1u << 29u)
#define RCC_CSR_WWDGRSTF (1u << 30u)
#define RCC_CSR_SFTRSTF  (1u << 28u)
#define RCC_CSR_PORRSTF  (1u << 27u)
#define RCC_CSR_PINRSTF  (1u << 26u)

RTC_Time_t Get_Time;

void LifeCounter(void)
{
    static uint32 counter = 0u;
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("Life counter: "), 14u);
    UART::SendNumber(UART_HardWare::UART2, static_cast<sint32>(counter));
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("\r\n"), 2u);
    UART::SendSyncBuffer(UART_HardWare::UART1,
        reinterpret_cast<const uint8*>("Life counter: "), 14u);
    UART::SendNumber(UART_HardWare::UART1, static_cast<sint32>(counter));
    UART::SendSyncBuffer(UART_HardWare::UART1,
        reinterpret_cast<const uint8*>("\r\n"), 2u);
    counter++;
}

void SW_VERSION(void)
{
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("STM Application Version: "), 25u);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION_STR) - 1u);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("\r\n"), 2u);
}

void RUN_TIME(void)
{
    RTC_GetTime(&Get_Time);
    auto send2 = [](const char *s, uint8 len) {
        UART::SendSyncBuffer(UART_HardWare::UART2,
            reinterpret_cast<const uint8*>(s), len);
    };
    auto send1 = [](const char *s, uint8 len) {
        UART::SendSyncBuffer(UART_HardWare::UART1,
            reinterpret_cast<const uint8*>(s), len);
    };
    send2("Hour: ",    6u); UART::SendNumber(UART_HardWare::UART2, Get_Time.hours);
    send2("\t \t",     3u);
    send2("Minuts: ",  8u); UART::SendNumber(UART_HardWare::UART2, Get_Time.minutes);
    send2("\t",        1u);
    send2("Seconds: ", 9u); UART::SendNumber(UART_HardWare::UART2, Get_Time.seconds);
    send2("\n",        1u);
    send1("Hour: ",    6u); UART::SendNumber(UART_HardWare::UART1, Get_Time.hours);
    send1("\t \t",     3u);
    send1("Minuts: ",  8u); UART::SendNumber(UART_HardWare::UART1, Get_Time.minutes);
    send1("\t",        1u);
    send1("Seconds: ", 9u); UART::SendNumber(UART_HardWare::UART1, Get_Time.seconds);
    send1("\n",        1u);
}

void INTERNAL_TEMP_TASK(void)
{
    uint16  raw        = ADC::ReadAveraged(ADC_Channel::TEMP);
    uint32  voltage_mV = (static_cast<uint32>(raw) * 3300u) / 4095u;
    sint32  temp       = (((sint32)760 - (sint32)voltage_mV) * 10) / 25 + 25;
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("Internal Temp: "), 15u);
    UART::SendNumber(UART_HardWare::UART2, temp);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(" C\r\n"), 4u);
    UART::SendSyncBuffer(UART_HardWare::UART1,
        reinterpret_cast<const uint8*>("Internal Temp: "), 15u);
    UART::SendNumber(UART_HardWare::UART1, temp);
    UART::SendSyncBuffer(UART_HardWare::UART1,
        reinterpret_cast<const uint8*>(" C\r\n"), 4u);
}

void LDR_TASK(void)
{
    uint16 raw        = ADC::ReadAveraged(ADC_Channel::CH0);
    uint32 voltage_mV = (static_cast<uint32>(raw) * 3300u) / 4096u;
    uint8  light      = static_cast<uint8>((raw * 100u) / 4095u);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("RAW: "), 5u);
    UART::SendNumber(UART_HardWare::UART2, raw);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(" | V: "), 6u);
    UART::SendNumber(UART_HardWare::UART2, static_cast<sint32>(voltage_mV));
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("mV | Light: "), 12u);
    UART::SendNumber(UART_HardWare::UART2, light);
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("%\r\n\r\n"), 5u);
}

void STACK_MONITOR(void)
{
    auto printStack = [](UART_HardWare hw) {
        UART::SendSyncBuffer(hw, reinterpret_cast<const uint8*>("Stack: "), 7u);
        for (uint8 i = 0u; i < RTOS_TASK_COUNT; i++)
        {
            UART::SendSyncBuffer(hw, reinterpret_cast<const uint8*>("T"), 1u);
            UART::SendNumber(hw, i);
            UART::SendSyncBuffer(hw, reinterpret_cast<const uint8*>(":"), 1u);
            UART::SendNumber(hw, RTOS::GetStackUsage(i));
            UART::SendSyncBuffer(hw, reinterpret_cast<const uint8*>("% "), 2u);
        }
        UART::SendSyncBuffer(hw, reinterpret_cast<const uint8*>("\r\n"), 2u);
    };
    printStack(UART_HardWare::UART2);
    printStack(UART_HardWare::UART1);
}

void BOOT_REASON_REPORT(void)
{
    uint32       csr    = RCC_CSR_REG;
    const char  *reason;
    uint8        len;

    if      (csr & RCC_CSR_IWDGRSTF) { reason = "BOOT:IWDG\r\n"; len = 11u; }
    else if (csr & RCC_CSR_WWDGRSTF) { reason = "BOOT:WWDG\r\n"; len = 11u; }
    else if (csr & RCC_CSR_SFTRSTF)  { reason = "BOOT:SFT\r\n";  len = 10u; }
    else if (csr & RCC_CSR_PORRSTF)  { reason = "BOOT:POR\r\n";  len = 10u; }
    else if (csr & RCC_CSR_PINRSTF)  { reason = "BOOT:PIN\r\n";  len = 10u; }
    else                              { reason = "BOOT:UNK\r\n";  len = 10u; }

    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>(reason), len);
    UART::SendSyncBuffer(UART_HardWare::UART1,
        reinterpret_cast<const uint8*>(reason), len);
    RCC_CSR_REG |= RCC_CSR_RMVF;
}
