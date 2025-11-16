#include "STD_TYPES.h"
#include "RCC.h"
#include "FLASH.h"
#include "NVIC_interface.h"
#include "RTOS.h"
#include "GPIO_interface.h"
#include "UART.h"
#include "Basic_Timer.h"
#include "ESP.h"
extern uint8 ESP_APPLICATION_FLAG;
RCC_Config_t RCC_Configuration =
{
  RCC_CLK_HSI,
  {0,0,0,0,0},
  AHB_PRE_1,
  APB_PRE_1,
  APB_PRE_1
};

UART_Config_t Uart_configuration={
  115200, //921600
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
void ENABLE_NVIC_INTERRUPTS(void);
void CallBackFunctions(void);
/**************************************************************/
/**************************************************************/
void LED (void)
{
  GPIO_TogglePin(GPIO_PORTA, PIN5);
}
void OS_500_Task (void)
{
  if (ESP_APPLICATION_FLAG == 0)
  {
     ESP_MainFunction();
  }
 
}
void UART1_ISR (uint8 num)
{
  uint8 Str[]={num};
  UART_SendSyncBuffer(UART2,(uint8*) Str, 1);
}
void UART2_ISR (uint8 num)
{
  UART_SendSyncBuffer(UART2,(uint8*)"UART2 ISR\r\n", 12);
}

void main (void)
{
APP_init();
GPIO_PIN_CONFIG();
ENABLE_NVIC_INTERRUPTS();
CallBackFunctions();
RTOS_voidCreateTask(0,200,LED);
RTOS_voidCreateTask(1,500,OS_500_Task);
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
GPIO_InitPin(GPIO_PORTA, PIN10,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);

/*UART2*/
GPIO_InitPin(GPIO_PORTA, PIN2,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_InitPin(GPIO_PORTA, PIN3,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
/*UART3*/
GPIO_InitPin(GPIO_PORTB, PIN10,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_InitPin(GPIO_PORTB, PIN11,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN9, 7);
GPIO_SetAF(GPIO_PORTA, PIN10, 7);
GPIO_SetAF(GPIO_PORTA, PIN2, 7);
GPIO_SetAF(GPIO_PORTA, PIN3, 7);
GPIO_SetAF(GPIO_PORTB, PIN10, 7);  
GPIO_SetAF(GPIO_PORTB, PIN11, 7);  
}

void ENABLE_NVIC_INTERRUPTS(void)
{
  NVIC_EnableInterrupt(UART1_IQ_NUM);
  NVIC_EnableInterrupt(UART2_IQ_NUM);
}

void CallBackFunctions(void)
{
  UART2_CALLBACK(UART2_ISR);
  UART1_CALLBACK(UART1_ISR);
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