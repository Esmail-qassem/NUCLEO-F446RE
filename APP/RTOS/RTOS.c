#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RTOS.h"
#include "SysTick_interface.h"
#include "PendSV.h"
#include "UART.h"

/*==============================================================================
 *  Private Variables
 *============================================================================*/

static Task_t SysTask[TASK_NUMBER] = {{0}};
static TCB_t  IdleTCB;                        /* idle task TCB (has its own stack) */

TCB_t *CurrentTask = NULL;
TCB_t *NextTask    = NULL;

/*==============================================================================
 *  Private Function Prototypes
 *============================================================================*/

static void RTOS_voidInitStack(TCB_t *tcb, void (*func)(void), uint8 arg);
static void RTOS_voidTickCallback(void);
static void RTOS_voidSchedular(void);
static void RTOS_TaskEntry(uint8 priority);
static void RTOS_voidIdleTask(void);

/*==============================================================================
 *  OS Control
 *============================================================================*/

void RTOS_voidStart(void)
{
    PendSV_Init();
    SysTick_voidInit();
    SysTick_voidSetIntervalPeriodoc(TICKS_PER_MS, &RTOS_voidTickCallback);

    /* Initialize and launch the idle task first */
    RTOS_voidInitStack(&IdleTCB, RTOS_voidIdleTask, 0);
    CurrentTask = &IdleTCB;

    __asm volatile ("SVC #0");

    /* Never reaches here */
    while (1);
}

/*==============================================================================
 *  Idle Task
 *============================================================================*/

static void RTOS_voidIdleTask(void)
{
    while (1)
    {
        __asm volatile ("NOP");
    }
}

/*==============================================================================
 *  Task Trampoline
 *
 *  Entry point for ALL user tasks. Wraps the user function in a while(1) so
 *  the task never returns. After each call it suspends itself and yields back
 *  to idle — the scheduler will wake it up on its next period.
 *============================================================================*/

static void RTOS_TaskEntry(uint8 priority)
{
    while (1)
    {
        SysTask[priority].TaskFunc();

        /* Task finished one period — suspend self and yield to idle */
        SysTask[priority].state = SUSPENDED;
        NextTask = &IdleTCB;
        Trigger_PendSV();
    }
}

/*==============================================================================
 *  Stack Initializer  (private helper)
 *
 *  Builds the initial fake exception frame so PendSV / SVC can "return" into
 *  a task as if it had been preempted.
 *
 *  Memory layout (low → high address, i.e. descending stack):
 *
 *    stack_pointer →  [R4 ][R5 ][R6 ][R7 ][R8 ][R9 ][R10][R11]  software frame
 *                     [R0 ][R1 ][R2 ][R3 ][R12][LR ][PC ][xPSR] hardware frame
 *============================================================================*/

static void RTOS_voidInitStack(TCB_t *tcb, void (*func)(void), uint8 arg)
{
    uint32 *top = &tcb->stack[TASK_STACK_SIZE - 1];

    /* Hardware frame — auto-saved/restored by CPU on exception entry/exit */
    *top-- = 0x01000000; /* xPSR  — Thumb bit set                        */
    *top-- = (uint32)func; /* PC  — entry point                           */
    *top-- = 0xFFFFFFFD; /* LR   — EXC_RETURN: Thread mode, PSP          */
    *top-- = 0;          /* R12                                           */
    *top-- = 0;          /* R3                                            */
    *top-- = 0;          /* R2                                            */
    *top-- = 0;          /* R1                                            */
    *top-- = (uint32)arg;/* R0   — task priority passed to trampoline     */

    /* Software frame — saved/restored manually by PendSV */
    *top-- = 0;          /* R11 */
    *top-- = 0;          /* R10 */
    *top-- = 0;          /* R9  */
    *top-- = 0;          /* R8  */
    *top-- = 0;          /* R7  */
    *top-- = 0;          /* R6  */
    *top-- = 0;          /* R5  */
    *top-- = 0;          /* R4  */

    tcb->stack_pointer = top + 1; /* points to R4 — bottom of full frame */
}

/*==============================================================================
 *  Scheduler  (runs from SysTick ISR every 1 ms)
 *============================================================================*/

static void RTOS_voidTickCallback(void)
{
    RTOS_voidSchedular();
}

static void RTOS_voidSchedular(void)
{
    uint8   i;
    TCB_t  *nextReady = NULL;

    for (i = 0; i < TASK_NUMBER; i++)
    {
        if (SysTask[i].TaskFunc == NULL)  continue;
        if (SysTask[i].state == REMOVED)  continue;
        if (SysTask[i].state == WAITING)  continue;

        /* READY: period already expired in a previous tick but task hasn't run yet.
         * Do NOT decrement its tick (already reset). Just pick it if nothing else found. */
        if (SysTask[i].state == READY)
        {
            if (&SysTask[i].TCB != CurrentTask && nextReady == NULL)
            {
                nextReady = &SysTask[i].TCB;
            }
            continue;
        }

        /* SUSPENDED: count down to next activation */
        SysTask[i].remaining_ticks--;

        if (SysTask[i].remaining_ticks == 0)
        {
            SysTask[i].remaining_ticks = SysTask[i].periodicity;
            SysTask[i].state           = READY;

            if (nextReady == NULL)
            {
                nextReady = &SysTask[i].TCB;
            }
        }
    }

    if (nextReady != NULL)
    {
        NextTask = nextReady;
        Trigger_PendSV();
    }
}

/*==============================================================================
 *  Task Management
 *============================================================================*/

Task_Status RTOS_voidCreateTask(uint8 Copy_priority, uint16 Copy_periodicity, void (*Copy_pvTaskFunc)(void))
{
    if (Copy_priority >= TASK_NUMBER)
    {
        return TASK_ERROR_INVALID_ID;
    }
    if (Copy_pvTaskFunc == NULL)
    {
        return TASK_ERROR_ALREADY_NULL;
    }

    SysTask[Copy_priority].periodicity     = Copy_periodicity;
    SysTask[Copy_priority].remaining_ticks = Copy_periodicity;
    SysTask[Copy_priority].TaskFunc        = Copy_pvTaskFunc;
    SysTask[Copy_priority].state           = SUSPENDED; /* starts suspended, scheduler wakes it */

    /* PC = trampoline, R0 = priority (passed as arg) */
    RTOS_voidInitStack(&SysTask[Copy_priority].TCB, (void(*)(void))RTOS_TaskEntry, Copy_priority);

    return TASK_OK;
}

Task_Status RTOS_voidDeleteTask(uint8 Copy_priority)
{
    if (Copy_priority >= TASK_NUMBER)
    {
        return TASK_ERROR_INVALID_ID;
    }
    if (SysTask[Copy_priority].TaskFunc == NULL)
    {
        return TASK_ERROR_ALREADY_NULL;
    }

    SysTask[Copy_priority].TaskFunc        = NULL;
    SysTask[Copy_priority].state           = REMOVED;
    SysTask[Copy_priority].periodicity     = 0;
    SysTask[Copy_priority].remaining_ticks = 0;

    return TASK_OK;
}

/*==============================================================================
 *  Task State Control
 *============================================================================*/

void RTOS_voidSuspendTask(uint8 Copy_priority)
{
    SysTask[Copy_priority].state = SUSPENDED;
}

void RTOS_voidResumeTask(uint8 Copy_priority)
{
    SysTask[Copy_priority].state = READY;
}

void RTOS_voidWaitEvent(uint8 Copy_priority)
{
    SysTask[Copy_priority].state = WAITING;
}

/*==============================================================================
 *  ISR Handlers
 *============================================================================*/

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile (

        /* --- Safety checks -------------------------------------------- */
        "LDR     R3, =NextTask          \n"
        "LDR     R0, [R3]               \n"   /* R0 = NextTask                */
        "CBZ     R0, 0f                 \n"   /* NULL → skip                  */
        "LDR     R3, =CurrentTask       \n"
        "LDR     R1, [R3]               \n"   /* R1 = CurrentTask             */
        "CMP     R0, R1                 \n"
        "BEQ     0f                     \n"   /* same task → skip             */

        /* --- Save current task context --------------------------------- */
        "MRS     R2, PSP                \n"
        "STMDB   R2!, {R4-R11}          \n"   /* push software frame onto PSP */
        "STR     R2, [R1]               \n"   /* CurrentTask->stack_pointer   */

        /* --- Switch CurrentTask = NextTask ----------------------------- */
        "STR     R0, [R3]               \n"   /* CurrentTask = NextTask       */

        /* --- Restore next task context --------------------------------- */
        "LDR     R2, [R0]               \n"   /* R2 = NextTask->stack_pointer */
        "LDMIA   R2!, {R4-R11}          \n"   /* pop software frame           */
        "MSR     PSP, R2                \n"   /* set PSP → hardware frame     */

        /* --- EXC_RETURN: hardware pops hardware frame from PSP --------- */
        "0:                             \n"
        "BX      LR                     \n"
    );
}

__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile (
        /* R0 = CurrentTask->stack_pointer (offset 0 of TCB) */
        "LDR  R0, =CurrentTask          \n"
        "LDR  R0, [R0]                  \n"   /* R0 = CurrentTask (TCB ptr)   */
        "LDR  R0, [R0]                  \n"   /* R0 = stack_pointer           */

        /* Skip the software frame (R4-R11 = 8 × 4 = 32 bytes)            */
        /* so PSP points directly at the hardware frame                    */
        "ADD  R0, R0, #32               \n"

        /* Set PSP and switch Thread mode to use PSP */
        "MSR  PSP, R0                   \n"
        "MOV  R0,  #0x02                \n"
        "MSR  CONTROL, R0               \n"
        "ISB                            \n"

        /* EXC_RETURN — hardware pops hardware frame from PSP → entry point */
        "MOV  LR,  #0xFFFFFFFD          \n"
        "BX   LR                        \n"
    );
}

void HardFault_Handler(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"HardFault\r\n", 11);
    while (1);
}

void MemManage_Handler(void)
{
    UART_SendSyncBuffer(UART2, (uint8 *)"MemManage Fault\r\n", 17);
    while (1);
}
