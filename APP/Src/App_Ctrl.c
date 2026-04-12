#include "App_Ctrl.h"
#include "GPIO_interface.h"
#include "GP_Timer.h"
#include "UART.h"

/*------------------------------------------------------------------
 *  SCB AIRCR — software system reset
 *------------------------------------------------------------------*/
#define SCB_AIRCR  (*((volatile uint32*)0xE000ED0Cu))

/*------------------------------------------------------------------
 *  Shared state
 *------------------------------------------------------------------*/
uint8 LED_Global = 0;

/* s_pwm_duty kept here for potential future readback */
static uint8 s_pwm_duty = 0;

/*------------------------------------------------------------------
 *  LED
 *------------------------------------------------------------------*/
void LED(void)
{
    GPIO_WritePin(GPIO_PORTA, PIN5, LED_Global);
}

void LED_ON(void)
{
    LED_Global = 1;
}

void LED_OFF(void)
{
    LED_Global = 0;
}

/*------------------------------------------------------------------
 *  SYS_Reset — Cortex-M4 SYSRESETREQ
 *------------------------------------------------------------------*/
void SYS_Reset(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"Resetting...\r\n", 14);
    volatile uint32 i;
    for (i = 0; i < 100000u; i++) { __asm("NOP"); }
    SCB_AIRCR = (0x5FAu << 16u) | (1u << 2u);
    while (1) {}
}

/*------------------------------------------------------------------
 *  Bootloader control
 *------------------------------------------------------------------*/
void BTLD_Jump(void)
{
    GPIO_InitPin(GPIO_PORTC, PIN3, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
    GPIO_WritePin(GPIO_PORTC, PIN3, 0);
}

void BTLD_Update(void)
{
    uint8 trigger = 0x04;
    UART_SendSyncBuffer(UART1, &trigger, 1);
}

/*------------------------------------------------------------------
 *  PWM setter — called from ISR after 2-byte sequence 0x07 + duty
 *------------------------------------------------------------------*/
void PWM_SET(uint8 duty)
{
    if (duty > 100U) duty = 100U;
    s_pwm_duty = duty;
    GP_Timer_PWM_SetDuty(TIMER3, 2, duty);
}

/*------------------------------------------------------------------
 *  duty_cycle_task — auto-sweep for testing
 *------------------------------------------------------------------*/
void duty_cycle_task(void)
{
    static uint8 i = 0;
    i++;
    if (i == 100) i = 0;
    GP_Timer_PWM_SetDuty(TIMER3, 2, i);
}
