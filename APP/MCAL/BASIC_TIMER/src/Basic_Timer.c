#include "Basic_Timer.h"


/* === Static callbacks and tick counters === */
static TimerCallback_t tim6_callback = 0;
static TimerCallback_t tim7_callback = 0;

volatile uint32 total_ticks = 0;

volatile NVIC_Status_t nvic_stat= NVIC_NOK;

uint32 static GET_TIMER_BASE_ADD(Timer_t value)
{
    if(value == TIMER6 )
    {
        return 0x40001000   ;
    }
    else if (value == TIMER7)
    {
        return  0x40001400  ;
    }
}
void Timer_Init(Timer_t timer)
{
    
    uint32 Base = GET_TIMER_BASE_ADD(timer);
    switch(timer)
    {
        case TIMER6:
            RCC_EnableClock(RCC_APB1,APB1_TIM6 );
            TIM_PSC(Base)  = TIM6_PRESCALER;
            TIM_ARR(Base)  = TIM6_RELOAD;
            TIM_DIER(Base) |= TIM_DIER_UIE;
            nvic_stat=NVIC_EnableInterrupt(TIM6_IRQn);
            break;

        case TIMER7:
            RCC_EnableClock(RCC_APB1,APB1_TIM7);   
            TIM_PSC(Base)  = TIM7_PRESCALER;
            TIM_ARR(Base)  = TIM7_RELOAD;
            TIM_DIER(Base) |= TIM_DIER_UIE;
           nvic_stat= NVIC_EnableInterrupt(TIM7_IRQn);

            break;
        default : break;    
    }
}

void Timer_Start(Timer_t timer)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);
    switch(timer) {
        case TIMER6: TIM_CR1(Base) |= TIM_CR1_CEN; break;
        case TIMER7: TIM_CR1(Base) |= TIM_CR1_CEN; break;
    }
}

void Timer_Stop(Timer_t timer)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);
    switch(timer) {
        case TIMER6: TIM_CR1(Base) &= ~TIM_CR1_CEN; break;
        case TIMER7: TIM_CR1(Base) &= ~TIM_CR1_CEN; break;
    }
}

void Timer_SetCallback(Timer_t timer, TimerCallback_t cb)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);
    switch(timer) {
        case TIMER6: tim6_callback = cb; break;
        case TIMER7: tim7_callback = cb; break;
    }
}

/* === IRQ Handlers === */
void TIM6_DAC_IRQHandler(void)
{
    if (TIM_SR(TIMER6_BASE) & TIM_SR_UIF) 
    {
        TIM_SR(TIMER6_BASE) &= ~TIM_SR_UIF;
        if (tim6_callback) tim6_callback();
    }
}

void TIM7_IRQHandler(void)
{
    if (TIM_SR(TIMER7_BASE) & TIM_SR_UIF) 
    {
        TIM_SR(TIMER7_BASE) &= ~TIM_SR_UIF;
        if (tim7_callback) tim7_callback();
    }
}
