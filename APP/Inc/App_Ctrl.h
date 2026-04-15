#ifndef APP_CTRL_H_
#define APP_CTRL_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  Function prototypes
 *------------------------------------------------------------------*/
void LED(void);
void LED_ON(void);
void LED_OFF(void);
void SYS_Reset(void);
void BTLD_Jump(void);
void BTLD_Update(void);
void PWM_SET(uint8 duty);
void duty_cycle_task(void);

#endif /* APP_CTRL_H_ */
