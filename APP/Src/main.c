#include "STD_TYPES.h"
#include "RCC.h"
#include "SwM.h"
#include "RTC.h"
#include "GPIO_interface.h"
#include "CRC.h"
RCC_Config_t RCC_Configuration =
    {
        RCC_CLK_HSI,
        {0, 0, 0, 0, 0},
        AHB_PRE_1,
        APB_PRE_1,
        APB_PRE_1};


void Schedular(void)
{
  RTOS_voidCreateTask(0, 5,    OS_5ms_Task);
  RTOS_voidCreateTask(1, 10,   OS_10ms_Task);
  RTOS_voidCreateTask(2, 20,   OS_20ms_Task);
  RTOS_voidCreateTask(3, 50,   OS_50ms_Task);
  RTOS_voidCreateTask(4, 100,  OS_100ms_Task);
  RTOS_voidCreateTask(5, 1000, OS_1000ms_Task);
  RTOS_voidStart();
}


int main(void)
{
  GPIO_PIN_CONFIG();
  ENABLE_NVIC_INTERRUPTS();
  CallBackFunctions();
  APP_init();
  Schedular();
}

void SystemInit(void)
{
  RCC_Init(&RCC_Configuration);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOB);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOC);
  RCC_EnableClock(RCC_APB1, APB1_USART2);
  RCC_EnableClock(RCC_APB2, APB2_USART1);
  RCC_EnableClock(RCC_APB2, APB2_ADC1);
  RCC_EnableClock(RCC_APB1, APB1_PWR);
  RCC_EnableClock(RCC_APB1, APB1_WWDG);
  RCC_EnableClock(RCC_AHB1, AHB1_CRC);

}