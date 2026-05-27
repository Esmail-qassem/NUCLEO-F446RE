#include "NVIC.hpp"
/* Legacy shims */
#define NVIC_EnableInterrupt(n)        NVIC::EnableInterrupt(n)
#define NVIC_DisableInterrupt(n)       NVIC::DisableInterrupt(n)
#define NVIC_SetPendingFlag(n)         NVIC::SetPendingFlag(n)
#define NVIC_ClearPendingFlag(n)       NVIC::ClearPendingFlag(n)
#define NVIC_GetActiveInterrupt(n,p)   NVIC::GetActiveFlag(n, *(p))
#define NVIC_SetPriority(id,g,s)       NVIC::SetPriority(id, g, s)
/* Status shims */
#define NVIC_OK  NVIC_Status::OK
#define NVIC_NOK NVIC_Status::NOK

/***********************************************************************/
/*Aauthor     : Esamil Qassem                                          */
/*Data        : 3 FEB 2024                                             */
/*SWC         : NVIC                                                   */
/*Version     : V.1                                                    */
/***********************************************************************/

#ifndef NVIC_INTERFACE_H_
#define NVIC_INTERFACE_H_

typedef enum
{
	NVIC_OK,
	NVIC_NOK
}NVIC_Status_t;


NVIC_Status_t NVIC_EnableInterrupt(uint8 Copy_uint8Peripheral);
NVIC_Status_t NVIC_DisableInterrupt(uint8 Copy_uint8Peripheral);

NVIC_Status_t NVIC_SetPendingFlag(uint8 Copy_uint8Peripheral);
NVIC_Status_t NVIC_ClearPendingFlag(uint8 Copy_uint8Peripheral);

NVIC_Status_t NVIC_GetActiveInterrupt(uint8 Copy_uint8Peripheral,uint8 * Copy_pvPeripheral);

void NVIC_SetPriority(sint8 Copy_sint8PriorityId,uint8 Copy_uint8GroupPriority,uint8 Copy_uint8SubGroupPriority);


#endif /* INC_NVIC_INTERFACE_H_ */
