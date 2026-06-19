#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RTOS.h"
#include "SysTick_interface.h"
#include "PendSV.h"
#include "UART.h"

/*==============================================================================
 *  DWT cycle counter  (ARM Cortex-M4 Data Watchpoint and Trace unit)
 *  Used to measure exact CPU cycles consumed by each task per second.
 *============================================================================*/
#define DWT_CTRL   (*((volatile uint32*)0xE0001000U))  /* bit 0  = CYCCNTENA */
#define DWT_CYCCNT (*((volatile uint32*)0xE0001004U))  /* 32-bit cycle count */
#define DEM_CR     (*((volatile uint32*)0xE000EDFCU))  /* bit 24 = TRCENA    */
#define CPU_FREQ_HZ 180000000UL                         /* PLL SYSCLK = 180 MHz */

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
    /* Enable DWT cycle counter */
    DEM_CR     |= (1UL << 24);  /* enable trace subsystem */
    DWT_CYCCNT  = 0U;           /* reset counter          */
    DWT_CTRL   |= (1UL << 0);   /* start counting cycles  */

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
        OS_IDLE_TASK();
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
        uint32 start = DWT_CYCCNT;

        SysTask[priority].TaskFunc();

        /* Accumulate cycles spent inside this task's body */
        SysTask[priority].exec_cycles += (DWT_CYCCNT - start);

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

    /* Fill unused stack (below the initial frame) with watermark pattern
     * so RTOS_u8GetStackUsage() can measure high-water mark at runtime */
    uint32 *p = tcb->stack;
    while (p <= top)
    {
        *p++ = 0xDEADBEEFU;
    }
}

/*==============================================================================
 *  Stack watermark — returns % of stack used by task (0–100)
 *  Scans from the bottom looking for first overwritten watermark word.
 *============================================================================*/

uint8 RTOS_u8GetStackUsage(uint8 priority)
{
    if (priority >= TASK_NUMBER || SysTask[priority].TaskFunc == NULL)
    {
        return 0U;
    }
    const uint32 *stack = SysTask[priority].TCB.stack;
    uint16 i;
    for (i = 0U; i < TASK_STACK_SIZE; i++)
    {
        if (stack[i] != 0xDEADBEEFU) break;
    }
    /* i = lowest touched index; everything from i to top is "used" */
    return (uint8)(((uint32)(TASK_STACK_SIZE - i) * 100U) / TASK_STACK_SIZE);
}

/*==============================================================================
 *  Scheduler  (runs from SysTick ISR every 1 ms)
 *============================================================================*/

static void RTOS_voidTickCallback(void)
{
    static uint16 load_tick = 0U;

    RTOS_voidSchedular();

    load_tick++;
    if (load_tick >= 1000U)   /* 1000 × 1ms = 1-second measurement window */
    {
        uint8 i;
        load_tick = 0U;

        for (i = 0U; i < TASK_NUMBER; i++)
        {
            if (SysTask[i].TaskFunc != NULL)
            {
                /* load% = exec_cycles / (CPU_FREQ_HZ / 100)
                 * Divide first to avoid uint32 overflow: exec_cycles can be
                 * as large as CPU_FREQ_HZ itself (a task using ~100% CPU). */
                SysTask[i].cpu_load  = (uint8)(
                    SysTask[i].exec_cycles / (CPU_FREQ_HZ / 100UL)
                );
                SysTask[i].exec_cycles = 0U;  /* reset for next window */
            }
        }
    }
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
 *  CPU load getters
 *============================================================================*/

uint8 RTOS_u8GetTaskCPULoad(uint8 priority)
{
    if (priority >= TASK_NUMBER || SysTask[priority].TaskFunc == NULL)
        return 0U;
    return SysTask[priority].cpu_load;
}

uint8 RTOS_u8GetCPULoad(void)
{
    uint8 i;
    uint16 total = 0U;
    for (i = 0U; i < TASK_NUMBER; i++)
    {
        if (SysTask[i].TaskFunc != NULL)
            total += SysTask[i].cpu_load;
    }
    return (total > 100U) ? 100U : (uint8)total;
}

/*==============================================================================
 *  Tick restart — call after waking from Stop mode to resume the scheduler
 *============================================================================*/
void RTOS_voidRestartTick(void)
{
    SysTick_voidSetIntervalPeriodoc(TICKS_PER_MS, &RTOS_voidTickCallback);
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
