#include "RTC.h"
#include "RCC.h"
/*
 * Usage example — set RTC to 2024-06-15 Saturday 13:30:00, LSE clock:
 *
 *   RCC_EnableClock(RCC_APB1, APB1_PWR);
 *
 *   RTC_Config_t cfg = { RTC_CLK_LSE, 127, 255, RTC_HOURFORMAT_24 };
 *   RTC_Init(&cfg);
 *
 *   RTC_Time_t t = { 13, 30, 0, RTC_AM };
 *   RTC_Date_t d = { 15, 6, 24, RTC_WEEKDAY_SATURDAY };
 *   RTC_SetTime(&t);
 *   RTC_SetDate(&d);
 *
 *   // Poll:
 *   RTC_Time_t now;
 *   RTC_GetTime(&now);
 *   // now.hours, now.minutes, now.seconds are in decimal
 *
 *   // Alarm every day at 07:00:00:
 *   RTC_Alarm_t alm = {
 *       .time = { 7, 0, 0, RTC_AM },
 *       .mask_date = 1,     // don't care about date
 *   };
 *   RTC_SetAlarm(RTC_ALARM_A, &alm, 1);
 *   NVIC_EnableInterrupt(RTC_Alarm_IRQ);   // RTC_Alarm_IRQn = 41
 *
 *   // In IRQ handler:
 *   void RTC_Alarm_IRQHandler(void) {
 *       if (RTC_GetAlarmFlag(RTC_ALARM_A)) {
 *           do_morning_stuff();
 *           RTC_ClearAlarmFlag(RTC_ALARM_A);
 *           // also clear EXTI line 17 pending bit
 *       }
 *   }
 */

/*------------------------------------------------------------------
 *  BCD ↔ decimal helpers
 *------------------------------------------------------------------*/
static inline uint8 bin2bcd(uint8 val)
{
    return (uint8)(((val / 10U) << 4U) | (val % 10U));
}

static inline uint8 bcd2bin(uint8 bcd)
{
    return (uint8)(((bcd >> 4U) & 0x0FU) * 10U + (bcd & 0x0FU));
}

/*------------------------------------------------------------------
 *  Write-protection helpers
 *------------------------------------------------------------------*/
static inline void RTC_Unlock(void)
{
    RTC_WPR = 0xCAU;
    RTC_WPR = 0x53U;
}

static inline void RTC_Lock(void)
{
    RTC_WPR = 0xFFU;
}

/*------------------------------------------------------------------
 *  Init mode helpers
 *------------------------------------------------------------------*/
static RTC_Status_t RTC_EnterInitMode(void)
{
    uint32 timeout = 100000U;
    RTC_ISR |= RTC_ISR_INIT;
    while (!(RTC_ISR & RTC_ISR_INITF)) {
        if (--timeout == 0U) return RTC_TIMEOUT;
    }
    return RTC_OK;
}

static void RTC_ExitInitMode(void)
{
    RTC_ISR &= ~RTC_ISR_INIT;
}

/*------------------------------------------------------------------
 *  RTC_Init
 *------------------------------------------------------------------*/
RTC_Status_t RTC_Init(const RTC_Config_t *config)
{
    if (!config) return RTC_ERROR;

    /* 1. Enable backup domain write access */
    PWR_CR_RTC |= PWR_CR_DBP;

    /* 2. Enable and wait for the chosen oscillator */
    if (config->clock_src == RTC_CLK_LSE) {
        RCC_BDCR |= RCC_BDCR_LSEON;
        uint32 to = 500000U;
        while (!(RCC_BDCR & RCC_BDCR_LSERDY)) {
            if (--to == 0U) return RTC_TIMEOUT;
        }
    } else if (config->clock_src == RTC_CLK_LSI) {
        RCC_CSR_RTC |= RCC_CSR_LSION;
        uint32 to = 100000U;
        while (!(RCC_CSR_RTC & RCC_CSR_LSIRDY)) {
            if (--to == 0U) return RTC_TIMEOUT;
        }
    }

    /* 3. Select RTC clock source (only writable after backup domain reset
          or if RTCSEL is currently 00 — do not change a running source) */
    uint32 current_sel = (RCC_BDCR >> RCC_BDCR_RTCSEL_Pos) & 0x3U;
    if (current_sel == 0U) {
        RCC_BDCR |= ((uint32)config->clock_src << RCC_BDCR_RTCSEL_Pos);
    }

    /* 4. Enable RTC clock */
    RCC_BDCR |= RCC_BDCR_RTCEN;

    /* 5. Unlock write protection */
    RTC_Unlock();

    /* 6. Enter init mode */
    RTC_Status_t s = RTC_EnterInitMode();
    if (s != RTC_OK) { RTC_Lock(); return s; }

    /* 7. Set prescalers:
          PRER[22:16] = PREDIV_A (async, 7-bit)
          PRER[14:0]  = PREDIV_S (sync, 15-bit)          */
    RTC_PRER = ((config->prediv_a & 0x7FU) << 16U)
             | ((config->prediv_s & 0x7FFFU));

    /* 8. Set hour format */
    if (config->hour_format == RTC_HOURFORMAT_12)
        RTC_CR |=  RTC_CR_FMT;
    else
        RTC_CR &= ~RTC_CR_FMT;

    /* 9. Enable bypass shadow registers for direct counter read */
    RTC_CR |= RTC_CR_BYPSHAD;

    /* 10. Exit init mode */
    RTC_ExitInitMode();

    RTC_Lock();
    return RTC_OK;
}

/*------------------------------------------------------------------
 *  RTC_SetTime
 *------------------------------------------------------------------*/
RTC_Status_t RTC_SetTime(const RTC_Time_t *time)
{
    if (!time) return RTC_ERROR;

    RTC_Unlock();
    RTC_Status_t s = RTC_EnterInitMode();
    if (s != RTC_OK) { RTC_Lock(); return s; }

    uint32 tr = 0U;
    tr |= ((uint32)bin2bcd(time->seconds) & 0x7FU);
    tr |= ((uint32)bin2bcd(time->minutes) & 0x7FU) << 8U;
    tr |= ((uint32)bin2bcd(time->hours)   & 0x3FU) << 16U;
    if (time->ampm == RTC_PM) tr |= (1U << 22U);   /* PM bit */
    RTC_TR = tr;

    RTC_ExitInitMode();
    RTC_Lock();
    return RTC_OK;
}

/*------------------------------------------------------------------
 *  RTC_GetTime
 *------------------------------------------------------------------*/
RTC_Status_t RTC_GetTime(RTC_Time_t *time)
{
    if (!time) return RTC_ERROR;

    uint32 tr = RTC_TR;   /* With BYPSHAD=1 this reads the counter directly */

    time->seconds = bcd2bin((uint8)(tr         & 0x7FU));
    time->minutes = bcd2bin((uint8)((tr >>  8U) & 0x7FU));
    time->hours   = bcd2bin((uint8)((tr >> 16U) & 0x3FU));
    time->ampm    = (tr & (1U << 22U)) ? RTC_PM : RTC_AM;
    return RTC_OK;
}

/*------------------------------------------------------------------
 *  RTC_SetDate
 *------------------------------------------------------------------*/
RTC_Status_t RTC_SetDate(const RTC_Date_t *date)
{
    if (!date) return RTC_ERROR;

    RTC_Unlock();
    RTC_Status_t s = RTC_EnterInitMode();
    if (s != RTC_OK) { RTC_Lock(); return s; }

    uint32 dr = 0U;
    dr |= ((uint32)bin2bcd(date->date)  & 0x3FU);
    dr |= ((uint32)bin2bcd(date->month) & 0x1FU) << 8U;
    dr |= ((uint32)(date->weekday)      & 0x07U) << 13U;
    dr |= ((uint32)bin2bcd(date->year)  & 0xFFU) << 16U;
    RTC_DR = dr;

    RTC_ExitInitMode();
    RTC_Lock();
    return RTC_OK;
}

/*------------------------------------------------------------------
 *  RTC_GetDate
 *------------------------------------------------------------------*/
RTC_Status_t RTC_GetDate(RTC_Date_t *date)
{
    if (!date) return RTC_ERROR;

    /* Read TR first (per RM: reading DR unlocks both shadow registers) */
    (void)RTC_TR;
    uint32 dr = RTC_DR;

    date->date    = bcd2bin((uint8)(dr        & 0x3FU));
    date->month   = bcd2bin((uint8)((dr >> 8U) & 0x1FU));
    date->weekday = (RTC_Weekday_t)((dr >> 13U) & 0x07U);
    date->year    = bcd2bin((uint8)((dr >> 16U) & 0xFFU));
    return RTC_OK;
}

/*------------------------------------------------------------------
 *  RTC_SetAlarm  (A or B)
 *------------------------------------------------------------------*/
RTC_Status_t RTC_SetAlarm(RTC_AlarmID_t id, const RTC_Alarm_t *alarm,
                           uint8 interrupt_enable)
{
    if (!alarm) return RTC_ERROR;

    RTC_Unlock();

    /* Disable alarm to allow write access */
    if (id == RTC_ALARM_A)
        RTC_CR &= ~RTC_CR_ALRAE;
    else
        RTC_CR &= ~RTC_CR_ALRBE;

    /* Wait for write-allowed flag (ALRAWF / ALRBWF) */
    uint32 flag = (id == RTC_ALARM_A) ? RTC_ISR_ALRAWF : RTC_ISR_ALRBWF;
    uint32 to = 100000U;
    while (!(RTC_ISR & flag)) {
        if (--to == 0U) { RTC_Lock(); return RTC_TIMEOUT; }
    }

    /* Build alarm register value */
    uint32 alrm = 0U;
    alrm |= ((uint32)bin2bcd(alarm->time.seconds) & 0x7FU);
    alrm |= ((uint32)bin2bcd(alarm->time.minutes) & 0x7FU) << 8U;
    alrm |= ((uint32)bin2bcd(alarm->time.hours)   & 0x3FU) << 16U;
    if (alarm->time.ampm == RTC_PM) alrm |= (1U << 22U);

    if (alarm->use_weekday)
        alrm |= (((uint32)alarm->date_or_day & 0x0FU) << 24U) | (1U << 30U);
    else
        alrm |= ((uint32)bin2bcd(alarm->date_or_day) & 0x3FU) << 24U;

    if (alarm->mask_seconds) alrm |= (1U <<  7U);
    if (alarm->mask_minutes) alrm |= (1U << 15U);
    if (alarm->mask_hours)   alrm |= (1U << 23U);
    if (alarm->mask_date)    alrm |= (1U << 31U);

    if (id == RTC_ALARM_A)
        RTC_ALRMAR = alrm;
    else
        RTC_ALRMBR = alrm;

    /* Re-enable alarm */
    if (id == RTC_ALARM_A) {
        RTC_CR |= RTC_CR_ALRAE;
        if (interrupt_enable) RTC_CR |= RTC_CR_ALRAIE;
    } else {
        RTC_CR |= RTC_CR_ALRBE;
        if (interrupt_enable) RTC_CR |= RTC_CR_ALRBIE;
    }

    RTC_Lock();
    return RTC_OK;
}

void RTC_DisableAlarm(RTC_AlarmID_t id)
{
    RTC_Unlock();
    if (id == RTC_ALARM_A)
        RTC_CR &= ~(RTC_CR_ALRAE | RTC_CR_ALRAIE);
    else
        RTC_CR &= ~(RTC_CR_ALRBE | RTC_CR_ALRBIE);
    RTC_Lock();
}

void RTC_ClearAlarmFlag(RTC_AlarmID_t id)
{
    if (id == RTC_ALARM_A)
        RTC_ISR &= ~RTC_ISR_ALRAF;
    else
        RTC_ISR &= ~RTC_ISR_ALRBF;
}

uint8 RTC_GetAlarmFlag(RTC_AlarmID_t id)
{
    uint32 flag = (id == RTC_ALARM_A) ? RTC_ISR_ALRAF : RTC_ISR_ALRBF;
    return (RTC_ISR & flag) ? 1U : 0U;
}

/*------------------------------------------------------------------
 *  Wakeup timer
 *------------------------------------------------------------------*/
RTC_Status_t RTC_SetWakeup(RTC_WakeupClk_t clk_sel, uint16 period,
                             uint8 interrupt_enable)
{
    RTC_Unlock();

    /* Disable wakeup timer */
    RTC_CR &= ~RTC_CR_WUTE;
    uint32 to = 100000U;
    while (!(RTC_ISR & RTC_ISR_WUTWF)) {
        if (--to == 0U) { RTC_Lock(); return RTC_TIMEOUT; }
    }

    /* Set clock selection (CR[2:0]) and period */
    RTC_CR = (RTC_CR & ~0x07U) | ((uint32)clk_sel & 0x07U);
    RTC_WUTR = period;

    /* Enable */
    RTC_CR |= RTC_CR_WUTE;
    if (interrupt_enable) RTC_CR |= RTC_CR_WUTIE;

    RTC_Lock();
    return RTC_OK;
}

void RTC_DisableWakeup(void)
{
    RTC_Unlock();
    RTC_CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
    RTC_Lock();
}

void RTC_ClearWakeupFlag(void)  { RTC_ISR &= ~RTC_ISR_WUTF; }
uint8 RTC_GetWakeupFlag(void)   { return (RTC_ISR & RTC_ISR_WUTF) ? 1U : 0U; }

/*------------------------------------------------------------------
 *  Timestamp
 *------------------------------------------------------------------*/
void RTC_EnableTimestamp(uint8 falling_edge, uint8 interrupt_enable)
{
    RTC_Unlock();
    if (falling_edge) RTC_CR |=  RTC_CR_TSEDGE;
    else              RTC_CR &= ~RTC_CR_TSEDGE;
    RTC_CR |= RTC_CR_TSE;
    if (interrupt_enable) RTC_CR |= RTC_CR_TSIE;
    RTC_Lock();
}

void RTC_DisableTimestamp(void)
{
    RTC_Unlock();
    RTC_CR &= ~(RTC_CR_TSE | RTC_CR_TSIE);
    RTC_Lock();
}

void RTC_GetTimestamp(RTC_Time_t *time, RTC_Date_t *date)
{
    if (time) {
        uint32 tr        = RTC_TSTR;
        time->seconds    = bcd2bin((uint8)(tr         & 0x7FU));
        time->minutes    = bcd2bin((uint8)((tr >>  8U) & 0x7FU));
        time->hours      = bcd2bin((uint8)((tr >> 16U) & 0x3FU));
        time->ampm       = (tr & (1U << 22U)) ? RTC_PM : RTC_AM;
    }
    if (date) {
        uint32 dr     = RTC_TSDR;
        date->date    = bcd2bin((uint8)(dr        & 0x3FU));
        date->month   = bcd2bin((uint8)((dr >> 8U) & 0x1FU));
        date->weekday = (RTC_Weekday_t)((dr >> 13U) & 0x07U);
        date->year    = 0U;   /* TSDR does not store year */
    }
}

void RTC_ClearTimestampFlag(void) { RTC_ISR &= ~RTC_ISR_TSF; }

/*------------------------------------------------------------------
 *  Backup registers
 *------------------------------------------------------------------*/
void RTC_WriteBackup(uint8 index, uint32 value)
{
    if (index > 19U) return;
    PWR_CR_RTC |= PWR_CR_DBP;          /* ensure backup domain is writable */
    RTC_BKP(index) = value;
}

uint32 RTC_ReadBackup(uint8 index)
{
    if (index > 19U) return 0U;
    return RTC_BKP(index);
}

/*------------------------------------------------------------------
 *  Status helpers
 *------------------------------------------------------------------*/
uint8 RTC_IsInitialized(void) { return (RTC_ISR & RTC_ISR_INITS) ? 1U : 0U; }

void RTC_WaitSync(void)
{
    /* Clear RSF then wait for it to be set again by hardware */
    RTC_ISR &= ~RTC_ISR_RSF;
    uint32 to = 100000U;
    while (!(RTC_ISR & RTC_ISR_RSF) && --to) {}
}
