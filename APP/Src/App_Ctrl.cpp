#include "App_Ctrl.hpp"
#include "GPIO.hpp"
#include "GP_Timer.h"
#include "UART.hpp"

constexpr uint32 SCB_AIRCR = 0xE000ED0Cu;

uint8  LED_Global  = 0u;
static uint8 s_pwm_duty = 0u;

void LED(void)
{
    GPIO::WritePin(GPIO_Port::A, GPIO_Pin::P5, LED_Global);
}

void LED_ON (void) { LED_Global = 1u; }
void LED_OFF(void) { LED_Global = 0u; }

void SYS_Reset(void)
{
    UART::SendSyncBuffer(UART_HardWare::UART2,
        reinterpret_cast<const uint8*>("Resetting...\r\n"), 14u);
    volatile uint32 i;
    for (i = 0u; i < 100000u; i++) { __asm("NOP"); }
    *reinterpret_cast<volatile uint32*>(SCB_AIRCR) = (0x5FAu << 16u) | (1u << 2u);
    while (1) {}
}

void BTLD_Jump(void)
{
    GPIO::InitPin(GPIO_Port::C, GPIO_Pin::P3, GPIO_Mode::OUTPUT,
                  GPIO_OType::PP, GPIO_Speed::FAST, GPIO_Pull::NONE);
    GPIO::WritePin(GPIO_Port::C, GPIO_Pin::P3, 0u);
}

void BTLD_Update(void)
{
    uint8 trigger = 0x04u;
    UART::SendSyncBuffer(UART_HardWare::UART1, &trigger, 1u);
}

void PWM_SET(uint8 duty)
{
    if (duty > 100u) duty = 100u;
    s_pwm_duty = duty;
    GP_Timer_PWM_SetDuty(TIMER3, 2u, duty);
}

void duty_cycle_task(void)
{
    static uint8 i = 0u;
    if (++i == 100u) i = 0u;
    GP_Timer_PWM_SetDuty(TIMER3, 2u, i);
}
