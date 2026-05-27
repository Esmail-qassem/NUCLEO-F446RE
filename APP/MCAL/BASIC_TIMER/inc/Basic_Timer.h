#include "BasicTimer.hpp"
/* Legacy shims */
#define TIMER6  BasicTimer_ID::TIM6
#define TIMER7  BasicTimer_ID::TIM7
#define Timer_Init(t)         BasicTimer::Init(t)
#define Timer_Start(t)        BasicTimer::Start(t)
#define Timer_Stop(t)         BasicTimer::Stop(t)
#define Timer_SetCallback(t,cb) BasicTimer::SetCallback(t,cb)

#ifndef BASIC_TIMER_H_
#define BASIC_TIMER_H_
#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RCC.h"
#include "NVIC_interface.h"
/* === Common bit definitions === */
#define TIM_DIER_UIE   (1U << 0)   // Update interrupt enable
#define TIM_CR1_CEN    (1U << 0)   // Counter enable
#define TIM_SR_UIF     (1U << 0)   // Update interrupt flag
#define TIM_SR_TIF     (1U << 6)   // trigger interrupt flag

#define TIMER6_BASE     0x40001000
#define TIMER7_BASE     0x40001400

#define TIM_CR1(base)   *((volatile uint16*)(base + 0x00))
#define TIM_CR2(base)   *((volatile uint16*)(base + 0x04))
#define TIM_DIER(base)  *((volatile uint16*)(base + 0x0C))
#define TIM_SR(base)    *((volatile uint16*)(base + 0x10))
#define TIM_CNT(base)    *((volatile uint16*)(base + 0x24))
#define TIM_PSC(base)   *((volatile uint16*)(base + 0x28))
#define TIM_ARR(base)   *((volatile uint16*)(base + 0x2C))


/* === IRQ numbers from stm32f103 vector table === */
#define TIM6_IRQn      54
#define TIM7_IRQn      55

/* === Configurable values (1 ms @ 72 MHz APB1/APB2) === */
#define TIM6_PRESCALER   8-1
#define TIM6_RELOAD      (2000-1)
#define TIM7_PRESCALER   8-1
#define TIM7_RELOAD      (2000-1)

/* === Timer selection enum === */
typedef enum {
    TIMER6 = 6,
    TIMER7

} Timer_t;

/* === User callback type === */
typedef void (*TimerCallback_t)(void);

/* === Functions === */
void Timer_Init(Timer_t timer);
void Timer_Start(Timer_t timer);
void Timer_Stop(Timer_t timer);
void Timer_SetCallback(Timer_t timer, TimerCallback_t cb);


#endif