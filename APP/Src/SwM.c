#include "SwM.h"
#include "Telemetry.h"
#include "App_Ctrl.h"
#include "OLED.h"
#include "IWDG.h"

/*------------------------------------------------------------------
 *  OS task wrappers — each function maps to one scheduler slot
 *------------------------------------------------------------------*/

void OS_5ms_Task(void)
{
    OLED_APP();
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
}

void OS_100ms_Task(void)
{
    IWDG_Refresh();
}

void OS_1000ms_Task(void)
{
    static uint8 tick = 0;
    tick++;

    LifeCounter();              /* every 1 s */

    if (tick % 5  == 0) INTERNAL_TEMP_TASK();  /* every 5 s  */
    if (tick % 10 == 0) RUN_TIME();             /* every 10 s */
    if (tick % 30 == 0) STACK_MONITOR();        /* every 30 s */
    if (tick == 1)      SW_VERSION();           /* once on first tick */
}

void OS_IDLE_TASK(void)
{
    __asm("NOP");
}
