#include "RCC.hpp"

void RCC::EnableHSI()
{
    SET_BIT(RCC_REGS->CR, 0);
    while (!GET_BIT(RCC_REGS->CR, 1));
}

void RCC::EnableHSE()
{
    SET_BIT(RCC_REGS->CR, 16);
    while (!GET_BIT(RCC_REGS->CR, 17));
}

void RCC::EnablePLL()
{
    SET_BIT(RCC_REGS->CR, 24);
    while (!GET_BIT(RCC_REGS->CR, 25));
}

void RCC::DisablePLL()
{
    CLEAR_BIT(RCC_REGS->CR, 24);
    while (GET_BIT(RCC_REGS->CR, 25));
}

void RCC::SelectClock(RCC_ClockSrc src)
{
    RCC_REGS->CFGR &= ~0x3u;
    RCC_REGS->CFGR |=  static_cast<uint32>(src) & 0x3u;
    while (((RCC_REGS->CFGR >> 2u) & 0x3u) != static_cast<uint32>(src));
}

void RCC::ConfigPLL(const RCC_PLLConfig_t& pll)
{
    DisablePLL();
    uint32 temp = 0;
    temp |=  (pll.M & 0x3Fu);
    temp |=  (pll.N & 0x1FFu)              << 6u;
    temp |= ((pll.P / 2u - 1u) & 0x3u)    << 16u;
    temp |=  static_cast<uint32>(pll.Source) << 22u;
    temp |=  (pll.Q & 0x0Fu)               << 24u;
    RCC_REGS->PLLCFGR = temp;
}

void RCC::SetAHBPre(RCC_AHBPre pre)
{
    RCC_REGS->CFGR &= ~(0xFu << 4u);
    RCC_REGS->CFGR |=   static_cast<uint32>(pre) << 4u;
}

void RCC::SetAPB1Pre(RCC_APBPre pre)
{
    RCC_REGS->CFGR &= ~(0x7u << 10u);
    RCC_REGS->CFGR |=   static_cast<uint32>(pre) << 10u;
}

void RCC::SetAPB2Pre(RCC_APBPre pre)
{
    RCC_REGS->CFGR &= ~(0x7u << 13u);
    RCC_REGS->CFGR |=   static_cast<uint32>(pre) << 13u;
}

void RCC::Init(const RCC_Config_t& cfg)
{
    switch (cfg.ClockSource)
    {
        case RCC_ClockSrc::HSI:
            EnableHSI();
            SelectClock(RCC_ClockSrc::HSI);
            break;

        case RCC_ClockSrc::HSE:
            EnableHSE();
            SelectClock(RCC_ClockSrc::HSE);
            break;

        case RCC_ClockSrc::PLL:
            if (cfg.PLL.Source == RCC_PLLSrc::HSE)
                EnableHSE();
            else
                EnableHSI();
            ConfigPLL  (cfg.PLL);
            EnablePLL  ();
            SetAHBPre  (cfg.AHB_Prescaler);
            SetAPB1Pre (cfg.APB1_Prescaler);
            SetAPB2Pre (cfg.APB2_Prescaler);
            SelectClock(RCC_ClockSrc::PLL);
            break;

        default:
            break;
    }
}

void RCC::EnableClock(RCC_Bus bus, RCC_Peripheral periph)
{
    uint8 id = static_cast<uint8>(periph);
    switch (bus)
    {
        case RCC_Bus::AHB1: SET_BIT(RCC_REGS->AHB1ENR, id); break;
        case RCC_Bus::AHB2: SET_BIT(RCC_REGS->AHB2ENR, id); break;
        case RCC_Bus::AHB3: SET_BIT(RCC_REGS->AHB3ENR, id); break;
        case RCC_Bus::APB1: SET_BIT(RCC_REGS->APB1ENR, id); break;
        case RCC_Bus::APB2: SET_BIT(RCC_REGS->APB2ENR, id); break;
        default: break;
    }
}

void RCC::DisableClock(RCC_Bus bus, RCC_Peripheral periph)
{
    uint8 id = static_cast<uint8>(periph);
    switch (bus)
    {
        case RCC_Bus::AHB1: CLEAR_BIT(RCC_REGS->AHB1ENR, id); break;
        case RCC_Bus::AHB2: CLEAR_BIT(RCC_REGS->AHB2ENR, id); break;
        case RCC_Bus::AHB3: CLEAR_BIT(RCC_REGS->AHB3ENR, id); break;
        case RCC_Bus::APB1: CLEAR_BIT(RCC_REGS->APB1ENR, id); break;
        case RCC_Bus::APB2: CLEAR_BIT(RCC_REGS->APB2ENR, id); break;
        default: break;
    }
}
