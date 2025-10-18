#include "STD_TYPES.h"
#include "RCC.h"
#include "GPIO_interface.h"
#include "NVIC_interface.h"
#include "UART.h"
#include "Parse.h"

RCC_Config_t RCC_Configuration=
{
  RCC_CLK_HSI,
  0,
  AHB_PRE_1,
  APB_PRE_1,
  APB_PRE_1
};

UART_Config_t Uart_configuration={
  921600,
  UART_MODE_TX_RX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B,
  Interrupt
};

/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
/**************************************************************/
/**************************************************************/

extern uint8 rx_test;
void main (void)
{
  GPIO_PIN_CONFIG();
  APP_init();
  NVIC_EnableInterrupt(38);
  UART_SendString(UART2, "\n BTLD \n");
while(1) 
  {
     UART_voidSendNumber(UART2,rx_test);
     for(int i=0;i<50000;i++);
  }
}



void APP_init(void)
{
 UART_Init(UART2, &Uart_configuration, 16000000);
}

void GPIO_PIN_CONFIG(void)
{
/*UART2*/
GPIO_InitPin(GPIO_PORTA, PIN2,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN2, 7);
GPIO_InitPin(GPIO_PORTA, PIN3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN3, 7);

}


void SystemInit(void)
{
RCC_Init(&RCC_Configuration);
RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
RCC_EnableClock(RCC_APB1,APB1_USART2);

}