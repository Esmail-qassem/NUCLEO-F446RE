/***********************************************************************/
/* Author     : Esmail Qassem                                          */
/* SWC        : RTC                                                    */
/* Version    : V2.0 - C++ (STM32F446RE)                              */
/***********************************************************************/
#include "RTC.hpp"

/* ── BCD helpers ──────────────────────────────────────────────────── */
static inline uint8 bin2bcd(uint8 val)
{
    return static_cast<uint8>(((val / 10U) << 4U) | (val % 10U));
}

static inline uint8 bcd2bin(uint8 bcd)
{
    return static_cast<uint8>(((bcd >> 4U) & 0x0FU) * 10U + (bcd & 0x0FU));
}

/* ── Write-protection helpers ─────────────────────────────────────── */
static inline void RTC_Unlock_WP(void)
{
    RTC_WPR = 0xCAU;
    RTC_WPR = 0x53U;
}

static inline void RTC_Lock_WP(void)
{
    RTC_WPR = 0xFFU;
}

/* ── Init mode helpers ────────────────────────────────────────────── */
static RTC_Status RTC_EnterInitMode(void)
{
    uint32 timeout = 100000U;
    RTC_ISR |= RTC_ISR_INIT;
    while (!(RTC_ISR & RTC_ISR_INITF))
    {
        if (--timeout == 0U) return RTC_Status::TIMEOUT;
    }
    return RTC_Status::OK;
}

static void RTC_ExitInitMode(void)
{
    RTC_ISR &= ~RTC_ISR_INIT;
}

/* ── Init ─────────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::Init(const RTC_Config_t &config)
{
    PWR_CR_RTC |= PWR_CR_DBP;

    if (config.clock_src == RTC_ClockSrc::LSE)
    {
        RCC_BDCR |= RCC_BDCR_LSEON;
        uint32 to = 500000U;
        while (!(RCC_BDCR & RCC_BDCR_LSERDY))
        {
            if (--to == 0U) return RTC_Status::TIMEOUT;
        }
    }
    else if (config.clock_src == RTC_ClockSrc::LSI)
    {
        RCC_CSR_RTC |= RCC_CSR_LSION;
        uint32 to = 100000U;
        while (!(RCC_CSR_RTC & RCC_CSR_LSIRDY))
        {
            if (--to == 0U) return RTC_Status::TIMEOUT;
        }
    }

    uint32 current_sel = (RCC_BDCR >> RCC_BDCR_RTCSEL_Pos) & 0x3U;
    if (current_sel == 0U)
    {
        RCC_BDCR |= (static_cast<uint32>(config.clock_src) << RCC_BDCR_RTCSEL_Pos);
    }

    RCC_BDCR |= RCC_BDCR_RTCEN;

    RTC_Unlock_WP();

    RTC_Status s = RTC_EnterInitMode();
    if (s != RTC_Status::OK) { RTC_Lock_WP(); return s; }

    RTC_PRER = ((config.prediv_a & 0x7FU) << 16U)
             | ((config.prediv_s & 0x7FFFU));

    if (config.hour_format == RTC_HourFormat::H12)
        RTC_CR |=  RTC_CR_FMT;
    else
        RTC_CR &= ~RTC_CR_FMT;

    RTC_CR |= RTC_CR_BYPSHAD;

    RTC_ExitInitMode();
    RTC_Lock_WP();
    return RTC_Status::OK;
}

/* ── SetTime ──────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::SetTime(const RTC_Time_t &time)
{
    RTC_Unlock_WP();
    RTC_Status s = RTC_EnterInitMode();
    if (s != RTC_Status::OK) { RTC_Lock_WP(); return s; }

    uint32 tr = 0U;
    tr |= static_cast<uint32>(bin2bcd(time.seconds) & 0x7FU);
    tr |= static_cast<uint32>(bin2bcd(time.minutes) & 0x7FU) << 8U;
    tr |= static_cast<uint32>(bin2bcd(time.hours)   & 0x3FU) << 16U;
    if (time.ampm == RTC_AMPM::PM) tr |= (1U << 22U);
    RTC_TR = tr;

    RTC_ExitInitMode();
    RTC_Lock_WP();
    return RTC_Status::OK;
}

/* ── GetTime ──────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::GetTime(RTC_Time_t &time)
{
    uint32 tr = RTC_TR;
    time.seconds = bcd2bin(static_cast<uint8>(tr         & 0x7FU));
    time.minutes = bcd2bin(static_cast<uint8>((tr >>  8U) & 0x7FU));
    time.hours   = bcd2bin(static_cast<uint8>((tr >> 16U) & 0x3FU));
    time.ampm    = (tr & (1U << 22U)) ? RTC_AMPM::PM : RTC_AMPM::AM;
    return RTC_Status::OK;
}

/* ── SetDate ──────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::SetDate(const RTC_Date_t &date)
{
    RTC_Unlock_WP();
    RTC_Status s = RTC_EnterInitMode();
    if (s != RTC_Status::OK) { RTC_Lock_WP(); return s; }

    uint32 dr = 0U;
    dr |= static_cast<uint32>(bin2bcd(date.date)  & 0x3FU);
    dr |= static_cast<uint32>(bin2bcd(date.month) & 0x1FU) << 8U;
    dr |= (static_cast<uint32>(date.weekday) & 0x07U) << 13U;
    dr |= static_cast<uint32>(bin2bcd(date.year)  & 0xFFU) << 16U;
    RTC_DR = dr;

    RTC_ExitInitMode();
    RTC_Lock_WP();
    return RTC_Status::OK;
}

/* ── GetDate ──────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::GetDate(RTC_Date_t &date)
{
    (void)RTC_TR;
    uint32 dr = RTC_DR;
    date.date    = bcd2bin(static_cast<uint8>(dr        & 0x3FU));
    date.month   = bcd2bin(static_cast<uint8>((dr >> 8U) & 0x1FU));
    date.weekday = static_cast<RTC_Weekday>((dr >> 13U) & 0x07U);
    date.year    = bcd2bin(static_cast<uint8>((dr >> 16U) & 0xFFU));
    return RTC_Status::OK;
}

/* ── SetAlarm ─────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::SetAlarm(RTC_AlarmID id, const RTC_Alarm_t &alarm, uint8 interrupt_enable)
{
    RTC_Unlock_WP();

    if (id == RTC_AlarmID::A)
        RTC_CR &= ~RTC_CR_ALRAE;
    else
        RTC_CR &= ~RTC_CR_ALRBE;

    uint32 flag = (id == RTC_AlarmID::A) ? RTC_ISR_ALRAWF : RTC_ISR_ALRBWF;
    uint32 to = 100000U;
    while (!(RTC_ISR & flag))
    {
        if (--to == 0U) { RTC_Lock_WP(); return RTC_Status::TIMEOUT; }
    }

    uint32 alrm = 0U;
    alrm |= static_cast<uint32>(bin2bcd(alarm.time.seconds) & 0x7FU);
    alrm |= static_cast<uint32>(bin2bcd(alarm.time.minutes) & 0x7FU) << 8U;
    alrm |= static_cast<uint32>(bin2bcd(alarm.time.hours)   & 0x3FU) << 16U;
    if (alarm.time.ampm == RTC_AMPM::PM) alrm |= (1U << 22U);

    if (alarm.use_weekday)
        alrm |= ((static_cast<uint32>(alarm.date_or_day) & 0x0FU) << 24U) | (1U << 30U);
    else
        alrm |= (static_cast<uint32>(bin2bcd(alarm.date_or_day)) & 0x3FU) << 24U;

    if (alarm.mask_seconds) alrm |= (1U <<  7U);
    if (alarm.mask_minutes) alrm |= (1U << 15U);
    if (alarm.mask_hours)   alrm |= (1U << 23U);
    if (alarm.mask_date)    alrm |= (1U << 31U);

    if (id == RTC_AlarmID::A)
        RTC_ALRMAR = alrm;
    else
        RTC_ALRMBR = alrm;

    if (id == RTC_AlarmID::A)
    {
        RTC_CR |= RTC_CR_ALRAE;
        if (interrupt_enable) RTC_CR |= RTC_CR_ALRAIE;
    }
    else
    {
        RTC_CR |= RTC_CR_ALRBE;
        if (interrupt_enable) RTC_CR |= RTC_CR_ALRBIE;
    }

    RTC_Lock_WP();
    return RTC_Status::OK;
}

/* ── DisableAlarm ─────────────────────────────────────────────────── */
void RTC_Driver::DisableAlarm(RTC_AlarmID id)
{
    RTC_Unlock_WP();
    if (id == RTC_AlarmID::A)
        RTC_CR &= ~(RTC_CR_ALRAE | RTC_CR_ALRAIE);
    else
        RTC_CR &= ~(RTC_CR_ALRBE | RTC_CR_ALRBIE);
    RTC_Lock_WP();
}

/* ── ClearAlarmFlag ───────────────────────────────────────────────── */
void RTC_Driver::ClearAlarmFlag(RTC_AlarmID id)
{
    if (id == RTC_AlarmID::A)
        RTC_ISR &= ~RTC_ISR_ALRAF;
    else
        RTC_ISR &= ~RTC_ISR_ALRBF;
}

/* ── GetAlarmFlag ─────────────────────────────────────────────────── */
uint8 RTC_Driver::GetAlarmFlag(RTC_AlarmID id)
{
    uint32 flag = (id == RTC_AlarmID::A) ? RTC_ISR_ALRAF : RTC_ISR_ALRBF;
    return (RTC_ISR & flag) ? 1U : 0U;
}

/* ── SetWakeup ────────────────────────────────────────────────────── */
RTC_Status RTC_Driver::SetWakeup(RTC_WakeupClk clk_sel, uint16 period, uint8 interrupt_enable)
{
    RTC_Unlock_WP();
    RTC_CR &= ~RTC_CR_WUTE;
    uint32 to = 100000U;
    while (!(RTC_ISR & RTC_ISR_WUTWF))
    {
        if (--to == 0U) { RTC_Lock_WP(); return RTC_Status::TIMEOUT; }
    }
    RTC_CR = (RTC_CR & ~0x07U) | (static_cast<uint32>(clk_sel) & 0x07U);
    RTC_WUTR = static_cast<uint32>(period);
    RTC_CR |= RTC_CR_WUTE;
    if (interrupt_enable) RTC_CR |= RTC_CR_WUTIE;
    RTC_Lock_WP();
    return RTC_Status::OK;
}

/* ── DisableWakeup ────────────────────────────────────────────────── */
void RTC_Driver::DisableWakeup(void)
{
    RTC_Unlock_WP();
    RTC_CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);
    RTC_Lock_WP();
}

void RTC_Driver::ClearWakeupFlag(void)  { RTC_ISR &= ~RTC_ISR_WUTF; }
uint8 RTC_Driver::GetWakeupFlag(void)   { return (RTC_ISR & RTC_ISR_WUTF) ? 1U : 0U; }

/* ── Timestamp ────────────────────────────────────────────────────── */
void RTC_Driver::EnableTimestamp(uint8 falling_edge, uint8 interrupt_enable)
{
    RTC_Unlock_WP();
    if (falling_edge) RTC_CR |=  RTC_CR_TSEDGE;
    else              RTC_CR &= ~RTC_CR_TSEDGE;
    RTC_CR |= RTC_CR_TSE;
    if (interrupt_enable) RTC_CR |= RTC_CR_TSIE;
    RTC_Lock_WP();
}

void RTC_Driver::DisableTimestamp(void)
{
    RTC_Unlock_WP();
    RTC_CR &= ~(RTC_CR_TSE | RTC_CR_TSIE);
    RTC_Lock_WP();
}

void RTC_Driver::GetTimestamp(RTC_Time_t &time, RTC_Date_t &date)
{
    uint32 tr     = RTC_TSTR;
    time.seconds  = bcd2bin(static_cast<uint8>(tr         & 0x7FU));
    time.minutes  = bcd2bin(static_cast<uint8>((tr >>  8U) & 0x7FU));
    time.hours    = bcd2bin(static_cast<uint8>((tr >> 16U) & 0x3FU));
    time.ampm     = (tr & (1U << 22U)) ? RTC_AMPM::PM : RTC_AMPM::AM;

    uint32 dr     = RTC_TSDR;
    date.date     = bcd2bin(static_cast<uint8>(dr        & 0x3FU));
    date.month    = bcd2bin(static_cast<uint8>((dr >> 8U) & 0x1FU));
    date.weekday  = static_cast<RTC_Weekday>((dr >> 13U) & 0x07U);
    date.year     = 0U;
}

void RTC_Driver::ClearTimestampFlag(void) { RTC_ISR &= ~RTC_ISR_TSF; }

/* ── Backup registers ─────────────────────────────────────────────── */
void RTC_Driver::WriteBackup(uint8 index, uint32 value)
{
    if (index > 19U) return;
    PWR_CR_RTC |= PWR_CR_DBP;
    RTC_BKP(index) = value;
}

uint32 RTC_Driver::ReadBackup(uint8 index)
{
    if (index > 19U) return 0U;
    return RTC_BKP(index);
}

/* ── Status helpers ───────────────────────────────────────────────── */
uint8 RTC_Driver::IsInitialized(void) { return (RTC_ISR & RTC_ISR_INITS) ? 1U : 0U; }

void RTC_Driver::WaitSync(void)
{
    RTC_ISR &= ~RTC_ISR_RSF;
    uint32 to = 100000U;
    while (!(RTC_ISR & RTC_ISR_RSF) && --to) {}
}
