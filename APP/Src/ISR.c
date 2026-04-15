#include "ISR.h"
#include "Cmd_Protocol.h"
#include "UART.h"
#include "App_Config.h"
#include "App_Ctrl.h"
#include "Telemetry.h"

/*------------------------------------------------------------------
 *  2-byte PWM command state (shared between both ISRs)
 *------------------------------------------------------------------*/
static volatile uint8 s_pwm_pending = 0U;

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static void ISR_Dispatch(UART_HardWare_t port, uint8 byte);

/*------------------------------------------------------------------
 *  UART2_ISR  (wired serial — PC tool)
 *------------------------------------------------------------------*/
void UART2_ISR(uint8 num)
{
    ISR_Dispatch(UART2, num);
}

/*------------------------------------------------------------------
 *  UART1_ISR  (ESP8266 wireless link)
 *------------------------------------------------------------------*/
void UART1_ISR(uint8 num)
{
    ISR_Dispatch(UART1, num);
}

/*------------------------------------------------------------------
 *  ISR_Dispatch — common command-byte handler for both UARTs
 *------------------------------------------------------------------*/
static void ISR_Dispatch(UART_HardWare_t port, uint8 byte)
{
    if (s_pwm_pending != 0U)
    {
        s_pwm_pending = 0U;
        PWM_SET(byte);
    }
    else
    {
        switch (byte)
        {
            case (uint8)CMD_LED_ON:
                LED_ON();
                break;

            case (uint8)CMD_LED_OFF:
                LED_OFF();
                break;

            case (uint8)CMD_BTLD_JUMP:
                BTLD_Jump();
                break;

            case (uint8)CMD_BTLD_UPDATE:
                /* Only the wired link may trigger a firmware update. */
                if (port == UART2)
                {
                    BTLD_Update();
                }
                break;

            case (uint8)CMD_RUN_TIME:
                RUN_TIME();
                break;

            case (uint8)CMD_RESET:
                SYS_Reset();
                break;

            case (uint8)CMD_PWM_SET:
                s_pwm_pending = 1U;
                break;

            case (uint8)CMD_GET_VERSION:
                if (port == UART1)
                {
                    UART_SendSyncBuffer(UART1, (uint8 *)"VER:", sizeof("VER:") - 1U);
                    UART_SendSyncBuffer(UART1, (uint8 *)FIRMWARE_VERSION,
                                        sizeof(FIRMWARE_VERSION_STR) - 1U);
                    UART_SendSyncBuffer(UART1, (uint8 *)"\n", 1U);
                }
                SW_VERSION();
                break;

            default:
                /* unknown command — ignore */
                break;
        }
    }
}
