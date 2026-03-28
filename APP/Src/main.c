#include "STD_TYPES.h"
#include "RCC.h"

RCC_Config_t RCC_Configuration =
    {
        RCC_CLK_HSI,
        {0, 0, 0, 0, 0},
        AHB_PRE_1,
        APB_PRE_1,
        APB_PRE_1};
/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
void ENABLE_NVIC_INTERRUPTS(void);
void CallBackFunctions(void);
void Schedular(void);
/**************************************************************/
/**************************************************************/

void main(void)
{ 
  GPIO_PIN_CONFIG();
  ENABLE_NVIC_INTERRUPTS();
  CallBackFunctions();
  APP_init();
  Schedular();
  while (1)
  {
  }
}


void SystemInit(void)
{
  RCC_Init(&RCC_Configuration);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOB);
  RCC_EnableClock(RCC_APB1, APB1_USART2);
  RCC_EnableClock(RCC_APB1, APB1_USART3);
  RCC_EnableClock(RCC_APB2, APB2_USART1);
  RCC_EnableClock(RCC_APB2, APB2_ADC1);
}