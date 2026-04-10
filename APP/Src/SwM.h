#ifndef SwM_H
#define SwM_H
#include "STD_TYPES.h"
/**************************************************************/
/*           function prototype              */
void GPIO_PIN_CONFIG(void);
void APP_init(void);
void ENABLE_NVIC_INTERRUPTS(void);
void CallBackFunctions(void);
void OS_5ms_Task(void);
void OS_10ms_Task(void);
void OS_20ms_Task(void);
void OS_50ms_Task(void);
void OS_100ms_Task(void);
void OS_1000ms_Task(void);
/**************************************************************/
/**************************************************************/
/*Function Prototypes*/
void LED(void);
void SYS_Reset(void);
void LifeCounter(void);
void SW_VERSION(void);
void LDR_TASK(void);
void UART1_ISR(uint8 num);
void UART2_ISR(uint8 num);
void INTERNAL_TEMP_TASK(void);
void duty_cycle_task(void);

#endif /* SwM_H */