#include "STD_TYPES.h"
#include "RCC.hpp"
#include "SwM.hpp"
#include "App_Config.hpp"
#include "RTOS.hpp"

static const RCC_Config_t RCC_Configuration = {
    RCC_ClockSrc::HSI,
    { RCC_PLLSrc::HSI, 0u, 0u, 0u, 0u },
    RCC_AHBPre::DIV1,
    RCC_APBPre::DIV1,
    RCC_APBPre::DIV1
};

static void Scheduler(void)
{
    RTOS::CreateTask(0u, 5u,    OS_5ms_Task);
    RTOS::CreateTask(1u, 10u,   OS_10ms_Task);
    RTOS::CreateTask(2u, 20u,   OS_20ms_Task);
    RTOS::CreateTask(3u, 50u,   OS_50ms_Task);
    RTOS::CreateTask(4u, 100u,  OS_100ms_Task);
    RTOS::CreateTask(5u, 1000u, OS_1000ms_Task);
    RTOS::Start();
}

int main(void)
{
    GPIO_PIN_CONFIG();
    ENABLE_NVIC_INTERRUPTS();
    CallBackFunctions();
    APP_init();
    Scheduler();
    return 0;
}

extern "C" void SystemInit(void)
{
    RCC::Init(RCC_Configuration);
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::GPIOA);
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::GPIOB);
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::GPIOC);
    RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::USART2);
    RCC::EnableClock(RCC_Bus::APB2, RCC_Peripheral::USART1);
    RCC::EnableClock(RCC_Bus::APB2, RCC_Peripheral::ADC1);
    RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::PWR);
    RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::WWDG);
    RCC::EnableClock(RCC_Bus::AHB1, RCC_Peripheral::CRC);
    RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::I2C1);
}
