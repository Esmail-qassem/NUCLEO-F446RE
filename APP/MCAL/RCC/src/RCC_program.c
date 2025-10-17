#include "RCC.h"

/* ---------- Internal Helpers ---------- */
static void RCC_EnableHSI(void)
{
    SET_BIT(RCC_CR, 0);
    while(!GET_BIT(RCC_CR, 1));
}

static void RCC_EnableHSE(void)
{
    SET_BIT(RCC_CR, 16);
    while(!GET_BIT(RCC_CR, 17));
}

static void RCC_EnablePLL(void)
{
    SET_BIT(RCC_CR, 24);
    while(!GET_BIT(RCC_CR, 25));
}

static void RCC_DisablePLL(void)
{
    CLEAR_BIT(RCC_CR, 24);
    while(GET_BIT(RCC_CR, 25));
}

static void RCC_SelectSystemClock(RCC_ClockSource_t clkSrc)
{
    RCC_CFGR &= ~(0x3);
    RCC_CFGR |= (clkSrc & 0x3);

    while(((RCC_CFGR >> 2) & 0x3) != clkSrc);
}

/* ---------- PLL Configuration ---------- */
static void RCC_ConfigPLL(const RCC_PLLConfig_t *pll)
{
    RCC_DisablePLL();

    uint32 temp = 0;
    temp |= (pll->PLL_M & 0x3F);
    temp |= (pll->PLL_N & 0x1FF) << 6;
    temp |= (((pll->PLL_P / 2) - 1) & 0x3) << 16;
    temp |= (pll->PLL_Source << 22);
    temp |= (pll->PLL_Q & 0x0F) << 24;

    RCC_PLLCFGR = temp;
}

/* ---------- Prescaler Configuration ---------- */
static void RCC_SetAHBPrescaler(RCC_AHBPrescaler_t prescaler)
{
    RCC_CFGR &= ~(0xF << 4);
    RCC_CFGR |= (prescaler << 4);
}

static void RCC_SetAPB1Prescaler(RCC_APBPrescaler_t prescaler)
{
    RCC_CFGR &= ~(0x7 << 10);
    RCC_CFGR |= (prescaler << 10);
}

static void RCC_SetAPB2Prescaler(RCC_APBPrescaler_t prescaler)
{
    RCC_CFGR &= ~(0x7 << 13);
    RCC_CFGR |= (prescaler << 13);
}

/* ---------- RCC Initialization Function ---------- */
void RCC_Init(const RCC_Config_t *cfg)
{
    if (!cfg) return;

    switch(cfg->ClockSource)
    {
        case RCC_CLK_HSI:
            RCC_EnableHSI();
            RCC_SelectSystemClock(RCC_CLK_HSI);
            break;

        case RCC_CLK_HSE:
            RCC_EnableHSE();
            RCC_SelectSystemClock(RCC_CLK_HSE);
            break;

        case RCC_CLK_PLL:
            if (cfg->PLL_Config.PLL_Source == RCC_PLLSRC_HSE)
                RCC_EnableHSE();
            else
                RCC_EnableHSI();

            RCC_ConfigPLL(&cfg->PLL_Config);
            RCC_EnablePLL();

            RCC_SetAHBPrescaler(cfg->AHB_Prescaler);
            RCC_SetAPB1Prescaler(cfg->APB1_Prescaler);
            RCC_SetAPB2Prescaler(cfg->APB2_Prescaler);

            RCC_SelectSystemClock(RCC_CLK_PLL);
            break;

        default:
            break;
    }
}

/* ---------- Peripheral Clock Control ---------- */
void RCC_EnableClock(uint8 bus, uint8 periphID)
{
    switch(bus)
    {
        case 0: SET_BIT(RCC_AHB1ENR, periphID); break;
        case 1: SET_BIT(RCC_AHB2ENR, periphID); break;
        case 2: SET_BIT(RCC_AHB3ENR, periphID); break;
        case 3: SET_BIT(RCC_APB1ENR, periphID); break;
        case 4: SET_BIT(RCC_APB2ENR, periphID); break;
        default: break;
    }
}

void RCC_DisableClock(uint8 bus, uint8 periphID)
{
    switch(bus)
    {
        case 0: CLEAR_BIT(RCC_AHB1ENR, periphID); break;
        case 1: CLEAR_BIT(RCC_AHB2ENR, periphID); break;
        case 2: CLEAR_BIT(RCC_AHB3ENR, periphID); break;
        case 3: CLEAR_BIT(RCC_APB1ENR, periphID); break;
        case 4: CLEAR_BIT(RCC_APB2ENR, periphID); break;
        default: break;
    }
}
