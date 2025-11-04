#include "GP_Timer.h"


/* === Static callbacks and tick counters === */
static TimerCallback_t tim2_callback = 0;
static TimerCallback_t tim3_callback = 0;
static TimerCallback_t tim4_callback = 0;
static TimerCallback_t tim5_callback = 0;

volatile uint32 total_ticks = 0;

volatile NVIC_Status_t nvic_stat= NVIC_NOK;

uint32 static GET_TIMER_BASE_ADD(Timer_t value)
{
    switch(value)
    {
        case TIMER2 :  return 0x40000000  ;break;
        case TIMER3 :  return 0x40000400  ;break;
        case TIMER4 :  return 0x40000800  ;break;
        case TIMER5 :  return 0x40000C00  ;break;
        default : break;
    }
}
void Timer_Init(Timer_t timer)
{
    
    uint32 Base = GET_TIMER_BASE_ADD(timer);
    switch(timer)
    {
        case TIMER2:
            RCC_EnableClock(RCC_APB1,APB1_TIM2 );
            TIM_PSC(Base)  = TIM6_PRESCALER;
            TIM_ARR(Base)  = TIM6_RELOAD;
            TIM_DIER(Base) |= TIM_DIER_UIE;
            nvic_stat=NVIC_EnableInterrupt(TIM2_IRQn);
            break;

        case TIMER3:
            RCC_EnableClock(RCC_APB1,APB1_TIM3);   
            TIM_PSC(Base)  = TIM7_PRESCALER;
            TIM_ARR(Base)  = TIM7_RELOAD;
            TIM_DIER(Base) |= TIM_DIER_UIE;
           nvic_stat= NVIC_EnableInterrupt(TIM3_IRQn);

            break;
            case TIMER4:
            RCC_EnableClock(RCC_APB1,APB1_TIM4);   
            TIM_PSC(Base)  = TIM7_PRESCALER;
            TIM_ARR(Base)  = TIM7_RELOAD;
            TIM_DIER(Base) |= TIM_DIER_UIE;
           nvic_stat= NVIC_EnableInterrupt(TIM4_IRQn);

            break;
            case TIMER5:
            RCC_EnableClock(RCC_APB1,APB1_TIM5);   
            TIM_PSC(Base)  = TIM7_PRESCALER;
            TIM_ARR(Base)  = TIM7_RELOAD;
            TIM_DIER(Base) |= TIM_DIER_UIE;
           nvic_stat= NVIC_EnableInterrupt(TIM5_IRQn);
            break;
        default : break;    
    }
}

void Timer_SetCallback(Timer_t timer, TimerCallback_t cb)
{
    switch(timer) {
        case TIMER2: tim2_callback = cb; break;
        case TIMER3: tim3_callback = cb; break;
        case TIMER4: tim4_callback = cb; break;
        case TIMER5: tim5_callback = cb; break;
    }
}

/* === IRQ Handlers === */
void TIM2_IRQHandler(void)
{
    if (TIM_SR(TIMER2_BASE) & TIM_SR_UIF) 
    {
        TIM_SR(TIMER2_BASE) &= ~TIM_SR_UIF;
        if (tim2_callback) tim2_callback();
    }
}
void TIM3_IRQHandler(void)
{
    if (TIM_SR(TIMER3_BASE) & TIM_SR_UIF) 
    {
        TIM_SR(TIMER3_BASE) &= ~TIM_SR_UIF;
        if (tim3_callback) tim3_callback();
    }
}
void TIM4_IRQHandler(void)
{
    if (TIM_SR(TIMER4_BASE) & TIM_SR_UIF) 
    {
        TIM_SR(TIMER4_BASE) &= ~TIM_SR_UIF;
        if (tim4_callback) tim4_callback();
    }
}
void TIM5_IRQHandler(void)
{
    if (TIM_SR(TIMER5_BASE) & TIM_SR_UIF) 
    {
        TIM_SR(TIMER5_BASE) &= ~TIM_SR_UIF;
        if (tim5_callback) tim5_callback();
    }
}


