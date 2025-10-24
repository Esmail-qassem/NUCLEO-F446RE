#include "STD_TYPES.h"
#include "RCC.h"
#include "RTOS.h"
#include "GPIO_interface.h"
#include "UART.h"
RCC_Config_t RCC_Configuration =
{
  RCC_CLK_HSI,
  {0,0,0,0,0},
  AHB_PRE_1,
  APB_PRE_1,
  APB_PRE_1
};

UART_Config_t Uart_configuration={
  921600,
  UART_MODE_TX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B,
  Polling
};

/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
/**************************************************************/
/**************************************************************/

void LED (void)
{
   GPIO_TogglePin(GPIO_PORTA, PIN5);
}
void UART (void)
{
  static uint32 counter=0;
  UART_voidSendNumber(UART2,counter);
   UART_SendByte(UART2,'\n');
   counter+=10;
  
}

void main (void)
{
APP_init();
GPIO_PIN_CONFIG();
RTOS_voidCreateTask(0,500,LED);
RTOS_voidCreateTask(1,100,UART);

/*Synchronous function*/
// Blink loop
while(1) {}
}



void APP_init(void)
{
 RTOS_voidStart(); 
 UART_Init(UART1, &Uart_configuration, 16000000);
 UART_Init(UART2, &Uart_configuration, 16000000);
 UART_Init(UART3, &Uart_configuration, 16000000);
}

void GPIO_PIN_CONFIG(void)
{
/*IN BOARD LED*/
GPIO_InitPin(GPIO_PORTA, PIN5,GPIO_MODE_OUTPUT,GPIO_OTYPE_PP,GPIO_SPEED_FAST,GPIO_NO_PULL);
/* MSO PIN*/
GPIO_InitPin(GPIO_PORTA, PIN8,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
/*UART1*/
GPIO_InitPin(GPIO_PORTA, PIN9,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
/*UART2*/
GPIO_InitPin(GPIO_PORTA, PIN2,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
/*UART3*/
GPIO_InitPin(GPIO_PORTB, PIN10,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN9, 7);
GPIO_SetAF(GPIO_PORTA, PIN2, 7);
GPIO_SetAF(GPIO_PORTB, PIN10, 7);  
}


void SystemInit(void)
{
RCC_Init(&RCC_Configuration);
RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
RCC_EnableClock(RCC_AHB1, AHB1_GPIOB);
RCC_EnableClock(RCC_APB1,APB1_USART2);
RCC_EnableClock(RCC_APB1,APB1_USART3);
RCC_EnableClock(RCC_APB2,APB2_USART1);

}