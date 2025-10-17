#include "STD_TYPES.h"
#include "RCC.h"
#include "GPIO_interface.h"
#include "UART.h"
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
  UART_MODE_TX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B
};


void main (void)
{


GPIO_InitPin(GPIO_PORTA, PIN5,GPIO_MODE_OUTPUT,GPIO_OTYPE_PP,GPIO_SPEED_FAST,GPIO_NO_PULL);
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

 UART_Init(UART1, &Uart_configuration, 16000000);
 UART_Init(UART2, &Uart_configuration, 16000000);
 UART_Init(UART3, &Uart_configuration, 16000000);



// Blink loop
while (1) {
   
   //UART_SendByte(UART2, 101);
   UART_SendString(UART2, "Esmail\n");
     //GPIO_TogglePin(GPIO_PORTA, PIN5);

    for (volatile uint32 i = 0; i < 10000; i++); // delay
}


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