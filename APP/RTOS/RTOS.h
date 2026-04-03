#ifndef RTOS_INTERFACE_H_
#define RTOS_INTERFACE_H_

#define TASK_NUMBER  4
#define STACK_SIZE   64

typedef enum
{
    READY ,
    SUSPENDED,
    WAITING ,
    REMOVED
}
Task_States;

typedef enum {
    TASK_OK,
    TASK_ERROR_INVALID_ID,
    TASK_ERROR_ALREADY_NULL
} Task_status;

typedef struct {
    uint32 total_ticks;
    uint32 idle_ticks;
    uint8 cpu_load_percent;
} cpu_load_type;

/* stack frame that saved by hardware*/
typedef struct
{
    uint32 r0;
    uint32 r1;
    uint32 r2;
    uint32 r3;
    uint32 r12;
    uint32 lr;
    uint32 pc;
    uint32 psr;
}HW_STACK_FRAME_t;

/* stack frame that saved by Software*/
typedef struct
{
    uint32 r4;
    uint32 r5;
    uint32 r6;
    uint32 r7;
    uint32 r8;
    uint32 r9;
    uint32 r10;
    uint32 r11;
}SW_STACK_FRAME_t;
typedef struct
{
    uint16 periodicity;
    uint16 remaining_ticks;
    Task_States state;
    void(*TaskFunc)(void);
    uint32 *pStackTop;
}task_type;



void OS_IDLE_TASK(void);
Task_status RTOS_voidCreateTask(uint8 Copy_priority,uint16 Copy_priodicity,void(*Copy_pvTaskFunc)(void));
void RTOS_voidSchedular(void);
void RTOS_voidStart(void);
Task_status RTOS_voidDeleteTask(uint8 Copy_priority);
void RTOS_voidSuspendTask(uint8 Copy_priority);
void RTOS_voidResumeTask(uint8 Copy_priority);
void RTOS_voidWaitEvent(uint8 Copy_priority);
uint8 RTOS_u8GetCPULoad(void);
#endif /* RTOS_INTERFACE_H_ */




