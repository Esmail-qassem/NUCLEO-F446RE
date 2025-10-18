#include "STD_TYPES.h"
#include "BIT_MATH.h"
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
  UART_WORDLEN_8B,
  Polling
};
#define VTOR  *((volatile uint32*)0xE000ED08)
void(*Jump_toApplication)(void)=NULL;
void(*Jump_toBootLoader)(void)=NULL;
uint8 SwReset,PowerReset,PinReset;


/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
/**************************************************************/
/**************************************************************/

void main (void)
{
APP_init();
GPIO_PIN_CONFIG();
 SwReset = GET_BIT(RCC_CSR, SFT_RSTF);
 PowerReset =GET_BIT(RCC_CSR,POR_RSTF);
 PinReset =GET_BIT(RCC_CSR,PIN_RSTF);
 /*REMOVE THE FLAG*/
 SET_BIT(RCC_CSR,RMVF);
while(1) 
{
UART_SendString(UART2, "\n BM \n");
if(PowerReset || SwReset)
{
  VTOR=0x08008000;
  Jump_toApplication=*((volatile uint32*)0x08008004);
  Jump_toApplication();



}
else if (PinReset)
{
  VTOR=0x08004000;
  Jump_toBootLoader= *((volatile uint32*)0x08004004);
  Jump_toBootLoader();


}



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
}


void SystemInit(void)
{
RCC_Init(&RCC_Configuration);
RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
RCC_EnableClock(RCC_APB1,APB1_USART2);

}