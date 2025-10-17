

#include "SysTick_interface.h"


void SysTick_voidInit(void)
{
/*Choosing the clock*/
STK_CTRL_Reg->CLKSOURCE=TSK_CLOCK;
}
/*Synchronous function*/
void SysTick_voidSetBusyWait(uint32 milliseconds)
{
	uint32 ticks =milliseconds * TICKS_PER_MS;
	/* Clear the current value register */
	STK_VAL_Reg = NULL;
    /* Load the value */
    STK_LOAD_Reg = ticks;

    /* Enable the timer */
    STK_CTRL_Reg->Sys_Enable = Enable;

    /* Polling */
    while ((STK_CTRL_Reg->COUNTFLAG) == 0);
    /* Stop the timer */
    SysTick_voidStopTimer();
}

/*ASynchronous function*/
/**************************************/
Status_t SysTick_voidSetIntervalSingle(uint32 Copy_uint32TicksCount,void (*Copy_pvfunction)(void))
{
Status_t LocaL_ErrorStatus=E_Not_Ok;
 if(NULL==Copy_pvfunction)
 {
	 LocaL_ErrorStatus=E_Null_Pointer;
 }
 else
 {
	 LocaL_ErrorStatus=E_Ok;
	 Local_PvFunction=Copy_pvfunction;

		/*Load the value*/
		STK_LOAD_Reg=Copy_uint32TicksCount;
		/*enable the interrput*/
		STK_CTRL_Reg->TICKINT=Enable;
		/*enable the timer */
		STK_CTRL_Reg->Sys_Enable=Enable;
 }

return LocaL_ErrorStatus;
}

Status_t SysTick_voidSetIntervalPeriodoc(uint32 Copy_uint32TicksCount,void (*Copy_pvfunction)(void))
{

	Status_t LocaL_ErrorStatus=E_Not_Ok;
	 if(NULL==Copy_pvfunction)
	 {
		 LocaL_ErrorStatus=E_Null_Pointer;
	 }
	 else
	 {
		 LocaL_ErrorStatus=E_Ok;
		 Local_PvFunction=Copy_pvfunction;

			/*Load the value*/
			STK_LOAD_Reg=Copy_uint32TicksCount-1;

			 /* Clear current value register */
        	STK_VAL_Reg = 0;
			/*enable the interrput*/
			STK_CTRL_Reg->TICKINT=Enable;
			/*enable the timer */
			STK_CTRL_Reg->Sys_Enable=Enable;
	 }


	return LocaL_ErrorStatus;
}


void SysTick_voidStopTimer(void)
{
	STK_CTRL_Reg->Sys_Enable=Disable;
}
uint32 SysTick_GetElapsedTime(void)
{
	return (STK_LOAD_Reg-STK_VAL_Reg);

}
uint32 SysTick_GetRemaningTime(void)
{
	return (STK_VAL_Reg);
}





void SysTick_Handler(void)
{
	Local_PvFunction();
}
