#include "Telemetry.h"
#include "Platform_Cfg.h"
#include "App_Config.h"
#include "UART.h"
#include "RTC.h"
#include "ADC.h"
#include "RTOS.h"

/*------------------------------------------------------------------
 *  RCC_CSR — reset-cause register (read once at boot)
 *------------------------------------------------------------------*/
#define RCC_CSR_REG     (*((volatile uint32 *)RCC_CSR_ADDR))

/*------------------------------------------------------------------
 *  Helper macro — string-literal length without the terminator
 *------------------------------------------------------------------*/
#define STRLEN_CONST(s) ((uint8)(sizeof(s) - 1U))

/*------------------------------------------------------------------
 *  Module state
 *------------------------------------------------------------------*/
static RTC_Time_t Get_Time;

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static void TLM_SendString(const uint8 *str, uint8 len);
static void TLM_SendNumber(sint32 value);

/*==================================================================
 *  LifeCounter — every 1 s
 *================================================================*/
void LifeCounter(void)
{
    static uint32 counter = 0U;

    TLM_SendString((const uint8 *)"Life counter: ", STRLEN_CONST("Life counter: "));
    TLM_SendNumber((sint32)counter);
    TLM_SendString((const uint8 *)"\r\n", STRLEN_CONST("\r\n"));

    counter++;
}

/*==================================================================
 *  SW_VERSION — prints firmware version string
 *================================================================*/
void SW_VERSION(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"STM Application Version: ",
                        STRLEN_CONST("STM Application Version: "));
    UART_SendSyncBuffer(UART2, (uint8 *)FIRMWARE_VERSION,
                        STRLEN_CONST(FIRMWARE_VERSION_STR));
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", STRLEN_CONST("\r\n"));
}

/*==================================================================
 *  RUN_TIME — prints RTC time
 *================================================================*/
void RUN_TIME(void)
{
    RTC_GetTime(&Get_Time);

    TLM_SendString((const uint8 *)"Hour: ",    STRLEN_CONST("Hour: "));
    TLM_SendNumber((sint32)Get_Time.hours);
    TLM_SendString((const uint8 *)"\t \t",     STRLEN_CONST("\t \t"));
    TLM_SendString((const uint8 *)"Minuts: ",  STRLEN_CONST("Minuts: "));
    TLM_SendNumber((sint32)Get_Time.minutes);
    TLM_SendString((const uint8 *)"\t",        STRLEN_CONST("\t"));
    TLM_SendString((const uint8 *)"Seconds: ", STRLEN_CONST("Seconds: "));
    TLM_SendNumber((sint32)Get_Time.seconds);
    TLM_SendString((const uint8 *)"\n",        STRLEN_CONST("\n"));
}

/*==================================================================
 *  INTERNAL_TEMP_TASK — every 5 s
 *================================================================*/
void INTERNAL_TEMP_TASK(void)
{
    uint16 raw;
    uint32 voltage_mV;
    sint32 temp;

    raw        = ADC_ReadAveraged(ADC_CHANNEL_TEMP);
    voltage_mV = ((uint32)raw * ADC_VREF_MV) / ADC_FULL_SCALE;
    temp       = (((TEMP_V25_MV - (sint32)voltage_mV) * 10L) / TEMP_AVG_SLOPE_X10)
                 + TEMP_OFFSET_C;

    TLM_SendString((const uint8 *)"Internal Temp: ", STRLEN_CONST("Internal Temp: "));
    TLM_SendNumber(temp);
    TLM_SendString((const uint8 *)" C\r\n", STRLEN_CONST(" C\r\n"));
}

/*==================================================================
 *  LDR_TASK — light sensor
 *================================================================*/
void LDR_TASK(void)
{
    uint16 raw;
    uint32 voltage_mV;
    uint8  light;

    raw        = ADC_ReadAveraged(ADC_CHANNEL_0);
    voltage_mV = ((uint32)raw * ADC_VREF_MV) / ADC_FULL_SCALE;
    light      = (uint8)(((uint32)raw * 100UL) / ADC_FULL_SCALE);

    UART_SendSyncBuffer(UART2, (uint8 *)"RAW: ",        STRLEN_CONST("RAW: "));
    UART_voidSendNumber(UART2, (sint32)raw);
    UART_SendSyncBuffer(UART2, (uint8 *)" | V: ",       STRLEN_CONST(" | V: "));
    UART_voidSendNumber(UART2, (sint32)voltage_mV);
    UART_SendSyncBuffer(UART2, (uint8 *)"mV | Light: ", STRLEN_CONST("mV | Light: "));
    UART_voidSendNumber(UART2, (sint32)light);
    UART_SendSyncBuffer(UART2, (uint8 *)"%\r\n",        STRLEN_CONST("%\r\n"));
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n",         STRLEN_CONST("\r\n"));
}

/*==================================================================
 *  STACK_MONITOR — every 30 s
 *================================================================*/
void STACK_MONITOR(void)
{
    uint8 i;

    TLM_SendString((const uint8 *)"Stack: ", STRLEN_CONST("Stack: "));
    for (i = 0U; i < TASK_NUMBER; i++)
    {
        TLM_SendString((const uint8 *)"T", 1U);
        TLM_SendNumber((sint32)i);
        TLM_SendString((const uint8 *)":", 1U);
        TLM_SendNumber((sint32)RTOS_u8GetStackUsage(i));
        TLM_SendString((const uint8 *)"% ", STRLEN_CONST("% "));
    }
    TLM_SendString((const uint8 *)"\r\n", STRLEN_CONST("\r\n"));
}

/*==================================================================
 *  BOOT_REASON_REPORT — call once right after UART init
 *================================================================*/
void BOOT_REASON_REPORT(void)
{
    uint32       csr = RCC_CSR_REG;
    const uint8 *reason;
    uint8        len;

    if      ((csr & RCC_CSR_IWDGRSTF) != 0U) { reason = (const uint8 *)"BOOT:IWDG\r\n"; len = STRLEN_CONST("BOOT:IWDG\r\n"); }
    else if ((csr & RCC_CSR_WWDGRSTF) != 0U) { reason = (const uint8 *)"BOOT:WWDG\r\n"; len = STRLEN_CONST("BOOT:WWDG\r\n"); }
    else if ((csr & RCC_CSR_SFTRSTF)  != 0U) { reason = (const uint8 *)"BOOT:SFT\r\n";  len = STRLEN_CONST("BOOT:SFT\r\n");  }
    else if ((csr & RCC_CSR_PORRSTF)  != 0U) { reason = (const uint8 *)"BOOT:POR\r\n";  len = STRLEN_CONST("BOOT:POR\r\n");  }
    else if ((csr & RCC_CSR_PINRSTF)  != 0U) { reason = (const uint8 *)"BOOT:PIN\r\n";  len = STRLEN_CONST("BOOT:PIN\r\n");  }
    else                                     { reason = (const uint8 *)"BOOT:UNK\r\n";  len = STRLEN_CONST("BOOT:UNK\r\n");  }

    TLM_SendString(reason, len);

    RCC_CSR_REG |= RCC_CSR_RMVF;    /* clear all reset flags */
}

/*==================================================================
 *  Local helpers — broadcast to both UARTs
 *================================================================*/
static void TLM_SendString(const uint8 *str, uint8 len)
{
    UART_SendSyncBuffer(UART2, (uint8 *)str, len);
    UART_SendSyncBuffer(UART1, (uint8 *)str, len);
}

static void TLM_SendNumber(sint32 value)
{
    UART_voidSendNumber(UART2, value);
    UART_voidSendNumber(UART1, value);
}
