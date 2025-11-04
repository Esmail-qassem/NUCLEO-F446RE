#ifndef GP_TIMER_H_
#define GP_TIMER_H_
#include "STD_TYPES.h"
#include "BIT_MATH.h"
#include "RCC.h"
#include "NVIC_interface.h"


/* === Common bit definitions === */
#define TIM_DIER_UIE   (1U << 0)   // Update interrupt enable
#define TIM_CR1_CEN    (1U << 0)   // Counter enable
#define TIM_SR_UIF     (1U << 0)   // Update interrupt flag
#define TIM_SR_TIF     (1U << 6)   // trigger interrupt flag

/* 32bit timers */
#define TIMER2_BASE     0x40000000
#define TIMER5_BASE     0x40000C00
/* 16bit timers */
#define TIMER3_BASE     0x40000400
#define TIMER4_BASE     0x40000800

#define TIM_CR1(base)   *((volatile uint16*)(base + 0x00))
#define TIM_CR2(base)   *((volatile uint16*)(base + 0x04))
#define TIM_SMCR(base)   *((volatile uint16*)(base + 0x08))
#define TIM_DIER(base)  *((volatile uint16*)(base + 0x0C))
#define TIM_SR(base)    *((volatile uint16*)(base + 0x10))
#define TIM_CNT(base)    *((volatile uint16*)(base + 0x24))
#define TIM_PSC(base)   *((volatile uint16*)(base + 0x28))
#define TIM_ARR(base)   *((volatile uint16*)(base + 0x2C))


/* === IRQ numbers from stm32f103 vector table === */
#define TIM2_IRQn      28
#define TIM3_IRQn      29
#define TIM4_IRQn      30
#define TIM5_IRQn      50


/* === Configurable values (1 ms @ 72 MHz APB1/APB2) === */
#define TIM2_PRESCALER   8-1
#define TIM2_RELOAD      (2000-1)
#define TIM3_PRESCALER   8-1
#define TIM3_RELOAD      (2000-1)
#define TIM4_PRESCALER   8-1
#define TIM4_RELOAD      (2000-1)
#define TIM5_PRESCALER   8-1
#define TIM5_RELOAD      (2000-1)
/* === Timer selection enum === */
typedef enum {
    TIMER2=2,
    TIMER3=3,
    TIMER4=4,
    TIMER5=5,
} Timer_t;

/* === User callback type === */
typedef void (*TimerCallback_t)(void);

/* === Functions === */
void Timer_Init(Timer_t timer);
void Timer_Start(Timer_t timer);
void Timer_Stop(Timer_t timer);
void Timer_SetCallback(Timer_t timer, TimerCallback_t cb);


#endif