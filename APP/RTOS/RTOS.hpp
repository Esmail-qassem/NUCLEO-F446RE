#pragma once
#include "STD_TYPES.h"

/* ── Config ───────────────────────────────────────────────────────────── */
constexpr uint8  RTOS_TASK_COUNT      = 6u;
constexpr uint16 RTOS_TASK_STACK_SIZE = 128u;

/* Keep old macro for legacy references */
#define TASK_NUMBER RTOS_TASK_COUNT

/* ── Types ────────────────────────────────────────────────────────────── */
enum class TaskState : uint8
{
    READY     = 0,
    SUSPENDED,
    WAITING,
    REMOVED
};

/* Legacy aliases */
constexpr auto READY     = TaskState::READY;
constexpr auto SUSPENDED = TaskState::SUSPENDED;
constexpr auto WAITING   = TaskState::WAITING;
constexpr auto REMOVED   = TaskState::REMOVED;

enum class TaskStatus : uint8
{
    OK                = 0,
    ERROR_INVALID_ID,
    ERROR_ALREADY_NULL
};

/* Legacy aliases */
constexpr auto TASK_OK                = TaskStatus::OK;
constexpr auto TASK_ERROR_INVALID_ID  = TaskStatus::ERROR_INVALID_ID;
constexpr auto TASK_ERROR_ALREADY_NULL= TaskStatus::ERROR_ALREADY_NULL;

struct TCB_t
{
    uint32 *stack_pointer;                    /* MUST be first — assembly offset 0 */
    uint32  stack[RTOS_TASK_STACK_SIZE];
};

struct Task_t
{
    uint16     periodicity;
    uint16     remaining_ticks;
    TaskState  state;
    void     (*TaskFunc)(void);
    TCB_t      TCB;
};

/* ── ISR handlers (accessed by startup vector table) ──────────────────── */
extern "C" {
    void PendSV_Handler (void);
    void SVC_Handler    (void);
    void HardFault_Handler (void);
    void MemManage_Handler (void);
}

/* ── Global pointers used by naked asm (must keep C linkage) ─────────── */
extern "C" TCB_t *CurrentTask;
extern "C" TCB_t *NextTask;

/* ── RTOS class ───────────────────────────────────────────────────────── */
class RTOS
{
public:
    static void       Start      (void);
    static TaskStatus CreateTask (uint8 priority, uint16 period, void(*fn)(void));
    static TaskStatus DeleteTask (uint8 priority);
    static void       SuspendTask(uint8 priority);
    static void       ResumeTask (uint8 priority);
    static void       WaitEvent  (uint8 priority);
    static uint8      GetCPULoad (void);
    static uint8      GetStackUsage(uint8 priority);
    RTOS() = delete;
};

/* user-defined idle hook */
void OS_IDLE_TASK(void);
