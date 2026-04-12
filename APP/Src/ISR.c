#include "ISR.h"
#include "UART.h"
#include "App_Config.h"
#include "App_Ctrl.h"
#include "Telemetry.h"

/*------------------------------------------------------------------
 *  Command bytes
 *------------------------------------------------------------------*/
#define CMD_LED_ON      0x01
#define CMD_LED_OFF     0x02
#define CMD_BTLD_JUMP   0x03
#define CMD_BTLD_UPDATE 0x04
#define CMD_RUN_TIME    0x05
#define CMD_RESET       0x06
#define CMD_PWM_SET     0x07   /* followed by duty byte 0–100 */
#define CMD_GET_VERSION 0xA1

/*------------------------------------------------------------------
 *  2-byte PWM command state (shared between both ISRs)
 *------------------------------------------------------------------*/
static volatile uint8 s_pwm_pending = 0;

/*------------------------------------------------------------------
 *  UART2_ISR  (wired serial — PC tool)
 *------------------------------------------------------------------*/
void UART2_ISR(uint8 num)
{
    if (s_pwm_pending)
    {
        s_pwm_pending = 0;
        PWM_SET(num);
        return;
    }

    switch (num)
    {
        case CMD_LED_ON:      LED_ON();          break;
        case CMD_LED_OFF:     LED_OFF();         break;
        case CMD_BTLD_JUMP:   BTLD_Jump();       break;
        case CMD_BTLD_UPDATE: BTLD_Update();     break;
        case CMD_RUN_TIME:    RUN_TIME();        break;
        case CMD_RESET:       SYS_Reset();       break;
        case CMD_PWM_SET:     s_pwm_pending = 1; break;
        case CMD_GET_VERSION: SW_VERSION();      break;
        default:                                 break;
    }
}

/*------------------------------------------------------------------
 *  UART1_ISR  (ESP8266 wireless link)
 *------------------------------------------------------------------*/
void UART1_ISR(uint8 num)
{
    if (s_pwm_pending)
    {
        s_pwm_pending = 0;
        PWM_SET(num);
        return;
    }

    switch (num)
    {
        case CMD_LED_ON:    LED_ON();    break;
        case CMD_LED_OFF:   LED_OFF();   break;
        case CMD_BTLD_JUMP: BTLD_Jump(); break;
        case CMD_RUN_TIME:  RUN_TIME();  break;
        case CMD_RESET:     SYS_Reset(); break;
        case CMD_PWM_SET:   s_pwm_pending = 1; break;
        case CMD_GET_VERSION:
            UART_SendSyncBuffer(UART1, (uint8 *)"VER:", 4);
            UART_SendSyncBuffer(UART1, (uint8 *)FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION_STR) - 1U);
            UART_SendSyncBuffer(UART1, (uint8 *)"\n", 1);
            SW_VERSION();
            break;
        default: break;
    }
}
