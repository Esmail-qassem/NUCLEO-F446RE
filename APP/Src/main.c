#include "STD_TYPES.h"
#include "App_Config.h"
#include "SwM_Cfg.h"
#include "RTOS.h"

/*------------------------------------------------------------------
 *  Local prototypes
 *------------------------------------------------------------------*/
static void Scheduler(void);

/*------------------------------------------------------------------
 *  main
 *------------------------------------------------------------------*/
int main(void)
{
    APP_Init();
    Scheduler();

    /* RTOS_voidStart() never returns; loop is defensive only. */
    for (;;)
    {
        /* unreachable */
    }
}

/*------------------------------------------------------------------
 *  Scheduler — register all periodic tasks and start the RTOS.
 *------------------------------------------------------------------*/
static void Scheduler(void)
{
    uint8 i;

    for (i = 0U; i < (uint8)TASK_ID_COUNT; i++)
    {
        (void)RTOS_voidCreateTask(SwM_TaskTable[i].priority,
                                  SwM_TaskTable[i].period_ms,
                                  SwM_TaskTable[i].handler);
    }
    RTOS_voidStart();
}

/*------------------------------------------------------------------
 *  SystemInit — called by the reset handler before main().
 *  Prototype provided here to satisfy MISRA Rule 8.4; the symbol
 *  is referenced only from startup assembly.
 *------------------------------------------------------------------*/
void SystemInit(void);
void SystemInit(void)
{
    APP_ClockInit();
}
