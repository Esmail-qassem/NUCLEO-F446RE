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
#define MOVE_UP    0x1
#define MOVE_DOWN  0x2
#define MOVE_RIGHT 0x3
#define MOVE_LEFT  0x4
#define GAME_RESET 0X5
/*------------------------------------------------------------------
 *  2-byte PWM command state (shared between both ISRs)
 *------------------------------------------------------------------*/
static volatile uint8 s_pwm_pending = 0;
extern volatile uint8 move;
extern volatile uint8 User_Option;
/*------------------------------------------------------------------
 *  UART1 startup lock — ESP8266 sends garbage bytes at 74880 baud
 *  during its boot sequence. Ignore all UART1 commands for the
 *  first 3 seconds so we don't accidentally call BTLD_Jump().
 *------------------------------------------------------------------*/
static volatile uint8 s_uart1_armed = 0;

void UART1_Arm(void)
{
    s_uart1_armed = 1;
}

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
        case 'w' :   move = MOVE_UP             ;break;
        case 'd' :   move =   MOVE_RIGHT        ;break;
        case 's' :   move =   MOVE_DOWN         ;break;
        case 'a' :   move =   MOVE_LEFT         ;break;
        case 'r' :   move =   GAME_RESET        ;
                     User_Option =GAME_RESET     ;break;
        case 'i' :   User_Option =   'i'        ;break;
        case 'j' :   User_Option =   'j'        ;break;
        default:                                 break;
    }
}

/*------------------------------------------------------------------
 *  UART1_ISR  (ESP8266 wireless link)
 *------------------------------------------------------------------*/
void UART1_ISR(uint8 num)
{
    if (!s_uart1_armed) return;   /* ignore until ESP boot is done */

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
        case CMD_BTLD_UPDATE: BTLD_Update(); break;
        case CMD_RUN_TIME:  RUN_TIME();  break;
        case CMD_RESET:     SYS_Reset(); break;
        case CMD_PWM_SET:   s_pwm_pending = 1; break;
        case CMD_GET_VERSION:
            /* ESP expects "VER:x.x.x\n" — must match getSTM32Version() parser */
            UART_SendSyncBuffer(UART1, (uint8 *)"VER:", 4);
            UART_SendSyncBuffer(UART1, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION_STR) - 1U);
            UART_SendSyncBuffer(UART1, (uint8 *)"\n", 1);
            SW_VERSION();   /* human-readable to both UARTs */
            break;
        default: break;
    }
}
