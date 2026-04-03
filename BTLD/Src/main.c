#include "STD_TYPES.h"
#include "SysTick_interface.h"
#include "RCC.h"
#include "GPIO_interface.h"
#include "NVIC_interface.h"
#include "UART.h"
#include "Parse.h"

#define VTOR  *((volatile uint32*)0xE000ED08)
/* Readable constants */
#define UART_BAUD_115200        115200u
#define UART_AF_NUM             7u
#define APP_FLASH_SECTOR        2u
#define BTLD_BANNER_MSG         "\n BTLD \n"
#define CLK_CFG_ERR_MSG         "CLK CFG ERR\n"
#define FLASH_ERASE_ERR_MSG     "FLASH ERASE ERR\n"
/* Select which UART receives bootloader bytes */
#ifndef BTLD_RX_UART
#define BTLD_RX_UART            UART1
#endif
/* Clock defaults can be overridden from the build system if needed. */
#ifndef HSI_VALUE_HZ
#define HSI_VALUE_HZ 16000000u
#endif
#ifndef HSE_VALUE_HZ
#define HSE_VALUE_HZ 8000000u
#endif
RCC_Config_t RCC_Configuration=
{
  RCC_CLK_HSI,
  {0, 0, 0, 0, 0},
  AHB_PRE_1,
  APB_PRE_1,
  APB_PRE_1
};

UART_Config_t Uart1_configuration={
  UART_BAUD_115200,
  UART_MODE_TX_RX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B,
  Interrupt
};
UART_Config_t Uart2_configuration={
  UART_BAUD_115200,
  UART_MODE_TX_RX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B,
  Interrupt
};
/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
uint8 APP_init(void);
void CallBackFunctions(void);
volatile uint32 ms_ticks = 0;
void systick_handler(void)
{
  ms_ticks++;
}

static uint32 AHBPrescalerToDiv(RCC_AHBPrescaler_t pre)
{
  switch (pre)
  {
    case AHB_PRE_1:   return 1u;
    case AHB_PRE_2:   return 2u;
    case AHB_PRE_4:   return 4u;
    case AHB_PRE_8:   return 8u;
    case AHB_PRE_16:  return 16u;
    case AHB_PRE_64:  return 64u;
    case AHB_PRE_128: return 128u;
    case AHB_PRE_256: return 256u;
    case AHB_PRE_512: return 512u;
    default:          return 0u;
  }
}

static uint32 APBPrescalerToDiv(RCC_APBPrescaler_t pre)
{
  switch (pre)
  {
    case APB_PRE_1:  return 1u;
    case APB_PRE_2:  return 2u;
    case APB_PRE_4:  return 4u;
    case APB_PRE_8:  return 8u;
    case APB_PRE_16: return 16u;
    default:         return 0u;
  }
}

static uint32 GetSysClockHz(void)
{
  if (RCC_Configuration.ClockSource == RCC_CLK_HSI)
  {
    return HSI_VALUE_HZ;
  }
  if (RCC_Configuration.ClockSource == RCC_CLK_HSE)
  {
    return HSE_VALUE_HZ;
  }
  if (RCC_Configuration.ClockSource == RCC_CLK_PLL)
  {
    uint32 m = RCC_Configuration.PLL_Config.PLL_M;
    uint32 n = RCC_Configuration.PLL_Config.PLL_N;
    uint32 p = RCC_Configuration.PLL_Config.PLL_P;
    uint32 src = (RCC_Configuration.PLL_Config.PLL_Source == RCC_PLLSRC_HSE)
                   ? HSE_VALUE_HZ
                   : HSI_VALUE_HZ;
    if (m == 0u || p == 0u)
    {
      return 0u;
    }
    /* STM32F4 PLL: VCO = (src / M) * N, SYSCLK = VCO / P */
    return (uint32)((((uint64)src * (uint64)n) / (uint64)m) / (uint64)p);
  }
  return 0u;
}

static uint32 GetApbClockHz(RCC_APBPrescaler_t apb_pre)
{
  uint32 sys = GetSysClockHz();
  uint32 ahb_div = AHBPrescalerToDiv(RCC_Configuration.AHB_Prescaler);
  uint32 apb_div = APBPrescalerToDiv(apb_pre);
  if (sys == 0u || ahb_div == 0u || apb_div == 0u)
  {
    return 0u;
  }
  return (sys / ahb_div) / apb_div;
}
int main (void)
{
  /* Ensure clocks are configured even if startup doesn't call SystemInit */
  SystemInit();
  GPIO_PIN_CONFIG();
  uint8 app_init_ok = APP_init();
  SysTick_voidInit();
  NVIC_EnableInterrupt(UART1_IQ_NUM);
  NVIC_EnableInterrupt(UART2_IQ_NUM);
  SysTick_voidSetIntervalPeriodoc(TICKS_PER_MS,&systick_handler);
  UART_SendSyncBuffer(UART2, (uint8 *)BTLD_BANNER_MSG,
                      (uint8)(sizeof(BTLD_BANNER_MSG) - 1u));
  if (app_init_ok == 0u)
  {
    UART_SendSyncBuffer(UART2, (uint8 *)CLK_CFG_ERR_MSG,
                        (uint8)(sizeof(CLK_CFG_ERR_MSG) - 1u));
  }
  CRC32_Init();
  CallBackFunctions();
  static uint8 app_erase_done = 0u;
while(1) 
  {
    /* Erase APP sector only when an update session starts */
    if (UART_START != 0u && app_erase_done == 0u)
    {
      FlashDrv_Status_t status = FlashDrv_EraseSector(APP_FLASH_SECTOR);
      if (status != FLASH_DRV_OK)
      {
        UART_SendSyncBuffer(UART2, (uint8 *)FLASH_ERASE_ERR_MSG,
                            (uint8)(sizeof(FLASH_ERASE_ERR_MSG) - 1u));
      }
      app_erase_done = 1u;
    }
    else if (UART_START == 0u)
    {
      app_erase_done = 0u;
    }
    BootLoader_MainFunction();
  }
}



uint8 APP_init(void)
{
 uint32 pclk1 = GetApbClockHz(RCC_Configuration.APB1_Prescaler);
 uint32 pclk2 = GetApbClockHz(RCC_Configuration.APB2_Prescaler);
 if (pclk1 == 0u || pclk2 == 0u)
 {
   /* Fallback to HSI to allow UART error reporting */
   UART_Init(UART2, &Uart2_configuration, HSI_VALUE_HZ);
   UART_Init(UART1, &Uart1_configuration, HSI_VALUE_HZ);
   return 0u;
 }
 UART_Init(UART2, &Uart2_configuration, pclk1);
 UART_Init(UART1, &Uart1_configuration, pclk2);
 return 1u;
}

void GPIO_PIN_CONFIG(void)
{
/*UART2*/
GPIO_InitPin(GPIO_PORTA, PIN2,GPIO_MODE_AF,GPIO_OTYPE_PP,GPIO_SPEED_HIGH,GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN2, UART_AF_NUM);
GPIO_InitPin(GPIO_PORTA, PIN3, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
GPIO_SetAF(GPIO_PORTA, PIN3, UART_AF_NUM);
  /*UART1*/
  GPIO_InitPin(GPIO_PORTA, PIN9, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);
  GPIO_InitPin(GPIO_PORTA, PIN10, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO_SPEED_HIGH, GPIO_NO_PULL);

  GPIO_SetAF(GPIO_PORTA, PIN9, UART_AF_NUM);
  GPIO_SetAF(GPIO_PORTA, PIN10, UART_AF_NUM);
}


void SystemInit(void)
{
RCC_Init(&RCC_Configuration);
RCC_EnableClock(RCC_AHB1, AHB1_GPIOA);
RCC_EnableClock(RCC_APB2, APB2_USART1);
RCC_EnableClock(RCC_APB1,APB1_USART2);

}

void CallBackFunctions(void)
{
#if (BTLD_RX_UART == UART1)
  UART1_CALLBACK(BootLoader_Handler);
#elif (BTLD_RX_UART == UART2)
  UART2_CALLBACK(BootLoader_Handler);
#else
#error "BTLD_RX_UART must be UART1 or UART2"
#endif
}
