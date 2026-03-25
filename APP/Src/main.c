#include "STD_TYPES.h"
#include "RCC.h"
#include "NVIC_interface.h"
#include "RTOS.h"
#include "GPIO_interface.h"
#include "UART.h"
//#include "SPI.h"
#include "Basic_Timer.h"
#include "FLASH.h"
#include "ESP.h"
#include "FLAPPY_Bird.h"
extern uint8 ESP_APPLICATION_FLAG;
RCC_Config_t RCC_Configuration =
    {
        RCC_CLK_HSI,
        {0, 0, 0, 0, 0},
        AHB_PRE_1,
        APB_PRE_1,
        APB_PRE_1};

UART_Config_t Uart1_configuration = {
    115200, // 921600
    UART_MODE_TX_RX,
    UART_PARITY_NONE,
    UART_STOPBITS_1,
    UART_WORDLEN_8B,
    Interrupt};
UART_Config_t Uart2_configuration = {
    115200, // 921600
    UART_MODE_TX_RX,
    UART_PARITY_NONE,
    UART_STOPBITS_1,
    UART_WORDLEN_8B,
    Interrupt};

//const SPI_Config_t spi_config =
//    {
//        SPI1,
//        2, /* 2,4,8,16,32,64,128,256 (driver maps to BR bits) */
//        SPI_MODE_0,
//        1, /* 1=master, 0=slave */
//        8, /* 8 or 16 bits */
//        0, /* 0=MSB first, 1=LSB first */
//        1, /* 1 -> SSM/SSI set (master) */
//};
I2C_Config_t config = {400000, 0, 1, 0};

/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
void ENABLE_NVIC_INTERRUPTS(void);
void CallBackFunctions(void);
void UART1_ISR(uint8 num);
void UART2_ISR(uint8 num);
/**************************************************************/
/**************************************************************/
void LED(void)
{
  GPIO_TogglePin(GPIO_PORTA, PIN5);
}
void OS_10ms_Task(void)
{
UART_SendSyncBuffer(UART2, "cpu load is ", 12);
UART_voidSendNumber(UART2, RTOS_u8GetCPULoad());
UART_SendSyncBuffer(UART2, "\r\n", 2);
}
void OS_5ms_Task(void)
{
  static uint8 x = 0;
  static uint8 y = 0;
  x++;
  BLOCK_DRAW(10, 10, x, y);

  swapBuffer();
  OLED_UpdateScreen(I2C1_PORT);
  OLED_Clear();
}
OS_IDLE_TASK()
{
 // GPIO_TogglePin(GPIO_PORTA, PIN5);
}
void LifeCounter(void)
{
  static uint32 counter = 0;
  UART_SendSyncBuffer(UART2, "Life counter: ", 14);
  UART_voidSendNumber(UART2, counter);
  UART_SendSyncBuffer(UART2, "\r\n", 2);
  counter++;
}
const char FIRMWARE_VERSION[] = "1.0.0";
#define CMD_GET_VERSION 0xA1
void main(void)
{ 
  GPIO_PIN_CONFIG();
  ENABLE_NVIC_INTERRUPTS();
  CallBackFunctions();
  APP_init();
  RTOS_voidCreateTask(0, 100, LED);
  RTOS_voidCreateTask(1, 500, LifeCounter);
  while (1)
  {
  }
}

void APP_init(void)
{
  UART_Init(UART1, &Uart1_configuration, 16000000);
  UART_Init(UART2, &Uart2_configuration, 16000000);
  UART_Init(UART3, &Uart2_configuration, 16000000);
  //_Init(&spi_config);

  //I2C_Init(I2C1_PORT, &config);
  //OLED_Init(I2C1_PORT);
  RTOS_voidStart();
}

void GPIO_PIN_CONFIG(void)
{
  /*IN BOARD LED*/
  GPIO_InitPin(GPIO_PORTA, PIN5, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
  /* MSO PIN*/
  GPIO_InitPin(GPIO_PORTA, PIN8, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  /*UART1*/
  GPIO_InitPin(GPIO_PORTA, PIN9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  GPIO_InitPin(GPIO_PORTA, PIN10, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

  /*UART2*/
  GPIO_InitPin(GPIO_PORTA, PIN2, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  GPIO_InitPin(GPIO_PORTA, PIN3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  /*UART3*/
  GPIO_InitPin(GPIO_PORTB, PIN10, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  GPIO_InitPin(GPIO_PORTB, PIN11, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

  /*I2C1  SDA*/
  GPIO_InitPin(GPIO_PORTB, PIN9, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  /*I2C1  SCL*/
  GPIO_InitPin(GPIO_PORTB, PIN8, GPIO_MODE_AF, GPIO_OTYPE_OD, GPIO_SPEED_HIGH, GPIO_NO_PULL);

  /* UART */
  GPIO_SetAF(GPIO_PORTA, PIN9, 7);
  GPIO_SetAF(GPIO_PORTA, PIN10, 7);
  GPIO_SetAF(GPIO_PORTA, PIN2, 7);
  GPIO_SetAF(GPIO_PORTA, PIN3, 7);
  GPIO_SetAF(GPIO_PORTB, PIN10, 7);
  GPIO_SetAF(GPIO_PORTB, PIN11, 7);

  /* I2C */
  GPIO_SetAF(GPIO_PORTB, PIN8, 4);
  GPIO_SetAF(GPIO_PORTB, PIN9, 4);
}

void ENABLE_NVIC_INTERRUPTS(void)
{
  NVIC_EnableInterrupt(UART1_IQ_NUM);
  NVIC_EnableInterrupt(UART2_IQ_NUM);

  // NVIC_EnableInterrupt(31);
  // NVIC_EnableInterrupt(32);
}

void CallBackFunctions(void)
{
  UART2_CALLBACK(UART2_ISR);
  UART1_CALLBACK(UART1_ISR);
}

void UART1_ISR(uint8 num)
{
  uint8 Str[] = {num};
  if(num == CMD_GET_VERSION )
  {
    UART_SendSyncBuffer(UART1, (uint8 *)FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION));
  }
}
void UART2_ISR(uint8 num)
{
  
}

void SystemInit(void)
{
  RCC_Init(&RCC_Configuration);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
  RCC_EnableClock(RCC_AHB1, AHB1_GPIOB);
  RCC_EnableClock(RCC_APB1, APB1_USART2);
  RCC_EnableClock(RCC_APB1, APB1_USART3);
  RCC_EnableClock(RCC_APB2, APB2_USART1);
  RCC_EnableClock(RCC_APB2, APB2_SPI1);
  RCC_EnableClock(RCC_APB1, APB1_I2C1);
}