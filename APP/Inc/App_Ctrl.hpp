#pragma once
#include "STD_TYPES.h"

extern uint8 LED_Global;

void LED(void);
void LED_ON(void);
void LED_OFF(void);
void SYS_Reset(void);
void BTLD_Jump(void);
void BTLD_Update(void);
void PWM_SET(uint8 duty);
void duty_cycle_task(void);
