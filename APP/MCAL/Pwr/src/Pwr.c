#include "Pwr.h"

/*
 * ── Prerequisites ──────────────────────────────────────────────────────────
 *
 *  All modes:
 *    RCC_EnableClock(RCC_APB1, APB1_PWR);  // enable PWR register access
 *
 *  Stop / Standby:
 *    Configure EXTI or RTC as wakeup source BEFORE calling LP_EnterStop()
 *    or LP_EnterStandby().
 *
 * ── Usage examples ─────────────────────────────────────────────────────────
 *
 *  1. Sleep (lightest, instant wake from any IRQ):
 *       LP_EnterSleep(LP_ENTRY_WFI);
 *
 *  2. Stop with normal regulator, wake via EXTI:
 *       // Configure EXTI line for the wakeup pin …
 *       LP_EnterStop(LP_REGULATOR_ON, 0, LP_ENTRY_WFI);
 *       // — execution resumes here after wake-up —
 *       // Re-init PLL if you were using it:
 *       RCC_Init(&my_rcc_config);
 *
 *  3. Standby (deepest, wake via WKUP pin PA0):
 *       LP_ClearFlags();
 *       LP_EnableWakeupPin(1);    // PA0
 *       LP_EnterStandby();        // does not return — resets on wake
 *
 *  4. PVD monitoring:
 *       RCC_EnableClock(RCC_APB1, APB1_PWR);
 *       LP_EnablePVD(LP_PVD_2V7);
 *       NVIC_EnableInterrupt(PVD_IRQ_NUM);   // EXTI16
 */

/*------------------------------------------------------------------
 *  Internal: WFI / WFE wrappers
 *------------------------------------------------------------------*/
static inline void exec_wfi(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("wfi"     ::: "memory");
}

static inline void exec_wfe(void)
{
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("wfe"     ::: "memory");
}

/*------------------------------------------------------------------
 *  Sleep mode
 *------------------------------------------------------------------*/
void LP_EnterSleep(LP_Entry_t entry)
{
    /* Clear SLEEPDEEP — only the CPU core clock is gated, not all clocks */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;

    if (entry == LP_ENTRY_WFI)
        exec_wfi();
    else
        exec_wfe();
}

void LP_EnableSleepOnExit(void)
{
    SCB_SCR |= SCB_SCR_SLEEPONEXIT;
}

void LP_DisableSleepOnExit(void)
{
    SCB_SCR &= ~SCB_SCR_SLEEPONEXIT;
}

/*------------------------------------------------------------------
 *  Stop mode
 *------------------------------------------------------------------*/
void LP_EnterStop(LP_Regulator_t regulator, uint8 flash_pd, LP_Entry_t entry)
{
    /* 1. Select Stop mode: PDDS = 0 */
    PWR_CR &= ~PWR_CR_PDDS;

    /* 2. Voltage regulator in Stop */
    if (regulator == LP_REGULATOR_LOW_POWER)
        PWR_CR |=  PWR_CR_LPDS;
    else
        PWR_CR &= ~PWR_CR_LPDS;

    /* 3. Flash power-down option in Stop */
    if (flash_pd)
        PWR_CR |=  PWR_CR_FPDS;
    else
        PWR_CR &= ~PWR_CR_FPDS;

    /* 4. Set SLEEPDEEP in SCB */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    /* 5. Enter Stop — CPU halts here until wakeup event */
    if (entry == LP_ENTRY_WFI)
        exec_wfi();
    else
        exec_wfe();

    /* 6. Execution resumes here after wakeup.
          HSI is now SYSCLK — caller must re-configure PLL if needed. */
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;   /* restore for next sleep operations */
}

/*------------------------------------------------------------------
 *  Standby mode
 *------------------------------------------------------------------*/
void LP_EnterStandby(void)
{
    /* 1. Select Standby mode: PDDS = 1 */
    PWR_CR |= PWR_CR_PDDS;

    /* 2. Clear wakeup and standby flags (mandatory before entering) */
    PWR_CR |= (PWR_CR_CWUF | PWR_CR_CSBF);

    /* 3. Set SLEEPDEEP */
    SCB_SCR |= SCB_SCR_SLEEPDEEP;

    /* 4. Enter Standby — this does not return.
          On wakeup the MCU performs a system reset. */
    exec_wfi();

    /* Should never reach here */
    while (1) {}
}

/*------------------------------------------------------------------
 *  Wakeup pin control
 *------------------------------------------------------------------*/
void LP_EnableWakeupPin(uint8 pin)
{
    if (pin == 1U)
        PWR_CSR |= PWR_CSR_EWUP1;
    else if (pin == 2U)
        PWR_CSR |= PWR_CSR_EWUP2;
}

void LP_DisableWakeupPin(uint8 pin)
{
    if (pin == 1U)
        PWR_CSR &= ~PWR_CSR_EWUP1;
    else if (pin == 2U)
        PWR_CSR &= ~PWR_CSR_EWUP2;
}

/*------------------------------------------------------------------
 *  Flag helpers
 *------------------------------------------------------------------*/
uint8 LP_IsWakeFromStandby(void)
{
    return (PWR_CSR & PWR_CSR_SBF) ? 1U : 0U;
}

uint8 LP_IsWakeupPinEvent(void)
{
    return (PWR_CSR & PWR_CSR_WUF) ? 1U : 0U;
}

void LP_ClearFlags(void)
{
    PWR_CR |= (PWR_CR_CWUF | PWR_CR_CSBF);
}

/*------------------------------------------------------------------
 *  PVD
 *------------------------------------------------------------------*/
void LP_EnablePVD(LP_PVDLevel_t level)
{
    uint32 cr = PWR_CR;
    cr &= ~(7U << PWR_CR_PLS_Pos);                 /* clear PLS field */
    cr |=  ((uint32)level << PWR_CR_PLS_Pos);       /* set threshold   */
    cr |=  PWR_CR_PVDE;                             /* enable PVD      */
    PWR_CR = cr;
}

void LP_DisablePVD(void)
{
    PWR_CR &= ~PWR_CR_PVDE;
}

uint8 LP_GetPVDOutput(void)
{
    return (PWR_CSR & PWR_CSR_PVDO) ? 1U : 0U;
}
