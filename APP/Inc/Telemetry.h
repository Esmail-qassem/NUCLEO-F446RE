#ifndef TELEMETRY_H_
#define TELEMETRY_H_

#include "STD_TYPES.h"
#include "RTC.h"

/*------------------------------------------------------------------
 *  Shared runtime state
 *------------------------------------------------------------------*/
extern RTC_Time_t Get_Time;

/*------------------------------------------------------------------
 *  Function prototypes
 *------------------------------------------------------------------*/
void LifeCounter(void);
void SW_VERSION(void);
void RUN_TIME(void);
void INTERNAL_TEMP_TASK(void);
void LDR_TASK(void);
void STACK_MONITOR(void);
void CPU_LOAD_TASK(void);
void BOOT_REASON_REPORT(void);

#endif /* TELEMETRY_H_ */
