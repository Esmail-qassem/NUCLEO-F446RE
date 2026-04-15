#include "App_Ctrl.h"
#include "App_Ctrl_Cfg.h"
#include "Platform_Cfg.h"
#include "Cmd_Protocol.h"
#include "GPIO_interface.h"
#include "GP_Timer.h"
#include "UART.h"

/*------------------------------------------------------------------
 *  SCB AIRCR — software system reset
 *------------------------------------------------------------------*/
#define SCB_AIRCR   (*((volatile uint32 *)SCB_AIRCR_ADDR))

/*------------------------------------------------------------------
 *  Module state
 *------------------------------------------------------------------*/
static uint8 LED_Global = 0U;

/* s_pwm_duty kept here for potential future readback */
static uint8 s_pwm_duty = 0U;

/*------------------------------------------------------------------
 *  LED
 *------------------------------------------------------------------*/
void LED(void)
{
    GPIO_WritePin(LED_PORT, LED_PIN, LED_Global);
}

void LED_ON(void)
{
    LED_Global = 1U;
}

void LED_OFF(void)
{
    LED_Global = 0U;
}

/*------------------------------------------------------------------
 *  SYS_Reset — Cortex-M4 SYSRESETREQ
 *------------------------------------------------------------------*/
void SYS_Reset(void)
{
    volatile uint32 i;

    UART_SendSyncBuffer(UART2, (uint8 *)"Resetting...\r\n",
                        sizeof("Resetting...\r\n") - 1U);

    for (i = 0U; i < SYS_RESET_DELAY_LOOPS; i++)
    {
        __asm("NOP");
    }

    SCB_AIRCR = (SCB_AIRCR_VECTKEY << SCB_AIRCR_VECTKEY_POS) | SCB_AIRCR_SYSRESETREQ;

    for (;;)
    {
        /* wait for reset */
    }
}

/*------------------------------------------------------------------
 *  Bootloader control
 *------------------------------------------------------------------*/
void BTLD_Jump(void)
{
    GPIO_InitPin(BTLD_TRIG_PORT, BTLD_TRIG_PIN,
                 GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
    GPIO_WritePin(BTLD_TRIG_PORT, BTLD_TRIG_PIN, 0U);
}

void BTLD_Update(void)
{
    uint8 trigger = (uint8)CMD_BTLD_UPDATE;
    UART_SendSyncBuffer(UART1, &trigger, 1U);
}

/*------------------------------------------------------------------
 *  PWM_SET — called from ISR after 2-byte sequence CMD_PWM_SET + duty
 *------------------------------------------------------------------*/
void PWM_SET(uint8 duty)
{
    if (duty > PWM_DUTY_MAX_PCT)
    {
        duty = PWM_DUTY_MAX_PCT;
    }
    s_pwm_duty = duty;
    GP_Timer_PWM_SetDuty(PWM_TIMER, PWM_CHANNEL, duty);
}

/*------------------------------------------------------------------
 *  duty_cycle_task — auto-sweep for testing
 *------------------------------------------------------------------*/
void duty_cycle_task(void)
{
    static uint8 i = 0U;

    i++;
    if (i >= PWM_DUTY_MAX_PCT)
    {
        i = 0U;
    }
    GP_Timer_PWM_SetDuty(PWM_TIMER, PWM_CHANNEL, i);
}
