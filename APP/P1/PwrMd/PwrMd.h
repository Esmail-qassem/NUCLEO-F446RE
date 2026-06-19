#ifndef PWRMD_H_
#define PWRMD_H_

#include "STD_TYPES.h"

#define SLEEP_MAGIC  0xDEAD   /* written to BKP(0) before any sleep */

typedef enum {
    PWR_MODE_SLEEP   = 0,
    PWR_MODE_STOP    = 1,
    PWR_MODE_STANDBY = 2
} sleep_state_t;

/*
 * Stm_ShutDown — enter the requested low-power mode with full teardown.
 *   Sleep   : CPU halts, RTOS/IWDG keep running. Returns on next SysTick.
 *   Stop    : all clocks off, SRAM kept. Returns after EXTI/RTC wake.
 *   Standby : everything off. Does NOT return — wake triggers full reset.
 */
void Stm_ShutDown(sleep_state_t sleep_mode);

/*
 * Stm_WakeUp — single entry point for all wake-up sequences.
 *   Standby : call from APP_init() — detects SBF, restores state, prints reason.
 *   Sleep   : call from Stm_ShutDown() after WFI loop — prints wake reason.
 *   Stop    : call from Stm_ShutDown() after LP_EnterStop() — restores PLL,
 *             IWDG, and RTOS tick, then prints wake reason.
 *   Cold boot (no prior sleep): no-op.
 */
void Stm_WakeUp(void);

#endif /* PWRMD_H_ */
