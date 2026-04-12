#include "SwM.h"
#include "Telemetry.h"
#include "App_Ctrl.h"
#include "OLED.h"
#include "IWDG.h"
#include "MPU.h"
#include "UART.h"

ACCEL_t accel_data;
GYRO_t gyro_data;
extern ACCEL_t Bias_accel_data;
extern GYRO_t Bias_gyro_data;
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
    
}

void OS_50ms_Task(void)
{
    /* Read sensors at 20 Hz — safe rate for breadboard I2C */
    MPU_GetAccelerometer(&accel_data);
    MPU_GetGyroscope(&gyro_data);

    /* Machine-readable line for Python visualizer — prefix $DATA so parser can filter it */
    UART_SendSyncBuffer(UART2, (uint8 *)"$DATA:", 6);
    UART_voidSendNumber(UART2, accel_data.Accel_X);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1);
    UART_voidSendNumber(UART2, accel_data.Accel_Y);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1);
    UART_voidSendNumber(UART2, accel_data.Accel_Z);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1);
    UART_voidSendNumber(UART2, gyro_data.GYRO_X - Bias_gyro_data.GYRO_X);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1);
    UART_voidSendNumber(UART2, gyro_data.GYRO_Y - Bias_gyro_data.GYRO_Y);
    UART_SendSyncBuffer(UART2, (uint8 *)",", 1);
    UART_voidSendNumber(UART2, gyro_data.GYRO_Z - Bias_gyro_data.GYRO_Z);
    UART_SendSyncBuffer(UART2, (uint8 *)"\r\n", 2);
}

void OS_100ms_Task(void)
{
    IWDG_Refresh();
}

void OS_1000ms_Task(void)
{
    static uint8 tick = 0;
    tick++;

    LifeCounter(); /* every 1 s */

    if (tick % 5 == 0)
        INTERNAL_TEMP_TASK(); /* every 5 s  */
    if (tick % 10 == 0)
        RUN_TIME(); /* every 10 s */
    if (tick % 30 == 0)
        STACK_MONITOR(); /* every 30 s */
    if (tick == 1)
        SW_VERSION(); /* once on first tick */

    /* MPU display — every 1 s, same task as other prints so no interleaving */
    UART_SendSyncBuffer(UART2, (uint8 *)"=== MPU-6050 ===\r\n", 18);
    UART_SendSyncBuffer(UART2, (uint8 *)"Temp  : ", 8);
    UART_voidSendNumber(UART2, MPU_GetTemp());
    UART_SendSyncBuffer(UART2, (uint8 *)" (x100 C)\r\n", 11);
}

void OS_IDLE_TASK(void)
{
    __asm("NOP");
}
