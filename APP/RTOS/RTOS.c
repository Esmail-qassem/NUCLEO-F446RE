#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RTOS.h"
#include "SysTick_interface.h"
#include "PendSV.h"
task_type SysTask[TASK_NUMBER]={{0}};
cpu_load_type CPU_Load = {0};
volatile uint8 OS_TickFlag = 0;


uint32 TaskStacks[TASK_NUMBER][STACK_SIZE];

uint32 *CurrentTask ;
uint32 *NextTask ;
void TaskExitFunction(void) {
    while(1);  // Infinite loop - never actually returns
}

void RTOS_SCHEDULAR_FLAG(void)
{
    OS_TickFlag = 1;
    Trigger_PendSV();
}
void Save_task_context(void)
{
// Get current PSP
// Push SW frame (pre-decrement)
// Update PSP
// Get CurrentTask pointer variable
// Get actual CurrentTask value
// Save PSP in CurrentTask->pStackTop
// Return
    __asm volatile("MRS R0, PSP\n"\
                    "STMDB R0!, {R4-R11}\n"\
                    "MSR PSP, R0\n"\
                    "LDR R1, =CurrentTask\n"\
                    "LDR R2, [R1]\n"\
                    "STR R0, [R2]\n"\
                    "BX LR\n");


}
void Check_next_task(void) {
    static uint8 current_priority = 0;    
    // Round-robin through READY tasks
    for(uint8 i = 0; i < TASK_NUMBER; i++) {
        current_priority++;
        if(current_priority >= TASK_NUMBER) 
            current_priority = 0;
            
        if(SysTask[current_priority].state == READY) {
            NextTask = SysTask[current_priority].pStackTop;
            return;
        }
    }
    
    // No task ready? Use idle task
    NextTask = OS_IDLE_TASK;
}
void Restore_task_context(void)
{
// Get NextTask pointer variable
// Get actual NextTask value
// Get NextTask->pStackTop
// Restore PSP
// Restore SW frame (post-increment)
// Return (hardware restores HW frame)
    __asm volatile("LDR R1, =NextTask\n"\
                    "LDR R2, [R1]\n"\
                    "LDR R0, [R2]\n"\
                    "MSR PSP, R0\n"\
                    "LDMIA R0!, {R4-R11}\n"\
                    "BX LR\n");


}
void PendSV_Handle(void)
{
__asm volatile("BL Save_task_context\n"\
        "BL Check_next_task\n"\
        "BL Restore_task_context\n"\
        "BX LR\n");

}
void RTOS_voidStart(void)
{
	SysTick_voidInit();
    PendSV_CallBack(PendSV_Handle);
    CurrentTask = SysTask[0].pStackTop;  // Start with first task

    SysTick_voidSetIntervalPeriodoc(TICKS_PER_MS,&RTOS_SCHEDULAR_FLAG);
    while(1)
    {
        if (OS_TickFlag)
        {
            OS_TickFlag = 0;
            RTOS_voidSchedular();
        }
    }
}

Task_status RTOS_voidCreateTask(uint8 Copy_priority,uint16 Copy_periodicity,void(*Copy_pvTaskFunc)(void))
{
    uint32* StackPointer=NULL;
	if (Copy_priority >= TASK_NUMBER) 
	{
    	return TASK_ERROR_INVALID_ID;
    }
    if (Copy_pvTaskFunc == NULL) 
	{
        return TASK_ERROR_ALREADY_NULL;
    }
	SysTask[Copy_priority].periodicity = Copy_periodicity;
	SysTask[Copy_priority].remaining_ticks = Copy_periodicity;
	SysTask[Copy_priority].TaskFunc = Copy_pvTaskFunc;
	SysTask[Copy_priority].state = READY;
    StackPointer =&TaskStacks[Copy_priority][STACK_SIZE -1u];

    StackPointer-=8;
    HW_STACK_FRAME_t *pHw= (HW_STACK_FRAME_t*)StackPointer;
    pHw->r0 = 0;
    pHw->r1 = 0;
    pHw->r2 = 0;
    pHw->r3 = 0;
    pHw->r12 = 0 ;
    pHw->lr =(uint32)TaskExitFunction;
    pHw->pc =(uint32)SysTask[Copy_priority].TaskFunc;
    pHw->psr =0x01000000; //thumb instruction

    StackPointer-=8;
    SW_STACK_FRAME_t *pSw= StackPointer;
    pSw->r4 = 0;
    pSw->r5 = 0;
    pSw->r6 = 0;
    pSw->r7 = 0;
    pSw->r8 = 0;
    pSw->r9 = 0;
    pSw->r10 = 0;
    pSw->r11 = 0;
    SysTask[Copy_priority].pStackTop = StackPointer;

	return TASK_OK;
}

void RTOS_voidSchedular(void) {
    
    for(uint8 i = 0; i < TASK_NUMBER; i++) {
        if(SysTask[i].state == READY && SysTask[i].periodicity > 0) {
            SysTask[i].remaining_ticks--;
            
            if(SysTask[i].remaining_ticks == 0) {
                SysTask[i].remaining_ticks = SysTask[i].periodicity;
                // Task is ready to run - PendSV will pick it
            }
        }
    }
}



uint8 RTOS_u8GetCPULoad(void)
{
    return CPU_Load.cpu_load_percent;
}

Task_status RTOS_voidDeleteTask(uint8 Copy_priority)
{
	if (Copy_priority >= TASK_NUMBER) 
	{
    	return TASK_ERROR_INVALID_ID;
    }
    if (SysTask[Copy_priority].TaskFunc == NULL) 
	{
        return TASK_ERROR_ALREADY_NULL;
    }

    SysTask[Copy_priority].TaskFunc = NULL;
    SysTask[Copy_priority].state = REMOVED; 
    SysTask[Copy_priority].periodicity = 0;    
    SysTask[Copy_priority].remaining_ticks = 0;    

    return TASK_OK;
}


void RTOS_voidSuspendTask(uint8 Copy_priority)
{
	SysTask[Copy_priority].state=SUSPENDED;
}
void RTOS_voidResumeTask(uint8 Copy_priority)
{
	SysTask[Copy_priority].state=READY;
}
void RTOS_voidWaitEvent(uint8 Copy_priority)
{
	SysTask[Copy_priority].state=WAITING;
}