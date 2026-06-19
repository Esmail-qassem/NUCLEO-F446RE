#ifndef RTOS_INTERFACE_H_
#define RTOS_INTERFACE_H_

#include "STD_TYPES.h"

/*==============================================================================
 *  Configuration
 *============================================================================*/

#define TASK_NUMBER   6u
#define TASK_STACK_SIZE    256u

/*==============================================================================
 *  Types
 *============================================================================*/

typedef enum
{
    READY     = 0,
    SUSPENDED,
    WAITING,
    REMOVED
} Task_States;

typedef enum
{
    TASK_OK,
    TASK_ERROR_INVALID_ID,
    TASK_ERROR_ALREADY_NULL
} Task_Status;

typedef struct
{
    uint32 *stack_pointer;            /* MUST be first — assembly accesses it at offset 0 */
    uint32  stack[TASK_STACK_SIZE];   /* task's private stack memory */
} TCB_t;


typedef struct
{
    uint16       periodicity;
    uint16       remaining_ticks;
    Task_States  state;
    void       (*TaskFunc)(void);
    TCB_t        TCB;
    uint32       exec_cycles;   /* cycles accumulated in current 1-s window */
    uint8        cpu_load;      /* calculated CPU load % (0–100)            */
} Task_t;

/*==============================================================================
 *  OS Control
 *============================================================================*/

void RTOS_voidStart(void);
void RTOS_voidRestartTick(void);  /* resume scheduler after Stop-mode wake */

/*==============================================================================
 *  Task Management
 *============================================================================*/

Task_Status RTOS_voidCreateTask(uint8 Copy_priority, uint16 Copy_periodicity, void (*Copy_pvTaskFunc)(void));
Task_Status RTOS_voidDeleteTask(uint8 Copy_priority);

/*==============================================================================
 *  Task State Control
 *============================================================================*/

void RTOS_voidSuspendTask(uint8 Copy_priority);
void RTOS_voidResumeTask(uint8 Copy_priority);
void RTOS_voidWaitEvent(uint8 Copy_priority);

/*==============================================================================
 *  Utilities
 *============================================================================*/

void  OS_IDLE_TASK(void);
uint8 RTOS_u8GetCPULoad(void);              /* total system load % (0–100)      */
uint8 RTOS_u8GetTaskCPULoad(uint8 priority);/* per-task CPU load % (0–100)      */
uint8 RTOS_u8GetStackUsage(uint8 priority); /* stack high-water mark % (0–100)  */

#endif /* RTOS_INTERFACE_H_ */
