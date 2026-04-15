#include "SwM.h"
#include "SwM_Cfg.h"
#include "App_Config.h"
#include "Telemetry.h"
#include "App_Ctrl.h"
#include "OLED.h"
#include "IWDG.h"
#include "MPU.h"
#include "UART.h"
#include "FLAPPY_Bird.h"

/*------------------------------------------------------------------
 *  Scheduler task table (consumed by main.c)
 *------------------------------------------------------------------*/
const SwM_TaskCfg_t SwM_TaskTable[TASK_ID_COUNT] =
{
    { (uint8)TASK_ID_5MS,    TASK_PERIOD_5MS,    &OS_5ms_Task    },
    { (uint8)TASK_ID_10MS,   TASK_PERIOD_10MS,   &OS_10ms_Task   },
    { (uint8)TASK_ID_20MS,   TASK_PERIOD_20MS,   &OS_20ms_Task   },
    { (uint8)TASK_ID_50MS,   TASK_PERIOD_50MS,   &OS_50ms_Task   },
    { (uint8)TASK_ID_100MS,  TASK_PERIOD_100MS,  &OS_100ms_Task  },
    { (uint8)TASK_ID_1000MS, TASK_PERIOD_1000MS, &OS_1000ms_Task },
};

/*------------------------------------------------------------------
 *  Module state
 *------------------------------------------------------------------*/
static ACCEL_t accel_data;
static GYRO_t  gyro_data;

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static void SwM_SendImuLine(void);
static void SwM_PrintMpuTemp(void);

/*==================================================================
 *  OS task wrappers — each function maps to one scheduler slot
 *================================================================*/
void OS_5ms_Task(void)
{
    /* reserved */
}

void OS_10ms_Task(void)
{
    LED();
}

void OS_20ms_Task(void)
{
    /* Read sensors at 50 Hz — safe rate for breadboard I2C */
    MPU_GetAccelerometer(&accel_data);
    MPU_GetGyroscope(&gyro_data);

    SwM_SendImuLine();
}

void OS_50ms_Task(void)
{
    Game_Task();    /* Flappy-Bird @ 20 Hz — owns the OLED */
}

void OS_100ms_Task(void)
{
    IWDG_Refresh();
}

void OS_1000ms_Task(void)
{
    static uint8 tick = 0U;

    tick++;
    if (tick >= TLM_TICK_WRAP_S)
    {
        tick = 0U;
    }

    LifeCounter();                              /* every 1 s */

    if ((tick % TLM_TEMP_PERIOD_S) == 0U)
    {
        INTERNAL_TEMP_TASK();                   /* every 5 s  */
    }
    if ((tick % TLM_RUNTIME_PERIOD_S) == 0U)
    {
        RUN_TIME();                             /* every 10 s */
    }
    if ((tick % TLM_STACK_PERIOD_S) == 0U)
    {
        STACK_MONITOR();                        /* every 30 s */
    }
    if (tick == 1U)
    {
        SW_VERSION();                           /* once on first tick */
    }

    SwM_PrintMpuTemp();
}

void OS_IDLE_TASK(void)
{
    __asm("NOP");
}

/*==================================================================
 *  Local helpers
 *================================================================*/

/*------------------------------------------------------------------
 *  SwM_SendImuLine
 *  Machine-readable line for the Python visualiser — prefix $DATA
 *  so the host parser can filter it.
 *------------------------------------------------------------------*/
static void SwM_SendImuLine(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"$DATA:", sizeof("$DATA:") - 1U);
    UART_voidSendNumber(UART2, accel_data.Accel_X);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1U);
    UART_voidSendNumber(UART2, accel_data.Accel_Y);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1U);
    UART_voidSendNumber(UART2, accel_data.Accel_Z);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1U);
    UART_voidSendNumber(UART2, gyro_data.GYRO_X - Bias_gyro_data.GYRO_X);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1U);
    UART_voidSendNumber(UART2, gyro_data.GYRO_Y - Bias_gyro_data.GYRO_Y);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1U);
    UART_voidSendNumber(UART2, gyro_data.GYRO_Z - Bias_gyro_data.GYRO_Z);
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", sizeof("\r\n") - 1U);
}

/*------------------------------------------------------------------
 *  SwM_PrintMpuTemp
 *  Human-readable MPU-6050 temperature (1 s, same task as other
 *  prints so output does not interleave).
 *------------------------------------------------------------------*/
static void SwM_PrintMpuTemp(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"=== MPU-6050 ===\r\n",
                        sizeof("=== MPU-6050 ===\r\n") - 1U);
    UART_SendSyncBuffer(UART2, (uint8 *)"Temp  : ",
                        sizeof("Temp  : ") - 1U);
    UART_voidSendNumber(UART2, MPU_GetTemp());
    UART_SendSyncBuffer(UART2, (uint8 *)" (x100 C)\r\n",
                        sizeof(" (x100 C)\r\n") - 1U);
}
