#ifndef RTOS_INTERFACE_H_
#define RTOS_INTERFACE_H_

#define TASK_NUMBER  4


typedef enum
{
    READY ,
    SUSPENDED,
    WAITING ,
    REMOVED
}
Task_States;

typedef struct
{
    uint16 periodicity;
    uint16 remaining_ticks;
    Task_States state;
    void(*TaskFunc)(void);
}task_type;


typedef enum {
    TASK_OK,
    TASK_ERROR_INVALID_ID,
    TASK_ERROR_ALREADY_NULL
} Task_status;


Task_status RTOS_voidCreateTask(uint8 Copy_priority,uint16 Copy_priodicity,void(*Copy_pvTaskFunc)(void));
void RTOS_voidSchedular(void);
void RTOS_voidStart(void);
Task_status RTOS_voidDeleteTask(uint8 Copy_priority);
void RTOS_voidSuspendTask(uint8 Copy_priority);
void RTOS_voidResumeTask(uint8 Copy_priority);
void RTOS_voidWaitEvent(uint8 Copy_priority);
#endif /* RTOS_INTERFACE_H_ */




