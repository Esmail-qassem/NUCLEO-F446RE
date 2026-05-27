/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : GP_TIMER                                               */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "GPTimer.hpp"
#include "NVIC.hpp"
#include "RCC.hpp"

/* ── Static callbacks ─────────────────────────────────────────────── */
static TimerCallback_t tim2_callback = nullptr;
static TimerCallback_t tim3_callback = nullptr;
static TimerCallback_t tim4_callback = nullptr;
static TimerCallback_t tim5_callback = nullptr;

/* ── GetBase ──────────────────────────────────────────────────────── */
uint32 GPTimer::GetBase(Timer_t timer)
{
    switch (timer)
    {
        case Timer_t::TIMER2: return TIMER2_BASE_ADDR;
        case Timer_t::TIMER3: return TIMER3_BASE_ADDR;
        case Timer_t::TIMER4: return TIMER4_BASE_ADDR;
        case Timer_t::TIMER5: return TIMER5_BASE_ADDR;
        default:              return 0U;
    }
}

/* ── Init ─────────────────────────────────────────────────────────── */
void GPTimer::Init(Timer_t timer)
{
    uint32 base = GetBase(timer);

    switch (timer)
    {
        case Timer_t::TIMER2:
            TIM_PSC(base) = TIM2_PRESCALER_VAL;
            TIM_ARR(base) = TIM2_RELOAD_VAL;
            NVIC::EnableInterrupt(TIM2_IRQn);
            break;
        case Timer_t::TIMER3:
            TIM_PSC(base) = TIM3_PRESCALER_VAL;
            TIM_ARR(base) = TIM3_RELOAD_VAL;
            NVIC::EnableInterrupt(TIM3_IRQn);
            break;
        case Timer_t::TIMER4:
            TIM_PSC(base) = TIM4_PRESCALER_VAL;
            TIM_ARR(base) = TIM4_RELOAD_VAL;
            NVIC::EnableInterrupt(TIM4_IRQn);
            break;
        case Timer_t::TIMER5:
            TIM_PSC(base) = TIM5_PRESCALER_VAL;
            TIM_ARR(base) = TIM5_RELOAD_VAL;
            NVIC::EnableInterrupt(TIM5_IRQn);
            break;
        default:
            return;
    }

    TIM_DIER(base) |= TIM_DIER_UIE;
    TIM_CNT(base)   = 0U;
}

/* ── PWM_Init ─────────────────────────────────────────────────────── */
void GPTimer::PWM_Init(Timer_t timer)
{
    uint32 base = GetBase(timer);

    switch (timer)
    {
        case Timer_t::TIMER3:
            RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::TIM3);
            break;
        case Timer_t::TIMER4:
            RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::TIM4);
            break;
        default:
            return;
    }

    TIM_PSC(base) = static_cast<uint16>(16U - 1U);
    TIM_ARR(base) = static_cast<uint16>(1000U - 1U);

    /* Channel 1 */
    TIM_CCMR1(base) |= (6U << 4U);
    TIM_CCMR1(base) |= (1U << 3U);
    TIM_CCER(base)  |= (1U << 0U);
    /* Channel 2 */
    TIM_CCMR1(base) |= (6U << 12U);
    TIM_CCMR1(base) |= (1U << 11U);
    TIM_CCER(base)  |= (1U << 4U);
    /* Channel 3 */
    TIM_CCMR2(base) |= (6U << 4U);
    TIM_CCMR2(base) |= (1U << 3U);
    TIM_CCER(base)  |= (1U << 8U);
    /* Channel 4 */
    TIM_CCMR2(base) |= (6U << 12U);
    TIM_CCMR2(base) |= (1U << 11U);
    TIM_CCER(base)  |= (1U << 12U);

    TIM_CCR1(base) = 0U;
    TIM_CCR2(base) = 0U;
    TIM_CCR3(base) = 0U;
    TIM_CCR4(base) = 0U;

    TIM_CR1(base) |= TIM_CR1_CEN;
}

/* ── PWM_SetDuty ──────────────────────────────────────────────────── */
void GPTimer::PWM_SetDuty(Timer_t timer, uint8 channel, uint8 duty)
{
    uint32 base = GetBase(timer);
    if (duty > 100U) duty = 100U;

    uint32 value = (static_cast<uint32>(TIM_ARR(base) + 1U) * duty) / 100U;

    switch (channel)
    {
        case 1U: TIM_CCR1(base) = value; break;
        case 2U: TIM_CCR2(base) = value; break;
        case 3U: TIM_CCR3(base) = value; break;
        case 4U: TIM_CCR4(base) = value; break;
        default: break;
    }
}

/* ── Start ────────────────────────────────────────────────────────── */
void GPTimer::Start(Timer_t timer)
{
    uint32 base = GetBase(timer);
    TIM_SR(base)  &= static_cast<uint16>(~TIM_SR_UIF);
    TIM_CR1(base) |= TIM_CR1_CEN;
}

/* ── Stop ─────────────────────────────────────────────────────────── */
void GPTimer::Stop(Timer_t timer)
{
    uint32 base = GetBase(timer);
    TIM_CR1(base) &= static_cast<uint16>(~TIM_CR1_CEN);
}

/* ── SetCallback ──────────────────────────────────────────────────── */
void GPTimer::SetCallback(Timer_t timer, TimerCallback_t cb)
{
    switch (timer)
    {
        case Timer_t::TIMER2: tim2_callback = cb; break;
        case Timer_t::TIMER3: tim3_callback = cb; break;
        case Timer_t::TIMER4: tim4_callback = cb; break;
        case Timer_t::TIMER5: tim5_callback = cb; break;
        default: break;
    }
}

/* ── IRQ Handlers ─────────────────────────────────────────────────── */
extern "C" void TIM2_IRQHandler(void)
{
    if (TIM_SR(TIMER2_BASE_ADDR) & TIM_SR_UIF)
    {
        TIM_SR(TIMER2_BASE_ADDR) &= static_cast<uint16>(~TIM_SR_UIF);
        if (tim2_callback) tim2_callback();
    }
}

extern "C" void TIM3_IRQHandler(void)
{
    if (TIM_SR(TIMER3_BASE_ADDR) & TIM_SR_UIF)
    {
        TIM_SR(TIMER3_BASE_ADDR) &= static_cast<uint16>(~TIM_SR_UIF);
        if (tim3_callback) tim3_callback();
    }
}

extern "C" void TIM4_IRQHandler(void)
{
    if (TIM_SR(TIMER4_BASE_ADDR) & TIM_SR_UIF)
    {
        TIM_SR(TIMER4_BASE_ADDR) &= static_cast<uint16>(~TIM_SR_UIF);
        if (tim4_callback) tim4_callback();
    }
}

extern "C" void TIM5_IRQHandler(void)
{
    if (TIM_SR(TIMER5_BASE_ADDR) & TIM_SR_UIF)
    {
        TIM_SR(TIMER5_BASE_ADDR) &= static_cast<uint16>(~TIM_SR_UIF);
        if (tim5_callback) tim5_callback();
    }
}
