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
#define CMD_SLEEP       0x08
#define CMD_STOP       0x09
#define CMD_STANDBY       0x0A
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
extern volatile uint8 shutdown_request;

/*------------------------------------------------------------------
 *  NTP time receive buffer (UART1)
 *  ISR accumulates "TIME:HH:MM:SS\r" → sets flag for task to apply
 *------------------------------------------------------------------*/
static char    s_ntp_buf[16];
static uint8   s_ntp_idx = 0;
volatile uint8 ntp_hh = 0, ntp_mm = 0, ntp_ss = 0;
volatile uint8 ntp_time_ready = 0;

static void NTP_ParseBuffer(void)
{
    /* expect "TIME:HH:MM:SS" exactly 13 chars */
    if (s_ntp_idx < 13) return;
    if (s_ntp_buf[0]!='T' || s_ntp_buf[1]!='I' ||
        s_ntp_buf[2]!='M' || s_ntp_buf[3]!='E' || s_ntp_buf[4]!=':') return;

    ntp_hh = (uint8)((s_ntp_buf[5]-'0')*10 + (s_ntp_buf[6]-'0'));
    ntp_mm = (uint8)((s_ntp_buf[8]-'0')*10 + (s_ntp_buf[9]-'0'));
    ntp_ss = (uint8)((s_ntp_buf[11]-'0')*10 + (s_ntp_buf[12]-'0'));

    if (ntp_hh < 24 && ntp_mm < 60 && ntp_ss < 60)
        ntp_time_ready = 1;
}
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
        case CMD_SLEEP:     shutdown_request = 1; break;
        case CMD_STOP:      shutdown_request = 2; break;
        case CMD_STANDBY:   shutdown_request = 3; break;
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
        case CMD_SLEEP:     shutdown_request = 1; break;
        case CMD_STOP:      shutdown_request = 2; break;
        case CMD_STANDBY:   shutdown_request = 3; break;
        case 'w' :   move = MOVE_UP;    break;
        case 'd' :   move = MOVE_RIGHT; break;
        case 's' :   move = MOVE_DOWN;  break;
        case 'a' :   move = MOVE_LEFT;  break;
        case 'r' :   move = GAME_RESET; User_Option = GAME_RESET; break;
        case 'i' :   User_Option = 'i'; break;
        case 'j' :   User_Option = 'j'; break;
        default:
            if (num == '\r')
            {
                /* end of line — try to parse accumulated buffer */
                s_ntp_buf[s_ntp_idx] = '\0';
                NTP_ParseBuffer();
                s_ntp_idx = 0;
            }
            else if (num >= 0x20 && s_ntp_idx < (uint8)(sizeof(s_ntp_buf) - 1))
            {
                s_ntp_buf[s_ntp_idx++] = (char)num;
            }
            break;
    }
}
