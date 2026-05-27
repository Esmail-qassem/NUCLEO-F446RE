/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : Basic Timer                                            */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "BasicTimer.hpp"
#include "RCC.hpp"
#include "NVIC.hpp"

static TimerCallback_t s_cb6 = nullptr;
static TimerCallback_t s_cb7 = nullptr;

BasicTimer_RegDef_t* BasicTimer::GetReg(BasicTimer_ID timer)
{
    switch (timer)
    {
        case BasicTimer_ID::TIM6: return reinterpret_cast<BasicTimer_RegDef_t*>(TIMER6_BASE_ADDR);
        case BasicTimer_ID::TIM7: return reinterpret_cast<BasicTimer_RegDef_t*>(TIMER7_BASE_ADDR);
        default: return nullptr;
    }
}

void BasicTimer::Init(BasicTimer_ID timer)
{
    BasicTimer_RegDef_t *reg = GetReg(timer);
    if (!reg) return;
    switch (timer)
    {
        case BasicTimer_ID::TIM6:
            RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::TIM6);
            reg->PSC   = TIM6_PRESCALER;
            reg->ARR   = TIM6_RELOAD;
            reg->DIER |= BTIM_DIER_UIE;
            NVIC::EnableInterrupt(TIM6_IRQn);
            break;
        case BasicTimer_ID::TIM7:
            RCC::EnableClock(RCC_Bus::APB1, RCC_Peripheral::TIM7);
            reg->PSC   = TIM7_PRESCALER;
            reg->ARR   = TIM7_RELOAD;
            reg->DIER |= BTIM_DIER_UIE;
            NVIC::EnableInterrupt(TIM7_IRQn);
            break;
        default: break;
    }
}

void BasicTimer::Start(BasicTimer_ID timer)
{
    BasicTimer_RegDef_t *reg = GetReg(timer);
    if (reg) reg->CR1 |= BTIM_CR1_CEN;
}

void BasicTimer::Stop(BasicTimer_ID timer)
{
    BasicTimer_RegDef_t *reg = GetReg(timer);
    if (reg) reg->CR1 &= static_cast<uint16>(~BTIM_CR1_CEN);
}

void BasicTimer::SetCallback(BasicTimer_ID timer, TimerCallback_t cb)
{
    switch (timer)
    {
        case BasicTimer_ID::TIM6: s_cb6 = cb; break;
        case BasicTimer_ID::TIM7: s_cb7 = cb; break;
        default: break;
    }
}

extern "C" void TIM6_DAC_IRQHandler(void)
{
    BasicTimer_RegDef_t *reg = reinterpret_cast<BasicTimer_RegDef_t*>(TIMER6_BASE_ADDR);
    if (reg->SR & BTIM_SR_UIF) {
        reg->SR &= static_cast<uint16>(~BTIM_SR_UIF);
        if (s_cb6) s_cb6();
    }
}

extern "C" void TIM7_IRQHandler(void)
{
    BasicTimer_RegDef_t *reg = reinterpret_cast<BasicTimer_RegDef_t*>(TIMER7_BASE_ADDR);
    if (reg->SR & BTIM_SR_UIF) {
        reg->SR &= static_cast<uint16>(~BTIM_SR_UIF);
        if (s_cb7) s_cb7();
    }
}
