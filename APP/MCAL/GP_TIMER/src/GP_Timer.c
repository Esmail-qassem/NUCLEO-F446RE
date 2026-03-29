#include "GP_Timer.h"

/* === Static callbacks === */
static TimerCallback_t tim2_callback = 0;
static TimerCallback_t tim3_callback = 0;
static TimerCallback_t tim4_callback = 0;
static TimerCallback_t tim5_callback = 0;

/* === Helper: Get base address === */
static uint32 GET_TIMER_BASE_ADD(Timer_t value)
{
    switch(value)
    {
        case TIMER2: return TIMER2_BASE;
        case TIMER3: return TIMER3_BASE;
        case TIMER4: return TIMER4_BASE;
        case TIMER5: return TIMER5_BASE;
        default:     return 0;
    }
}

/* === Timer Init === */
void GP_Timer_Init(Timer_t timer)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);

    switch(timer)
    {
        case TIMER2:
            TIM_PSC(Base) = TIM2_PRESCALER;
            TIM_ARR(Base) = TIM2_RELOAD;
            NVIC_EnableInterrupt(TIM2_IRQn);
            break;

        case TIMER3:
            TIM_PSC(Base) = TIM3_PRESCALER;
            TIM_ARR(Base) = TIM3_RELOAD;
            NVIC_EnableInterrupt(TIM3_IRQn);
            break;

        case TIMER4:
            TIM_PSC(Base) = TIM4_PRESCALER;
            TIM_ARR(Base) = TIM4_RELOAD;
            NVIC_EnableInterrupt(TIM4_IRQn);
            break;

        case TIMER5:
            TIM_PSC(Base) = TIM5_PRESCALER;   
            TIM_ARR(Base) = TIM5_RELOAD;      
            NVIC_EnableInterrupt(TIM5_IRQn);
            break;

        default:
            return;
    }

    /* Enable update interrupt */
    TIM_DIER(Base) |= TIM_DIER_UIE;

    /* Reset counter */
    TIM_CNT(Base) = 0;
}
void GP_Timer_PWM_Init(Timer_t timer)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);

    switch(timer)
    {
        case TIMER3:
            RCC_EnableClock(RCC_APB1, APB1_TIM3);
            break;

        case TIMER4:
            RCC_EnableClock(RCC_APB1, APB1_TIM4);
            break;

        default:
            return; // only PWM timers
    }

    /* Set frequency (example: 1 kHz PWM) */
    TIM_PSC(Base) = 16 - 1;     // 1 MHz
    TIM_ARR(Base) = 1000 - 1;   // 1 kHz

    /* ===== CHANNEL 1 ===== */
    TIM_CCMR1(Base) |= (6 << 4);   // OC1M = PWM mode 1
    TIM_CCMR1(Base) |= (1 << 3);   // preload enable
    TIM_CCER(Base)  |= (1 << 0);   // enable CH1

    /* ===== CHANNEL 2 ===== */
    TIM_CCMR1(Base) |= (6 << 12);  // OC2M
    TIM_CCMR1(Base) |= (1 << 11);
    TIM_CCER(Base)  |= (1 << 4);

    /* ===== CHANNEL 3 ===== */
    TIM_CCMR2(Base) |= (6 << 4);
    TIM_CCMR2(Base) |= (1 << 3);
    TIM_CCER(Base)  |= (1 << 8);

    /* ===== CHANNEL 4 ===== */
    TIM_CCMR2(Base) |= (6 << 12);
    TIM_CCMR2(Base) |= (1 << 11);
    TIM_CCER(Base)  |= (1 << 12);

    /* Init duty = 0 */
    TIM_CCR1(Base) = 0;
    TIM_CCR2(Base) = 0;
    TIM_CCR3(Base) = 0;
    TIM_CCR4(Base) = 0;

    /* Enable timer */
    TIM_CR1(Base) |= TIM_CR1_CEN;
}
void GP_Timer_PWM_SetDuty(Timer_t timer, uint8 channel, uint8 duty)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);

    if (duty > 100) duty = 100;

    uint32 value = ((TIM_ARR(Base) + 1) * duty) / 100;

    switch(channel)
    {
        case 1: TIM_CCR1(Base) = value; break;
        case 2: TIM_CCR2(Base) = value; break;
        case 3: TIM_CCR3(Base) = value; break;
        case 4: TIM_CCR4(Base) = value; break;
        default: break;
    }
}
/* === Start Timer === */
void GP_Timer_Start(Timer_t timer)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);

    /* Clear pending flag */
    TIM_SR(Base) &= ~TIM_SR_UIF;

    /* Enable counter */
    TIM_CR1(Base) |= TIM_CR1_CEN;
}

/* === Stop Timer === */
void GP_Timer_Stop(Timer_t timer)
{
    uint32 Base = GET_TIMER_BASE_ADD(timer);

    /* Disable counter */
    TIM_CR1(Base) &= ~TIM_CR1_CEN;
}

/* === Set Callback === */
void GP_Timer_SetCallback(Timer_t timer, TimerCallback_t cb)
{
    switch(timer)
    {
        case TIMER2: tim2_callback = cb; break;
        case TIMER3: tim3_callback = cb; break;
        case TIMER4: tim4_callback = cb; break;
        case TIMER5: tim5_callback = cb; break;
        default: break;
    }
}

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