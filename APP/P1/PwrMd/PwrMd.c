
#include "STD_TYPES.h"
#include "SysTick_interface.h"
#include "PwrMd.h"
#include "UART.h"
#include "Pwr.h"
#include "RTC.h"
#include "OLED.h"
#include "RCC.h"
#include "RTOS.h"
#include "GPIO_interface.h"
#include "IWDG.h"

extern RCC_Config_t RCC_Configuration;

/* Cortex-M4 debug halting control — bit 0 set when ST-Link is connected */
#define DHCSR  (*((volatile uint32*)0xE000EDF0U))
#define C_DEBUGEN (1U << 0)

/* EXTI / SYSCFG bare registers — no EXTI driver exists yet */
#define SYSCFG_EXTICR4  (*((volatile uint32*)0x40013814U))
#define EXTI_EMR        (*((volatile uint32*)0x40013C04U))
#define EXTI_RTSR       (*((volatile uint32*)0x40013C08U))
#define EXTI_FTSR       (*((volatile uint32*)0x40013C0CU))
#define EXTI_PR         (*((volatile uint32*)0x40013C14U))

/* B1 (PC13) falling-edge event for Stop-mode WFE wake */
static void B1_ExtiEvent_Enable(void)
{
    RCC_EnableClock(RCC_APB2, APB2_SYSCFG);
    SYSCFG_EXTICR4 &= ~(0xFU << 4);
    SYSCFG_EXTICR4 |=  (0x2U << 4);   /* EXTI13 = PC */
    EXTI_RTSR      &= ~(1U << 13);
    EXTI_FTSR      |=  (1U << 13);    /* falling edge (button press) */
    EXTI_EMR       |=  (1U << 13);    /* event, not interrupt        */
    EXTI_PR        |=  (1U << 13);    /* clear any pending           */
}

static void B1_ExtiEvent_Disable(void)
{
    EXTI_EMR  &= ~(1U << 13);
    EXTI_FTSR &= ~(1U << 13);
    EXTI_PR   |=  (1U << 13);
}

/*==============================================================================
 *  Stm_WakeUp — single entry point for ALL wake-up sequences.
 *
 *  Standby wake: called from APP_init() on every boot.
 *    Detects SBF flag, reads saved mode from RTC backup, runs recovery.
 *
 *  Sleep / Stop wake: called inside Stm_ShutDown() after WFI returns.
 *    Reads saved mode from RTC backup, runs mode-specific recovery.
 *============================================================================*/
void Stm_WakeUp(void)
{
    uint32 magic = RTC_ReadBackup(0);
    uint32 mode  = RTC_ReadBackup(1);

    /* ── Standby wake ─────────────────────────────────────────────────────── */
    if (LP_IsWakeFromStandby())
    {
        LP_ClearFlags();
        RTC_DisableWakeup();
        RTC_ClearWakeupFlag();

        if (magic != SLEEP_MAGIC)
        {
            UART_SendSyncBuffer(UART2, (uint8*)"WAKE:STANDBY (no context)\r\n", 27);
            return;
        }

        UART_SendSyncBuffer(UART2, (uint8*)"WAKE:STANDBY\r\n", 14);
        OLED_Clear();
        OLED_DrawString(0, 0,  "  Welcome back!");
        OLED_DrawString(0, 24, "  System awake");
        OLED_UpdateScreen(I2C1_PORT, 0, 8);

        RTC_WriteBackup(0, 0x00000000);
        RTC_WriteBackup(1, 0x00000000);
        return;
    }

    /* ── Sleep / Stop resume ──────────────────────────────────────────────── */
    if (magic != SLEEP_MAGIC) return;

    switch ((sleep_state_t)mode)
    {
        case PWR_MODE_SLEEP:
            UART_SendSyncBuffer(UART2, (uint8*)"WAKE:SLEEP\r\n", 12);
            break;

        case PWR_MODE_STOP:
            RCC_Init(&RCC_Configuration);   /* restore PLL 180MHz  */
            IWDG_Refresh();
            RTOS_voidRestartTick();         /* re-arm RTOS tick    */
            UART_SendSyncBuffer(UART2, (uint8*)"WAKE:STOP\r\n", 11);
            break;

        default: break;
    }

    RTC_WriteBackup(0, 0x00000000);
    RTC_WriteBackup(1, 0x00000000);
}

/*==============================================================================
 *  Stm_ShutDown — shutdown only. Enters the requested power mode.
 *  Calls Stm_WakeUp() on return (Sleep/Stop). Standby does not return.
 *============================================================================*/
void Stm_ShutDown(sleep_state_t sleep_mode)
{
    /* Skip all sleep modes when ST-Link debugger is connected */
    if (DHCSR & C_DEBUGEN)
    {
        UART_SendSyncBuffer(UART2, (uint8*)"Debugger connected - sleep skipped.\r\n", 37);
        return;
    }

    UART_SendSyncBuffer(UART2, (uint8*)"Shutting down...\r\n", 18);

    RTC_WriteBackup(0, SLEEP_MAGIC);
    RTC_WriteBackup(1, (uint32)sleep_mode);

    OLED_Clear();
    OLED_DrawString(0, 24, "  Sleeping...");
    OLED_UpdateScreen(I2C1_PORT, 0, 8);

    /* 3-second ST-Link flash window @ 180MHz (~3 cycles/iter) */
    UART_SendSyncBuffer(UART2, (uint8*)"Flash window: 3s\r\n", 18);
    for (volatile uint32 i = 0; i < 180000000UL; i++);

    switch (sleep_mode)
    {
        case PWR_MODE_SLEEP:
        {
            uint32 iwdg_tick = 0;
            uint8  btn = 1;
            UART_SendSyncBuffer(UART2, (uint8*)"Sleep - press B1 to wake.\r\n", 27);
            do {
                LP_EnterSleep(LP_ENTRY_WFI);
                if (++iwdg_tick >= 100U) { IWDG_Refresh(); iwdg_tick = 0U; }
                GPIO_ReadPin(GPIO_PORTC, PIN13, &btn);
            } while (btn == 1);
            Stm_WakeUp();
            break;
        }

        case PWR_MODE_STOP:
            SysTick_voidStopTimer();
            B1_ExtiEvent_Enable();
            UART_SendSyncBuffer(UART2, (uint8*)"Stop - press B1 to wake.\r\n", 26);
            IWDG_Refresh();
            LP_EnterStop(LP_REGULATOR_LOW_POWER, 1, LP_ENTRY_WFE);
            B1_ExtiEvent_Disable();
            Stm_WakeUp();
            break;

        case PWR_MODE_STANDBY:
            SysTick_voidStopTimer();
            RTC_SetWakeup(RTC_WUCKSEL_CK_SPRE, 10U, 1U);
            UART_SendSyncBuffer(UART2, (uint8*)"Standby - auto wake in 10s.\r\n", 29);
            LP_ClearFlags();
            LP_EnterStandby();   /* does not return */
            break;
    }
}
