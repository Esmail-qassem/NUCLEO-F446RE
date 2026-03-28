#include "STD_TYPES.h"
#include "NVIC_interface.h"
#include "RTOS.h"
#include "GPIO_interface.h"
#include "UART.h"
#include "ADC.h"

/*Global Variables*/
extern uint8 ESP_APPLICATION_FLAG;
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
    Polling};
const char FIRMWARE_VERSION[] = "1.0.0";
uint16 adc_value;
uint32 voltage;


/*MACROS*/
#define CMD_GET_VERSION 0xA1


/*Function Prototypes*/
void OS_IDLE_TASK(void);
void LED(void);
void LifeCounter(void);
void SW_VERSION(void);
void ADC_TASK(void);
void UART1_ISR(uint8 num);
void UART2_ISR(uint8 num);
/*************************************************/

void Schedular(void)
{
  RTOS_voidCreateTask(0, 100, LED);
  RTOS_voidCreateTask(1, 500, LifeCounter);
  RTOS_voidCreateTask(2, 1000, SW_VERSION);
  RTOS_voidCreateTask(3, 100, ADC_TASK);
}







void APP_init(void)
{
  UART_Init(UART1, &Uart1_configuration, 16000000);
  UART_Init(UART2, &Uart2_configuration, 16000000);
  ADC_Init();
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

  /* UART */
  GPIO_SetAF(GPIO_PORTA, PIN9, 7);
  GPIO_SetAF(GPIO_PORTA, PIN10, 7);
  GPIO_SetAF(GPIO_PORTA, PIN2, 7);
  GPIO_SetAF(GPIO_PORTA, PIN3, 7);
  /* ADC1  NO NEED TO SET AF */
  GPIO_InitPin(GPIO_PORTA, PIN0, GPIO_MODE_ANALOG, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
}

void ENABLE_NVIC_INTERRUPTS(void)
{
  NVIC_EnableInterrupt(UART1_IQ_NUM);

}

void CallBackFunctions(void)
{
  UART1_CALLBACK(UART1_ISR);
}

void UART1_ISR(uint8 num)
{
  uint8 Str[] = {num};
  if(num == CMD_GET_VERSION )
  {
    UART_SendSyncBuffer(UART1, (uint8 *)FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION)-1U);
  }
}
void LED(void)
{
  GPIO_TogglePin(GPIO_PORTA, PIN5);
}

void OS_IDLE_TASK(void){}
void LifeCounter(void)
{
  static uint32 counter = 0;
  UART_SendSyncBuffer(UART2, "Life counter: ", 14);
  UART_voidSendNumber(UART2, counter);
  UART_SendSyncBuffer(UART2, "\r\n", 2);
  counter++;
}
void SW_VERSION(void)
{
  
  
    UART_SendSyncBuffer(UART2, "STM Application Version: ", 25);
    UART_SendSyncBuffer(UART2, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION)-1U);
    UART_SendSyncBuffer(UART2, "\r\n", 2);

}

void ADC_TASK(void)
{
  adc_value = ADC_Read(ADC_CHANNEL_0);
  voltage = (adc_value* 5000) / 4095;
  UART_SendSyncBuffer(UART2, "ADC Value: ", 11);
  UART_voidSendNumber(UART2, adc_value);
  UART_SendSyncBuffer(UART2, "\r\n", 2);
  UART_SendSyncBuffer(UART2, "Voltage: ", 9);
  UART_voidSendNumber(UART2, voltage); // Send voltage in mV
  UART_SendSyncBuffer(UART2, " mV\r\n", 5);
}


