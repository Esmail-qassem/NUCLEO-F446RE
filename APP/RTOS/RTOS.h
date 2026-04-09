#ifndef RTOS_INTERFACE_H_
#define RTOS_INTERFACE_H_

#include "STD_TYPES.h"

/*==============================================================================
 *  Configuration
 *============================================================================*/

#define TASK_NUMBER   5u
#define TASK_STACK_SIZE    128u

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
} Task_t;

/*==============================================================================
 *  OS Control
 *============================================================================*/

void RTOS_voidStart(void);

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
uint8 RTOS_u8GetCPULoad(void);

#endif /* RTOS_INTERFACE_H_ */
