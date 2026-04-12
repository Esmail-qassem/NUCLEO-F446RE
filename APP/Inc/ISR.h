#ifndef ISR_H_
#define ISR_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  UART interrupt service routines
 *------------------------------------------------------------------*/
void UART1_ISR(uint8 num);
void UART2_ISR(uint8 num);

#endif /* ISR_H_ */
