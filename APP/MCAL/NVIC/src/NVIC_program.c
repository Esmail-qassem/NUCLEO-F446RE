/***********************************************************************/
/*Aauthor     : Esamil Qassem                                          */
/*Data        : 3 FEB 2024                                             */
/*SWC         : NVIC                                                   */
/*Version     : V.1                                                    */
/***********************************************************************/
#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "NVIC_interface.h"
#include "NVIC_private.h"
#include "NVIC_register.h"
#include "NVIC_config.h"


NVIC_Status_t NVIC_EnableInterrupt(uint8 Copy_uint8Peripheral)
{
	NVIC_Status_t Local_ErrStatus=NVIC_OK;
if(Copy_uint8Peripheral<32)
{
	NVIC_ISER0=(1<<Copy_uint8Peripheral);
}
else if(Copy_uint8Peripheral<60)
{

	Copy_uint8Peripheral-=32;
	NVIC_ISER1=(1<<Copy_uint8Peripheral);
}
else
{
	Local_ErrStatus=NVIC_NOK;
}

	return Local_ErrStatus;
}
NVIC_Status_t NVIC_DisableInterrupt(uint8 Copy_uint8Peripheral)
{
	NVIC_Status_t Local_ErrStatus=NVIC_OK;
if(Copy_uint8Peripheral<32)
{
	NVIC_ICER0=(1<<Copy_uint8Peripheral);
}
else if(Copy_uint8Peripheral<60)
{

	Copy_uint8Peripheral-=32;
	NVIC_ICER1=(1<<Copy_uint8Peripheral);
}
else
{
	Local_ErrStatus=NVIC_NOK;
}

	return Local_ErrStatus;
}

NVIC_Status_t NVIC_SetPendingFlag(uint8 Copy_uint8Peripheral)
{
	NVIC_Status_t Local_ErrStatus=NVIC_OK;
if(Copy_uint8Peripheral<32)
{
	NVIC_ISPR0=(1<<Copy_uint8Peripheral);
}
else if(Copy_uint8Peripheral<60)
{

	Copy_uint8Peripheral-=32;
	NVIC_ISPR1=(1<<Copy_uint8Peripheral);
}
else
{
	Local_ErrStatus=NVIC_NOK;
}

	return Local_ErrStatus;
}
NVIC_Status_t NVIC_ClearPendingFlag(uint8 Copy_uint8Peripheral)
{
	NVIC_Status_t Local_ErrStatus=NVIC_OK;
if(Copy_uint8Peripheral<32)
{
	NVIC_ICPR0=(1<<Copy_uint8Peripheral);
}
else if(Copy_uint8Peripheral<60)
{

	Copy_uint8Peripheral-=32;
	NVIC_ICPR1=(1<<Copy_uint8Peripheral);
}
else
{
	Local_ErrStatus=NVIC_NOK;
}

	return Local_ErrStatus;
}

NVIC_Status_t NVIC_GetActiveInterrupt(uint8 Copy_uint8Peripheral,uint8* Copy_pvPeripheral)
{
	NVIC_Status_t Local_ErrStatus=NVIC_OK;
if(Copy_uint8Peripheral<32)
{
	*Copy_pvPeripheral=GET_BIT(NVIC_IABR0,Copy_uint8Peripheral);

}
else if(Copy_uint8Peripheral<60)
{

	Copy_uint8Peripheral-=32;
	*Copy_pvPeripheral=GET_BIT(NVIC_IABR1,Copy_uint8Peripheral);
}
else
{
	Local_ErrStatus=NVIC_NOK;
}
	return Local_ErrStatus;
}
void NVIC_SetPriority(sint8 Copy_sint8PriorityId,uint8 Copy_uint8GroupPriority,uint8 Copy_uint8SubPriority)
{
	/*uint8 priority=Copy_uint8SubPriority|(Copy_uint8GroupPriority<<((GROUP-0x05FA0300)/256))  */
	NVIC_SCB_AIRCR_Reg->x=0x05FA0000|(priority_PreBuidConfig<<8);

	uint8 Local_variable=priority_PreBuidConfig -3;

	if(Copy_sint8PriorityId<0)
	{
		/*CORE PERIPHERAL*/

	}else if(Copy_sint8PriorityId > -1)
	{
		*(NVIC_IPRn+Copy_sint8PriorityId)=((Copy_uint8SubPriority)|(Copy_uint8GroupPriority<<(Local_variable)))<<4;
	}
}
