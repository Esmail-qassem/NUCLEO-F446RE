#include "STD_TYPES.h"
#include "SysTick_interface.h"
#include "RCC.h"
#include "GPIO_interface.h"
#include "NVIC_interface.h"
#include "UART.h"
#include "Parse.h"

#define VTOR  *((volatile uint32*)0xE000ED08)
void (*Pt2Func_App)(void)=NULL;
RCC_Config_t RCC_Configuration=
{
  RCC_CLK_PLL,
  {RCC_PLLSRC_HSE, 4, 180, 2, 8},
  AHB_PRE_1,
  APB_PRE_4,
  APB_PRE_2
};

UART_Config_t Uart1_configuration={
  115200,
  UART_MODE_TX_RX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B,
  Interrupt
};
UART_Config_t Uart2_configuration={
  115200,
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
void CallBackFunctions(void);
 uint32 ms_ticks = 0;
void systick_handler(void)
{
  ms_ticks++;
  if(ms_ticks % 1000 == 0)
  {

   // UART_SendSyncBuffer(UART2, (uint8 *)".", 1);
    UART_SendSyncBuffer(UART1, (uint8 *)".", 1);
  }
  
}
int main (void)
{
  GPIO_PIN_CONFIG();
  APP_init();
  UART_SendSyncBuffer(UART2, (uint8 *)"\nBTLD Session\n", 14);
  UART_SendSyncBuffer(UART2, (uint8 *)"BTLD will jump to application in 20 seconds if no data is received\n", 67);
  UART_SendSyncBuffer(UART2, (uint8 *)"Waiting for data", 16);
  UART_SendSyncBuffer(UART1, (uint8 *)"\nBTLD Session\n", 14);
  UART_SendSyncBuffer(UART1, (uint8 *)"BTLD will jump to application in 20 seconds if no data is received\n", 67);
   UART_SendSyncBuffer(UART1, (uint8 *)"Waiting for data", 16);

  SysTick_voidInit();
  NVIC_EnableInterrupt(UART1_IQ_NUM);
  //NVIC_EnableInterrupt(UART2_IQ_NUM);
  SysTick_voidSetIntervalPeriodoc(TICKS_PER_MS,&systick_handler);
  CRC32_Init();
  CallBackFunctions();
while(1) 
  {
    BootLoader_MainFunction();
  }
}



void APP_init(void)
{
 UART_Init(UART2, &Uart2_configuration, 45000000);
 UART_Init(UART1, &Uart1_configuration, 90000000);
}

void GPIO_PIN_CONFIG(void)
{
/*UART2*/
GPIO_InitPin(GPIO_PORTA, PIN2,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN2, 7);
GPIO_InitPin(GPIO_PORTA, PIN3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN3, 7);
  /*UART1*/
  GPIO_InitPin(GPIO_PORTA, PIN9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  GPIO_InitPin(GPIO_PORTA, PIN10, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

  GPIO_SetAF(GPIO_PORTA, PIN9, 7);
  GPIO_SetAF(GPIO_PORTA, PIN10, 7);
}


void SystemInit(void)
{
RCC_Init(&RCC_Configuration);
RCC_CSR |= (1U << 24);   /* clear all reset flags */  
RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
RCC_EnableClock(RCC_APB2, APB2_USART1);
RCC_EnableClock(RCC_APB1,APB1_USART2);

}

void CallBackFunctions(void)
{
  UART1_CALLBACK(BootLoader_Handler);
 // UART2_CALLBACK(BootLoader_Handler);
}