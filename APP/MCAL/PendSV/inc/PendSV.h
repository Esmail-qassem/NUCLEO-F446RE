#ifndef PENDSV_H
#define PENDSV_H
#include "STD_TYPES.h"

#define SCB_SHPR3 *((volatile uint32*)0xE000ED20)
#define SCB_ICSR  *((volatile uint32*)0xE000ED04)

#define PENDSV_PRIORITY 0xFF
void PendSV_Init(void);
void Trigger_PendSV(void);
Status_t PendSV_CallBack(void (*Copy_pvCallBackFunc)(void));


#endif