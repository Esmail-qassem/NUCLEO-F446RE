#include "STD_TYPES.h"
#include "SwM.h"
#include "NVIC_interface.h"
#include "RTOS.h"
#include "GPIO_interface.h"
#include "GP_Timer.h"
#include "UART.h"
#include "ADC.h"
#include "RTC.h"
#include "IWDG.h"
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
    Interrupt};

/* LSI ~32 kHz: 32000 / 100 / 320 = 1 Hz calendar tick.
   Use RTC_CLK_LSE + prediv_a=127, prediv_s=255 if you have an LSE crystal. */
RTC_Config_t RTC_config =
{
  RTC_CLK_LSI,
  99,
  319,
  RTC_HOURFORMAT_24
};

RTC_Time_t Time =
{
  0,
  0,
  0,
  RTC_AM
};
const uint8 FIRMWARE_VERSION[] = "1.0.5";
uint16 adc_value;
uint32 voltage;
uint8 LED_Global=0;
RTC_Time_t Get_Time;
/*************************************************************************************************/
/*MACROS*/
#define CMD_GET_VERSION 0xA1
#define VREF 5U              // Reference voltage in volts
#define ADC_RESOLUTION 4096U // 12-bit ADC resolution (2^12 =
#define CMD_RESET       0x06

#define SCB_AIRCR  (*((volatile uint32*)0xE000ED0Cu))
/*************************************************************************************************/

/*************************************************/
void OS_5ms_Task(void)
{

}
void OS_10ms_Task(void)
{
  LED();
}
void OS_20ms_Task(void)
{
  /* nothing */
}
void OS_50ms_Task(void)
{
  /* nothing */
}
void OS_100ms_Task(void)
{
  IWDG_Refresh();
}

/* Called from main scheduler loop — slow periodic output to UART2 */
void OS_1000ms_Task(void)
{
  static uint8 tick = 0;
  tick++;

  LifeCounter();          /* every 1 s — life counter + version */

  if (tick % 5 == 0)     /* every 5 s — internal temperature */
  {
    INTERNAL_TEMP_TASK();
  }

  if (tick % 10 == 0)    /* every 10 s — RTC time */
  {
    RUN_TIME();
  }

  if (tick == 1)          /* once on first tick — firmware version */
  {
    SW_VERSION();
  }
}

void RUN_TIME (void)
{
   RTC_GetTime(&Get_Time);
   UART_SendSyncBuffer(UART2, (uint8 *)"Hour: ", sizeof("Hour: ")-1);
  UART_voidSendNumber(UART2, Get_Time.hours);
  UART_SendSyncBuffer(UART2, (uint8 *)"\t \t", 3);
  UART_SendSyncBuffer(UART2, (uint8 *)"Minuts: ", sizeof("Minuts: ")-1);
  UART_voidSendNumber(UART2, Get_Time.minutes);
  UART_SendSyncBuffer(UART2, (uint8 *)"\t", 1);
  UART_SendSyncBuffer(UART2, (uint8 *)"Seconds: ", sizeof("Seconds: ")-1);
  UART_voidSendNumber(UART2, Get_Time.seconds);
UART_SendSyncBuffer(UART2, (uint8 *)"\n", 1);

}
void duty_cycle_task(void)
{
  static uint8 i = 0;
  i++;
  if (i == 100)
  {
    i = 0;
  }
  GP_Timer_PWM_SetDuty(TIMER3, 2, i); // Gradually increase duty cycle on channel 1
}
void LDR_TASK(void)
{
  uint16 raw = ADC_ReadAveraged(ADC_CHANNEL_0);
  uint32 voltage_mV = ((uint32)raw * 3300) / 4096;

  /* Inverted mapping: raw=4095 → bright, raw=0 → dark */
  uint8 light = (raw * 100) / 4095; // Direct mapping instead of inverted

  UART_SendSyncBuffer(UART2, (uint8 *)"RAW: ", 5);
  UART_voidSendNumber(UART2, raw);
  UART_SendSyncBuffer(UART2, (uint8 *)" | V: ", 6);
  UART_voidSendNumber(UART2, voltage_mV);
  UART_SendSyncBuffer(UART2, (uint8 *)"mV | Light: ", 12);
  UART_voidSendNumber(UART2, light);
  UART_SendSyncBuffer(UART2, (uint8 *)"%\r\n", 3);
  UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);
}
void INTERNAL_TEMP_TASK(void)
{
  /* Read channel 16 — internal temperature sensor */
  uint16 raw = ADC_ReadAveraged(ADC_CHANNEL_TEMP);
  uint32 voltage_mV = ((uint32)raw * 3300) / 4095;

  /* STM32F446 internal temp sensor formula from datasheet:
   * Temperature = ((V25 - Vsense) / Avg_Slope) + 25
   * V25       = 760mV  (voltage at 25°C)
   * Avg_Slope = 2.5mV/°C
   * Vsense    = measured voltage in mV */

  sint32 temp = (((sint32)760 - (sint32)voltage_mV) * 10) / 25 + 25;

  UART_SendSyncBuffer(UART2, (uint8 *)"Internal Temp: ", 15);
  UART_voidSendNumber(UART2, temp);
  UART_SendSyncBuffer(UART2, (uint8 *)" C\r\n", 4);
}

void APP_init(void)
{
  UART_Init(UART1, &Uart1_configuration, 16000000);
  UART_Init(UART2, &Uart2_configuration, 16000000);
  ADC_Init();
  GP_Timer_PWM_Init(TIMER3);
  RTC_Init(&RTC_config);
  if (!RTC_IsInitialized()) {
  RTC_SetTime(&Time);
  }
  IWDG_Init(IWDG_PRE_32, IWDG_CalcReload(120, IWDG_PRE_32, 32000)); // 250-ms timeout
}
/*********************************************************************************************************/
/****************************** GIO PIN CONFIGURATION*****************************************************/

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
  GPIO_InitPin(GPIO_PORTA, PIN1, GPIO_MODE_ANALOG, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);

  /* PWM */
  /* CHANNEL 2 TIMER 3 */
  GPIO_InitPin(GPIO_PORTC, PIN7, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  GPIO_SetAF(GPIO_PORTC, PIN7, 2);

  /* PC3 BOOTLOADER PIN */
  GPIO_InitPin(GPIO_PORTC, PIN3, GPIO_MODE_INPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_PULL_UP);
}
/*************************************************************************************************/
/*************************************************************************************************/

void ENABLE_NVIC_INTERRUPTS(void)
{
  NVIC_EnableInterrupt(UART1_IQ_NUM);
  NVIC_EnableInterrupt(UART2_IQ_NUM);
}

void CallBackFunctions(void)
{
  UART1_CALLBACK(UART1_ISR);
  UART2_CALLBACK(UART2_ISR);
}


void SYS_Reset(void)
{
  /* Cortex-M4 SYSRESETREQ — resets CPU + peripherals, cleaner than NRST */
  UART_SendSyncBuffer(UART2, (uint8 *)"Resetting...\r\n", 14);
  /* Small delay so UART flushes before reset */
  volatile uint32 i;
  for (i = 0; i < 100000u; i++) { __asm("NOP"); }
  SCB_AIRCR = (0x5FAu << 16u) | (1u << 2u);
  /* Never returns */
  while(1){}
}

void LED_ON (void)
{
  LED_Global= 1;
}
void LED_OFF (void)
{
  LED_Global= 0;
}
void BTLD_Jump (void)
{
  GPIO_InitPin(GPIO_PORTC, PIN3, GPIO_MODE_OUTPUT, GPIO_OTYPE_PP, GPIO_SPEED_FAST, GPIO_NO_PULL);
  GPIO_WritePin(GPIO_PORTC, PIN3, 0);

  
}
void BTLD_Update (void)
{
  /* Signal the ESP8266 to perform an immediate OTA poll.
     ESP loop() reads this byte and forces lastPollTime = 0. */
  uint8 trigger = 0x04;
  UART_SendSyncBuffer(UART1, &trigger, 1);
}

void LED(void)
{
   GPIO_WritePin(GPIO_PORTA, PIN5,LED_Global);
}

void OS_IDLE_TASK(void) {}
void LifeCounter(void)
{
  static uint32 counter = 0;
  UART_SendSyncBuffer(UART2, (uint8 *)"Life counter: ", 14);
  UART_voidSendNumber(UART2, counter);
  UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);
  counter++;
}
void SW_VERSION(void)
{

  UART_SendSyncBuffer(UART2, (uint8 *)"STM Application Version: ", 25);
  UART_SendSyncBuffer(UART2, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION) - 1U);
  UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);
}




/*************************************************************************************************/
/***************************************ISR*******************************************************/

void UART2_ISR (uint8 num)
{
  switch (num) {
         case 0x01: LED_ON();       break;
         case 0x02: LED_OFF();      break;
         case 0x03: BTLD_Jump();    break;
         case 0x04: BTLD_Update();  break;
         case 0x05: RUN_TIME();     break;
         case 0x06: SYS_Reset();    break;
         case 0xA1: SW_VERSION();   break;
         default:   break;
     }
}
void UART1_ISR(uint8 num)
{
  /* UART1 = ESP8266 link — mirrors UART2_ISR so WiFi and Wire paths
     behave identically.  0x04 (BTLD_Update) is intentionally absent:
     ESP handles it internally (triggers OTA) without forwarding here. */
  switch (num)
  {
    case 0x01: LED_ON();    break;
    case 0x02: LED_OFF();   break;
    case 0x03: BTLD_Jump(); break;
    case 0x05: RUN_TIME();  break;
    case 0x06: SYS_Reset(); break;
    case 0xA1:
      /* Reply to ESP on UART1 (OTA version check) */
      UART_SendSyncBuffer(UART1, (uint8 *)FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION) - 1U);
      /* Also echo full string to UART2 so the server dashboard updates */
      SW_VERSION();
      break;
    default: break;
  }
}