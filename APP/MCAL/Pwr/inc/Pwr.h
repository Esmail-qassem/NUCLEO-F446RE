#ifndef PWR_H_
#define PWR_H_

#include "STD_TYPES.h"

/*------------------------------------------------------------------
 *  Low-Power Modes — STM32F446RE
 *
 *  Three modes, ordered by increasing depth (and wake-up cost):
 *
 *  ┌──────────────┬──────────────────────────────────────────────────────────┐
 *  │ Mode         │ What stops / what stays on                               │
 *  ├──────────────┼──────────────────────────────────────────────────────────┤
 *  │ Sleep        │ CPU core clock gated. All peripherals & SRAM keep state. │
 *  │              │ Fastest wake-up (any IRQ). Clocks unchanged on exit.     │
 *  ├──────────────┼──────────────────────────────────────────────────────────┤
 *  │ Stop         │ All clocks stopped (CPU + peripherals). 1.2V domain and  │
 *  │              │ SRAM retained. Regulator optional low-power mode.        │
 *  │              │ Wake via EXTI line / RTC alarm / I2C address match.      │
 *  │              │ On exit: HSI selected as SYSCLK — re-init PLL if needed. │
 *  ├──────────────┼──────────────────────────────────────────────────────────┤
 *  │ Standby      │ 1.2V domain powered off. Only LSE/LSI, RTC, IWDG, and   │
 *  │              │ backup domain active. SRAM and registers lost (except    │
 *  │              │ backup registers). Wake-up acts like a system reset.     │
 *  │              │ Wake sources: WKUP pin, RTC alarm/tamper, IWDG, NRST.   │
 *  └──────────────┴──────────────────────────────────────────────────────────┘
 *
 *  WFI vs WFE entry:
 *      LP_ENTRY_WFI — enters low-power on WFI; wakes on any unmasked IRQ
 *      LP_ENTRY_WFE — enters low-power on WFE; wakes on any event (including
 *                     a pending IRQ or SEV instruction from another core)
 *------------------------------------------------------------------*/

/*------------------------------------------------------------------
 *  PWR register base and definitions
 *------------------------------------------------------------------*/
#define PWR_BASE    0x40007000U

#define PWR_CR      (*((volatile uint32*)(PWR_BASE + 0x00U)))
#define PWR_CSR     (*((volatile uint32*)(PWR_BASE + 0x04U)))

/* PWR_CR bit masks */
#define PWR_CR_LPDS   (1U <<  0)  /* Low-power deep-sleep (Stop mode)    */
#define PWR_CR_PDDS   (1U <<  1)  /* Power-down deep-sleep (0=Stop,1=Standby) */
#define PWR_CR_CWUF   (1U <<  2)  /* Clear wakeup flag                   */
#define PWR_CR_CSBF   (1U <<  3)  /* Clear standby flag                  */
#define PWR_CR_PVDE   (1U <<  4)  /* PVD enable                          */
#define PWR_CR_PLS_Pos   5U       /* PVD level selection [7:5]           */
#define PWR_CR_DBP    (1U <<  8)  /* Disable backup-domain write protect */
#define PWR_CR_FPDS   (1U <<  9)  /* Flash power-down in Stop mode       */
#define PWR_CR_ODEN   (1U << 16)  /* Over-drive enable                   */
#define PWR_CR_ODSWEN (1U << 17)  /* Over-drive switch enable            */

/* PWR_CSR bit masks */
#define PWR_CSR_WUF   (1U <<  0)  /* Wakeup flag                         */
#define PWR_CSR_SBF   (1U <<  1)  /* Standby flag                        */
#define PWR_CSR_PVDO  (1U <<  2)  /* PVD output                          */
#define PWR_CSR_BRR   (1U <<  3)  /* Backup regulator ready              */
#define PWR_CSR_EWUP1 (1U <<  8)  /* Enable WKUP pin 1 (PA0)            */
#define PWR_CSR_EWUP2 (1U <<  9)  /* Enable WKUP pin 2 (PC13)           */
#define PWR_CSR_BRE   (1U << 10)  /* Backup regulator enable             */

/*------------------------------------------------------------------
 *  SCB System Control Register (Cortex-M4)
 *------------------------------------------------------------------*/
#define SCB_SCR     (*((volatile uint32*)0xE000ED10U))
#define SCB_SCR_SLEEPONEXIT  (1U << 1)  /* Sleep on return from ISR  */
#define SCB_SCR_SLEEPDEEP    (1U << 2)  /* Enable deep-sleep (Stop/Standby) */
#define SCB_SCR_SEVONPEND    (1U << 4)  /* Send event on pending IRQ */

/*------------------------------------------------------------------
 *  Enumerations
 *------------------------------------------------------------------*/

/* How to enter the low-power state */
typedef enum {
    LP_ENTRY_WFI = 0,   /* Wait For Interrupt */
    LP_ENTRY_WFE = 1    /* Wait For Event     */
} LP_Entry_t;

/* Voltage regulator behaviour in Stop mode */
typedef enum {
    LP_REGULATOR_ON         = 0,  /* Normal regulator (faster wake-up)   */
    LP_REGULATOR_LOW_POWER  = 1   /* Low-power regulator (lower current) */
} LP_Regulator_t;

/* PVD threshold (PWR_CR PLS[2:0]) */
typedef enum {
    LP_PVD_2V0 = 0,
    LP_PVD_2V1 = 1,
    LP_PVD_2V3 = 2,
    LP_PVD_2V5 = 3,
    LP_PVD_2V6 = 4,
    LP_PVD_2V7 = 5,
    LP_PVD_2V8 = 6,
    LP_PVD_2V9 = 7
} LP_PVDLevel_t;

/*------------------------------------------------------------------
 *  Sleep mode
 *------------------------------------------------------------------*/

/**
 * LP_EnterSleep — Enter Sleep mode (CPU clocked off, peripherals on).
 * @param entry  LP_ENTRY_WFI or LP_ENTRY_WFE
 * Wake-up: any enabled interrupt (WFI) or any event/interrupt (WFE).
 * Clocks are unchanged on exit — no re-configuration needed.
 */
void LP_EnterSleep(LP_Entry_t entry);

/**
 * LP_EnableSleepOnExit — CPU goes to Sleep automatically each time
 * it returns from an interrupt handler (useful in ISR-driven designs).
 */
void LP_EnableSleepOnExit(void);

/**
 * LP_DisableSleepOnExit — Disable the automatic sleep-on-exit behaviour.
 */
void LP_DisableSleepOnExit(void);

/*------------------------------------------------------------------
 *  Stop mode
 *------------------------------------------------------------------*/

/**
 * LP_EnterStop — Enter Stop mode (all clocks stopped, SRAM retained).
 *
 * @param regulator  LP_REGULATOR_ON or LP_REGULATOR_LOW_POWER
 * @param flash_pd   1 = power down flash in Stop (saves ~1 mA, slower wake)
 * @param entry      LP_ENTRY_WFI or LP_ENTRY_WFE
 *
 * Wake-up sources: any EXTI line, RTC alarm/wakeup, I2C address match,
 *                  USART/CAN wakeup (if configured).
 *
 * IMPORTANT: On exit from Stop the HSI oscillator is selected as SYSCLK.
 *            If you were running on PLL you must re-enable and re-lock it
 *            after this function returns.
 */
void LP_EnterStop(LP_Regulator_t regulator, uint8 flash_pd, LP_Entry_t entry);

/*------------------------------------------------------------------
 *  Standby mode
 *------------------------------------------------------------------*/

/**
 * LP_EnterStandby — Enter Standby mode (1.2V domain off).
 * Wake-up causes a system reset; execution restarts from the reset vector.
 * Enable at least one wakeup source before calling (WKUP pin or RTC).
 */
void LP_EnterStandby(void);

/**
 * LP_EnableWakeupPin — Enable the WKUP pin wakeup source from Standby.
 * @param pin  1 = WKUP1 (PA0),  2 = WKUP2 (PC13)
 */
void LP_EnableWakeupPin(uint8 pin);

/**
 * LP_DisableWakeupPin — Disable a WKUP pin.
 * @param pin  1 or 2
 */
void LP_DisableWakeupPin(uint8 pin);

/*------------------------------------------------------------------
 *  Wakeup / standby flag helpers
 *------------------------------------------------------------------*/

/** Return 1 if the system woke from Standby mode (SBF flag set). */
uint8 LP_IsWakeFromStandby(void);

/** Return 1 if a WKUP pin wakeup event was detected (WUF flag set). */
uint8 LP_IsWakeupPinEvent(void);

/** Clear both WUF and SBF flags. Call before entering Standby. */
void  LP_ClearFlags(void);

/*------------------------------------------------------------------
 *  PVD — Programmable Voltage Detector
 *------------------------------------------------------------------*/

/**
 * LP_EnablePVD — Enable the PVD and set the detection threshold.
 * PVD generates an EXTI line 16 interrupt when VDD crosses the threshold.
 * Requires PWR clock enabled (RCC_APB1 bit 28).
 */
void LP_EnablePVD(LP_PVDLevel_t level);

/** LP_DisablePVD — Disable the PVD. */
void LP_DisablePVD(void);

/** LP_GetPVDOutput — Return 1 if VDD is below the PVD threshold. */
uint8 LP_GetPVDOutput(void);

#endif /* LOW_POWER_H_ */
