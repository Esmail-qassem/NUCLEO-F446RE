#include "SwM.h"
#include "Telemetry.h"
#include "App_Ctrl.h"
#include "IWDG.h"
#include "PwrMd.h"
#include "MPU_6050.h"
#include "UART.h"
#include "Oledh.h"
#include "RTC.h"



ACCEL_t accel_data;
GYRO_t gyro_data;
extern ACCEL_t Bias_accel_data;
extern GYRO_t Bias_gyro_data;
uint8 shutdown_request;
/*------------------------------------------------------------------
 *  OS task wrappers — each function maps to one scheduler slot
 *------------------------------------------------------------------*/
void OS_5ms_Task(void)
{

}

void OS_10ms_Task(void)
{
    LED();
}

void OS_20ms_Task(void)
{
    if(shutdown_request == 1u)
    {
        Stm_ShutDown(PWR_MODE_SLEEP);
        shutdown_request=0;
    }
    else if (shutdown_request == 2u)
    {
        Stm_ShutDown(PWR_MODE_STOP);
        shutdown_request=0;
    }
    else if (shutdown_request == 3u)
    {
        Stm_ShutDown(PWR_MODE_STANDBY);
        shutdown_request=0;
    }
}

void OS_50ms_Task(void)
{
   OLED_APP();
}

void OS_100ms_Task(void)
{
    IWDG_Refresh();
}

void OS_1000ms_Task(void)
{
    static uint8 tick = 0;
    tick++;

    /* Apply NTP time received from ESP via UART1 */
    extern volatile uint8 ntp_time_ready, ntp_hh, ntp_mm, ntp_ss;
    if (ntp_time_ready)
    {
        ntp_time_ready = 0;
        RTC_Time_t ntp = { ntp_hh, ntp_mm, ntp_ss, RTC_AM };
        RTC_SetTime(&ntp);
        UART_SendSyncBuffer(UART2, (uint8*)"[NTP] RTC updated\r\n", 19);
    }

    LifeCounter(); /* every 1 s */

    if (tick % 5 == 0)
        INTERNAL_TEMP_TASK(); /* every 5 s  */
    if (tick % 10 == 0)
    {
        RUN_TIME();       /* every 10 s */
        CPU_LOAD_TASK();  /* every 10 s */
    }
    if (tick % 30 == 0)
        STACK_MONITOR(); /* every 30 s */
    if (tick == 1)
        SW_VERSION(); /* once on first tick */

    if (tick == 1)
        UART1_Arm();  /* arm ESP commands after 3 s — ESP boot garbage is gone */

}

void OS_IDLE_TASK(void)
{
    __asm("NOP");
}
