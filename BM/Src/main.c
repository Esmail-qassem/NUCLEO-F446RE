#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RCC.h"
#include "GPIO_interface.h"
#include "UART.h"
RCC_Config_t RCC_Configuration=
{
  RCC_CLK_HSI,
  {0, 0, 0, 0, 0},
  AHB_PRE_1,
  APB_PRE_1,
  APB_PRE_1
};

UART_Config_t Uart_configuration={
  115200,
  UART_MODE_TX,
  UART_PARITY_NONE,
  UART_STOPBITS_1,
  UART_WORDLEN_8B,
  Polling
};
#define VTOR       *((volatile uint32*)0xE000ED08)
#define PWR_CR     *((volatile uint32*)0x40007000)   /* PWR control register   */
#define RTC_BKP0R  *((volatile uint32*)0x40002850)   /* RTC backup register 0  */
#define BTLD_MAGIC  0xB007B007U                       /* "boot-boot"            */
void(*Jump_toApplication)(void)=NULL;
void(*Jump_toBootLoader)(void)=NULL;
uint8 SwReset,PowerReset,PinReset;


/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
/**************************************************************/
/**************************************************************/

int main (void)
{
APP_init();
GPIO_PIN_CONFIG();
 SwReset = GET_BIT(RCC_CSR, SFT_RSTF);
 PowerReset = GET_BIT(RCC_CSR,POR_RSTF);
 PinReset = GET_BIT(RCC_CSR,PIN_RSTF);
 uint8 IwdgReset = GET_BIT(RCC_CSR, IWDG_RSTF);

 /* Do NOT clear RMVF here — APP's BOOT_REASON_REPORT() reads and clears flags */
while(1) 
{
UART_SendSyncBuffer(UART2, (uint8 *)"\n BM \n",7);
if(PowerReset || SwReset || IwdgReset)
{
  VTOR=0x08008000;
  Jump_toApplication = (void (*)(void)) *((volatile uint32*)0x08008004);
  Jump_toApplication();



}
else if (PinReset)
{
  VTOR=0x08004000;
  Jump_toBootLoader = (void (*)(void)) *((volatile uint32*)0x08004004);
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