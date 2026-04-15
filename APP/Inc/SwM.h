#ifndef SWM_H_
#define SWM_H_

#include "STD_TYPES.h"
#include "RTOS.h"

/*------------------------------------------------------------------
 *  OS task wrappers — called by the scheduler
 *------------------------------------------------------------------*/
void OS_5ms_Task(void);
void OS_10ms_Task(void);
void OS_20ms_Task(void);
void OS_50ms_Task(void);
void OS_100ms_Task(void);
void OS_1000ms_Task(void);
void OS_IDLE_TASK(void);

#endif /* SWM_H_ */
