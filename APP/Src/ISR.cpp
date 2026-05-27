#include "ISR.hpp"
#include "UART.hpp"
#include "App_Config.hpp"
#include "App_Ctrl.hpp"
#include "Telemetry.hpp"

#define CMD_LED_ON      0x01u
#define CMD_LED_OFF     0x02u
#define CMD_BTLD_JUMP   0x03u
#define CMD_BTLD_UPDATE 0x04u
#define CMD_RUN_TIME    0x05u
#define CMD_RESET       0x06u
#define CMD_PWM_SET     0x07u
#define CMD_GET_VERSION 0xA1u

static volatile uint8 s_pwm_pending = 0u;

void UART2_ISR(uint8 num)
{
    if (s_pwm_pending) { s_pwm_pending = 0u; PWM_SET(num); return; }
    switch (num)
    {
        case CMD_LED_ON:      LED_ON();          break;
        case CMD_LED_OFF:     LED_OFF();         break;
        case CMD_BTLD_JUMP:   BTLD_Jump();       break;
        case CMD_BTLD_UPDATE: BTLD_Update();     break;
        case CMD_RUN_TIME:    RUN_TIME();        break;
        case CMD_RESET:       SYS_Reset();       break;
        case CMD_PWM_SET:     s_pwm_pending = 1u; break;
        case CMD_GET_VERSION: SW_VERSION();      break;
        default: break;
    }
}

void UART1_ISR(uint8 num)
{
    if (s_pwm_pending) { s_pwm_pending = 0u; PWM_SET(num); return; }
    switch (num)
    {
        case CMD_LED_ON:    LED_ON();    break;
        case CMD_LED_OFF:   LED_OFF();   break;
        case CMD_BTLD_JUMP: BTLD_Jump(); break;
        case CMD_RUN_TIME:  RUN_TIME();  break;
        case CMD_RESET:     SYS_Reset(); break;
        case CMD_PWM_SET:   s_pwm_pending = 1u; break;
        case CMD_GET_VERSION:
            UART::SendSyncBuffer(UART_HardWare::UART1,
                reinterpret_cast<const uint8*>("VER:"), 4u);
            UART::SendSyncBuffer(UART_HardWare::UART1,
                FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION_STR) - 1u);
            UART::SendSyncBuffer(UART_HardWare::UART1,
                reinterpret_cast<const uint8*>("\n"), 1u);
            SW_VERSION();
            break;
        default: break;
    }
}
