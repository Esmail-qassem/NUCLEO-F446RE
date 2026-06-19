#include "Telemetry.h"
#include "App_Config.h"
#include "UART.h"
#include "RTC.h"
#include "ADC.h"
#include "RTOS.h"
#include "STD_TYPES.h"
/*------------------------------------------------------------------
 *  RCC_CSR reset-cause register (used only by BOOT_REASON_REPORT)
 *------------------------------------------------------------------*/
#define RCC_CSR_REG      (*((volatile uint32*)0x40023874U))
#define RCC_CSR_RMVF     (1U << 24)
#define RCC_CSR_IWDGRSTF (1U << 29)
#define RCC_CSR_WWDGRSTF (1U << 30)
#define RCC_CSR_SFTRSTF  (1U << 28)
#define RCC_CSR_PORRSTF  (1U << 27)
#define RCC_CSR_PINRSTF  (1U << 26)
#define PWR_CSR_REG      (*((volatile uint32*)0x40007004U))
#define PWR_CSR_SBF      (1U << 1)   /* Standby flag — set on any Standby wakeup */

/*------------------------------------------------------------------
 *  Shared state
 *------------------------------------------------------------------*/
RTC_Time_t Get_Time;

/*------------------------------------------------------------------
 *  LifeCounter — every 1 s
 *------------------------------------------------------------------*/
void LifeCounter(void)
{
    static uint32 counter = 0;

    UART_SendSyncBuffer(UART2, (uint8 *)"Life counter: ", 14);
    UART_voidSendNumber(UART2, counter);
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);

    UART_SendSyncBuffer(UART1, (uint8 *)"Life counter: ", 14);
    UART_voidSendNumber(UART1, counter);
    UART_SendSyncBuffer(UART1, (uint8 *)"\r\n", 2);

    counter++;
}

/*------------------------------------------------------------------
 *  SW_VERSION — prints firmware version string
 *------------------------------------------------------------------*/
void SW_VERSION(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"STM Application Version: ", 25);
    UART_SendSyncBuffer(UART2, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION_STR) - 1U);
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);

    UART_SendSyncBuffer(UART1, (uint8 *)"STM Application Version: ", 25);
    UART_SendSyncBuffer(UART1, FIRMWARE_VERSION, sizeof(FIRMWARE_VERSION_STR) - 1U);
    UART_SendSyncBuffer(UART1, (uint8 *)"\r\n", 2);
}

/*------------------------------------------------------------------
 *  RUN_TIME — prints RTC time
 *------------------------------------------------------------------*/
void RUN_TIME(void)
{
    RTC_GetTime(&Get_Time);

    UART_SendSyncBuffer(UART2, (uint8 *)"Hour: ",    sizeof("Hour: ")    - 1);
    UART_voidSendNumber(UART2, Get_Time.hours);
    UART_SendSyncBuffer(UART2, (uint8 *)"\t \t",     3);
    UART_SendSyncBuffer(UART2, (uint8 *)"Minuts: ",  sizeof("Minuts: ")  - 1);
    UART_voidSendNumber(UART2, Get_Time.minutes);
    UART_SendSyncBuffer(UART2, (uint8 *)"\t",        1);
    UART_SendSyncBuffer(UART2, (uint8 *)"Seconds: ", sizeof("Seconds: ") - 1);
    UART_voidSendNumber(UART2, Get_Time.seconds);
    UART_SendSyncBuffer(UART2, (uint8 *)"\n",        1);

    UART_SendSyncBuffer(UART1, (uint8 *)"Hour: ",    sizeof("Hour: ")    - 1);
    UART_voidSendNumber(UART1, Get_Time.hours);
    UART_SendSyncBuffer(UART1, (uint8 *)"\t \t",     3);
    UART_SendSyncBuffer(UART1, (uint8 *)"Minuts: ",  sizeof("Minuts: ")  - 1);
    UART_voidSendNumber(UART1, Get_Time.minutes);
    UART_SendSyncBuffer(UART1, (uint8 *)"\t",        1);
    UART_SendSyncBuffer(UART1, (uint8 *)"Seconds: ", sizeof("Seconds: ") - 1);
    UART_voidSendNumber(UART1, Get_Time.seconds);
    UART_SendSyncBuffer(UART1, (uint8 *)"\n",        1);
}

/*------------------------------------------------------------------
 *  INTERNAL_TEMP_TASK — every 5 s
 *------------------------------------------------------------------*/
void INTERNAL_TEMP_TASK(void)
{
    uint16  raw        = ADC_ReadAveraged(ADC_CHANNEL_TEMP);
    uint32  voltage_mV = ((uint32)raw * 3300) / 4095;
    sint32  temp       = (((sint32)760 - (sint32)voltage_mV) * 10) / 25 + 25;

    UART_SendSyncBuffer(UART2, (uint8 *)"Internal Temp: ", 15);
    UART_voidSendNumber(UART2, temp);
    UART_SendSyncBuffer(UART2, (uint8 *)" C\r\n", 4);

    UART_SendSyncBuffer(UART1, (uint8 *)"Internal Temp: ", 15);
    UART_voidSendNumber(UART1, temp);
    UART_SendSyncBuffer(UART1, (uint8 *)" C\r\n", 4);
}

/*------------------------------------------------------------------
 *  LDR_TASK — light sensor
 *------------------------------------------------------------------*/
void LDR_TASK(void)
{
    // uint16 raw        = ADC_ReadAveraged(ADC_CHANNEL_0);
    // uint32 voltage_mV = ((uint32)raw * 3300) / 4096;
    // uint8  light      = (raw * 100) / 4095;

    // UART_SendSyncBuffer(UART2, (uint8 *)"RAW: ",      5);
    // UART_voidSendNumber(UART2, raw);
    // UART_SendSyncBuffer(UART2, (uint8 *)" | V: ",     6);
    // UART_voidSendNumber(UART2, voltage_mV);
    // UART_SendSyncBuffer(UART2, (uint8 *)"mV | Light: ", 12);
    // UART_voidSendNumber(UART2, light);
    // UART_SendSyncBuffer(UART2, (uint8 *)"%\r\n", 3);
    // UART_SendSyncBuffer(UART2, (uint8 *)"\r\n",   2);

    // UART_SendSyncBuffer(UART1, (uint8 *)"RAW: ",      5);
    // UART_voidSendNumber(UART1, raw);
    // UART_SendSyncBuffer(UART1, (uint8 *)" | V: ",     6);
    // UART_voidSendNumber(UART1, voltage_mV);
    // UART_SendSyncBuffer(UART1, (uint8 *)"mV | Light: ", 12);
    // UART_voidSendNumber(UART1, light);
    // UART_SendSyncBuffer(UART1, (uint8 *)"%\r\n", 3);
    // UART_SendSyncBuffer(UART1, (uint8 *)"\r\n",   2);
}

/*------------------------------------------------------------------
 *  STACK_MONITOR — every 30 s
 *------------------------------------------------------------------*/
void STACK_MONITOR(void)
{
    uint8 i;

    UART_SendSyncBuffer(UART2, (uint8 *)"Stack: ", 7);
    for (i = 0; i < TASK_NUMBER; i++)
    {
        UART_SendSyncBuffer(UART2, (uint8 *)"T", 1);
        UART_voidSendNumber(UART2, i);
        UART_SendSyncBuffer(UART2, (uint8 *)":", 1);
        UART_voidSendNumber(UART2, RTOS_u8GetStackUsage(i));
        UART_SendSyncBuffer(UART2, (uint8 *)"% ", 2);
    }
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);

    UART_SendSyncBuffer(UART1, (uint8 *)"Stack: ", 7);
    for (i = 0; i < TASK_NUMBER; i++)
    {
        UART_SendSyncBuffer(UART1, (uint8 *)"T", 1);
        UART_voidSendNumber(UART1, i);
        UART_SendSyncBuffer(UART1, (uint8 *)":", 1);
        UART_voidSendNumber(UART1, RTOS_u8GetStackUsage(i));
        UART_SendSyncBuffer(UART1, (uint8 *)"% ", 2);
    }
    UART_SendSyncBuffer(UART1, (uint8 *)"\r\n", 2);
}

/*------------------------------------------------------------------
 *  CPU_LOAD_TASK — prints per-task CPU load every 10 s
 *  Format: $CPU:T0:2% T1:0% T2:24% T3:0% T4:0% T5:1%
 *------------------------------------------------------------------*/
void CPU_LOAD_TASK(void)
{
    uint8 i;

    UART_SendSyncBuffer(UART2, (uint8 *)"$CPU:", 5);
    for (i = 0; i < TASK_NUMBER; i++)
    {
        UART_SendSyncBuffer(UART2, (uint8 *)"T", 1);
        UART_voidSendNumber(UART2, i);
        UART_SendSyncBuffer(UART2, (uint8 *)":", 1);
        UART_voidSendNumber(UART2, RTOS_u8GetTaskCPULoad(i));
        UART_SendSyncBuffer(UART2, (uint8 *)"% ", 2);
    }
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);

    UART_SendSyncBuffer(UART1, (uint8 *)"$CPU:", 5);
    for (i = 0; i < TASK_NUMBER; i++)
    {
        UART_SendSyncBuffer(UART1, (uint8 *)"T", 1);
        UART_voidSendNumber(UART1, i);
        UART_SendSyncBuffer(UART1, (uint8 *)":", 1);
        UART_voidSendNumber(UART1, RTOS_u8GetTaskCPULoad(i));
        UART_SendSyncBuffer(UART1, (uint8 *)"% ", 2);
    }
    UART_SendSyncBuffer(UART1, (uint8 *)"\r\n", 2);
}

/*------------------------------------------------------------------
 *  BOOT_REASON_REPORT — call once right after UART init
 *------------------------------------------------------------------*/
void BOOT_REASON_REPORT(void)
{
    uint32        csr = RCC_CSR_REG;
    const uint8  *reason;
    uint8         len;

    /* SFT is checked first — an intentional software reset takes priority
     * over a coincidental IWDG flag that may also be set in RCC_CSR.     */
    if      (PWR_CSR_REG & PWR_CSR_SBF)   { reason = (uint8 *)"BOOT:STDBY\r\n";   len = 12U; } /* Standby wakeup — no RCC flag set */
    else if (csr & RCC_CSR_SFTRSTF)      { reason = (uint8 *)"BOOT:SFT\r\n";     len = 10U; }
    else if (csr & RCC_CSR_IWDGRSTF)     { reason = (uint8 *)"BOOT:IWDG\r\n";    len = 11U; }
    else if (csr & RCC_CSR_WWDGRSTF)     { reason = (uint8 *)"BOOT:WWDG\r\n";    len = 11U; }
    else if (csr & RCC_CSR_PORRSTF)      { reason = (uint8 *)"BOOT:POR\r\n";     len = 10U; }
    else if (csr & RCC_CSR_PINRSTF)      { reason = (uint8 *)"BOOT:PIN\r\n";     len = 10U; }
    else                                  { reason = (uint8 *)"BOOT:UNK\r\n";     len = 10U; }

    UART_SendSyncBuffer(UART2, (uint8 *)reason, len);
    UART_SendSyncBuffer(UART1, (uint8 *)reason, len);

    RCC_CSR_REG |= RCC_CSR_RMVF;   /* clear all reset flags */
}
