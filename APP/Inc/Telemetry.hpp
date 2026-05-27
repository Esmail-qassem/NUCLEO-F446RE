#pragma once
#include "STD_TYPES.h"
#include "RTC.h"

extern RTC_Time_t Get_Time;

void LifeCounter(void);
void SW_VERSION(void);
void RUN_TIME(void);
void INTERNAL_TEMP_TASK(void);
void LDR_TASK(void);
void STACK_MONITOR(void);
void BOOT_REASON_REPORT(void);
