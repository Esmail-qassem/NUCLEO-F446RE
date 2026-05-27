#include "RTOS.hpp"
#include "SysTick.hpp"
#include "PendSV.hpp"
#include "UART.hpp"

/* ── Private storage ──────────────────────────────────────────────────── */
static Task_t SysTask[RTOS_TASK_COUNT] = {};
static TCB_t  IdleTCB;

extern "C" TCB_t *CurrentTask = nullptr;
extern "C" TCB_t *NextTask    = nullptr;

/* ── Forward declarations ─────────────────────────────────────────────── */
static void initStack   (TCB_t *tcb, void(*fn)(void), uint8 arg);
static void tickCallback(void);
static void scheduler   (void);
static void taskEntry   (uint8 priority);
static void idleTask    (void);

/* ── RTOS::Start ──────────────────────────────────────────────────────── */
void RTOS::Start(void)
{
    PendSV::Init();
    SysTick::Init();
    SysTick::SetIntervalPeriodic(TICKS_PER_MS, &tickCallback);

    initStack(&IdleTCB, idleTask, 0u);
    CurrentTask = &IdleTCB;

    __asm volatile("SVC #0");
    while (1) {}
}

/* ── Idle task ────────────────────────────────────────────────────────── */
static void idleTask(void)
{
    while (1) { OS_IDLE_TASK(); }
}

/* ── Task entry trampoline ────────────────────────────────────────────── */
static void taskEntry(uint8 priority)
{
    while (1)
    {
        SysTask[priority].TaskFunc();
        SysTask[priority].state = TaskState::SUSPENDED;
        NextTask = &IdleTCB;
        PendSV::Trigger();
    }
}

/* ── Stack initializer ────────────────────────────────────────────────── */
static void initStack(TCB_t *tcb, void(*fn)(void), uint8 arg)
{
    uint32 *top = &tcb->stack[RTOS_TASK_STACK_SIZE - 1u];

    *top-- = 0x01000000u;          /* xPSR  */
    *top-- = reinterpret_cast<uint32>(fn); /* PC    */
    *top-- = 0xFFFFFFFDu;          /* LR    */
    *top-- = 0u;                   /* R12   */
    *top-- = 0u;                   /* R3    */
    *top-- = 0u;                   /* R2    */
    *top-- = 0u;                   /* R1    */
    *top-- = static_cast<uint32>(arg); /* R0 */
    *top-- = 0u; *top-- = 0u; *top-- = 0u; *top-- = 0u;
    *top-- = 0u; *top-- = 0u; *top-- = 0u; *top-- = 0u;

    tcb->stack_pointer = top + 1u;

    uint32 *p = tcb->stack;
    while (p <= top) *p++ = 0xDEADBEEFu;
}

/* ── Stack usage watermark ────────────────────────────────────────────── */
uint8 RTOS::GetStackUsage(uint8 priority)
{
    if (priority >= RTOS_TASK_COUNT || SysTask[priority].TaskFunc == nullptr)
        return 0u;
    const uint32 *stack = SysTask[priority].TCB.stack;
    uint16 i = 0u;
    for (; i < RTOS_TASK_STACK_SIZE; i++)
        if (stack[i] != 0xDEADBEEFu) break;
    return static_cast<uint8>(((RTOS_TASK_STACK_SIZE - i) * 100u) / RTOS_TASK_STACK_SIZE);
}

/* ── Scheduler (1 ms tick) ────────────────────────────────────────────── */
static void tickCallback(void) { scheduler(); }

static void scheduler(void)
{
    TCB_t *nextReady = nullptr;
    for (uint8 i = 0u; i < RTOS_TASK_COUNT; i++)
    {
        if (!SysTask[i].TaskFunc)                          continue;
        if (SysTask[i].state == TaskState::REMOVED)        continue;
        if (SysTask[i].state == TaskState::WAITING)        continue;

        if (SysTask[i].state == TaskState::READY)
        {
            if (&SysTask[i].TCB != CurrentTask && !nextReady)
                nextReady = &SysTask[i].TCB;
            continue;
        }

        if (--SysTask[i].remaining_ticks == 0u)
        {
            SysTask[i].remaining_ticks = SysTask[i].periodicity;
            SysTask[i].state           = TaskState::READY;
            if (!nextReady) nextReady  = &SysTask[i].TCB;
        }
    }
    if (nextReady) { NextTask = nextReady; PendSV::Trigger(); }
}

/* ── Task management ──────────────────────────────────────────────────── */
TaskStatus RTOS::CreateTask(uint8 priority, uint16 period, void(*fn)(void))
{
    if (priority >= RTOS_TASK_COUNT) return TaskStatus::ERROR_INVALID_ID;
    if (!fn)                         return TaskStatus::ERROR_ALREADY_NULL;
    SysTask[priority].periodicity     = period;
    SysTask[priority].remaining_ticks = period;
    SysTask[priority].TaskFunc        = fn;
    SysTask[priority].state           = TaskState::SUSPENDED;
    initStack(&SysTask[priority].TCB, reinterpret_cast<void(*)(void)>(taskEntry), priority);
    return TaskStatus::OK;
}

TaskStatus RTOS::DeleteTask(uint8 priority)
{
    if (priority >= RTOS_TASK_COUNT)        return TaskStatus::ERROR_INVALID_ID;
    if (!SysTask[priority].TaskFunc)        return TaskStatus::ERROR_ALREADY_NULL;
    SysTask[priority].TaskFunc        = nullptr;
    SysTask[priority].state           = TaskState::REMOVED;
    SysTask[priority].periodicity     = 0u;
    SysTask[priority].remaining_ticks = 0u;
    return TaskStatus::OK;
}

void RTOS::SuspendTask(uint8 p) { SysTask[p].state = TaskState::SUSPENDED; }
void RTOS::ResumeTask (uint8 p) { SysTask[p].state = TaskState::READY;     }
void RTOS::WaitEvent  (uint8 p) { SysTask[p].state = TaskState::WAITING;   }
uint8 RTOS::GetCPULoad(void)    { return 0u; }

/* ── ISR handlers (naked — assembly must see C-linkage symbols) ───────── */
extern "C" __attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile (
        "LDR     R3, =NextTask          \n"
        "LDR     R0, [R3]               \n"
        "CBZ     R0, 0f                 \n"
        "LDR     R3, =CurrentTask       \n"
        "LDR     R1, [R3]               \n"
        "CMP     R0, R1                 \n"
        "BEQ     0f                     \n"
        "MRS     R2, PSP                \n"
        "STMDB   R2!, {R4-R11}          \n"
        "STR     R2, [R1]               \n"
        "STR     R0, [R3]               \n"
        "LDR     R2, [R0]               \n"
        "LDMIA   R2!, {R4-R11}          \n"
        "MSR     PSP, R2                \n"
        "0:                             \n"
        "BX      LR                     \n"
    );
}

extern "C" __attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile (
        "LDR  R0, =CurrentTask          \n"
        "LDR  R0, [R0]                  \n"
        "LDR  R0, [R0]                  \n"
        "ADD  R0, R0, #32               \n"
        "MSR  PSP, R0                   \n"
        "MOV  R0,  #0x02                \n"
        "MSR  CONTROL, R0               \n"
        "ISB                            \n"
        "MOV  LR,  #0xFFFFFFFD          \n"
        "BX   LR                        \n"
    );
}

extern "C" void HardFault_Handler(void)
{
    UART::SendSyncBuffer(UART_HardWare::UART2, reinterpret_cast<const uint8*>("HardFault\r\n"), 11u);
    while (1) {}
}

extern "C" void MemManage_Handler(void)
{
    UART::SendSyncBuffer(UART_HardWare::UART2, reinterpret_cast<const uint8*>("MemManage Fault\r\n"), 17u);
    while (1) {}
}
