/*
 * SysTick_interface.h
 *
 *  Created on: Feb 19, 2024
 *      Author: s_a_a
 */

#ifndef SYSTICK_INTERFACE_H_
#define SYSTICK_INTERFACE_H_
#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "SysTick_config.h"


static void(*Local_PvFunction)(void)=NULL;

#define AHB_BY_8 0
#define AHB 1


typedef struct
{
	uint32 Sys_Enable    :1; /*Counter enable*/
	uint32 TICKINT   :1; /*SysTick exception request enable*/
	uint32 CLKSOURCE :1; /*Clock source selection*/
    uint32           :13;
    uint32 COUNTFLAG :1 ;
    uint32           :15;
}STK_CTRL;

#define SysTick_Add 0xE000E010

#define STK_CTRL_Reg        ((volatile STK_CTRL*)SysTick_Add)

#define STK_LOAD_Reg       *((volatile uint32*)(SysTick_Add+0x04))
#define STK_VAL_Reg        *((volatile uint32*)(SysTick_Add+0x08))
#define STK_CALIB_Reg      *((volatile uint32*)(SysTick_Add+0x0C))


void SysTick_voidInit(void);

/*Synchronous function*/
void SysTick_voidSetBusyWait(uint32 milliseconds);

/*ASynchronous function*/
/**************************************/
Status_t SysTick_voidSetIntervalSingle(uint32 Copy_uint32TicksCount,void (*Copy_pvfunction)(void));
Status_t SysTick_voidSetIntervalPeriodoc(uint32 Copy_uint32TicksCount,void (*Copy_pvfunction)(void));
/**************************************/
void SysTick_voidStopTimer(void);
uint32 SysTick_GetElapsedTime(void);
uint32 SysTick_GetRemaningTime(void);


#endif /* INC_SYSTICK_INTERFACE_H_ */
